using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Binary.Parameters;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;

namespace Runefall.Build;

internal sealed record PluginOutput(string Name, string Path, SkyrimMod Mod);

internal sealed class FormKeyAllocator
{
    private readonly SkyrimMod mod;
    private uint next = 0x800;

    public FormKeyAllocator(SkyrimMod mod) => this.mod = mod;

    public FormKey Next() => new(mod.ModKey, next++);

    // Mutagen allocates new records from ModHeader.Stats.NextFormID; moving the floor before a
    // category creates the fixed L3 §1.1 FormID windows.
    public void StartAt(uint a_id) => mod.ModHeader.Stats.NextFormID = a_id;
}

internal static class PluginFactory
{
    public static List<PluginOutput> Create(Content content, string dataDir)
    {
        var outputs = new List<PluginOutput>();

        var baseMod = NewMod("Runefall.esp");
        var baseKeys = new FormKeyAllocator(baseMod);
        AddBaseRecords(baseMod, baseKeys, content);
        Write(baseMod, Path.Combine(dataDir, "Runefall.esp"), outputs);

        var uniqueMod = NewMod("Runefall - Unique.esp");
        var uniqueKeys = new FormKeyAllocator(uniqueMod);
        uniqueKeys.StartAt(0x900);
        foreach (var editor in content.UniqueNames.Where(e => e.StartsWith("RF_MGEF_Unique_", StringComparison.Ordinal)))
            AddEffect(uniqueMod, uniqueKeys, editor);
        Write(uniqueMod, Path.Combine(dataDir, "Runefall - Unique.esp"), outputs);

        var huntMod = NewMod("Runefall - Hunt.esp");
        var huntKeys = new FormKeyAllocator(huntMod);
        huntKeys.StartAt(0x900);
        foreach (var editor in content.HuntNames)
            AddEffect(huntMod, huntKeys, editor);
        huntKeys.StartAt(0xB80);
        foreach (var (group, _, _) in content.RaceFamilyNames.Select(g => (g, 0, 0)))
        {
            var editor = "RF_MISC_Mat_Hunt_" + ContentFactory.SlugPascal(group);
            AddMisc(huntMod, huntKeys, editor, group + " Essence", 25u, 0.5f);
        }
        Write(huntMod, Path.Combine(dataDir, "Runefall - Hunt.esp"), outputs);

        return outputs;
    }

    private static SkyrimMod NewMod(string name)
    {
        var mod = new SkyrimMod(new ModKey(Path.GetFileNameWithoutExtension(name), ModType.Plugin), SkyrimRelease.SkyrimSE);
        mod.ModHeader.Flags |= SkyrimModHeader.HeaderFlag.Small;
        mod.ModHeader.MasterReferences.Add(new MasterReference { Master = ModKey.FromNameAndExtension("Skyrim.esm") });
        return mod;
    }

    private static void AddBaseRecords(SkyrimMod mod, FormKeyAllocator keys, Content content)
    {
        keys.StartAt(0x800);
        var rarity = new[] { "Common", "Refined", "Exquisite", "Radiant", "Mythic", "Runeforged" };
        foreach (var name in rarity)
            mod.Keywords.Add(new Keyword(mod) { EditorID = "RF_KW_Rarity_" + name });

        var ranks = new[] { "RuneTouched", "Runebound", "Runelord", "Runesworn", "RuneMythic" };
        foreach (var name in ranks)
            mod.Keywords.Add(new Keyword(mod) { EditorID = "RF_KW_Rank_" + name });

        foreach (var family in new[] { "Ore", "Essence", "Hide" })
            mod.Keywords.Add(new Keyword(mod) { EditorID = "RF_KW_Mat_" + family });
        foreach (var tier in new[] { "Common", "Refined", "Exotic", "Mythic" })
            mod.Keywords.Add(new Keyword(mod) { EditorID = "RF_KW_MatTier_" + tier });

        foreach (var attribute in content.AttributeNames)
            mod.Keywords.Add(new Keyword(mod) { EditorID = "RF_KW_Attr_" + attribute });
        foreach (var skill in content.SkillNames)
            mod.Keywords.Add(new Keyword(mod) { EditorID = "RF_KW_Skill_" + ContentFactory.SlugPascal(skill) });
        foreach (var slot in new[] { "WEAPON", "HEAD", "CHEST", "HANDS", "FEET", "SHIELD", "AMULET", "RING" })
            mod.Keywords.Add(new Keyword(mod) { EditorID = "RF_KW_Slot_" + slot });
        for (var socket = 1; socket <= 3; ++socket)
            mod.Keywords.Add(new Keyword(mod) { EditorID = "RF_KW_Socket_" + socket });
        for (var tier = 0; tier <= 5; ++tier)
            mod.Keywords.Add(new Keyword(mod) { EditorID = "RF_KW_Tier_" + tier });

        keys.StartAt(0x880);
        foreach (var (build, attribute) in new[] { ("Warrior", "Might"), ("Warrior", "Endurance"), ("Warrior", "Precision"), ("Thief", "Agility"), ("Thief", "Guile"), ("Thief", "Cunning"), ("Mage", "Intellect"), ("Mage", "Wisdom"), ("Mage", "Willpower") })
            mod.Globals.Add(new GlobalFloat(mod) { EditorID = "RF_GLOB_Attr_" + attribute, Data = 0f });
        mod.Globals.Add(new GlobalFloat(mod) { EditorID = "RF_GLOB_State_RuneTier", Data = 0f });
        mod.Globals.Add(new GlobalInt(mod) { EditorID = "RF_GLOB_State_LootAuthority", Data = 0 });
        foreach (var feature in new[] { "Menu", "Loot", "Input", "CoSave", "Papyrus", "Probes", "LootHooks" })
            mod.Globals.Add(new GlobalInt(mod) { EditorID = "RF_GLOB_Feature_" + feature, Data = 1 });
        foreach (var state in new[] { "Pity", "MagicFind", "BossKillTier", "Attunement" })
            mod.Globals.Add(new GlobalInt(mod) { EditorID = "RF_GLOB_State_" + state, Data = 0 });
        foreach (var skill in content.SkillNames)
            mod.Globals.Add(new GlobalInt(mod) { EditorID = "RF_GLOB_Skill_" + ContentFactory.SlugPascal(skill), Data = 0 });

        keys.StartAt(0x900);
        foreach (var editor in content.BaseMgefEditors)
            AddEffect(mod, keys, editor);

        var skillEffects = new Dictionary<string, FormKey>(StringComparer.OrdinalIgnoreCase);
        foreach (var row in content.SkillRows)
        {
            var skill = row[1];
            if (row[4] == "active" && !skillEffects.ContainsKey(skill))
                skillEffects[skill] = AddEffect(mod, keys, "RF_MGEF_Skill_" + skill.Replace(' ', '_'));
        }

        foreach (var utility in new[] { "Pity", "MagicFind", "SmartDrop", "BossKill", "HuntFirstKill", "AttunementReady", "RuneFind", "MaterialYield", "CraftPower", "FallReduction", "SneakReduction", "Movement", "Cooldown", "MonsterAffix1", "MonsterAffix2", "MonsterAffix3", "MonsterAffix4", "MonsterAffix5", "Utility1", "Utility2", "Utility3", "Utility4", "Utility5", "Utility6", "Utility7", "Utility8", "Utility9", "Utility10" })
            AddEffect(mod, keys, "RF_MGEF_Utility_" + utility);
        for (var extra = 1; extra <= 44; ++extra)
            AddEffect(mod, keys, "RF_MGEF_Utility_Extra" + extra.ToString("00"));

        keys.StartAt(0xA00);
        foreach (var row in content.SkillRows)
        {
            if (row[4] != "active")
                continue;
            var skill = row[1];
            for (var rank = 0; rank < 3; ++rank)
            {
                var editor = row[5 + rank];
                var name = skill + " Rank " + (rank + 1);
                var spell = new Spell(mod)
                {
                    EditorID = editor,
                    Name = name,
                    Type = SpellType.Ability,
                    CastType = CastType.FireAndForget,
                    TargetType = TargetType.Self,
                    BaseCost = 0,
                    ChargeTime = 0,
                    CastDuration = 0,
                    Effects = new Noggog.ExtendedList<Effect>()
                };
                spell.Effects!.Add(new Effect
                {
                    BaseEffect = new FormLinkNullable<IMagicEffectGetter>(skillEffects[skill]),
                    Data = new EffectData
                    {
                        Magnitude = 1.0f,
                        Area = 0,
                        Duration = 0
                    }
                });
                mod.Spells.Add(spell);
            }
        }

        keys.StartAt(0xB00);
        foreach (var row in content.SkillRows)
        {
            if (row[4] == "active")
                continue;
            var skill = row[1];
            for (var rank = 0; rank < 3; ++rank)
            {
                var editor = row[5 + rank];
                var name = skill + " Rank " + (rank + 1);
                mod.Perks.Add(new Perk(mod)
                {
                    EditorID = editor,
                    Name = name,
                    Description = string.Empty,
                    NumRanks = 1,
                    Playable = true,
                    Hidden = false
                });
            }
        }

        keys.StartAt(0xB80);
        var miscByEditor = new Dictionary<string, FormKey>(StringComparer.Ordinal);
        for (int rank = 1; rank <= 5; rank++)
        {
            var editor = "RF_MISC_Rune_" + rank;
            miscByEditor[editor] = AddMisc(mod, keys, editor, "Runefall Rune R" + rank, (uint)(10 * rank), 0.1f);
        }

        var tierNames = new[] { "Common", "Refined", "Exotic", "Mythic" };
        foreach (var tier in tierNames)
            foreach (var family in new[] { "Ore", "Essence", "Hide" })
            {
                var editor = "RF_MISC_Mat_" + tier + "_" + family;
                miscByEditor[editor] = AddMisc(mod, keys, editor, tier + " " + family, 5u, 0.2f);
            }

        keys.StartAt(0xC00);
        AddRecipes(mod, miscByEditor);

        keys.StartAt(0xC80);
        mod.Quests.Add(new Quest(mod) { EditorID = "RF_QUST_Core", Name = "Runefall Core" });
        var debugQuest = new Quest(mod) { EditorID = "RF_QUST_Debug", Name = "Runefall Debug" };
        debugQuest.VirtualMachineAdapter = new QuestAdapter();
        debugQuest.VirtualMachineAdapter.Scripts.Add(new ScriptEntry
        {
            Name = "RunefallProbe",
            Flags = ScriptEntry.Flag.Local
        });
        debugQuest.VirtualMachineAdapter.Scripts.Add(new ScriptEntry
        {
            Name = "RunefallMenuBridge",
            Flags = ScriptEntry.Flag.Local
        });
        mod.Quests.Add(debugQuest);
        var attunementQuest = new Quest(mod) { EditorID = "RF_QUST_Attunement", Name = "Runefall Attunement" };
        attunementQuest.VirtualMachineAdapter = new QuestAdapter();
        attunementQuest.VirtualMachineAdapter.Scripts.Add(new ScriptEntry
        {
            Name = "RunefallQuestHost",
            Flags = ScriptEntry.Flag.Local
        });
        mod.Quests.Add(attunementQuest);
        for (var pool = 1; pool <= 5; ++pool)
            mod.FormLists.Add(new FormList(mod) { EditorID = "RF_FLST_Pool_" + pool });
    }

    private static FormKey AddEffect(SkyrimMod mod, FormKeyAllocator keys, string editor)
    {
        var effect = new MagicEffect(mod)
        {
            EditorID = editor,
            Name = editor.Replace("RF_MGEF_", string.Empty).Replace("RF_UMGEF_", string.Empty).Replace('_', ' ')
        };
        effect.Archetype = new MagicEffectArchetype
        {
            Type = MagicEffectArchetype.TypeEnum.ValueModifier,
            ActorValue = ActorValue.Health
        };
        effect.CastType = CastType.FireAndForget;
        effect.TargetType = TargetType.Self;
        mod.MagicEffects.Add(effect);
        return effect.FormKey;
    }

    private static FormKey AddMisc(SkyrimMod mod, FormKeyAllocator keys, string editor, string name, uint value, float weight)
    {
        var item = new MiscItem(mod)
        {
            EditorID = editor,
            Name = name,
            Value = value,
            Weight = weight
        };
        mod.MiscItems.Add(item);
        return item.FormKey;
    }

    private static void AddRecipes(SkyrimMod mod, Dictionary<string, FormKey> miscByEditor)
    {
        void AddCobj(string editor, FormKey output, IEnumerable<(string Editor, int Count)> inputs)
        {
            var cobj = new ConstructibleObject(mod)
            {
                EditorID = editor,
                CreatedObject = new FormLinkNullable<IConstructibleGetter>(output),
                CreatedObjectCount = 1,
                Items = new Noggog.ExtendedList<ContainerEntry>()
            };
            foreach (var (inputEditor, count) in inputs)
            {
                if (!miscByEditor.TryGetValue(inputEditor, out var inputKey))
                    throw new InvalidOperationException("missing COBJ input " + inputEditor);
                cobj.Items!.Add(new ContainerEntry
                {
                    Item = new ContainerItem
                    {
                        Item = new FormLink<IItemGetter>(inputKey),
                        Count = count
                    }
                });
            }
            mod.ConstructibleObjects.Add(cobj);
        }

        for (var rank = 1; rank <= 4; ++rank)
            AddCobj("RF_COBJ_RuneConvert_" + rank, miscByEditor["RF_MISC_Rune_" + (rank + 1)],
                new[] { ("RF_MISC_Rune_" + rank, 5), ("RF_MISC_Mat_Refined_Ore", 3) });

        AddCobj("RF_COBJ_Socket_1", miscByEditor["RF_MISC_Rune_2"],
            new[] { ("RF_MISC_Mat_Refined_Essence", 2), ("RF_MISC_Rune_1", 1) });
        AddCobj("RF_COBJ_Socket_2", miscByEditor["RF_MISC_Rune_3"],
            new[] { ("RF_MISC_Mat_Exotic_Essence", 2), ("RF_MISC_Rune_2", 1) });
        AddCobj("RF_COBJ_Utility_Salvage", miscByEditor["RF_MISC_Mat_Common_Ore"],
            new[] { ("RF_MISC_Mat_Refined_Ore", 2) });
        AddCobj("RF_COBJ_Utility_Respec", miscByEditor["RF_MISC_Mat_Mythic_Essence"],
            new[] { ("RF_MISC_Mat_Exotic_Essence", 2), ("RF_MISC_Rune_4", 1) });
    }

    private static void Write(SkyrimMod mod, string path, List<PluginOutput> outputs)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        Console.WriteLine($"Writing {Path.GetFileName(path)}: masters before write={mod.ModHeader.MasterReferences.Count}");
        var parameters = BinaryWriteParameters.Default with { MastersListContent = MastersListContentOption.NoCheck };
        mod.WriteToBinary(path, parameters);
        outputs.Add(new PluginOutput(Path.GetFileName(path), path, mod));
    }
}