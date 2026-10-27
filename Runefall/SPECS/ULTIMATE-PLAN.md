# ULTIMATE PLAN v2 - Runefall from foundation to release

Status: full plan, 2026-10-28. Built from the clarified decisions. JSON is the only authored and runtime content format. The plan is executed one slice at a time; each slice ends with data, implementation, tests and evidence.

## 1. Goal

Build Runefall for Skyrim SE/AE 1.6.1170 from the current POC to a complete release: 100-stage/10-tier ladder, threat protection, races/roles, encounters, hunts, anomaly content, loot, crafting, player progression/classes, Prisma UI, content packs, compatibility and release.

## 2. Architecture rules

1. Data first: content, balance, pacing, ratings and fallback rules are JSON data. Code loads, validates, resolves and executes.
2. One content schema, two producers: offline compiler for shipped packs; runtime DLL resolver for unknown mods and events. Both use the same entity shapes.
3. Content keys, not FormIDs: authored content uses stable keys. FormIDs appear only inside generated ESP records and explicitly allowed unique/master references.
4. Offline packs are pre-generated; the DLL applies them. Runtime fallback is generated lazily, cached under Data/Runefall/runtime/ and delayed (default 10 minutes) to avoid load spikes.
5. No JContainers and no heavy Papyrus. The DLL owns JSON I/O, storage and gameplay.
6. Minimal dependencies: Prisma UI is the hard UI dependency; everything else optional unless proven necessary.
7. Packs avoid conflicts by namespaced keys and design; last loaded wins on the same key, like Skyrim. No explicit load-order language is required.
8. English only for release; localization fields reserved, other languages are not supported by default.
9. Fail closed: invalid data disables the feature/pack with a log, never crashes the game.
10. One slice at a time; no next slice until the current one has build/test/game evidence.

## 3. Layers

| Layer | Purpose | Exit |
|---|---|---|
| L0 Foundation | ladder fixes, JSON pipeline, records, co-save, runtime resolver, tests | data-driven foundation proven |
| L1 Combat | protection, roles/races, NPC packages, player attributes/ratings | first staged fights and ratings |
| L2 World | bands, spawns, rare spawns, hunts, bosses, anomaly, runtime cache | world content appears correctly |
| L3 Loot and crafting | loot, materials, runtime items, crafting, rune gear, economy | loot-to-craft loop |
| L4 Player and UI | Rune Level, 9 classes, skills, Prisma UI, HUD, menus | playable end-to-end |
| L5 Packs/endgame | pack tooling, base packs, mod packs, agent skills, raids/quests, localization | content scaling |
| L6 Release | compatibility, save/upgrade, calibration, packaging | ship gate |

## 4. Full slice table

| ID | Layer | Slice | Output | Acceptance | Depends |
|---|---|---|---|---|---|
| F0.1 | L0 | Ladder audit fixes (first slice) | fix difficulty default, full RequestStage ABI, curve/legacy load, validation, actor-stage loot bridge, co-save v3 ladder state, retire old tier keywords, generator validator, host data/benchmark | audit blockers closed, 20+ self-tests, batch PASS, stage drives actor loot ilvl, no crash; legacy rank tables remain a documented shim until A3.1 | current |
| F0.2 | L0 | Freeze JSON schema | one JSON Schema per entity; pack manifest; versioning; examples | all current content represented; schema validation passes; no CSV authoring | F0.1 |
| F0.3 | L0 | JSON content pipeline | generator CLI validate/build/preview; JSON to ESP/data/manifest; deterministic IDs | base pack builds from JSON; invalid pack fails with path/line | F0.2 |
| F0.4 | L0 | JSON runtime loader | DataStore JSON reader; table registry; schema version checks; no CSV code path | runtime loads JSON; missing/corrupt table fails closed | F0.3 |
| F0.5 | L0 | Co-save v3 | section registry; ladder state; migration v2 to v3; caps/truncation; downgrade log | old save loads; corruption rejected; save size budget | F0.4 |
| F0.6 | L0 | Host core and tests | RunefallCoreHost target; Catch2; benchmarks; no-allocation gates; static schema tests | host CI PASS; load/lookup budgets measured | F0.3-F0.5 |
| F0.7 | L0 | Runtime content resolver | identity/role resolver; deterministic fallback; cache; pack override; unknown race path | unknown race resolves; repeatable; cache reload; no crash | F0.4/F0.6 |
| F0.8 | L0 | Diagnostics and rescan | read-only snapshot; settings Rescan button; automatic rescan on pack/version change; delayed startup scan | manual and automatic rescan work; no load spike | F0.7 |
| F0.9 | L0 | POC JSON migration | convert all current CSV content/balance to JSON; keep POC behavior; deprecate CSV | generator and runtime both use JSON only; regression PASS | F0.3/F0.4 |
| C1.1 | L1 | Protection spike | damage hook proof; category matrix; apply-once; pipeline position; perf | spike covers direct/DoT/reflected/environmental; no crash | F0.4 |
| C1.2 | L1 | Roles and race classification | primary role + tags; host/anomaly/provisional; unknown fallback; patch table | vanilla/DLC mapped; modded race defaults; no content gap | F0.7 |
| C1.3 | L1 | Threat protection | 70% cap, 3%/stage, 5% floor, 50% DoT, reflected/environmental bypass, penetration; player/anomaly policy | staged fights respect gap; apply-once; no crash | C1.1/C1.2 |
| C1.4 | L1 | NPC packages | spawn stage + role package + 2-4 perks + gear/outfit + loot; light archetype tag; no deep sim | humanoid/creature packages apply once; gear remains vanilla-lootable | C1.3 |
| C1.5 | L1 | Actor equipment distribution | equipment sets/outfits; random archetype variation; toggles/cleanup; performance caps | selected actors get gear/perks; toggle safe; save/load safe | C1.4 |
| C1.6 | L1 | Player attributes | 9 attributes; 3 points/stage; breakthroughs 9/12/15...; effects and caps; respec | attribute budget data-driven; PCR follows gear/class | F0.4 |
| C1.7 | L1 | Combat ratings | PCR/NCR/RCR; TTK/TTD/Q; Skyrim and Action pace profiles; both number+label; data JSON | ratings reproducible; formulas match data; display profiles | C1.3/C1.6 |
| C1.8 | L1 | Player attacker/penetration | player stage input; attack channel; penetration sources/cap; mage channel | warrior/mage both penetrate; player no protection default | C1.3/C1.6 |
| W2.1 | L2 | World bands | encounter zones/cells/regions to stage bands; content bands; exclusions | data drives area stage; vanilla zones respected | C1.4 |
| W2.2 | L2 | Spawn and encounter system | spawn rate tables; encounter classes; caps/cooldowns; identity/cleanup; safe locations | spawns follow data; reload-safe; no city/quest leaks | W2.1 |
| W2.3 | L2 | Rare high-stage spawns | rate-table high-stage chance; condition hooks reserved for packs | low-level zone can rarely spawn high stage; rate tunable | W2.2 |
| W2.4 | L2 | Hunt contracts | player-called hunts; hunt identity; reward/evidence; safety | hunts work end-to-end; no duplicate rewards | W2.2 |
| W2.5 | L2 | Bosses and unique actors | fixed-stage overrides; unique FormID refs allowed; boss packages; quest bands | authored bosses at designed stage; unique refs resolve | W2.2/W2.4 |
| W2.6 | L2 | Anomaly ladder | families, curves, tier labels, anomaly encounters, cores/trophies | anomaly content works without host protection | C1.2/W2.2 |
| W2.7 | L2 | Runtime cache lifecycle | race-role JSON cache; pack/version invalidation; manual rescan; old cache migration | cache correct after pack change; rescan safe | F0.7/W2.2 |
| A3.1 | L3 | Ladder-driven loot | loot tables by stage/ilvl; containers/quests/bosses; budgets | loot follows stage and data; no hardcoded values | W2.2/C1.4 |
| A3.2 | L3 | Materials and tokens | real MISC items; family/grade parts; fallback tokens; inventory ledger | materials are items; unknown roles get tokens | C1.2 |
| A3.3 | L3 | Runtime item generation | item instances from base forms; stats/affixes/runewords; no new item records for ordinary crafting | generated item persists; visuals/name correct; no crash | A3.2/F0.7 |
| A3.4 | L3 | Crafting system | MH/Last Epoch style destroy-and-create; recipes/stations/currencies; validation | full crafting loop; transaction-safe | A3.3 |
| A3.5 | L3 | Rune equipment and inventory | rune gear generation; sockets/runewords; rune inventory UI data | rune gear works; inventory data stable | A3.4 |
| A3.6 | L3 | Rune economy and attunement | rune XP, wallet, attunement, respec, glob mirrors | progression with ladder; save-safe | A3.1 |
| A3.7 | L3 | Loot authority compat | additive/takeover modes; OWL and major loot mods; pricing | compatibility smoke PASS; no crash | A3.1 |
| P4.1 | L4 | Rune Level progression | XP gauge; kills/potions/quests hooks; 40-hour pacing; tier breakthroughs | 40-hour targets data-driven; breakthrough works | C1.6/F0.5 |
| P4.2 | L4 | 9 classes | class identities, tiers, advantages, class quests, respec | 9 classes data-driven; class build differs | P4.1 |
| P4.3 | L4 | Skills and passives | player skill tree; active skills; NPC packages separate; no shared player nodes | builds playable; no NPC skill contamination | P4.2 |
| P4.4 | L4 | Prisma UI foundation | hard dependency; web UI shell; settings/rescan; data bridge | UI loads; read-only snapshot works; no gameplay in UI | F0.8 |
| P4.5 | L4 | Character/HUD/indicators | character page; PCR/NCR labels/numbers; HUD indicators | ratings display correct; HUD optional/low cost | P4.4/C1.7 |
| P4.6 | L4 | Breakthrough/class menus | breakthrough UI; class/tier/advantage UI; materials/raid gates | menus complete; no Papyrus polling | P4.4/P4.2 |
| P4.7 | L4 | Codex/inspect/tooltip | anomaly codex; item inspect; tooltip slices; rune details | pages work; data from registry only | A3.5/P4.4 |
| R5.1 | L5 | Pack tooling | pack format, build/preview, versioning, dependency checks, conflict diagnostics | base + test pack; no load-order language | F0.3/F0.9 |
| R5.2 | L5 | Base content packs | Skyrim + official DLC foundation packs; races/roles/encounters/loot | base world fully staged; no gaps | R5.1/A3.7 |
| R5.3 | L5 | Famous-mod packs | pack per supported mod; race/role/loot additions; patchable overrides | supported mod smoke PASS | R5.1/C1.2 |
| R5.4 | L5 | Agent skills and criteria | content skills per entity; criteria rules; generator prompts; validation loop | agent pack produced, validated, built, tested | R5.1/F0.6 |
| R5.5 | L5 | Endgame, raids, quests | raid bosses; quests; endgame maps; dungeon hunts; unique rewards | endgame loop complete | W2.5/W2.6/A3.6 |
| R5.6 | L5 | Localization structure | English only; string keys; optional language packs reserved | all player text localized by key; no unsupported language claim | R5.1 |
| R6.1 | L6 | Compatibility matrix | OWL, Worlds Dawn, Fortunate Foes, KYE2, loot/combat mods; load-order profiles | target smoke PASS; conflicts documented | all gameplay |
| R6.2 | L6 | Save/upgrade/uninstall | old save migration; pack disable; DLL remove; downgrade | all uninstall modes PASS | F0.5/A3.7 |
| R6.3 | L6 | Balance calibration | PCR/NCR/TTK/TTD report; 40-hour pacing; pace profiles; playtest tuning | targets met or documented | C1.7/P4.1/P4.2 |
| R6.4 | L6 | Release packaging | version; FOMOD; Nexus assets; archive verification; ship gate | ship gate PASS; archive verified | R6.1-R6.3 |

## 5. First slice F0.1 detail

1. Fix StageRequest default difficulty and full ABI (source, ruleId, category, player rank, precedence, jitter, clamp, rejection, golden vectors).
2. Load and validate curve profiles and legacy RF tier map.
3. Validate tiers, content bands, difficulties, rules, capacities, keyword ids.
4. Wire StageEngine into actor rank/loot paths; remove the POC 5-rank dual system.
5. Co-save v3 ladder section and v2 migration.
6. Retire RF_KW_Tier_0..5 with mapping; regenerate keywords.
7. Generator validator, host core tests and benchmarks.
8. Batch checks and no-crash evidence; update docs and commit.

## 6. Balance defaults

- Player attributes provide 33 percent of same-stage enemy power; gear/class provides 67 percent.
- Three tuning levers: attribute points earned, enemy power, per-attribute power. Default: keep 3 points/stage; tune per-attribute power first.
- 40-hour rune play: tier hours 1.5/2.5/3/3.5/4/4.5/5/5/5/6; about 3600 rune kills, 36 per stage average; data-driven.
- Protection: 70 percent cap, 3 percent per stage, 5 percent floor, DoT 50 percent protection, reflected/environmental none, penetration cap 20 stages.
- Two pace profiles: Skyrim and Action/ARPG, selected in settings; default Skyrim.
- Ratings visible as number + label; HUD later.

## 7. Deferred clarification gates

G1 data pipeline: answered, JSON only; G2 protection: answered; G3 roles: answered; G4 world: rate tables plus later conditions; G5 enemy packages: simple; G6 equipment: vanilla loot; G7 anomaly: later; G8 crafting: MH/Last Epoch style; G9 player: 9 classes, 33 percent; G10 UI: Prisma; G11 compatibility: known mods; G12 runtime generation: offline and runtime, own JSON.

Remaining design work is inside the slice that owns it, not before foundation.

## 8. Goal proposal

Objective: Execute ULTIMATE-PLAN v2 from F0.1 through R6.4, one slice at a time, with JSON as the only content format, proving each slice with build, host tests, in-game batch and evidence, and finishing with a verified release candidate.

Completion: all 48 slices PASS; ladder/combat/world/loot/crafting/player/UI/packs/release gates PASS; no crash in batch; archive verified; docs/version/changelog updated; no unresolved blocker.

Execution rule: before each slice, read its exact row; after each slice, record evidence and update STATE/VALIDATION. No slice skips tests, and no deferred feature is invented outside its slice.