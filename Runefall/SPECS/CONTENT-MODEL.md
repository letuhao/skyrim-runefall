# CONTENT MODEL - JSON, offline + runtime

Status: draft 1, 2026-10-28. Shared entity schema for agent-authored content, offline packs and runtime fallback generation.

## Principles

1. One schema, two producers: offline compiler and runtime generator. The DLL consumes both the same way.
2. Content is referenced by stable content keys, never load-order FormIDs. FormIDs appear only inside generated ESP records and for explicitly allowed unique/master references.
3. Every entity has a schema version, a pack owner and deterministic identity.
4. Every reference is resolved and validated before it reaches the DLL; unresolved references fail the pack, not the game.
5. Runtime-generated profiles use the same entity shapes and are cached as versioned JSON.
6. Generated ESP/records are artifacts; agents author JSON only.

## Identity

Content key format: rf:entity-type/slug.

Examples: rf:actor/draugr_gravewarden, rf:encounter/ember_wolf_pack, rf:item/rune_iron_sword, rf:material/grave_core, rf:loot/draugr_elite, rf:affix/ember_brand.

Rules: slug is lower-kebab and stable forever; renames require a migration mapping. Cross-content references use keys. References to external forms use FormRef objects, minimized: editorId for normal master records, masterForm FormID only for official masters or unique content, never generated Runefall FormIDs. Runtime race descriptors use plugin plus localFormId plus optional editorId; FormID is transient.

## Core entities

| Entity | Purpose | Key fields |
|---|---|---|
| Pack | distributable content unit | id, version, schemaVersion, dependencies, loadPriority, conflicts |
| Localization | strings | key, language, text, context |
| StageLadder, Tier, Difficulty | power ruler | stage, tier, multipliers, capacities, content bands |
| Role | fallback and classification identity | roleId, parentRole, tags, defaultFamily, hostPolicy, protectionPolicy |
| RaceProfile | race to role mapping | raceRef or descriptor, roles, family, hostEligible, protectionEligible, priority |
| ActorTemplate | spawnable actor | key, baseActorRef, role, stageRange, levelBand, packages, equipmentSet, lootTable, tags, exclusions |
| ActorArchetype | build variation | key, role, packagePool, gearPool, statBias, abilityPool, weight |
| Ability, Effect | gameplay effects | key, trigger, target, effects, conditions, cooldown, budgetCost, vfx/sound refs |
| EncounterTemplate | spawn contract | key, class, roleWeights, stageRange, actorCount, caps, cooldown, conditions, rewardTable, identityPolicy |
| SpawnRule, WorldBand | where and when | region/cell/encounterZone ref, stageBand, rate, conditions, maxPerCell, maxActive |
| LootTable, LootEntry | rewards | key, entries, weights, count, stageBudget, luckBias, firstOnly |
| ItemDefinition | craftable or droppable item template | key, baseItemRef, slot, tags, rarityBand, ilvlBand, affixPool, sockets, stats, value, recipeRefs |
| MaterialDefinition | monster parts and tokens | key, itemRef/base, family, grade, tags, tierBand, tokenFallback, sources |
| Affix, RuneWord | item modifiers | key, tier, slots, weight, effectRef, magnitudeRange, prerequisites, exclusionGroup |
| Recipe, CraftOp | crafting | key, opType, inputs, outputs, currencyCost, station, skillGate, levelGate |
| Quest, HuntContract | quests, hunts, raids | key, type, stages, objectives, rewards, encounterRefs, unlockRequirements |
| PlayerProgression | player rank, attributes, class data | rankCurve, pointsPerLevel, breakthroughBonuses, attributeDefs, classRefs, skillTreeRefs |
| RuntimeProfile | runtime-generated fallback | raceDescriptor, role, stageRange, materialRefs, itemRefs, lootTable, generatorVersion, cacheKey |

## Relationships

Pack contains entities. RaceProfile points to Role and ActorTemplate. EncounterTemplate points to ActorTemplates, LootTable and SpawnRule. ActorTemplate points to Role, ActorArchetype, Ability, equipment ItemDefinition and LootTable. LootTable points to ItemDefinition, MaterialDefinition or Affix. ItemDefinition points to base FormRef, affix pool and Recipe/CraftOp. MaterialDefinition points to item/token, family and grade. Quest/Hunt points to EncounterTemplate, rewards and requirements. PlayerProgression points to classRefs, attributeDefs and rankCurve.

## Runtime fallback

Runtime generates only content a race needs for hunting: race detected, RaceProfile/Role resolution, RuntimeProfile with raceDescriptor, role, stageBand, family, materialRefs, itemRefs, lootTable and generatorVersion. It is cached as JSON under Data/Runefall/runtime/ and applied by the DLL. General fallback is role-based and shared by any race with the same role. Unknown race defaults to the Unknown role with a conservative generic profile; content packs override by race key. Generation is lazy and delayed (initial scan delayed, default ten minutes) to avoid load spikes.

## Versioning

Entity schemaVersion, pack schemaVersion and generatorVersion. Pack manifest lists required schema ranges; incompatible packs fail closed. RuntimeProfile stores generatorVersion; regeneration policy is data and config driven. Content key renames or deprecations require a migration mapping.

## Validation criteria

1. JSON schema valid, all required fields and enums.
2. All content refs resolve; no duplicate keys; no cycles.
3. Role/race/pack ownership consistent; no cross-pack key collisions.
4. Balance budgets: stage ranges, capacity, encounter cost, loot budgets.
5. Localization keys exist for every player-facing string.
6. FormRef allowed kind only: editorId, masterForm or unique; no generated FormIDs.
7. Record and asset caps and load-time budgets.
8. Runtime rule data passes the same schema and fallback simulation.


## 8. Extended entities (Skyrim + ARPG gaps)

### Skyrim-native entities

| Entity | Purpose | Key fields | Status |
|---|---|---|---|
| Worldspace, Region, Cell | location tree | key, parent, worldspace, region, tags | later |
| EncounterZone | vanilla difficulty source | zoneRef, minLevel, maxLevel, flags | later |
| Location | named place | locationRef, keywords, parent, hold | later |
| PlacedRef | authored reference | cellRef, formRef, position, rotation, scale, tags | content pack |
| MapMarker | map visibility | ref, type, nameKey, discovered | content pack |
| Faction | ownership/relations | key, parent, ranks, crime, reactions | later |
| AI Package / CombatStyle / Outfit | behavior | key, templateRef, data | later |
| LeveledList | engine distribution | key, entries, level, flags | later, avoid for Runefall loot |
| Container / Vendor / Economy | world containers, trade | key, items, respawn, vendor data | later |
| Quest / Stage / Objective / Alias | quest graph | key, parent, stages, conditions, scripts (data only) | later |
| Dialogue / Scene / Voice | conversation | key, speaker, lines, conditions, audioRef | later |
| StoryEvent | story manager trigger | manager, event, conditions, questRef | later |
| Weather / TimeCondition | spawn conditions | weatherRef, timeWindow, season | later |
| Sound / Music / VisualEffect | presentation | key, formRef, attach point, tags | later |
| Armor / Weapon / Body assets | item visuals | key, baseFormRef, modelRef, bodySlots, tags | content pack |

### ARPG entities

| Entity | Purpose | Key fields | Status |
|---|---|---|---|
| ItemBase | base item family | key, baseItemRef, slot, tags, ilvlRange, valueCurve | core later |
| Rarity / AffixTier / RollRange | item power | key, rank, weights, ilvlBand, magnitude | core later |
| UniqueItem / SetItem | authored special gear | key, baseRef, fixed affixes, setRef, loreKey | content pack |
| Socket / Rune / RuneWord | runic crafting | key, sockets, runeSequence, resultAffix, baseRestriction | core later |
| CraftingCurrency | crafting materials | key, itemRef, grade, source, exchange | core later |
| CraftStation / CraftOp | crafting UI/logic | key, station, operation, inputs, outputs, gates | core later |
| Recipe | crafting graph | key, outputs, inputs, currency, skillGate, levelGate | core later |
| SkillTree / PassiveNode / ActiveSkill | player build | key, classRef, nodes, costs, prerequisites, effects | future module |
| Class / Ascendancy | player class | key, name, advantages, treeRefs, unlockQuest | future module |
| MonsterModifier / Aura / Phase | enemy variation | key, role, effects, conditions, budget | core later |
| BossPhase / EncounterWave | encounter script | key, phase, triggers, adds, rewards | content pack |
| LootPool / Pity / MagicFind | loot control | key, entries, weights, pity, authority | later |
| Map / Raid | endgame content | key, region, waves, bosses, rewards, difficulty | future module |
| ChallengeBand / TTKTarget | balance | key, class, ttk/ttd/q ranges, weight | core later |
| RatingProfile / ActorThreatProfile | PCR/NCR/RCR | key, formulas, weights, fallbackGroup | core later |

## 9. Deferred entity extensions

These entities are intentionally reserved and added with their module, not guessed into the first schema:

- dialogues, scenes, voice, story manager, weather/time conditions
- cells/worldspaces/regions/placed refs, map markers, level lists, vendors
- factions, AI packages, combat styles, outfits
- unique items, set items, sockets/runes/runewords, crafting currencies/stations
- player classes, skill trees, active skills, raids/maps
- rating bands, TTK targets and calibration reports

Each extension must follow the same pack/version/ref/criteria rules as the core entities.
