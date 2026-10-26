using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;

namespace Runefall.Build;

internal static class Validator
{
    public static string Validate(List<PluginOutput> plugins, Content content, string dataDir)
    {
        var lines = new List<string>();
        bool failed = false;
        var allEditors = new HashSet<string>(StringComparer.Ordinal);
        var counts = new Dictionary<string, int>();

        foreach (var plugin in plugins)
        {
            var loaded = SkyrimMod.CreateFromBinary(plugin.Path, SkyrimRelease.SkyrimSE);
            var flags = loaded.ModHeader.Flags;
            var esl = flags.HasFlag(SkyrimModHeader.HeaderFlag.Small);
            if (!esl)
            {
                lines.Add($"FAIL plugin {plugin.Name}: ESL flag missing");
                failed = true;
            }
            var masters = loaded.ModHeader.MasterReferences.Select(m => m.Master.Name).ToHashSet(StringComparer.OrdinalIgnoreCase);
            if (!masters.Contains("Skyrim"))
            {
                lines.Add($"FAIL plugin {plugin.Name}: missing Skyrim.esm master");
                failed = true;
            }
            int count = 0;
            foreach (var record in loaded.EnumerateMajorRecords())
            {
                ++count;
                if (record.FormKey.ID >= 0x1000)
                {
                    lines.Add($"FAIL plugin {plugin.Name}: FormID 0x{record.FormKey.ID:X} outside ESL window");
                    failed = true;
                    break;
                }
                var editor = record.EditorID;
                if (!string.IsNullOrWhiteSpace(editor))
                    allEditors.Add(editor);
            }
            counts[plugin.Name] = count;
            lines.Add($"{(esl ? "PASS" : "FAIL")} plugin {plugin.Name}: {count} records, ESL={esl}, masters={string.Join("+", masters)}");
        }

        var baseEditors = plugins.FirstOrDefault(p => p.Name == "Runefall.esp");
        if (baseEditors is null)
        {
            lines.Add("FAIL base plugin Runefall.esp missing");
            failed = true;
        }

        Expect(content.BaseMgefEditors.Count == 120, "base affix MGEF count is 120", lines, ref failed);
        Expect(content.UniqueRows.Count == 59, "unique_affixes.csv has 59 rows", lines, ref failed);
        Expect(content.RaceGroupRows.Count == 19, "race_groups.csv has 19 families", lines, ref failed);
        Expect(content.AffixRows.Count == 600, "affixes.csv has 600 rows", lines, ref failed);
        Expect(content.UniqueSourceRows.Count == 59, "unique_sources.csv has 59 rows", lines, ref failed);

        foreach (var effect in content.BaseMgefEditors.Concat(content.UniqueRecords).Concat(content.HuntNames))
        {
            if (!allEditors.Contains(effect))
            {
                lines.Add($"FAIL data reference missing record: {effect}");
                failed = true;
                break;
            }
        }

        foreach (var row in content.MaterialRows)
        {
            var item = row[4];
            if (!allEditors.Contains(item))
            {
                lines.Add($"FAIL material item record missing: {item}");
                failed = true;
                break;
            }
        }

        // Skill rank records are intentionally not generated yet; report as a warning.
        lines.Add("WARN SPEL/PERK skill records are not generated yet (next content phase)");
        lines.Add("WARN COBJ/QUST/NPC records are not generated yet (next content phase)");

        lines.Add(failed ? "RESULT: FAIL" : "RESULT: PASS");
        return string.Join(Environment.NewLine, lines);
    }

    private static void Expect(bool condition, string message, List<string> lines, ref bool failed)
    {
        if (condition)
        {
            lines.Add("PASS " + message.Replace("FAIL ", string.Empty).Replace("must", "is"));
            return;
        }
        lines.Add(message);
        failed = true;
    }
}