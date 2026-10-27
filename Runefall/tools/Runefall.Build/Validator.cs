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

        void CheckRange<T>(string pluginName, string category, IEnumerable<T> records, uint min, uint max) where T : IFormKeyGetter
        {
            foreach (var record in records)
            {
                var id = record.FormKey.ID;
                if (id < min || id > max)
                {
                    lines.Add($"FAIL plugin {pluginName}: {category} FormID 0x{id:X} outside 0x{min:X}-0x{max:X}");
                    failed = true;
                    return;
                }
            }
        }

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
            CheckRange(plugin.Name, "KYWD", loaded.Keywords, 0x800, 0x87F);
            CheckRange(plugin.Name, "GLOB", loaded.Globals, 0x880, 0x8FF);
            CheckRange(plugin.Name, "MGEF", loaded.MagicEffects, 0x900, 0x9FF);
            CheckRange(plugin.Name, "SPEL", loaded.Spells, 0xA00, 0xAFF);
            CheckRange(plugin.Name, "PERK", loaded.Perks, 0xB00, 0xB7F);
            CheckRange(plugin.Name, "MISC", loaded.MiscItems, 0xB80, 0xBFF);
            CheckRange(plugin.Name, "COBJ", loaded.ConstructibleObjects, 0xC00, 0xC7F);
            CheckRange(plugin.Name, "QUST/FLST", loaded.Quests, 0xC80, 0xCFF);
            CheckRange(plugin.Name, "QUST/FLST", loaded.FormLists, 0xC80, 0xCFF);
            CheckRange(plugin.Name, "NPC/CLAS", loaded.Npcs, 0xD00, 0xD7F);
            CheckRange(plugin.Name, "NPC/CLAS", loaded.Classes, 0xD00, 0xD7F);
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

        var baseOut = plugins.FirstOrDefault(x => x.Name == "Runefall.esp");
        if (baseOut is null)
        {
            lines.Add("FAIL base plugin Runefall.esp missing for record-family checks");
            failed = true;
        }
        else
        {
            Expect(content.AttributeRows.Count == 9, "attributes.csv has 9 rows", lines, ref failed);
            Expect(content.AttributeEffectRows.Count >= 30, "attribute_effects.csv has >=30 rows", lines, ref failed);
            Expect(baseOut.Mod.Keywords.Count >= 60, "base plugin has >=60 keywords", lines, ref failed);
            Expect(baseOut.Mod.Globals.Count >= 39, "base plugin has >=39 globals", lines, ref failed);
            Expect(baseOut.Mod.MagicEffects.Count >= 200, "base plugin has >=200 MGEF", lines, ref failed);
            Expect(baseOut.Mod.Npcs.Count == 20, "base plugin has 20 boss NPC templates", lines, ref failed);
            Expect(baseOut.Mod.Classes.Count == 20, "base plugin has 20 boss class templates", lines, ref failed);
            Expect(baseOut.Mod.Spells.Count == 32, "base plugin has 32 spells (27 skill + 5 rank)", lines, ref failed);
            Expect(baseOut.Mod.Spells.Count(s => s.EditorID?.StartsWith("RF_SPEL_Rank_", StringComparison.Ordinal) == true) == 5, "base plugin has 5 rank abilities", lines, ref failed);
            Expect(baseOut.Mod.Perks.Count == 27, "base plugin has 27 skill perks", lines, ref failed);
            Expect(baseOut.Mod.Quests.Count == 3, "base plugin has 3 quest hosts", lines, ref failed);
            Expect(baseOut.Mod.FormLists.Count == 5, "base plugin has 5 form lists", lines, ref failed);
            Expect(baseOut.Mod.ConstructibleObjects.Count == 8, "base plugin has 8 constructible objects", lines, ref failed);

            var badSpells = 0;
            foreach (var spell in baseOut.Mod.Spells)
            {
                if (spell.Effects == null || spell.Effects.Count == 0)
                {
                    ++badSpells;
                    continue;
                }
                foreach (var effect in spell.Effects)
                    if (effect.BaseEffect.IsNull)
                        ++badSpells;
            }
            Expect(badSpells == 0, "base plugin spells have no null EFID effects", lines, ref failed);
        }

        // P3.6 cross-reference checks: every data-referenced EditorID must exist in a generated plugin.
        string[] ReadCsv(string fileName, out string[] header)
        {
            var path = Path.Combine(dataDir, fileName);
            if (!File.Exists(path))
            {
                header = Array.Empty<string>();
                return Array.Empty<string>();
            }
            var rows = File.ReadAllLines(path).Where(l => !string.IsNullOrWhiteSpace(l)).ToArray();
            header = rows.Length > 0 ? rows[0].Split(',') : Array.Empty<string>();
            return rows.Skip(1).ToArray();
        }
        int ColumnIndex(string[] header, string name) => Array.IndexOf(header, name);

        var missingRefs = 0;
        void RequireRef(string context, string value)
        {
            if (string.IsNullOrWhiteSpace(value))
                return;
            if (!allEditors.Contains(value))
            {
                if (missingRefs < 10)
                    lines.Add($"FAIL data ref missing record: {context} -> {value}");
                ++missingRefs;
            }
        }

        {
            var rows = ReadCsv("affixes.csv", out var header);
            var cEffect = ColumnIndex(header, "effectEditorId");
            foreach (var row in rows)
            {
                var cells = row.Split(',');
                if (cEffect >= 0 && cEffect < cells.Length)
                    RequireRef("affixes.csv", cells[cEffect]);
            }
        }
        {
            var rows = ReadCsv("unique_affixes.csv", out var header);
            var cEffect = ColumnIndex(header, "effectEditorId");
            foreach (var row in rows)
            {
                var cells = row.Split(',');
                if (cEffect >= 0 && cEffect < cells.Length)
                    RequireRef("unique_affixes.csv", cells[cEffect]);
            }
        }
        {
            var rows = ReadCsv("skills.csv", out var header);
            var c1 = ColumnIndex(header, "rank1");
            var c2 = ColumnIndex(header, "rank2");
            var c3 = ColumnIndex(header, "rank3");
            foreach (var row in rows)
            {
                var cells = row.Split(',');
                foreach (var c in new[] { c1, c2, c3 })
                    if (c >= 0 && c < cells.Length)
                        RequireRef("skills.csv", cells[c]);
            }
        }
        {
            var uniqueRows = ReadCsv("unique_affixes.csv", out var uniqueHeader);
            var cUniqueId = ColumnIndex(uniqueHeader, "uniqueId");
            var uniqueIds = new HashSet<string>(StringComparer.Ordinal);
            foreach (var row in uniqueRows)
            {
                var cells = row.Split(',');
                if (cUniqueId >= 0 && cUniqueId < cells.Length)
                    uniqueIds.Add(cells[cUniqueId]);
            }
            var rows = ReadCsv("drops_races.csv", out var header);
            var cType = ColumnIndex(header, "rewardType");
            var cId = ColumnIndex(header, "rewardId");
            foreach (var row in rows)
            {
                var cells = row.Split(',');
                if (cType < 0 || cId < 0 || cType >= cells.Length || cId >= cells.Length)
                    continue;
                if (cells[cType] == "material")
                    RequireRef("drops_races.csv material", cells[cId]);
                else if (cells[cType] == "unique_affix" && !uniqueIds.Contains(cells[cId]))
                {
                    lines.Add($"FAIL data ref missing unique affix: drops_races.csv -> {cells[cId]}");
                    ++missingRefs;
                }
            }
        }
        Expect(missingRefs == 0, "data cross-reference EditorIDs resolve", lines, ref failed);


        // P3.6/L3E coverage gates: race/family and affix-table invariants.
        {
            var groups = new HashSet<string>(StringComparer.Ordinal);
            foreach (var row in content.RaceGroupRows)
            {
                if (row.Length < 5 || string.IsNullOrWhiteSpace(row[0])) { lines.Add("FAIL race_groups.csv malformed row"); failed = true; continue; }
                groups.Add(row[0]);
                if (row[1].Split('|', StringSplitOptions.RemoveEmptyEntries).Length == 0) { lines.Add($"FAIL race_groups.csv {row[0]} has no member races"); failed = true; }
                if (string.IsNullOrWhiteSpace(row[3]) || string.IsNullOrWhiteSpace(row[4])) { lines.Add($"FAIL race_groups.csv {row[0]} missing unique/material"); failed = true; }
            }
            Expect(groups.Count == 19, "race_groups.csv has 19 unique families", lines, ref failed);

            var uniqueFirst = new HashSet<string>(StringComparer.Ordinal);
            foreach (var row in content.RaceDropRows)
            {
                if (row.Length < 5) { lines.Add("FAIL drops_races.csv malformed row"); failed = true; continue; }
                if (row[1] == "unique_affix" && row[4] == "1") uniqueFirst.Add(row[0]);
                if (row[1] == "material" && row[4] == "1") { lines.Add($"FAIL drops_races.csv {row[0]} material must not be firstOnly"); failed = true; }
            }
            Expect(uniqueFirst.Count == groups.Count, "drops_races.csv has one firstOnly unique per family", lines, ref failed);

            var categories = new Dictionary<string, int>(StringComparer.Ordinal);
            var slots = new HashSet<string>(StringComparer.Ordinal);
            foreach (var row in content.AffixRows)
            {
                if (row.Length < 6) { lines.Add("FAIL affixes.csv malformed row"); failed = true; continue; }
                categories[row[2]] = categories.TryGetValue(row[2], out var count) ? count + 1 : 1;
                foreach (var token in row[5].Split('|', StringSplitOptions.RemoveEmptyEntries))
                    slots.Add(token);
            }
            Expect(categories.Count == 5, "affixes.csv covers 5 categories", lines, ref failed);
            foreach (var category in categories)
                Expect(category.Value >= 20, $"affix category {category.Key} has >=20 rows", lines, ref failed);
            Expect(slots.Count >= 11, "affixes.csv covers >=11 slot tokens", lines, ref failed);

            var uniqueIds = content.UniqueRows.Select(r => r[0]).Distinct(StringComparer.Ordinal).Count();
            Expect(uniqueIds == 59, "unique_affixes.csv has 59 distinct ids", lines, ref failed);
            Expect(content.SkillRows.Count == 18, "skills.csv has 18 skills", lines, ref failed);
            Expect(content.StringRows.Count >= 250, "strings.csv covers >=250 keys", lines, ref failed);
        }

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