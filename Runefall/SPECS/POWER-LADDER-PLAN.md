# POWER-LADDER - implementation plan (audited revision)

Status: **revised 2026-10-28 after design/technical/consistency audits**. This plan matches
SPECS/POWER-LADDER.md. No deferred module data is implemented here. A re-audit must pass before S1.

Audit inputs: DESIGN (FAIL, 6 blockers), TECHNICAL (FAIL, 7 blockers), CONSISTENCY (PASS WITH
FIXES, 6 blockers). Fixes are folded into the slices. Audit evidence belongs in
Runefall/build/evidence/.

## 0. Definition of done

1. Seven ladder-owned tables generated, validated, loaded with formats v2 and table descriptors.
2. 100 stages/10 tiers; curves; capacities; content bands; difficulty profiles; assignment rules.
3. StageEngine exposes the full API in POWER-LADDER section 10: O(1), zero allocation after load.
4. RequestStage deterministic ABI with golden vectors; unknown input fails closed.
5. Tier keywords RF_KW_Tier_01..10 generated/catalogued; old six retired; form_ids updated.
6. Co-save v3 ladder section plus v2 migration; caps/truncation; downgrade log.
7. Host core target plus Catch2 tests; in-game self-tests/batch; performance gates.
8. POC RF-Tier/rank migration map and save migration plan; no dual power system.
9. Docs (LORE/GAME-DESIGN/L1/L4/ARCHITECTURE/CHANGELOG/VALIDATION/STATE) aligned.
10. No threat-protection/world-band/anomaly/crafting/actor-enhancement/player-economy/UI code.

## 1. Requirements coverage

| Requirement | Coverage | Owner |
|---|---|---|
| 100 stages / 10 tiers / names | stage_ladder + tier_definitions | ladder |
| x15 curve, health/damage/regen/armor | curve_profiles + stage_ladder | ladder |
| Neutral capacities | stage_ladder capacity columns + formulas | ladder |
| Content gating | stage_content_bands: player level, ilvl, gear check, unlock, v1 coverage | ladder |
| Difficulty presets | difficulty_profiles without protection params | ladder |
| Deterministic assignment | StageRequest ABI + rules + golden vectors | ladder |
| Player reference | kPlayerRank source, rank==stage contract; economy deferred | ladder + PLAYER-PROGRESSION |
| Tier breakpoints | tier_definitions budgetStep/unlockTag + measurable deltas | ladder |
| Localization | nameKey + strings.csv keys | ladder |
| Composition | one stage layer; consumer overlays once | ladder contract |
| Persistence | co-save v3 ladder section: player rank/stage, difficulty, overrides only | ladder |
| POC migration | legacy_rf_tier_map, save map, old keyword/spell retirement | ladder migration slice |
| Protection/world/anomaly/abilities/crafting/equipment/UI | deferred | consumer modules |
| Calibration | reproducible curve/capacity report; TTK deferred to combat module | ladder tool |
| Compatibility/uninstall | pack removal/downgrade fallback, no required external mod | ladder + compatibility |

## 2. Slices

| ID | Slice | Deliverables | Acceptance | Depends |
|---|---|---|---|---|
| S0 | Fix blockers and freeze | revised spec/plan; D-068; LORE/GAME-DESIGN/L1/L4/ARCHITECTURE co-save sync; audit evidence; re-audit pass | all audits no BLOCKER; spec/plan table inventory identical | - |
| S1 | Schema and loader plumbing | formats.json format 2 per-table schemas; DataStore descriptors; generator skeleton; seven tables; legacy map | DataStore loads seven; unknown table rejected/logged; format bump test | S0 |
| S2 | Generate ladder data | 100 rows; 10 tiers; curve/difficulty/assignment/content/legacy rows; nameKeys; deterministic output | PASS: row counts, endpoints, monotonic, capacity formulas, bands | S1 |
| S3 | Validator and gates | rule matrix mapping spec gates 1-11; static-gate wiring; bad-data diagnostics | validator PASS and intentional bad-data tests fail; run-static-gates includes it | S2 |
| S4 | Host core target | RunefallCoreHost CMake target without RE/SKSE; pure types/tables/curve evaluator; Catch2 | host tests run with no game headers | S1 |
| S5 | StageEngine API | immutable StageTable; Power, StatMultiplier, TierOf, Capacity, ContentBand, Difficulty, StageGap, PowerRatio | API tests; O(1); zero allocation; no hardcoded curve | S4 |
| S6 | StageRequest determinism | ABI structs/enums; precedence; FNV-1a seed/jitter; clamp order; rejection; golden JSON vectors | vectors reproduce across builds; unknown source/rule fails; same seed same profile | S5 |
| S7 | Difficulty and composition | difficulty loading; stageDelta/stat/damage/reward multipliers; one stage layer; composition tests | profile applied once; no double-stage vector; out-of-range profile rejected | S5 |
| S8 | Content bands and capacities | content bands plus capacity API; tier breakpoint validation; consumer examples | 100 stages covered; tier deltas measurable; v1 coverage report | S5 |
| S9 | Co-save v3 ladder state | section 11 encode/decode; 256 override cap; v2 to v3 migration; downgrade log; truncation tests | round-trip/migration/corrupt rejection; save-size budget | S5 |
| S10 | Tier keyword/record pipeline | generate 10 keywords and strings; form_ids rows; retire RF_KW_Tier_0..5 from generation; maps; FormCatalog test | 10 keywords resolve; old IDs not reused; catalog gate | S2 |
| S11 | POC migration | legacy map application; old rank tables/keywords/spells retirement check; rfTier consumer guidance; old-save test | old save loads; no dual multipliers; migration report | S9/S10 |
| S12 | Calibration report | synthetic stage/capacity/stat report; curve/budget invariants; no TTK until protection module; deterministic output | report reproducible; mismatches fail | S2/S5 |
| S13 | Diagnostics snapshot | read-only snapshot JSON/log: table, profiles, counts, errors; no UI page | F7/console snapshot; no mutation | S5/S7/S8 |
| S14 | Host tests/benchmark | unit tests for pure logic; load/lookup/allocation benchmark; static grep for duplicated tables | load <=25 ms p99; lookup <=100 ns; zero allocations; grep gate | S4-S8 |
| S15 | In-game self-tests/batch | ladder self-tests in DebugHarness; batch checks; crash scan | self-tests pass; batch PASS; no new crash | S9-S14 |
| S16 | Performance/compat/uninstall | pack removal/downgrade fallback; uninstall test; large-table/load-order test | no crash; readable fallback logs; uninstall PASS | S9-S15 |
| S17 | Consumer contract review | read-only review of THREAT-PROTECTION, LEVEL-DESIGN, PLAYER-PROGRESSION, ANOMALY, ENEMY-ABILITIES, EQUIPMENT, CRAFTING, UI contracts | each names consumed ladder API; no duplicate ladder data | S5-S8 |
| S18 | Docs/release gate | update docs, changelog/validation/state/version; static gates and archive verify | all gates PASS; no unresolved BLOCKER | all |

## 3. Milestones

| Milestone | Slices | Result |
|---|---|---|
| M0 Audit closure | S0 | blocker-free spec/plan and reconciled docs |
| M1 Data and validation | S1-S3 | generated/validated tables, format v2 |
| M2 Ruler runtime | S4-S6 | host-tested StageEngine and RequestStage |
| M3 Overlays/content | S7-S8 | difficulty, content bands, capacities |
| M4 Persistence/migration | S9-S11 | v3 co-save and POC migration |
| M5 Tooling/evidence | S12-S15 | calibration, diagnostics, tests |
| M6 Hardening/release | S16-S18 | compatibility, docs, gates |

## 4. Acceptance vectors

- Curve: P(1)=1.0, P(100)=15.0, tolerance 1e-4; health/damage/regen/armor match data.
- Tier: every stage maps to one tier; ten contiguous ranges; each breakpoint has a capacity step.
- Capacity: formulas match exactly; monotonic; integer rounding; stage 1/50/100 match spec table.
- Content bands: 100 stages covered without overlap; item/player ranges monotonic; v1 coverage reported.
- Assignment: fixed golden vectors for source, seed, override, jitter, clamp; same seed same profile.
- Composition: synthetic actor vector proves one stage application; no rank double.
- Co-save: v2 to v3 migration runeTier 0..5 to player rank stages 1/20/40/60/80/100; override cap 256; truncation rejection.
- Performance: load p99 <=25 ms; lookup <=100 ns; zero allocations after load; no per-frame hook.
- Static: no duplicated curve/tier/capacity table in C++ or Papyrus; Papyrus policy gate PASS.

## 5. Migration mapping

| POC data | New ladder |
|---|---|
| RF Tier 0-5 world axis | stage ranges 0:1-15, 1:16-30, 2:31-45, 3:46-60, 4:61-80, 5:81-100 |
| player runeTier 0-5 | player rank stage 1/20/40/60/80/100 in co-save v3 |
| old enemy ranks 0-5 with hp/dmg/armor multipliers | retired; stage curve replaces; no dual scaling |
| RF_KW_Tier_0..5 | retired from generation; map to RF_KW_Tier_01..10; IDs not reused |
| RF_SPEL_Rank_1..5 and buff MGEF | retired from generation; stage packages owned by later enemy module |
| enemy_ranks.csv, spawn_weights.csv, drops_* rfTier keys | owning modules migrate via legacy_rf_tier_map; ladder does not edit them |
| co-save v2 | v3 migration; section 11 ladder state; downgrade refusal |

## 6. Risks as acceptance criteria

| Risk | Acceptance |
|---|---|
| Curve not expressive late | capacity/stat table and tier breakpoint deltas validated; calibration report |
| Two sources of truth | spec/table inventory diff = 0; static grep gate |
| Nondeterminism | golden vectors and same-seed test across runs/builds |
| Double scaling | composition vector fails if any consumer applies stage twice |
| Save bloat | ladder section <=1 KB for 256 overrides; truncation test |
| Mod compatibility | no required external mod; pack removal fallback log; uninstall test |
| Scope creep | S17 review rejects duplicate ladder data |
| Old data breaks | migration test from 0.1.0 save/pack; old keywords mapped/retired |
| Fictional gates | every gate names an existing script and expected output |
| DataStore hardcoding | descriptor-driven table list and format v2 load test |

## 7. Re-audit gate

Before S1: DESIGN audit no BLOCKER; TECHNICAL audit no BLOCKER; CONSISTENCY audit no BLOCKER.
Write evidence to Runefall/build/evidence/audit-power-ladder-KIND-STAMP.md and summarize in
VALIDATION.md. Only then start S1.
