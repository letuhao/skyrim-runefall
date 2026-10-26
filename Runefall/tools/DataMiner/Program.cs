// DataMiner: read-only mining of Skyrim.esm + official DLC for unique-affix candidate sources.
// Usage:
//   dotnet run --project Runefall\tools\DataMiner -- --data-dir "<Data folder>" --out "<out dir>"
// Writes JSON candidates + candidates.csv + a mining report. Never writes to the game folder.
using System.Text;
using System.Text.Encodings.Web;
using System.Text.Json;
using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Skyrim;

internal static class Program
{
    private static readonly string[] Masters =
    {
        "Skyrim.esm", "Update.esm", "Dawnguard.esm", "HearthFires.esm", "Dragonborn.esm"
    };

    private static readonly System.Text.RegularExpressions.Regex GenericName = new(
        "(?i)\\b(iron|steel|leather|elven|glass|ebony|daedric|dwarven|orcish|hide|fur|studded|scaled|imperial|nord|falmer|forsworn|vampire|ancient nord|chitin|stalhrim|bonemold|carved|nordic|bone|wooden|hunting|long|imperial|iron|steel)\\b.*\\b(sword|dagger|axe|battleaxe|greatsword|warhammer|mace|bow|arrow|helmet|armor|boots|gauntlets|shield|ring|amulet|cuirass|greaves|bracers)\\b",
        System.Text.RegularExpressions.RegexOptions.Compiled);

    private sealed record EffectRow(string mgef, float magnitude, int area, int duration);
    private sealed record EnchRow(string plugin, string formKey, string editorId, string name, List<EffectRow> effects);
    private sealed record ItemRow(string sourceType, string plugin, string formKey, string editorId, string name, string extra, string confidence, string reason);
    private sealed record NpcRow(string plugin, string formKey, string editorId, string name, string confidence, string reason);
    private sealed record QuestRow(string plugin, string formKey, string editorId, string name, int stages, int aliases, string confidence, string reason);
    private sealed record RaceRow(string plugin, string formKey, string editorId, string name, int npcCount);

    private static int Main(string[] args)
    {
        string dataDir = @"H:\Skyrim\MantrasOfMara\Stock Game\Data";
        string outDir = "SPECS\\data\\mining";
        for (int i = 0; i < args.Length; i++)
        {
            if (args[i] == "--data-dir" && i + 1 < args.Length) dataDir = args[++i];
            else if (args[i] == "--out" && i + 1 < args.Length) outDir = args[++i];
        }
        Directory.CreateDirectory(outDir);

        var enchMap = new Dictionary<string, (string editorId, List<EffectRow> effects)>();
        var enchRows = new List<EnchRow>();
        var itemRows = new List<ItemRow>();
        var npcRows = new List<NpcRow>();
        var questRows = new List<QuestRow>();
        var raceRows = new List<RaceRow>();
        var raceNpcCounts = new Dictionary<string, int>();
        var counts = new Dictionary<string, int>();

        foreach (var master in Masters)
        {
            string path = Path.Combine(dataDir, master);
            if (!File.Exists(path)) { Console.WriteLine("MISSING " + path); continue; }
            Console.WriteLine("loading " + master + " ...");
            using var mod = SkyrimMod.CreateFromBinaryOverlay(path, SkyrimRelease.SkyrimSE);
            string plugin = master;

            foreach (var rec in mod.EnumerateMajorRecords())
            {
                switch (rec)
                {
                    case IObjectEffectGetter ench:
                    {
                        var fx = new List<EffectRow>();
                        foreach (var eff in ench.Effects)
                        {
                            var mgefFk = eff.BaseEffect.FormKeyNullable;
                            string mgef = mgefFk.HasValue ? mgefFk.Value.ToString() : "";
                            var d = eff.Data;
                            fx.Add(new EffectRow(mgef, d is null ? 0f : d.Magnitude, d is null ? 0 : d.Area, d is null ? 0 : d.Duration));
                        }
                        string eid = ench.EditorID ?? "";
                        string key = ench.FormKey.ToString();
                        enchMap[key] = (eid, fx);
                        enchRows.Add(new EnchRow(plugin, key, eid, ench.Name?.String ?? "", fx));
                        counts["ench"] = counts.GetValueOrDefault("ench") + 1;
                        break;
                    }
                    case IWeaponGetter w:
                    {
                        itemRows.Add(MakeItem("weapon", plugin, w.FormKey, w.EditorID, w.Name?.String, w.ObjectEffect.FormKeyNullable, enchMap));
                        counts["weapon"] = counts.GetValueOrDefault("weapon") + 1;
                        break;
                    }
                    case IArmorGetter a:
                    {
                        itemRows.Add(MakeItem("armor", plugin, a.FormKey, a.EditorID, a.Name?.String, a.ObjectEffect.FormKeyNullable, enchMap));
                        counts["armor"] = counts.GetValueOrDefault("armor") + 1;
                        break;
                    }
                    case INpcGetter n:
                    {
                        string eid = n.EditorID ?? "";
                        string name = n.Name?.String ?? "";
                        bool uniqueFlag = (n.Configuration.Flags & NpcConfiguration.Flag.Unique) != 0;
                        bool bossPattern = System.Text.RegularExpressions.Regex.IsMatch(eid + " " + name, "(?i)boss|dragon|priest|matriarch|jarl|king|queen|lord|chief|champion|arcmage|general|commander|captain|warden|sentinel|overlord|tyrant|dremora|dragonborn|ancient|elder");
                        string conf = "LOW", reason = "named";
                        if (uniqueFlag && bossPattern) { conf = "HIGH"; reason = "unique+boss"; }
                        else if (uniqueFlag) { conf = "MEDIUM"; reason = "unique-flag"; }
                        else if (bossPattern) { conf = "MEDIUM"; reason = "boss-pattern"; }
                        npcRows.Add(new NpcRow(plugin, n.FormKey.ToString(), eid, name, conf, reason));
                        var raceFk = n.Race.FormKeyNullable;
                        if (raceFk.HasValue)
                        {
                            string rk = raceFk.Value.ToString();
                            raceNpcCounts[rk] = raceNpcCounts.GetValueOrDefault(rk) + 1;
                        }
                        counts["npc"] = counts.GetValueOrDefault("npc") + 1;
                        break;
                    }
                    case IQuestGetter q:
                    {
                        string eid = q.EditorID ?? "";
                        string name = q.Name?.String ?? "";
                        bool pattern = System.Text.RegularExpressions.Regex.IsMatch(eid + " " + name, "(?i)reward|artifact|daedric|main|boss|unique|guild");
                        questRows.Add(new QuestRow(plugin, q.FormKey.ToString(), eid, name, q.Stages.Count, q.Aliases.Count, pattern ? "MEDIUM" : "LOW", pattern ? "reward-pattern" : "quest"));
                        counts["quest"] = counts.GetValueOrDefault("quest") + 1;
                        break;
                    }
                    case IRaceGetter race:
                    {
                        string eid = race.EditorID ?? "";
                        raceRows.Add(new RaceRow(plugin, race.FormKey.ToString(), eid, race.Name?.String ?? "", 0));
                        counts["race"] = counts.GetValueOrDefault("race") + 1;
                        break;
                    }
                }
            }
        }

        foreach (var race in raceRows)
            raceNpcCounts.TryGetValue(race.formKey, out _);
        var enrichedRaces = raceRows.Select(r => r with { npcCount = raceNpcCounts.GetValueOrDefault(r.formKey) })
            .OrderByDescending(r => r.npcCount).ToList();

        var opts = new JsonSerializerOptions { WriteIndented = true, Encoder = JavaScriptEncoder.UnsafeRelaxedJsonEscaping };
        File.WriteAllText(Path.Combine(outDir, "races.json"), JsonSerializer.Serialize(enrichedRaces, opts));
        File.WriteAllText(Path.Combine(outDir, "unique-enchants.json"), JsonSerializer.Serialize(enchRows, opts));
        File.WriteAllText(Path.Combine(outDir, "unique-items.json"), JsonSerializer.Serialize(itemRows, opts));
        File.WriteAllText(Path.Combine(outDir, "unique-npcs.json"), JsonSerializer.Serialize(npcRows, opts));
        File.WriteAllText(Path.Combine(outDir, "quests.json"), JsonSerializer.Serialize(questRows, opts));

        var candidates = new List<ItemRow>();
        candidates.AddRange(itemRows.Where(x => x.confidence != "LOW" || x.extra.Length > 0));
        candidates.AddRange(npcRows.Where(x => x.confidence != "LOW").Select(x => new ItemRow("npc", x.plugin, x.formKey, x.editorId, x.name, "", x.confidence, x.reason)));
        candidates.AddRange(questRows.Where(x => x.confidence != "LOW").Select(x => new ItemRow("quest", x.plugin, x.formKey, x.editorId, x.name, "", x.confidence, x.reason)));
        var csv = new StringBuilder();
        csv.AppendLine("sourceType,plugin,formKey,editorId,name,extra,confidence,reason");
        foreach (var r in candidates)
            csv.AppendLine(string.Join(",", new[] { r.sourceType, r.plugin, r.formKey, Esc(r.editorId), Esc(r.name), Esc(r.extra), r.confidence, r.reason }));
        File.WriteAllText(Path.Combine(outDir, "candidates.csv"), csv.ToString());

        int highItems = itemRows.Count(x => x.confidence == "HIGH");
        int medItems = itemRows.Count(x => x.confidence == "MEDIUM");
        int highNpcs = npcRows.Count(x => x.confidence == "HIGH");
        int medQuests = questRows.Count(x => x.confidence == "MEDIUM");
        var report = new StringBuilder();
        report.AppendLine("# Data mining report");
        report.AppendLine();
        report.AppendLine("Generated by Runefall\\tools\\DataMiner from: " + string.Join(", ", Masters));
        report.AppendLine("Data dir: " + dataDir);
        report.AppendLine();
        report.AppendLine("## Counts");
        foreach (var kv in counts.OrderBy(k => k.Key)) report.AppendLine("- " + kv.Key + ": " + kv.Value);
        report.AppendLine("- HIGH items: " + highItems + "; MEDIUM items: " + medItems);
        report.AppendLine("- HIGH npcs: " + highNpcs);
        report.AppendLine("- MEDIUM quest candidates: " + medQuests);
        report.AppendLine();
        report.AppendLine("## Output files");
        report.AppendLine("- unique-enchants.json / unique-items.json / unique-npcs.json / quests.json");
        report.AppendLine("- candidates.csv (HIGH/MEDIUM sources only)");
        File.WriteAllText(Path.Combine(outDir, "MINING-REPORT.md"), report.ToString());

        Console.WriteLine("counts: " + string.Join(", ", counts.OrderBy(k => k.Key).Select(kv => kv.Key + "=" + kv.Value)));
        Console.WriteLine("HIGH items=" + highItems + " MEDIUM items=" + medItems + " HIGH npcs=" + highNpcs + " MEDIUM quests=" + medQuests);
        Console.WriteLine("wrote to " + Path.GetFullPath(outDir));
        return 0;
    }

    private static ItemRow MakeItem(string type, string plugin, FormKey key, string? editorId, string? name,
        FormKey? enchFk, Dictionary<string, (string editorId, List<EffectRow> effects)> enchMap)
    {
        string eid = editorId ?? "";
        string nm = name ?? "";
        string extra = "";
        if (enchFk.HasValue && enchMap.TryGetValue(enchFk.Value.ToString(), out var e)) extra = e.editorId;
        bool named = nm.Length > 0;
        bool generic = GenericName.IsMatch(nm);
        bool prefix = System.Text.RegularExpressions.Regex.IsMatch(eid, "^(DA|MQ|TG|DB|CW|DLC1|DLC2)");
        bool uniqueName = named && !generic;
        string conf = "LOW", reason = named ? "named-generic" : "unnamed";
        if (uniqueName && (prefix || extra.Length > 0 || nm.Contains("'s") || nm.Contains("of the"))) { conf = "HIGH"; reason = "unique-name"; }
        else if (uniqueName) { conf = "MEDIUM"; reason = "unique-name-plain"; }
        return new ItemRow(type, plugin, key.ToString(), eid, nm, extra, conf, reason);
    }

    private static string Esc(string s) => s.Replace("\"", "\\\"").Replace(",", ";");
}
