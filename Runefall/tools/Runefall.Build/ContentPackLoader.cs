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
    public List<string[]> RaceRuleRows { get; } = new();
    public List<string[]> ProtectionRows { get; } = new();
    public List<string[]> PackageRows { get; } = new();
    public List<string[]> EquipmentRows { get; } = new();
    public List<string[]> ProgressionRows { get; } = new();
    public List<string[]> RatingRows { get; } = new();
    public List<string[]> PaceRows { get; } = new();
    public List<string[]> DisplayRows { get; } = new();
    public List<string[]> PenetrationRows { get; } = new();
    public List<string[]> WorldBandRows { get; } = new();
    public List<string[]> EncounterRows { get; } = new();
    public List<string[]> RareRows { get; } = new();
    public List<string[]> ContractRows { get; } = new();
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
            ReadRaceRules(pack, files, errors);
            ReadProtection(pack, files, errors);
            ReadPackages(pack, files, errors);
            ReadEquipment(pack, files, errors);
            ReadPlayerProgression(pack, files, errors);
            ReadRatings(pack, files, errors);
            ReadPenetration(pack, files, errors);
            ReadWorldBands(pack, files, errors);
            ReadEncounters(pack, files, errors);
            ReadRareSpawns(pack, files, errors);
            ReadHuntContracts(pack, files, errors);
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


    private static void ReadRaceRules(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("raceClassificationRules", files, errors, "rules");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"race_classification_rules.rules[{index++}]";
            pack.RaceRuleRows.Add(new[]
            {
                I(Int(item, "ruleId", path, errors)), Text(item, "keywordEditorId", path, errors),
                Text(item, "roleKey", path, errors), Text(item, "family", path, errors),
                B(Bool(item, "hostEligible", path, errors)), B(Bool(item, "protectionEligible", path, errors)),
                B(Bool(item, "provisional", path, errors)), I(Int(item, "priority", path, errors))
            });
        }
        if (pack.RaceRuleRows.Count == 0) errors.Add("race_classification_rules: no rows");
    }


    private static void ReadProtection(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("threatProtection", files, errors, "protections");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"threat_protection.protections[{index++}]";
            pack.ProtectionRows.Add(new[]
            {
                Text(item, "profileId", path, errors), F(Number(item, "cap", path, errors)),
                F(Number(item, "perStage", path, errors)), F(Number(item, "floor", path, errors)),
                F(Number(item, "dotFactor", path, errors)), B(Bool(item, "playerProtected", path, errors)),
                B(Bool(item, "anomalyProtected", path, errors)), I(Int(item, "penetrationCap", path, errors)),
                B(Bool(item, "enabled", path, errors))
            });
        }
        if (pack.ProtectionRows.Count == 0) errors.Add("threat_protection: no rows");
    }


    private static void ReadPackages(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("enemyPackages", files, errors, "packages");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"enemy_packages.packages[{index++}]";
            var spells = string.Join("|", item.TryGetProperty("spellEditorIds", out var spellArray) && spellArray.ValueKind == JsonValueKind.Array ? spellArray.EnumerateArray().Select(x => x.GetString() ?? "") : Enumerable.Empty<string>());
            var perks = string.Join("|", item.TryGetProperty("perkEditorIds", out var perkArray) && perkArray.ValueKind == JsonValueKind.Array ? perkArray.EnumerateArray().Select(x => x.GetString() ?? "") : Enumerable.Empty<string>());
            var elite = string.Join("|", item.TryGetProperty("eliteSpellEditorIds", out var eliteArray) && eliteArray.ValueKind == JsonValueKind.Array ? eliteArray.EnumerateArray().Select(x => x.GetString() ?? "") : Enumerable.Empty<string>());
            var boss = string.Join("|", item.TryGetProperty("bossSpellEditorIds", out var bossArray) && bossArray.ValueKind == JsonValueKind.Array ? bossArray.EnumerateArray().Select(x => x.GetString() ?? "") : Enumerable.Empty<string>());
            pack.PackageRows.Add(new[]
            {
                Text(item, "packageId", path, errors), Text(item, "roleKey", path, errors),
                I(Int(item, "stageMin", path, errors)), I(Int(item, "stageMax", path, errors)),
                spells, perks, elite, boss, Text(item, "lootTable", path, errors), F(Number(item, "weight", path, errors)), F(Number(item, "budgetCost", path, errors))
            });
        }
        if (pack.PackageRows.Count == 0) errors.Add("enemy_packages: no rows");
    }


    private static void ReadEquipment(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("equipmentSets", files, errors, "sets");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"equipment_sets.sets[{index++}]";
            var items = string.Join("|", item.TryGetProperty("itemEditorIds", out var itemArray) && itemArray.ValueKind == JsonValueKind.Array ? itemArray.EnumerateArray().Select(x => x.GetString() ?? "") : Enumerable.Empty<string>());
            var perks = string.Join("|", item.TryGetProperty("perkEditorIds", out var perkArray) && perkArray.ValueKind == JsonValueKind.Array ? perkArray.EnumerateArray().Select(x => x.GetString() ?? "") : Enumerable.Empty<string>());
            pack.EquipmentRows.Add(new[]
            {
                Text(item, "setId", path, errors), Text(item, "roleKey", path, errors),
                I(Int(item, "stageMin", path, errors)), I(Int(item, "stageMax", path, errors)),
                items, perks, F(Number(item, "weight", path, errors)), F(Number(item, "applyChance", path, errors))
            });
        }
        if (pack.EquipmentRows.Count == 0) errors.Add("equipment_sets: no rows");
    }


    private static void ReadPlayerProgression(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        if (!files.TryGetValue("playerProgression", out var path))
        {
            errors.Add("pack missing entity 'playerProgression'");
            return;
        }
        try
        {
            using var document = JsonDocument.Parse(File.ReadAllText(path));
            var root = document.RootElement;
            if (root.GetProperty("schemaVersion").GetInt32() != 1)
            {
                errors.Add($"{path}: unsupported schemaVersion");
                return;
            }
            var bonuses = string.Join("|", root.GetProperty("breakthroughBonuses").EnumerateArray().Select(x => x.GetInt32().ToString(CultureInfo.InvariantCulture)));
            pack.ProgressionRows.Add(new[]
            {
                I(Int(root, "pointsPerLevel", "player_progression", errors)),
                I(Int(root, "maxRuneLevel", "player_progression", errors)),
                bonuses
            });
        }
        catch (Exception exception)
        {
            errors.Add($"{path}: {exception.Message}");
        }
    }


    private static void ReadRatings(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var rating = Root("ratingProfiles", files, errors, "profiles");
        if (rating is not null)
        {
            var index = 0;
            foreach (var item in rating.Value.EnumerateArray())
            {
                var path = $"rating_profiles.profiles[{index++}]";
                pack.RatingRows.Add(new[]
                {
                    Text(item, "profileId", path, errors), F(Number(item, "baseDPS", path, errors)),
                    F(Number(item, "baseEHP", path, errors)), F(Number(item, "offenseWeight", path, errors)),
                    F(Number(item, "defenseWeight", path, errors)), F(Number(item, "utilityWeight", path, errors)),
                    F(Number(item, "mitigationCap", path, errors)), F(Number(item, "sustainCap", path, errors))
                });
            }
        }
        var pace = Root("combatPaceProfiles", files, errors, "profiles");
        if (pace is not null)
        {
            var index = 0;
            foreach (var profile in pace.Value.EnumerateArray())
            {
                var profileId = Text(profile, "profileId", $"combat_pace.profiles[{index}]", errors);
                var bands = profile.GetProperty("bands");
                var bandIndex = 0;
                foreach (var band in bands.EnumerateArray())
                {
                    var path = $"combat_pace.profiles[{index}].bands[{bandIndex++}]";
                    pack.PaceRows.Add(new[]
                    {
                        profileId, Text(band, "encounterClass", path, errors),
                        F(Number(band, "ttkMin", path, errors)), F(Number(band, "ttkMax", path, errors)),
                        F(Number(band, "ttdMin", path, errors)), F(Number(band, "ttdMax", path, errors)),
                        F(Number(band, "qMin", path, errors)), F(Number(band, "qMax", path, errors))
                    });
                }
                ++index;
            }
        }
        var display = Root("ratingDisplayProfiles", files, errors, "profiles");
        if (display is not null)
        {
            var index = 0;
            foreach (var item in display.Value.EnumerateArray())
            {
                var path = $"rating_display.profiles[{index++}]";
                pack.DisplayRows.Add(new[]
                {
                    Text(item, "profileId", path, errors), B(Bool(item, "numberVisible", path, errors)),
                    B(Bool(item, "labelVisible", path, errors)), I(Int(item, "decimals", path, errors))
                });
            }
        }
    }


    private static void ReadPenetration(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("penetrationProfiles", files, errors, "profiles");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"penetration_profiles.profiles[{index++}]";
            pack.PenetrationRows.Add(new[]
            {
                Text(item, "profileId", path, errors), I(Int(item, "base", path, errors)),
                I(Int(item, "per10Stages", path, errors)), I(Int(item, "cap", path, errors)),
                B(Bool(item, "mageChannel", path, errors)), B(Bool(item, "weaponChannel", path, errors)),
                B(Bool(item, "consumableChannel", path, errors))
            });
        }
    }


    private static void ReadWorldBands(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("worldBands", files, errors, "bands");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"world_bands.bands[{index++}]";
            pack.WorldBandRows.Add(new[]
            {
                Text(item, "bandId", path, errors), Text(item, "matchType", path, errors),
                Text(item, "matchValue", path, errors), I(Int(item, "stageMin", path, errors)),
                I(Int(item, "stageMax", path, errors)), F(Number(item, "rate", path, errors)),
                Text(item, "exclusions", path, errors)
            });
        }
    }


    private static void ReadEncounters(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("encounterTables", files, errors, "encounters");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"encounter_tables.encounters[{index++}]";
            pack.EncounterRows.Add(new[]
            {
                Text(item, "encounterId", path, errors), Text(item, "bandId", path, errors),
                Text(item, "class", path, errors), Text(item, "actorEditorId", path, errors),
                I(Int(item, "stageMin", path, errors)), I(Int(item, "stageMax", path, errors)),
                F(Number(item, "weight", path, errors)), I(Int(item, "maxPerCell", path, errors)),
                I(Int(item, "cooldownSeconds", path, errors)), B(Bool(item, "enabled", path, errors))
            });
        }
    }


    private static void ReadRareSpawns(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("rareSpawns", files, errors, "rareSpawns");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"rare_spawns.rareSpawns[{index++}]";
            pack.RareRows.Add(new[]
            {
                Text(item, "rareId", path, errors), Text(item, "bandId", path, errors),
                Text(item, "actorEditorId", path, errors), I(Int(item, "chancePermille", path, errors)),
                I(Int(item, "stageMin", path, errors)), I(Int(item, "stageMax", path, errors)),
                Text(item, "conditionTag", path, errors), F(Number(item, "weight", path, errors)),
                B(Bool(item, "enabled", path, errors))
            });
        }
    }


    private static void ReadHuntContracts(LoadedPack pack, Dictionary<string, string> files, List<string> errors)
    {
        var array = Root("huntContracts", files, errors, "contracts");
        if (array is null) return;
        var index = 0;
        foreach (var item in array.Value.EnumerateArray())
        {
            var path = $"hunt_contracts.contracts[{index++}]";
            pack.ContractRows.Add(new[]
            {
                Text(item, "contractId", path, errors), Text(item, "family", path, errors),
                I(Int(item, "minStage", path, errors)), I(Int(item, "maxStage", path, errors)),
                I(Int(item, "rewardRunes", path, errors)), I(Int(item, "cooldownSeconds", path, errors)),
                I(Int(item, "durationSeconds", path, errors)), B(Bool(item, "enabled", path, errors))
            });
        }
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
