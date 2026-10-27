using System.Globalization;
using System.Text;

namespace Runefall.Build;

internal static class LadderContent
{
    public static readonly string[] StageColumns =
    {
        "stage", "tier", "curveId", "powerIndex", "healthMult", "damageMult", "regenMult", "armorMult",
        "statCapacity", "offenseCapacity", "defenseCapacity", "utilityCapacity", "abilityCapacity",
        "lootCapacity", "rewardCapacity", "encounterCost"
    };

    public static readonly string[] TierColumns =
    {
        "tier", "name", "nameKey", "stageMin", "stageMax", "keywordEditorId", "breakpointTag",
        "budgetStep", "unlockTag", "description"
    };

    public static readonly string[] CurveColumns =
    {
        "curveId", "powerBase", "healthExp", "damageExp", "regenExp", "armorExp", "maxStage", "enabled", "notes"
    };

    public static readonly string[] DifficultyColumns =
    {
        "profileId", "name", "nameKey", "stageDelta", "statMult", "damageDealtMult", "damageTakenMult",
        "xpMult", "rewardMult", "dropMult", "maxStage", "enabled", "notes"
    };

    public static readonly string[] AssignmentColumns =
    {
        "ruleId", "source", "appliesTo", "minStage", "maxStage", "offsetMin", "offsetMax", "jitter",
        "seedSalt", "priority", "enabled", "notes"
    };

    public static readonly string[] ContentBandColumns =
    {
        "stageMin", "stageMax", "tier", "playerLevelMin", "playerLevelMax", "itemLevelMin", "itemLevelMax",
        "gearCheckBand", "unlockTag", "v1Populated", "notes"
    };

    public static readonly string[] LegacyTierColumns =
    {
        "rfTier", "stageMin", "stageMax", "playerRankStage", "notes"
    };

    public static void WriteAll(string dir)
    {
        Directory.CreateDirectory(dir);
        WriteCsv(dir, "stage_ladder.csv", StageColumns, StageLadderRows());
        WriteCsv(dir, "tier_definitions.csv", TierColumns, TierRows());
        WriteCsv(dir, "curve_profiles.csv", CurveColumns, CurveRows());
        WriteCsv(dir, "difficulty_profiles.csv", DifficultyColumns, DifficultyRows());
        WriteCsv(dir, "stage_assignment_rules.csv", AssignmentColumns, AssignmentRows());
        WriteCsv(dir, "stage_content_bands.csv", ContentBandColumns, ContentBandRows());
        WriteCsv(dir, "legacy_rf_tier_map.csv", LegacyTierColumns, LegacyRows());
    }

    private static double Power(int stage) => Math.Pow(15.0, (stage - 1) / 99.0);
    private static double Mult(int stage, double exponent) => Math.Pow(Power(stage), exponent);
    private static string F(double value) => value.ToString("0.0000", CultureInfo.InvariantCulture);
    private static string C(double value) => ((long)Math.Round(value, MidpointRounding.AwayFromZero)).ToString(CultureInfo.InvariantCulture);

    private static string[] TierNames =
    {
        "Runetouched", "Runebound", "Runeforged", "Runecrowned", "Runelord",
        "Runebreaker", "Runefall Cataclysm", "Runewarden", "Runesovereign", "Runefall Ascendant"
    };

    private static List<string[]> StageLadderRows()
    {
        var rows = new List<string[]>();
        for (var stage = 1; stage <= 100; stage++)
        {
            var p = Power(stage);
            var tier = (stage - 1) / 10 + 1;
            rows.Add(new[]
            {
                stage.ToString(CultureInfo.InvariantCulture),
                tier.ToString(CultureInfo.InvariantCulture),
                "1",
                F(p),
                F(Math.Pow(p, 0.65)),
                F(Math.Pow(p, 0.45)),
                F(Math.Pow(p, 0.30)),
                F(Math.Pow(p, 0.25)),
                C(100.0 * p),
                C(100.0 * Math.Pow(p, 0.45)),
                C(100.0 * Math.Pow(p, 0.65)),
                C(100.0 * Math.Pow(p, 0.30)),
                C(100.0 * Math.Pow(p, 0.25)),
                C(100.0 * Math.Pow(p, 0.50)),
                C(100.0 * Math.Pow(p, 0.40)),
                C(10.0 * Math.Pow(p, 0.35))
            });
        }
        return rows;
    }

    private static List<string[]> TierRows()
    {
        var rows = new List<string[]>();
        for (var tier = 1; tier <= 10; tier++)
        {
            var min = (tier - 1) * 10 + 1;
            var max = tier * 10;
            var key = "RF_KW_Tier_" + tier.ToString("00", CultureInfo.InvariantCulture);
            var tag = "RF_TIER_" + tier.ToString("00", CultureInfo.InvariantCulture);
            var name = TierNames[tier - 1];
            rows.Add(new[]
            {
                tier.ToString(CultureInfo.InvariantCulture), name, tag + "_NAME", min.ToString(CultureInfo.InvariantCulture),
                max.ToString(CultureInfo.InvariantCulture), key, tag, "100", tag, "Runefall power tier " + tier
            });
        }
        return rows;
    }

    private static List<string[]> CurveRows() => new()
    {
        new[] { "1", "15", "0.65", "0.45", "0.30", "0.25", "100", "1", "Runefall host curve (D-065/D-068)" }
    };

    private static List<string[]> DifficultyRows() => new()
    {
        new[] { "0", "Novice", "RF_DIFF_00", "-5", "0.80", "0.80", "1.20", "0.80", "0.80", "0.80", "100", "1", "easy" },
        new[] { "1", "Adept", "RF_DIFF_01", "0", "1.00", "1.00", "1.00", "1.00", "1.00", "1.00", "100", "1", "baseline" },
        new[] { "2", "Expert", "RF_DIFF_02", "5", "1.20", "1.20", "0.90", "1.20", "1.20", "1.10", "100", "1", "hard" },
        new[] { "3", "Master", "RF_DIFF_03", "10", "1.40", "1.40", "0.80", "1.40", "1.30", "1.20", "100", "1", "harder" },
        new[] { "4", "Legendary", "RF_DIFF_04", "15", "1.70", "1.70", "0.70", "1.70", "1.50", "1.30", "100", "1", "hardest" }
    };

    private static List<string[]> AssignmentRows() => new()
    {
        new[] { "1", "worldband", "any", "1", "100", "0", "0", "0", "w", "100", "1", "world band default" },
        new[] { "2", "encounter", "any", "1", "100", "0", "2", "1", "e", "200", "1", "encounter offset" },
        new[] { "3", "override", "any", "1", "100", "0", "0", "0", "o", "1000", "1", "explicit override" },
        new[] { "4", "playerrank", "player", "1", "100", "0", "0", "0", "p", "150", "1", "player rank input" },
        new[] { "5", "default", "any", "1", "100", "0", "0", "0", "d", "0", "1", "fallback" }
    };

    private static List<string[]> ContentBandRows()
    {
        var rows = new List<string[]>();
        for (var tier = 1; tier <= 10; tier++)
        {
            var min = (tier - 1) * 10 + 1;
            var max = tier * 10;
            var tag = "RF_TIER_" + tier.ToString("00", CultureInfo.InvariantCulture);
            rows.Add(new[]
            {
                min.ToString(CultureInfo.InvariantCulture), max.ToString(CultureInfo.InvariantCulture),
                tier.ToString(CultureInfo.InvariantCulture), min.ToString(CultureInfo.InvariantCulture),
                max.ToString(CultureInfo.InvariantCulture), min.ToString(CultureInfo.InvariantCulture),
                max.ToString(CultureInfo.InvariantCulture), "Band" + tier.ToString("00", CultureInfo.InvariantCulture),
                tag, "1", "v1 coverage stub"
            });
        }
        return rows;
    }

    private static List<string[]> LegacyRows() => new()
    {
        new[] { "0", "1", "15", "1", "POC RF tier 0" },
        new[] { "1", "16", "30", "20", "POC RF tier 1" },
        new[] { "2", "31", "45", "40", "POC RF tier 2" },
        new[] { "3", "46", "60", "60", "POC RF tier 3" },
        new[] { "4", "61", "80", "80", "POC RF tier 4" },
        new[] { "5", "81", "100", "100", "POC RF tier 5" }
    };

    private static void WriteCsv(string dir, string name, string[] columns, List<string[]> rows)
    {
        var sb = new StringBuilder();
        sb.AppendLine(string.Join(",", columns));
        foreach (var row in rows)
            sb.AppendLine(string.Join(",", row.Select(Escape)));
        File.WriteAllText(Path.Combine(dir, name), sb.ToString(), new UTF8Encoding(false));
    }

    private static string Escape(string value) =>
        value.Contains(',') || value.Contains('"') || value.Contains('\n')
            ? "\"" + value.Replace("\"", "\"\"") + "\""
            : value;
}
