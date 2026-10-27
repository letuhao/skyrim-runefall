using System.Globalization;
using System.Text;
using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;

namespace Runefall.Build;

internal static class Program
{
    private static readonly UTF8Encoding Utf8NoBom = new(false);
    private static string outDir = Path.Combine("..", "..", "build", "dist");
    private static string? deployDataDir;

    private static void Main(string[] args)
    {
        for (int i = 0; i < args.Length; i++)
        {
            if (args[i] == "--out" && i + 1 < args.Length) outDir = args[++i];
            else if (args[i] == "--deploy" && i + 1 < args.Length) deployDataDir = args[++i];
        }

        outDir = Path.GetFullPath(outDir);
        var dataDir = Path.Combine(outDir, "Data");
        var runefallData = Path.Combine(dataDir, "Runefall", "data");
        var packDir = Path.Combine(dataDir, "Runefall", "packs");
        Directory.CreateDirectory(runefallData);
        Directory.CreateDirectory(packDir);

        var content = ContentFactory.Create();
        WriteData(content, runefallData);
        WriteManifests(packDir, content);
        WriteIni(Path.Combine(dataDir, "Runefall", "runefall.ini"));

        var plugins = PluginFactory.Create(content, dataDir);
        WriteFormIds(plugins, runefallData);
        var report = Validator.Validate(plugins, content, runefallData);
        Console.WriteLine(report);

        WriteBuildReport(Path.Combine(outDir, "BUILD-REPORT.md"), report, content);

        if (deployDataDir is not null)
        {
            var target = Path.GetFullPath(deployDataDir);
            Console.WriteLine($"Deploying Data tree to {target}");
            Deploy(dataDir, target);
        }

        if (report.Contains("FAIL", StringComparison.Ordinal))
            Environment.ExitCode = 1;
    }

    private static void WriteData(Content content, string dir)
    {
        WriteCsv(Path.Combine(dir, "affixes.csv"), content.AffixColumns, content.AffixRows);
        WriteCsv(Path.Combine(dir, "rarity.csv"), content.RarityColumns, content.RarityRows);
        WriteCsv(Path.Combine(dir, "rarity_weights.csv"), content.RarityWeightColumns, content.RarityWeightRows);
        WriteCsv(Path.Combine(dir, "enemy_ranks.csv"), content.RankColumns, content.RankRows);
        WriteCsv(Path.Combine(dir, "spawn_weights.csv"), content.SpawnColumns, content.SpawnRows);
        WriteCsv(Path.Combine(dir, "drops_general.csv"), content.GeneralDropColumns, content.GeneralDropRows);
        WriteCsv(Path.Combine(dir, "drops_containers.csv"), content.ContainerDropColumns, content.ContainerDropRows);
        WriteCsv(Path.Combine(dir, "drops_quests.csv"), content.QuestDropColumns, content.QuestDropRows);
        WriteCsv(Path.Combine(dir, "drops_bosses.csv"), content.BossDropColumns, content.BossDropRows);
        WriteCsv(Path.Combine(dir, "race_groups.csv"), content.RaceGroupColumns, content.RaceGroupRows);
        WriteCsv(Path.Combine(dir, "drops_races.csv"), content.RaceDropColumns, content.RaceDropRows);
        WriteCsv(Path.Combine(dir, "materials.csv"), content.MaterialColumns, content.MaterialRows);
        WriteCsv(Path.Combine(dir, "recipes.csv"), content.RecipeColumns, content.RecipeRows);
        WriteCsv(Path.Combine(dir, "skills.csv"), content.SkillColumns, content.SkillRows);
        WriteCsv(Path.Combine(dir, "attributes.csv"), content.AttributeColumns, content.AttributeRows);
        WriteCsv(Path.Combine(dir, "attribute_effects.csv"), content.AttributeEffectColumns, content.AttributeEffectRows);
        WriteCsv(Path.Combine(dir, "unique_affixes.csv"), content.UniqueColumns, content.UniqueRows);
        WriteCsv(Path.Combine(dir, "unique_sources.csv"), content.UniqueSourceColumns, content.UniqueSourceRows);
        WriteCsv(Path.Combine(dir, "strings.csv"), content.StringColumns, content.StringRows);

        var formats = new StringBuilder();
        formats.AppendLine("{");
        formats.AppendLine("  \"format\": 1,");
        formats.AppendLine("  \"columns\": {");
        var tables = new (string Name, string[] Cols)[]
        {
            ("affixes.csv", content.AffixColumns), ("rarity.csv", content.RarityColumns),
            ("rarity_weights.csv", content.RarityWeightColumns), ("enemy_ranks.csv", content.RankColumns),
            ("spawn_weights.csv", content.SpawnColumns), ("drops_general.csv", content.GeneralDropColumns),
            ("drops_containers.csv", content.ContainerDropColumns), ("drops_quests.csv", content.QuestDropColumns),
            ("drops_bosses.csv", content.BossDropColumns), ("race_groups.csv", content.RaceGroupColumns),
            ("drops_races.csv", content.RaceDropColumns), ("materials.csv", content.MaterialColumns),
            ("recipes.csv", content.RecipeColumns), ("skills.csv", content.SkillColumns),
            ("attributes.csv", content.AttributeColumns), ("attribute_effects.csv", content.AttributeEffectColumns), ("unique_affixes.csv", content.UniqueColumns),
            ("unique_sources.csv", content.UniqueSourceColumns), ("strings.csv", content.StringColumns),
            ("form_ids.csv", new[] { "editorId", "plugin", "localFormId", "recordType" }),
        };
        for (int i = 0; i < tables.Length; i++)
        {
            var comma = i == tables.Length - 1 ? "" : ",";
            formats.Append("    ").Append(JsonString(tables[i].Name)).Append(": [")
                .Append(string.Join(", ", tables[i].Cols.Select(JsonString))).Append(']').AppendLine(comma);
        }
        formats.AppendLine("  }");
        formats.AppendLine("}");
        File.WriteAllText(Path.Combine(dir, "formats.json"), formats.ToString(), Utf8NoBom);
    }

    private static string JsonString(string value) => "\"" + value.Replace("\"", "\\\"") + "\"";

    private static void WriteCsv(string path, string[] columns, IEnumerable<string[]> rows)
    {
        var sb = new StringBuilder();
        sb.AppendLine(string.Join(",", columns));
        foreach (var row in rows)
            sb.AppendLine(string.Join(",", row.Select(Escape)));
        File.WriteAllText(path, sb.ToString(), Utf8NoBom);
    }

    private static string Escape(string value)
    {
        if (value.Contains(',') || value.Contains('"') || value.Contains('\n'))
            return "\"" + value.Replace("\"", "\"\"") + "\"";
        return value;
    }

    private static void WriteFormIds(List<PluginOutput> plugins, string dir)
    {
        var rows = new List<string[]>();
        foreach (var output in plugins)
        {
            foreach (var record in output.Mod.EnumerateMajorRecords())
            {
                var editorID = record.EditorID;
                if (string.IsNullOrEmpty(editorID))
                    continue;
                rows.Add(new[]
                {
                    editorID,
                    output.Name,
                    record.FormKey.ID.ToString("X6", CultureInfo.InvariantCulture),
                    record.GetType().Name
                });
            }
        }
        rows.Sort((a, b) => string.CompareOrdinal(a[0], b[0]));
        WriteCsv(Path.Combine(dir, "form_ids.csv"),
            new[] { "editorId", "plugin", "localFormId", "recordType" }, rows);
    }

    private static void WriteManifests(string packDir, Content content)
    {
        var uniqueEditors = content.UniqueNames
            .Where(e => e.StartsWith("RF_MGEF_Unique_", StringComparison.Ordinal))
            .Distinct(StringComparer.Ordinal).ToArray();
        var huntEditors = content.HuntNames
            .Concat(content.HuntRecords.Where(e => e.StartsWith("RF_MISC_", StringComparison.Ordinal)))
            .Distinct(StringComparer.Ordinal).ToArray();
        WriteManifest(Path.Combine(packDir, "unique.json"), "unique", "Runefall - Unique.esp",
            new[] { "data/unique_affixes.csv", "data/unique_sources.csv" }, uniqueEditors.Length, uniqueEditors);
        WriteManifest(Path.Combine(packDir, "hunt.json"), "hunt", "Runefall - Hunt.esp",
            new[] { "data/race_groups.csv", "data/drops_races.csv", "data/materials.csv" }, huntEditors.Length, huntEditors);
    }

    private static void WriteManifest(string path, string packId, string plugin, string[] files, int count, IEnumerable<string> editors)
    {
        var editorsJson = string.Join(", ", editors.Select(JsonString));
        var filesJson = string.Join(", ", files.Select(JsonString));
        var text = "{\n" +
                   $"  \"packId\": {JsonString(packId)},\n" +
                   $"  \"plugin\": {JsonString(plugin)},\n" +
                   "  \"format\": 1,\n" +
                   "  \"minBaseVersion\": \"0.1.0\",\n" +
                   $"  \"contentFiles\": [{filesJson}],\n" +
                   $"  \"recordCount\": {count},\n" +
                   $"  \"recordEditors\": [{editorsJson}],\n" +
                   "  \"conflicts\": []\n" +
                   "}\n";
        File.WriteAllText(path, text, Utf8NoBom);
    }

    private static void WriteIni(string path)
    {
        var text = "[General]\n" +
                   "; Implement-first build: probe harness on by default.\n" +
                   "bDebugSelfTest=1\n" +
                   "bDebugTrace=0\n" +
                   "iLootAuthority=0\n" +
            "iGameplayDelaySeconds=15\n" +
            "bEnableRuntimeLoot=1\n" +
            "bEnableContainerLoot=1\n" +
            "bEnableActorRanks=1\n" +
            "bEnableQuestDrops=1\n" +
            "bEnableDebugNatives=0\n" +
            "iContainerLootPerCell=8\n\n" +
                   "[Paths]\n" +
                   "sDataDir=\n" +
                   "sPackDir=\n";
        File.WriteAllText(path, text, Utf8NoBom);
    }

    private static void WriteBuildReport(string path, string report, Content content)
    {
        var sb = new StringBuilder();
        sb.AppendLine("# Runefall build report");
        sb.AppendLine();
        sb.AppendLine($"Generated: {DateTime.UtcNow:u}");
        sb.AppendLine($"Affix families: {content.AffixFamilies} ({content.AffixRows.Count} rows)");
        sb.AppendLine($"Unique affixes: {content.UniqueRows.Count}");
        sb.AppendLine($"Hunt families: {content.RaceGroupRows.Count}");
        sb.AppendLine();
        sb.AppendLine("----");
        sb.AppendLine(report.TrimEnd());
        sb.AppendLine("----");
        File.WriteAllText(path, sb.ToString(), Utf8NoBom);
    }

    private static void Deploy(string dataDir, string targetData)
    {
        foreach (var file in Directory.GetFiles(dataDir, "*", SearchOption.AllDirectories))
        {
            var rel = Path.GetRelativePath(dataDir, file);
            var dst = Path.Combine(targetData, rel);
            Directory.CreateDirectory(Path.GetDirectoryName(dst)!);
            File.Copy(file, dst, true);
        }
    }
}