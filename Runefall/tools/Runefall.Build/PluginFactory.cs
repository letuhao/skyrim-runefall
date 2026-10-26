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
        foreach (var editor in content.UniqueNames.Where(e => e.StartsWith("RF_MGEF_Unique_", StringComparison.Ordinal)))
            AddEffect(uniqueMod, uniqueKeys, editor);
        Write(uniqueMod, Path.Combine(dataDir, "Runefall - Unique.esp"), outputs);

        var huntMod = NewMod("Runefall - Hunt.esp");
        var huntKeys = new FormKeyAllocator(huntMod);
        foreach (var editor in content.HuntNames)
            AddEffect(huntMod, huntKeys, editor);
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

        foreach (var (build, attribute) in new[] { ("Warrior", "Might"), ("Warrior", "Endurance"), ("Warrior", "Precision"), ("Thief", "Agility"), ("Thief", "Guile"), ("Thief", "Cunning"), ("Mage", "Intellect"), ("Mage", "Wisdom"), ("Mage", "Willpower") })
            mod.Globals.Add(new GlobalFloat(mod) { EditorID = "RF_GLOB_Attr_" + attribute, Data = 0f });
        mod.Globals.Add(new GlobalFloat(mod) { EditorID = "RF_GLOB_State_RuneTier", Data = 0f });
        mod.Globals.Add(new GlobalInt(mod) { EditorID = "RF_GLOB_State_LootAuthority", Data = 0 });
        foreach (var skill in content.SkillNames)
            mod.Globals.Add(new GlobalInt(mod) { EditorID = "RF_GLOB_Skill_" + ContentFactory.SlugPascal(skill), Data = 0 });

        for (int rank = 1; rank <= 5; rank++)
            AddMisc(mod, keys, "RF_MISC_Rune_" + rank, "Runefall Rune R" + rank, (uint)(10 * rank), 0.1f);

        var tierNames = new[] { "Common", "Refined", "Exotic", "Mythic" };
        foreach (var tier in tierNames)
            foreach (var family in new[] { "Ore", "Essence", "Hide" })
                AddMisc(mod, keys, "RF_MISC_Mat_" + tier + "_" + family, tier + " " + family, 5u, 0.2f);

        foreach (var editor in content.BaseMgefEditors)
            AddEffect(mod, keys, editor);

        foreach (var row in content.SkillRows)
        {
            var skill = row[1];
            var skillType = row[4];
            for (var rank = 0; rank < 3; ++rank)
            {
                var editor = row[5 + rank];
                var name = skill + " Rank " + (rank + 1);
                if (skillType == "active")
                {
                    mod.Spells.Add(new Spell(mod)
                    {
                        EditorID = editor,
                        Name = name,
                        Type = SpellType.Ability,
                        CastType = CastType.FireAndForget,
                        TargetType = TargetType.Self,
                        BaseCost = 0,
                        ChargeTime = 0,
                        CastDuration = 0
                    });
                }
                else
                {
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
        }

        mod.Quests.Add(new Quest(mod) { EditorID = "RF_QUST_Core", Name = "Runefall Core" });
        var debugQuest = new Quest(mod) { EditorID = "RF_QUST_Debug", Name = "Runefall Debug" };
        debugQuest.VirtualMachineAdapter = new QuestAdapter();
        debugQuest.VirtualMachineAdapter.Scripts.Add(new ScriptEntry
        {
            Name = "RunefallProbe",
            Flags = ScriptEntry.Flag.Local
        });
        mod.Quests.Add(debugQuest);
        mod.Quests.Add(new Quest(mod) { EditorID = "RF_QUST_Attunement", Name = "Runefall Attunement" });
        for (var pool = 1; pool <= 5; ++pool)
            mod.FormLists.Add(new FormList(mod) { EditorID = "RF_FLST_Pool_" + pool });
    }

    private static void AddEffect(SkyrimMod mod, FormKeyAllocator keys, string editor)
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
    }

    private static void AddMisc(SkyrimMod mod, FormKeyAllocator keys, string editor, string name, uint value, float weight)
    {
        mod.MiscItems.Add(new MiscItem(mod)
        {
            EditorID = editor,
            Name = name,
            Value = value,
            Weight = weight
        });
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