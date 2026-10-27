using System.Globalization;
using System.Text.Json;

namespace Runefall.Build;

internal sealed class LoadedPack
{
    public List<string[]> StageRows { get; } = new();
    public List<string[]> TierRows { get; } = new();
    public List<string[]> CurveRows { get; } = new();
    public List<string[]> DifficultyRows { get; } = new();
    public List<string[]> AssignmentRows { get; } = new();
    public List<string[]> ContentBandRows { get; } = new();
    public List<string[]> LegacyTierRows { get; } = new();
    public List<string[]> RoleRows { get; } = new();
    public List<string[]> RaceProfileRows { get; } = new();
}

internal static class ContentPackLoader
{
    public static LoadedPack? Load(string packPath, out List<string> errors)
    {
        errors = new List<string>();
        var packFile = new FileInfo(packPath);
        if (!packFile.Exists)
        {
            errors.Add($"{packPath}: pack.json not found");
            return null;
        }
        try
        {
            using var document = JsonDocument.Parse(File.ReadAllText(packFile.FullName));
            var root = document.RootElement;
            if (root.GetProperty("schemaVersion").GetInt32() != 1)
            {
                errors.Add($"{packFile.FullName}: unsupported schemaVersion");
                return null;
            }
            var content = root.GetProperty("content");
            var files = new Dictionary<string, string>(StringComparer.Ordinal);
            foreach (var entry in content.EnumerateArray())
            {
                var entity = entry.GetProperty("entity").GetString() ?? "";
                var file = entry.GetProperty("file").GetString() ?? "";
                if (entity.Length == 0 || file.Length == 0)
                {
                    errors.Add($"{packFile.FullName}: content entry missing entity/file");
                    continue;
                }
                files[entity] = Path.Combine(packFile.DirectoryName!, file);
            }
            var pack = new LoadedPack();
            ReadStage(pack, files, errors);
            ReadTiers(pack, files, errors);
            ReadCurves(pack, files, errors);
            ReadDifficulty(pack, files, errors);
            ReadAssignment(pack, files, errors);
            ReadBands(pack, files, errors);
            ReadLegacy(pack, files, errors);
            ReadRoles(pack, files, errors);
            ReadRaceProfiles(pack, files, errors);
            return errors.Count == 0 ? pack : null;
        }
        catch (Exception exception)
        {
            errors.Add($"{packFile.FullName}: {exception.Message}");
            return null;
        }
    }

    private static JsonElement? Root(string entity, Dictionary<string, string> files, List<string> errors, string arrayName)
    {
        if (!files.TryGetValue(entity, out var path))
        {
            errors.Add($"pack missing entity '{entity}'");
            return null;
        }
        try
        {
            using var document = JsonDocument.Parse(File.ReadAllText(path));
            var root = document.RootElement;
            if (root.GetProperty("schemaVersion").GetInt32() != 1)
            {
                errors.Add($"{path}: unsupported schemaVersion");
                return null;
            }
            if (!root.TryGetProperty(arrayName, out var array) || array.ValueKind != JsonValueKind.Array)
            {
                errors.Add($"{path}: missing array '{arrayName}'");
                return null;
            }
            return array.Clone();
        }
        catch (Exception exception)
        {
            errors.Add($"{path}: {exception.Message}");
            return null;
        }
    }

    private static string Text(JsonElement element, string property, string path, List<string> errors)
    {
        if (element.TryGetProperty(property, out var value) && value.ValueKind == JsonValueKind.String)
            return value.GetString() ?? "";
        errors.Add($"{path}: missing string '{property}'");
        return "";
    }

    private static int Int(JsonElement element, string property, string path, List<string> errors, int fallback = 0)
    {
        if (element.TryGetProperty(property, out var value) && value.TryGetInt32(out var parsed))
            return parsed;
        errors.Add($"{path}: missing integer '{property}'");
        return fallback;
    }

    private static double Number(JsonElement element, string property, string path, List<string> errors)
    {
        if (element.TryGetProperty(property, out var value) && value.TryGetDouble(out var parsed))
            return parsed;
        errors.Add($"{path}: missing number '{property}'");
        return 0.0;
    }

    private static bool Bool(JsonElement element, string property, string path, List<string> errors)
    {
        if (element.TryGetProperty(property, out var value) && (value.ValueKind == JsonValueKind.True || value.ValueKind == JsonValueKind.False))
            return value.GetBoolean();
        errors.Add($"{path}: missing boolean '{property}'");
        return false;
    }

    private static string F(double value) => value.ToString("0.####", CultureInfo.InvariantCulture);
    private static string I(int value) => value.ToString(CultureInfo.InvariantCulture);
    private static string B(bool value) => value ? "1" : "0";

    private static void ReadStage(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("stageLadder", files, errors, "stages");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"stage_ladder.stages[{index}]";
            pack.StageRows.Add(new[]
            {
                I(Int(item, "stage", path, errors)), I(Int(item, "tier", path, errors)), I(Int(item, "curveId", path, errors)),
                F(Number(item, "powerIndex", path, errors)), F(Number(item, "healthMult", path, errors)),
                F(Number(item, "damageMult", path, errors)), F(Number(item, "regenMult", path, errors)),
                F(Number(item, "armorMult", path, errors)), I(Int(item, "statCapacity", path, errors)),
                I(Int(item, "offenseCapacity", path, errors)), I(Int(item, "defenseCapacity", path, errors)),
                I(Int(item, "utilityCapacity", path, errors)), I(Int(item, "abilityCapacity", path, errors)),
                I(Int(item, "lootCapacity", path, errors)), I(Int(item, "rewardCapacity", path, errors)),
                I(Int(item, "encounterCost", path, errors))
            });
            ++index;
        }
        if (pack.StageRows.Count != 100) errors.Add($"stage_ladder: expected 100 rows, got {pack.StageRows.Count}");
    }

    private static void ReadTiers(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("tierDefinitions", files, errors, "tiers");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"tier_definitions.tiers[{index++}]";
            pack.TierRows.Add(new[]
            {
                I(Int(item, "tier", path, errors)), Text(item, "name", path, errors), Text(item, "nameKey", path, errors),
                I(Int(item, "stageMin", path, errors)), I(Int(item, "stageMax", path, errors)),
                Text(item, "keywordEditorId", path, errors), Text(item, "breakpointTag", path, errors),
                I(Int(item, "budgetStep", path, errors)), Text(item, "unlockTag", path, errors), Text(item, "description", path, errors)
            });
        }
        if (pack.TierRows.Count != 10) errors.Add($"tier_definitions: expected 10 rows, got {pack.TierRows.Count}");
    }

    private static void ReadCurves(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("curveProfiles", files, errors, "curves");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"curve_profiles.curves[{index++}]";
            pack.CurveRows.Add(new[]
            {
                I(Int(item, "curveId", path, errors)), F(Number(item, "powerBase", path, errors)),
                F(Number(item, "healthExp", path, errors)), F(Number(item, "damageExp", path, errors)),
                F(Number(item, "regenExp", path, errors)), F(Number(item, "armorExp", path, errors)),
                I(Int(item, "maxStage", path, errors)), B(Bool(item, "enabled", path, errors)), Text(item, "notes", path, errors)
            });
        }
    }

    private static void ReadDifficulty(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("difficultyProfiles", files, errors, "profiles");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"difficulty_profiles.profiles[{index++}]";
            pack.DifficultyRows.Add(new[]
            {
                I(Int(item, "profileId", path, errors)), Text(item, "name", path, errors), Text(item, "nameKey", path, errors),
                I(Int(item, "stageDelta", path, errors)), F(Number(item, "statMult", path, errors)),
                F(Number(item, "damageDealtMult", path, errors)), F(Number(item, "damageTakenMult", path, errors)),
                F(Number(item, "xpMult", path, errors)), F(Number(item, "rewardMult", path, errors)),
                F(Number(item, "dropMult", path, errors)), I(Int(item, "maxStage", path, errors)),
                B(Bool(item, "enabled", path, errors)), Text(item, "notes", path, errors)
            });
        }
    }

    private static void ReadAssignment(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("assignmentRules", files, errors, "rules");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"stage_assignment_rules.rules[{index++}]";
            pack.AssignmentRows.Add(new[]
            {
                I(Int(item, "ruleId", path, errors)), Text(item, "source", path, errors), Text(item, "appliesTo", path, errors),
                I(Int(item, "minStage", path, errors)), I(Int(item, "maxStage", path, errors)),
                I(Int(item, "offsetMin", path, errors)), I(Int(item, "offsetMax", path, errors)),
                I(Int(item, "jitter", path, errors)), Text(item, "seedSalt", path, errors),
                I(Int(item, "priority", path, errors)), B(Bool(item, "enabled", path, errors)), Text(item, "notes", path, errors)
            });
        }
    }

    private static void ReadBands(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("contentBands", files, errors, "bands");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"stage_content_bands.bands[{index++}]";
            pack.ContentBandRows.Add(new[]
            {
                I(Int(item, "stageMin", path, errors)), I(Int(item, "stageMax", path, errors)),
                I(Int(item, "tier", path, errors)), I(Int(item, "playerLevelMin", path, errors)),
                I(Int(item, "playerLevelMax", path, errors)), I(Int(item, "itemLevelMin", path, errors)),
                I(Int(item, "itemLevelMax", path, errors)), Text(item, "gearCheckBand", path, errors),
                Text(item, "unlockTag", path, errors), B(Bool(item, "v1Populated", path, errors)), Text(item, "notes", path, errors)
            });
        }
        if (pack.ContentBandRows.Count != 10) errors.Add($"stage_content_bands: expected 10 rows, got {pack.ContentBandRows.Count}");
    }


    private static void ReadRoles(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("roles", files, errors, "roles");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"roles.roles[{index++}]";
            var tags = "";
            if (item.TryGetProperty("tags", out var tagArray) && tagArray.ValueKind == JsonValueKind.Array)
                tags = string.Join("|", tagArray.EnumerateArray().Select(t => t.GetString() ?? ""));
            else
                errors.Add($"{path}: missing tags");
            pack.RoleRows.Add(new[]
            {
                Text(item, "key", path, errors),
                item.TryGetProperty("parentRole", out var parent) && parent.ValueKind == JsonValueKind.String ? parent.GetString() ?? "" : "",
                tags,
                Text(item, "hostPolicy", path, errors),
                Text(item, "protectionPolicy", path, errors)
            });
        }
        if (pack.RoleRows.Count == 0) errors.Add("roles: no rows");
    }

    private static void ReadRaceProfiles(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("raceProfiles", files, errors, "profiles");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"race_profiles.profiles[{index++}]";
            var editorId = "";
            if (item.TryGetProperty("raceRef", out var raceRef) && raceRef.ValueKind == JsonValueKind.Object)
                editorId = Text(raceRef, "editorId", path + ".raceRef", errors);
            else
                errors.Add($"{path}: missing raceRef.editorId");
            var roles = "";
            if (item.TryGetProperty("roles", out var roleArray) && roleArray.ValueKind == JsonValueKind.Array)
                roles = string.Join("|", roleArray.EnumerateArray().Select(r => r.GetString() ?? ""));
            else
                errors.Add($"{path}: missing roles");
            pack.RaceProfileRows.Add(new[]
            {
                Text(item, "key", path, errors), editorId, roles, Text(item, "family", path, errors), I(Int(item, "priority", path, errors))
            });
        }
        if (pack.RaceProfileRows.Count == 0) errors.Add("race_profiles: no rows");
    }

    private static void ReadLegacy(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("legacyTierMap", files, errors, "tiers");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"legacy_rf_tier_map.tiers[{index++}]";
            pack.LegacyTierRows.Add(new[]
            {
                I(Int(item, "rfTier", path, errors)), I(Int(item, "stageMin", path, errors)),
                I(Int(item, "stageMax", path, errors)), I(Int(item, "playerRankStage", path, errors)), Text(item, "notes", path, errors)
            });
        }
        if (pack.LegacyTierRows.Count != 6) errors.Add($"legacy_rf_tier_map: expected 6 rows, got {pack.LegacyTierRows.Count}");
    }
}
