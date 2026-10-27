# Deferred modules - register and interfaces

Status: **aligned to the audited ladder, 2026-10-28**. The ladder owns stages, curves, capacities,
content bands and difficulty profiles. Everything else is a separate module. No module may
duplicate ladder tables or edit another module's data.

## 1. Register

| # | Module | Owns | Consumes from ladder | Must not own |
|---|---|---|---|---|
| 1 | POWER-LADDER (current) | 100 stages, 10 tiers, curve, capacities, content bands, difficulty profiles, assignment, ladder co-save state | - | protection, world bands, races, abilities, materials, UI |
| 2 | THREAT-PROTECTION | damage hook, category matrix, protection/penetration, player/anomaly target policy | StageGap/PowerRatio, StageProfile | stage/curve tables |
| 3 | RACE-CLASSIFICATION | race/template/keyword classifier, host/anomaly/provisional tags, unknown report | StageRequest only if it needs a stage; normally tags only | stage curve |
| 4 | LEVEL-DESIGN / ENCOUNTERS | world bands, encounter classes, spawn tables, placement, caps, encounter identity | StageProfile schema, RequestStage, content bands | ladder tables |
| 5 | ENEMY-ABILITIES | passive/active packages per tier/family, ability records | tier, capacity, content bands | stage curve |
| 6 | ANOMALY-LADDER | anomaly families, anomaly curve registration, anomaly tier labels, anomaly abilities, anomaly rewards | CurveEvaluator/curve registration, shared stage arithmetic | host ladder tables |
| 7 | CRAFTING-MATERIALS | material items/tokens, recipes, stations, Monster Hunter gear trees | rewardCapacity, encounter reward tags | stage/encounter/combat |
| 8 | EQUIPMENT / ACTOR-ENHANCEMENT | humanoid gear/perk distribution, random enhancement tables, tag/cleanup policy | tier/capacity/content bands | stage curve |
| 9 | PLAYER-PROGRESSION | player Rune Rank economy, attributes, skill trees, active skills, respec | kPlayerRank, rank==stage contract | enemy/encounter data |
| 10 | UI-REDESIGN | Anomaly Codex, character pages, inspect/tooltip slices | snapshots | gameplay rules |
| 11 | COMPATIBILITY / RELEASE | load-order tests, uninstall, packaging, versioning | all module contracts | module internals |

## 2. Interfaces

### POWER-LADDER -> consumers
- StageProfile { stage, tier, curve, source, flags }.
- RequestStage(StageRequest) with explicit precedence and deterministic seed.
- Power, StatMultiplier, TierOf, Capacity, ContentBand, Difficulty, StageGap, PowerRatio.
- Ladder co-save: player rank/stage, difficulty profile, manual overrides only (cap 256).

### THREAT-PROTECTION
- Input: StageProfile attacker/target, damage category, penetration.
- Output: final damage multiplier (once), bypass reason.
- Owns protection formula, category matrix, penetration sources, player/anomaly policy.
- Must prove its hook with the Phase-1 spike; ladder exposes only gap/power-ratio arithmetic.

### RACE-CLASSIFICATION
- Outputs tags HOST, ANOMALY, PROVISIONAL plus family/class ids; never sets ladder stage.
- Unknown race = provisional host by default; patchable table; log/report.

### LEVEL-DESIGN / ENCOUNTERS
- Owns world bands/spawn tables/placement; produces StageRequest with band/stage/source/seed.
- Uses ladder content bands for gating and caps; no ladder edits.

### ENEMY-ABILITIES
- Consumes tier/capacity/content bands; owns ability records and per-tier packages.
- Applied at spawn/load with caps; no per-frame scanning.

### ANOMALY-LADDER
- Registers anomaly curve profiles (same stage axis) and own tier labels/families/rewards.
- No anomaly data in the host ladder; host curve remains unchanged.

### CRAFTING-MATERIALS
- Owns real MISC material items, fallback tokens, recipes, stations, MH gear trees.
- Consumes ladder rewardCapacity/encounter tags; no ladder/encounter logic.
- Materials are items, not currency; tokens are explicit fallback placeholders.

### EQUIPMENT / ACTOR-ENHANCEMENT
- Consumes tier/capacity/content bands; owns gear/perk distribution, random apply tables.
- Equipment is persistent vanilla state; toggles gate future application only, no mass removal.

### PLAYER-PROGRESSION
- Owns Rune Rank 1-100 economy/attributes/skills/trees; supplies kPlayerRank.
- Ladder does not own rank gain, respec or attunement.

### UI-REDESIGN
- Separate slice; consumes read-only snapshots; no gameplay rules.

## 3. Build order

1. POWER-LADDER data, host core, StageEngine/RequestStage.
2. THREAT-PROTECTION Phase-1 damage spike.
3. RACE-CLASSIFICATION.
4. LEVEL-DESIGN / ENCOUNTERS.
5. ENEMY-ABILITIES.
6. ANOMALY-LADDER.
7. EQUIPMENT / ACTOR-ENHANCEMENT.
8. PLAYER-PROGRESSION.
9. CRAFTING-MATERIALS.
10. UI-REDESIGN.
11. COMPATIBILITY / RELEASE.

Items 2-4 are the critical path for the first playable enemy slice.

## 4. Rule

No module may duplicate ladder tables (stage, tier, curve, capacity, content band, difficulty).
Consumers use the ladder API and their own module data. If a requirement appears in two modules, it
belongs to the consumer, not the ladder.
