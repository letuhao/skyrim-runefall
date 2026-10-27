using System.Globalization;

namespace Runefall.Build;

internal sealed class Content
{
    public int AffixFamilies { get; set; } = 120;

    public string[] AffixColumns { get; } = { "affixId", "name", "category", "tier", "unit", "slots", "minValue", "maxValue", "duration", "area", "weight", "minItemLevel", "effectEditorId", "prefix", "suffix", "description" };
    public List<string[]> AffixRows { get; } = new();

    public string[] RarityColumns { get; } = { "rarityId", "name", "enhancementCount", "socketMin", "socketMax", "colorToken", "prefixStyle" };
    public List<string[]> RarityRows { get; } = new();

    public string[] RarityWeightColumns { get; } = { "rfTier", "rarityId", "weight" };
    public List<string[]> RarityWeightRows { get; } = new();

    public string[] RankColumns { get; } = { "rank", "name", "hpPct", "dmgPct", "armorPct", "affixCount", "itemRarityBonus", "runeRank", "runeMin", "runeMax", "materialTierBonus" };
    public List<string[]> RankRows { get; } = new();

    public string[] SpawnColumns { get; } = { "rfTier", "rank", "weight" };
    public List<string[]> SpawnRows { get; } = new();

    public string[] GeneralDropColumns { get; } = { "rank", "rfTier", "itemChancePct", "itemCountMin", "itemCountMax", "rarityBias", "runeMin", "runeMax", "uniqueChancePct" };
    public List<string[]> GeneralDropRows { get; } = new();

    public string[] ContainerDropColumns { get; } = { "containerTier", "itemChancePct", "rarityBias", "itemCountMin", "itemCountMax", "runeMin", "runeMax", "extraRollChancePct" };
    public List<string[]> ContainerDropRows { get; } = new();

    public string[] QuestDropColumns { get; } = { "questBand", "itemCount", "minRarity", "runeMin", "runeMax" };
    public List<string[]> QuestDropRows { get; } = new();

    public string[] BossDropColumns { get; } = { "bossId", "itemCount", "guaranteedRarity", "runeMin", "runeMax", "materialTier", "uniqueTableId" };
    public List<string[]> BossDropRows { get; } = new();

    public string[] RaceGroupColumns { get; } = { "raceGroup", "memberRaceEditorIds", "minRfTier", "uniqueAffixId", "materialId", "description" };
    public List<string[]> RaceGroupRows { get; } = new();

    public string[] RaceDropColumns { get; } = { "raceGroup", "rewardType", "rewardId", "dropChancePct", "firstOnly", "note" };
    public List<string[]> RaceDropRows { get; } = new();

    public string[] MaterialColumns { get; } = { "materialId", "name", "tier", "family", "itemEditorId", "salvageYieldMin", "salvageYieldMax", "source" };
    public List<string[]> MaterialRows { get; } = new();

    public string[] RecipeColumns { get; } = { "recipeId", "outputType", "materials", "runeCost", "ilvlBand", "requirementSkill", "requirementRank" };
    public List<string[]> RecipeRows { get; } = new();

    public string[] SkillColumns { get; } = { "build", "skillName", "skyrimSkill", "attribute", "type", "rank1", "rank2", "rank3", "gateSkill", "gateAttribute", "description" };
    public List<string[]> SkillRows { get; } = new();

    public string[] AttributeColumns { get; } = { "attributeId", "name", "build", "effectKey", "perPoint", "softCap", "hardCap" };
    public List<string[]> AttributeRows { get; } = new();
    public string[] AttributeEffectColumns { get; } = { "attributeId", "effectKey", "perPoint", "unit" };
    public List<string[]> AttributeEffectRows { get; } = new();

    public string[] UniqueColumns { get; } = { "uniqueId", "name", "prefix", "suffix", "category", "effectEditorId", "minValue", "maxValue", "duration", "area", "slots", "description" };
    public List<string[]> UniqueRows { get; } = new();

    public string[] UniqueSourceColumns { get; } = { "uniqueId", "sourceType", "sourceEditorId", "dropRule", "firstOnly", "note" };
    public List<string[]> UniqueSourceRows { get; } = new();

    public string[] StringColumns { get; } = { "key", "english", "context" };
    public List<string[]> StringRows { get; } = new();

    public List<string> BaseMgefEditors { get; } = new();
    public List<string> UniqueNames { get; } = new();
    public List<string> HuntNames { get; } = new();
    public List<string> UniqueRecords { get; } = new();
    public List<string> HuntRecords { get; } = new();
    public List<string> AttributeNames { get; } = new();
    public List<string> SkillNames { get; } = new();
    public List<string> RaceFamilyNames { get; } = new();
}

internal static class ContentFactory
{
    private static readonly string[] Rarities = { "Common", "Refined", "Exquisite", "Radiant", "Mythic", "Runeforged" };
    private static readonly int[] Enhancements = { 0, 1, 2, 3, 4, 5 };
    private static readonly int[] SocketMin = { 0, 0, 1, 1, 2, 3 };
    private static readonly int[] SocketMax = { 0, 1, 1, 2, 2, 3 };
    private static readonly string[] Colors = { "white", "green", "blue", "purple", "orange", "gold" };
    private static readonly string[] PrefixStyles = { "none", "light", "medium", "heavy", "ornate", "full" };

    private static readonly (string Build, string Attribute)[] Attributes =
    {
        ("Warrior", "Might"), ("Warrior", "Endurance"), ("Warrior", "Precision"),
        ("Thief", "Agility"), ("Thief", "Guile"), ("Thief", "Cunning"),
        ("Mage", "Intellect"), ("Mage", "Wisdom"), ("Mage", "Willpower"),
    };

    private static readonly (string Build, string Skill, string Skyrim, string Attribute, string Type)[] Skills =
    {
        ("Warrior", "Cleave", "One-Handed", "Might", "active"),
        ("Warrior", "Ruinous Blows", "Two-Handed", "Might", "active"),
        ("Warrior", "Piercing Shot", "Archery", "Precision", "active"),
        ("Warrior", "Bulwark", "Block", "Endurance", "passive"),
        ("Warrior", "Ironclad", "Heavy Armor", "Endurance", "passive"),
        ("Warrior", "Forgecraft", "Smithing", "Precision", "passive"),
        ("Thief", "Shadowstrike", "Sneak", "Agility", "active"),
        ("Thief", "Evasive Step", "Light Armor", "Agility", "passive"),
        ("Thief", "Tumbler's Touch", "Lockpicking", "Guile", "passive"),
        ("Thief", "Pilfer", "Pickpocket", "Guile", "active"),
        ("Thief", "Silver Tongue", "Speech", "Cunning", "passive"),
        ("Thief", "Venomcraft", "Alchemy", "Cunning", "passive"),
        ("Mage", "Runic Bolt", "Destruction", "Intellect", "active"),
        ("Mage", "Mending Rune", "Restoration", "Wisdom", "active"),
        ("Mage", "Wardskin", "Alteration", "Wisdom", "passive"),
        ("Mage", "Runeforged Summon", "Conjuration", "Willpower", "active"),
        ("Mage", "Dread Mark", "Illusion", "Willpower", "active"),
        ("Mage", "Rune Binding", "Enchanting", "Intellect", "passive"),
    };

    private static readonly (string Group, string[] Races, int MinTier)[] HuntFamilies =
    {
        ("Draugr", new[] { "DraugrRace", "SkeletonArmorRace", "DLC2AshSpawnRace" }, 1),
        ("Vampire", new[] { "NordRaceVampire", "BretonRaceVampire", "DarkElfRaceVampire", "ImperialRaceVampire", "HighElfRaceVampire" }, 2),
        ("Skeleton", new[] { "SkeletonRace", "SkeletonNecroRace", "RigidSkeletonRace" }, 1),
        ("Daedra", new[] { "DremoraRace", "AtronachFrostRace", "DLC2SeekerRace" }, 3),
        ("Dragon", new[] { "DragonRace" }, 4),
        ("Falmer", new[] { "FalmerRace", "FalmerFrozenVampRace" }, 2),
        ("Spider", new[] { "FrostbiteSpiderRace", "DLC2ExpSpiderBaseRace", "DwarvenSpiderRace" }, 1),
        ("Dwarven Automaton", new[] { "DwarvenSphereRace", "DwarvenSpiderRace", "DwarvenCenturionRace" }, 3),
        ("Werewolf", new[] { "WerewolfBeastRace", "DLC2WerebearBeastRace" }, 3),
        ("Riekling", new[] { "DLC2RieklingRace", "DLC2MountedRieklingRace" }, 2),
        ("Wolf", new[] { "WolfRace" }, 1),
        ("Dragon Priest", new[] { "DragonPriestRace", "DLC2AcolyteDragonPriestRace" }, 4),
        ("Hagraven", new[] { "HagravenRace" }, 2),
        ("Troll", new[] { "TrollRace", "TrollFrostRace" }, 2),
        ("Spriggan", new[] { "SprigganRace", "SprigganEarthMotherRace" }, 2),
        ("Chaurus", new[] { "ChaurusRace", "DLC1ChaurusHunterRace" }, 2),
        ("Giant", new[] { "GiantRace" }, 3),
        ("Sabre Cat", new[] { "SabreCatRace", "SabreCatSnowyRace" }, 1),
        ("Mammoth", new[] { "MammothRace" }, 2),
    };

    private static readonly string[] BossSources =
    {
        "MQ206Alduin", "MG07LabyrinthianDragonPriest", "DunForelhostDragonPriestRahgot", "BlackreachDragon",
        "DLC1VQDragonPriest", "DLC2Miraak", "DunMq304Torygg", "dunLabyrinthianUndeadDragon",
        "DA10VaerminaPriest", "DA13Orchendor", "DA14PelagiusWakingNightmare", "TG05Mercer",
        "DB10Astrid", "C06WolfSpirit", "MS08ForswornBoss", "MS13Movarth", "MS14ValthumePriest",
        "FreeformRiften09Boss", "DLC1SeranaChaseBoss", "DLC2ThirskRieklingBoss",
    };

    private static readonly string[] QuestSources =
    {
        "MQ101", "MQ206", "DA10", "DA13", "DA14", "TG05", "TG09", "DB10", "DB11", "MS08", "MS13",
        "MS14", "C06", "DLC1VQ02", "DLC1VQ08", "DLC2MQ02", "DLC2MQ06", "FavorJarlsMakeFriends",
        "FreeformRiften09", "FreeformSkyhavenTempleA",
    };

    private static readonly string[] Categories = { "offense", "defense", "utility", "economy", "elemental" };
    private static readonly string[] Prefixes = { "Runed", "Gleaming", "Ancient", "Frostbound", "Emberforged", "Stormwrought", "Hollow", "Gilded", "Whispering", "Bloodforged", "Starbound", "Riftborn" };
    private static readonly string[] Suffixes = { "of the Rune", "of Cleaving", "of Warding", "of the Hunt", "of Embers", "of the Deep", "of Twilight", "of Storms", "of the Fallen", "of Fortune", "of the Pact", "of Ruin" };
    private static readonly string[] SlotTokens = { "WEAPON", "ONEHANDED", "ARMOR", "HEAD", "HANDS", "FEET", "RING", "AMULET", "SHIELD", "BOW", "STAFF" };

    public static Content Create()
    {
        var c = new Content();
        BuildAttributes(c);
        BuildSkills(c);
        BuildRarity(c);
        BuildAffixes(c);
        BuildRanks(c);
        BuildDrops(c);
        BuildHunt(c);
        BuildMaterialsAndRecipes(c);
        BuildUnique(c);
        BuildStrings(c);
        return c;
    }

    private static void BuildAttributes(Content c)
    {
        var effectKeys = new Dictionary<string, string>
        {
            ["Might"] = "meleeDamagePct:2;carryWeight:2;staggerResistPct:0.5;gatherYieldPct:0.5",
            ["Endurance"] = "health:4;armorPct:1.5;blockEffectivenessPct:0.5;stamina:1;staminaRegenPct:0.5",
            ["Precision"] = "rangedDamagePct:1.5;attackSpeedPct:0.5;craftedItemPowerPct:1;materialEfficiencyPct:0.5",
            ["Agility"] = "critChancePct:0.5;attackSpeedPct:0.5;evasionPct:0.4;movementPct:0.3;fallDamageReductionPct:0.3",
            ["Guile"] = "lockpickPct:1;magicFindPct:0.4;runeFindPct:0.3;sneakDetectionReductionPct:0.3",
            ["Cunning"] = "vendorValuePct:1;poisonPotencyPct:1.5;stealthDamagePct:0.5;persuasionPct:0.5",
            ["Intellect"] = "magicka:2;destructionMagnitudePct:1.5;enchantmentPowerPct:0.5;spellLearningPct:0.5",
            ["Wisdom"] = "restorationMagnitudePct:1.5;wardArmorPct:1;magickaRegenPct:0.5;potionEffectivenessPct:1",
            ["Willpower"] = "conjurationIllusionPct:1.5;summonDurationPct:1;cooldownReductionPct:0.5;intimidationPct:0.5",
        };
        var flatStats = new HashSet<string> { "health", "stamina", "magicka", "carryWeight" };
        foreach (var (build, attribute) in Attributes)
        {
            c.AttributeNames.Add(attribute);
            var effects = effectKeys[attribute].Split(';', StringSplitOptions.RemoveEmptyEntries);
            var first = effects[0].Split(':');
            c.AttributeRows.Add(new[]
            {
                attribute, attribute, build, "attr." + attribute.ToLowerInvariant(), first[1], "30", "60"
            });
            foreach (var effect in effects)
            {
                var parts = effect.Split(':');
                if (parts.Length != 2)
                    continue;
                c.AttributeEffectRows.Add(new[]
                {
                    attribute, parts[0], parts[1], flatStats.Contains(parts[0]) ? "flat" : "pct"
                });
            }
        }
    }

    private static void BuildSkills(Content c)
    {
        foreach (var (build, skill, skyrim, attribute, type) in Skills)
        {
            c.SkillNames.Add(skill);
            var prefix = type == "active" ? "RF_SPEL_Skill_" : "RF_PERK_Skill_";
            c.SkillRows.Add(new[]
            {
                build, skill, skyrim, attribute, type,
                prefix + Slug(skill) + "_1", prefix + Slug(skill) + "_2", prefix + Slug(skill) + "_3",
                skyrim, attribute + " 10/20/30", type == "active" ? "Active ability" : "Passive line"
            });
        }
    }

    private static void BuildRarity(Content c)
    {
        for (int i = 0; i < Rarities.Length; i++)
            c.RarityRows.Add(new[] { i.ToString(), Rarities[i], Enhancements[i].ToString(), SocketMin[i].ToString(), SocketMax[i].ToString(), Colors[i], PrefixStyles[i] });

        var baseWeights = new[] { 55.0, 25.0, 11.0, 6.0, 2.5, 0.5 };
        for (int tier = 0; tier < 6; tier++)
        {
            for (int rarity = 0; rarity < 6; rarity++)
            {
                var weight = baseWeights[rarity];
                if (tier >= 3) weight *= 1.0 + (tier - 2) * 0.35;
                if (rarity == 5) weight *= 1.0 + tier * 0.8;
                c.RarityWeightRows.Add(new[] { tier.ToString(), rarity.ToString(), weight.ToString("0.0", CultureInfo.InvariantCulture) });
            }
        }
    }

    private static void BuildAffixes(Content c)
    {
        int families = 120;
        c.AffixFamilies = families;
        for (int i = 0; i < families; i++)
        {
            var attribute = c.AttributeNames[i % c.AttributeNames.Count];
            var skillsForAttribute = Skills.Where(s => s.Attribute == attribute).Select(s => s.Skill).ToArray();
            var skill = skillsForAttribute.Length > 0 ? skillsForAttribute[i % skillsForAttribute.Length] : "Cleave";
            var category = Categories[(i / 9) % Categories.Length];
            var prefix = Prefixes[i % Prefixes.Length];
            var suffix = Suffixes[(i / 5) % Suffixes.Length];
            var slotA = SlotTokens[i % SlotTokens.Length];
            var slotB = SlotTokens[(i + 3) % SlotTokens.Length];
            var effect = $"RF_MGEF_Affix_{i:000}";
            c.BaseMgefEditors.Add(effect);
            var affixId = $"rf_affix_{i:000}";

            for (int tier = 1; tier <= 5; tier++)
            {
                var unit = category == "utility" ? "seconds" : (category == "economy" ? "pct" : "flat");
                var min = tier * 2 + (i % 4);
                var max = min + 2 + tier;
                var duration = unit == "seconds" ? (5 + tier * 2).ToString() : "0";
                var minLevel = Math.Max(1, (tier - 1) * 15);
                c.AffixRows.Add(new[]
                {
                    affixId, prefix + " " + skill + " " + suffix, category, tier.ToString(), unit,
                    slotA + "|" + slotB, min.ToString(), max.ToString(), duration, "0",
                    (tier * 10).ToString(), minLevel.ToString(), effect, prefix, suffix,
                    "affix." + affixId + ".tier" + tier
                });
            }
        }
    }

    private static void BuildRanks(Content c)
    {
        string[] names = { "Normal", "Rune-Touched", "Runebound", "Runelord", "Runesworn", "Rune-Mythic" };
        int[] hp = { 0, 50, 150, 350, 600, 900 };
        int[] dmg = { 0, 20, 40, 70, 100, 150 };
        int[] armor = { 0, 10, 20, 30, 40, 50 };
        int[] affixes = { 0, 1, 1, 2, 3, 4 };
        int[] rarityBonus = { 0, 2, 4, 6, 9, 12 };
        for (int rank = 0; rank < 6; rank++)
        {
            c.RankRows.Add(new[]
            {
                rank.ToString(), names[rank], hp[rank].ToString(), dmg[rank].ToString(), armor[rank].ToString(),
                affixes[rank].ToString(), rarityBonus[rank].ToString(), Math.Max(1, rank).ToString(),
                Math.Max(1, rank - 1).ToString(), (rank + 1).ToString(), Math.Max(0, rank - 2).ToString()
            });
        }
    }

    private static void BuildDrops(Content c)
    {
        for (int rank = 0; rank < 6; rank++)
        {
            for (int tier = 0; tier < 6; tier++)
            {
                var chance = rank == 0 ? 25 + tier * 2 : 60 + rank * 8 + tier * 2;
                chance = Math.Min(100, chance);
                var countMax = rank >= 3 ? 2 + rank / 2 : 1;
                var unique = rank >= 4 ? 5 + (rank - 4) * 20 + tier * 2 : 0;
                c.GeneralDropRows.Add(new[]
                {
                    rank.ToString(), tier.ToString(), chance.ToString(), "1", countMax.ToString(),
                    (rank * 2 + tier).ToString(), Math.Max(1, rank).ToString(), (Math.Max(1, rank) + 1).ToString(),
                    unique.ToString()
                });
            }
        }

        for (int tier = 0; tier < 6; tier++)
        {
            c.ContainerDropRows.Add(new[]
            {
                tier.ToString(), (45 + tier * 5).ToString(), (tier * 2).ToString(), "1",
                (1 + tier / 2).ToString(), (tier + 1).ToString(), (tier + 2).ToString(), (3 + tier * 2).ToString()
            });
        }

        for (int band = 0; band < 5; band++)
            c.QuestDropRows.Add(new[] { band.ToString(), "1", Math.Max(1, band).ToString(), (band + 1).ToString(), (band + 2).ToString() });

        for (int boss = 0; boss < 20; boss++)
        {
            var tier = boss % 6;
            c.BossDropRows.Add(new[]
            {
                "rf_boss_" + boss.ToString("00"), (2 + tier / 3).ToString(), Math.Min(5, 2 + tier / 2).ToString(),
                (tier + 2).ToString(), (tier + 4).ToString(), tier.ToString(), "rf_unique_boss_" + boss.ToString("00")
            });
        }

        for (int tier = 0; tier < 6; tier++)
        {
            foreach (var rank in new[] { 1, 2, 3, 4, 5 })
            {
                var weight = rank == 4 || rank == 5 ? (tier >= 4 ? 1 : 0) : (6 - rank);
                c.SpawnRows.Add(new[] { tier.ToString(), rank.ToString(), Math.Max(0, weight).ToString() });
            }
        }
    }

    private static void BuildHunt(Content c)
    {
        foreach (var (group, races, minTier) in HuntFamilies)
        {
            var slug = Slug(group);
            var unique = "rf_hunt_unique_" + slug;
            var material = "RF_MISC_Mat_Hunt_" + SlugPascal(group);
            c.RaceFamilyNames.Add(group);
            c.RaceGroupRows.Add(new[] { group, string.Join("|", races), minTier.ToString(), unique, material, "Hunt family: " + group });
            c.RaceDropRows.Add(new[] { group, "unique_affix", unique, "100", "1", "first kill only" });
            c.RaceDropRows.Add(new[] { group, "material", material, "15", "0", "+5 percent per RF tier above minRfTier" });
            c.HuntNames.Add("RF_MGEF_Hunt_" + SlugPascal(group));
            c.HuntRecords.Add(material);
        }
    }

    private static void BuildMaterialsAndRecipes(Content c)
    {
        var families = new[] { "ore", "essence", "hide" };
        var tierNames = new[] { "Common", "Refined", "Exotic", "Mythic" };
        int n = 0;
        foreach (var tier in new[] { 0, 1, 2, 3 })
        {
            foreach (var family in families)
            {
                var id = $"rf_mat_{tierNames[tier].ToLowerInvariant()}_{family}";
                var item = "RF_MISC_Mat_" + tierNames[tier] + "_" + SlugPascal(family);
                c.MaterialRows.Add(new[] { id, tierNames[tier] + " " + family, tier.ToString(), family, item, "1", "3", tier == 0 ? "salvage" : "node" });
                ++n;
            }
        }
        foreach (var (group, _, _) in HuntFamilies)
        {
            var id = "rf_mat_hunt_" + Slug(group);
            var item = "RF_MISC_Mat_Hunt_" + SlugPascal(group);
            c.MaterialRows.Add(new[] { id, group + " Essence", "4", "essence", item, "1", "2", "drop" });
        }

        for (int rank = 1; rank <= 4; rank++)
            c.RecipeRows.Add(new[] { "rf_recipe_rune_convert_" + rank, "rune_convert", "rf_mat_refined_ore:3", (5 * rank).ToString(), "0", "Smithing", "20" });
        c.RecipeRows.Add(new[] { "rf_recipe_socket_1", "socket_craft", "rf_mat_refined_essence:2", "10", "1", "Enchanting", "20" });
        c.RecipeRows.Add(new[] { "rf_recipe_socket_2", "socket_craft", "rf_mat_exotic_essence:2", "25", "3", "Enchanting", "50" });
        c.RecipeRows.Add(new[] { "rf_recipe_utility_salvage", "utility", "", "0", "0", "Smithing", "0" });
        c.RecipeRows.Add(new[] { "rf_recipe_utility_respec", "utility", "rf_mat_mythic_essence:1", "100", "5", "Enchanting", "75" });
    }

    private static void BuildUnique(Content c)
    {
        var categories = new[] { "offense", "defense", "utility", "elemental", "economy" };
        for (int i = 0; i < 20; i++)
        {
            var id = "rf_unique_boss_" + i.ToString("00");
            var effect = "RF_MGEF_Unique_Boss_" + i.ToString("00");
            c.UniqueRecords.Add(effect);
            c.UniqueNames.Add(effect);
            c.UniqueRows.Add(new[]
            {
                id, "Boss Unique " + i.ToString("00"), "Runebound", "of the Fallen", categories[i % categories.Length],
                effect, (20 + i).ToString(), (30 + i).ToString(), "0", "0",
                SlotTokens[i % SlotTokens.Length], "unique." + id
            });
            c.UniqueSourceRows.Add(new[] { id, "boss", BossSources[i], "guaranteed_first", "1", "mined boss source" });
        }
        for (int i = 0; i < 20; i++)
        {
            var id = "rf_unique_quest_" + i.ToString("00");
            var effect = "RF_MGEF_Unique_Quest_" + i.ToString("00");
            c.UniqueRecords.Add(effect);
            c.UniqueNames.Add(effect);
            c.UniqueRows.Add(new[]
            {
                id, "Quest Unique " + i.ToString("00"), "Runeforged", "of the Pact", categories[i % categories.Length],
                effect, (15 + i).ToString(), (25 + i).ToString(), "0", "0",
                SlotTokens[(i + 2) % SlotTokens.Length], "unique." + id
            });
            c.UniqueSourceRows.Add(new[] { id, "quest", QuestSources[i], "guaranteed_first", "1", "mined quest source" });
        }
        for (int i = 0; i < HuntFamilies.Length; i++)
        {
            var (group, races, _) = HuntFamilies[i];
            var id = "rf_hunt_unique_" + Slug(group);
            var effect = "RF_MGEF_Hunt_" + SlugPascal(group);
            c.UniqueRecords.Add(effect);
            c.UniqueNames.Add(effect);
            c.UniqueRows.Add(new[]
            {
                id, group + " Hunt", "Runed", Suffixes[i % Suffixes.Length], Categories[i % Categories.Length],
                effect, (25 + i).ToString(), (35 + i).ToString(), "0", "0",
                SlotTokens[i % SlotTokens.Length], "unique." + id
            });
            c.UniqueSourceRows.Add(new[] { id, "race", races[0], "guaranteed_first", "1", "hunt family first kill" });
        }
        foreach (var (group, _, _) in HuntFamilies)
            c.HuntRecords.Add("RF_MISC_Mat_Hunt_" + SlugPascal(group));
    }

    private static void BuildStrings(Content c)
    {
        void Add(string key, string english, string context) => c.StringRows.Add(new[] { key, english, context });
        foreach (var (build, attr) in Attributes)
            Add("attribute." + attr.ToLowerInvariant(), attr, "attribute");
        foreach (var (_, skill, _, _, _) in Skills)
            Add("skill." + Slug(skill), skill, "skill");
        for (int i = 0; i < Rarities.Length; i++)
            Add("rarity." + i, Rarities[i], "rarity");
        for (int i = 0; i < Rarities.Length; i++)
            Add("rarity." + i + ".desc", Rarities[i] + " item", "rarity");
        foreach (var (group, _, _) in HuntFamilies)
            Add("hunt." + Slug(group), group, "hunt");
        for (int i = 0; i < Prefixes.Length; i++)
            Add("prefix." + i, Prefixes[i], "affix");
        for (int i = 0; i < Suffixes.Length; i++)
            Add("suffix." + i, Suffixes[i], "affix");
    }

    internal static string Slug(string value)
    {
        var sb = new System.Text.StringBuilder();
        foreach (var ch in value.ToLowerInvariant())
            sb.Append(char.IsLetterOrDigit(ch) ? ch : '_');
        return sb.ToString().Trim('_');
    }

    internal static string SlugPascal(string value)
    {
        var slug = Slug(value);
        var parts = slug.Split('_', StringSplitOptions.RemoveEmptyEntries);
        return string.Concat(parts.Select(p => char.ToUpperInvariant(p[0]) + p.Substring(1)));
    }
}