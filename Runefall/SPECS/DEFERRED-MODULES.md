# Deferred modules - register and interfaces

Status: **scope register 2026-10-28**. The power-ladder module is the ruler only
(`SPECS/POWER-LADDER.md`). Everything below is a separate module/slice. Do not put its data,
records, UI or balance into the ladder.

## 1. Module register

| # | Module | Owns | Consumes from ladder | Does not own |
|---|---|---|---|---|
| 1 | **POWER-LADDER** (current) | stages 1-100, 10 tiers, curve, StageProfile, assignment/clamp | - | races, damage, spawns, abilities, materials, UI |
| 2 | **THREAT-PROTECTION** | damage hook, category matrix, protection formula/params, penetration sources/stacking, player/anomaly target policy | StageProfile attacker/target, stage gap | stage tables, race tables, spawning |
| 3 | **RACE-CLASSIFICATION** | race/template/keyword classifier, host/anomaly/provisional tags, unknown-race report/patch table | stage profile tags | stage curve, damage |
| 4 | **LEVEL-DESIGN / ENCOUNTERS** | world bands, encounter classes, spawn tables, placement, caps/cooldowns, encounter identity | StageProfile schema, RequestStage | race classifier, abilities |
| 5 | **ENEMY-ABILITIES** | passive/active packages per tier/family, tier keywords usage, ability records | StageProfile/tier | stage curve, spawn placement |
| 6 | **ANOMALY-LADDER** | anomaly families, anomaly curves/tier labels, anomaly abilities, anomaly reward sources | shared 100-stage axis + arithmetic | host ladder tiers, crafting recipes |
| 7 | **CRAFTING-MATERIALS** | material items/tokens, recipes, stations, Monster Hunter gear trees, material economy | reward tags/item IDs from encounters | stage/encounter/damage |
| 8 | **EQUIPMENT / ACTOR-ENHANCEMENT** | humanoid gear/perk distribution, random enhancement tables, spawn/apply tables, toggle/cleanup policy | StageProfile/tier, encounter class | stage curve, player progression |
| 9 | **PLAYER-PROGRESSION** | player Rune Rank 1-100, attributes, skill trees, active skills, respec | shared ladder arithmetic | enemy ladder/content |
| 10 | **UI-REDESIGN** | Anomaly Codex, character pages, inspect/tooltip, menu slices | snapshots from services | gameplay rules |
| 11 | **COMPATIBILITY / RELEASE** | load-order tests, uninstall, packaging, versioning | all modules | module internals |

## 2. Interface contracts

### POWER-LADDER -> consumers
- `StageProfile { stage, tier, source, flags, profileId }`
- `RequestStage(StageRequest)` validates/clamps a request supplied by the consumer.
- `Power(stage)`, `StatMultiplier(stage, kind)`, `TierOf(stage)`.
- No consumer data is read by the ladder.

### THREAT-PROTECTION
- Input: attacker StageProfile, target StageProfile, damage category, penetration points.
- Output: final damage multiplier (once), bypass reason.
- Owns `damage_categories.csv`, `protection_rules.csv`, `penetration_sources.csv`.
- Must prove the hook with the Phase-1 spike before stage scaling ships.

### RACE-CLASSIFICATION
- Input: actor base/ref, race, template chain, keywords.
- Output: tags `HOST`, `ANOMALY`, `PROVISIONAL`, family/class ids.
- Owns `race_classification.csv`, class taxonomy, provisional fallback/report.
- Unknown race = provisional host by default; never skipped.

### LEVEL-DESIGN / ENCOUNTERS
- Owns `world_bands.csv`, `encounter_classes.csv`, `spawn_tables.csv`.
- Produces `StageRequest` (band/stage/source/seed) for the ladder.
- Excludes cities/quest actors and applies caps/cooldowns.

### ENEMY-ABILITIES
- Consumes tier/stage; owns ability tables/records.
- No per-frame scanning; packages applied at spawn/load with caps.

### ANOMALY-LADDER
- Separate spec/module. Shares the 100-stage axis and arithmetic, but defines its own tier labels,
  family curves, abilities and reward sources. No anomaly data in the host ladder.

### CRAFTING-MATERIALS
- Owns real material items/tokens, recipes, stations, gear trees.
- Consumes reward tags/item IDs (host materials, anomaly parts, trophies); the encounter/ladder
  module never defines recipes.
- Material items are physical MISC items, not currency; tokens are explicit fallback placeholders.

### EQUIPMENT / ACTOR-ENHANCEMENT
- Consumes StageProfile/encounter class; owns spawn/apply tables, random enhancement chance,
  humanoid gear/perk distribution and toggle/cleanup policy.
- Equipment is vanilla persistent state: toggles gate future application only; no mass removal.

### PLAYER-PROGRESSION
- Player Rune Rank 1-100 later; uses the shared ladder; no enemy content.
- No player protection by default; compatibility-first.

### UI-REDESIGN
- Separate slice; current UI is read-only/debug and will be redesigned.
- Consumes snapshots; never drives gameplay decisions.

## 3. Build order

1. POWER-LADDER data + validator + StageEngine (current).
2. THREAT-PROTECTION Phase-1 damage spike (hook/coverage).
3. RACE-CLASSIFICATION + provisional fallback.
4. LEVEL-DESIGN / ENCOUNTERS (world bands first).
5. ENEMY-ABILITIES packages.
6. ANOMALY-LADDER.
7. EQUIPMENT / ACTOR-ENHANCEMENT.
8. PLAYER-PROGRESSION.
9. CRAFTING-MATERIALS.
10. UI-REDESIGN.
11. COMPATIBILITY / RELEASE.

Items 2-4 are the critical path for the first playable enemy slice. Items 7-10 can run in parallel
once the ladder and encounter contracts are stable.

## 4. Rule

**No module may edit another module's data or records directly.** Cross-module behavior goes
through the interfaces above. If a requirement seems to belong in two modules, it belongs in the
consumer.
