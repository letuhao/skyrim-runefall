# ULTIMATE PLAN - Runefall, layer by layer

Status: **draft 2026-10-28**, data-driven architecture. This is the master plan for completing
the mod. The first slice is the ladder audit fixes; every later slice adds one layer and must leave
the build/game in a proven state.

## 1. Final goal

Runefall 1.6.1170: a four-pillar RPG overhaul with a complete 100-stage/10-tier power ruler, enemy
threat system, encounters, loot/economy, crafting, player progression and UI, tested on the target
J: environment and packaged for release.

## 2. Architecture rule: data first, code second

All content and balance is data. Code only loads, validates, resolves and executes it.

    content source (CSV/JSON + pack manifests)
        -> Runefall.Build (records + data tables + form_ids)
        -> DataStore (typed tables, schema version, pack overlays)
        -> FormCatalog (record identity)
        -> services (ladder, loot, protection, encounters, abilities, UI)
        -> game

Rules:
1. Adding/altering content must be a data/pack change, not a C++ or Papyrus edit.
2. Generated ESP records come from the same data; no hand-edited records.
3. Base + optional packs + external patch overlays; every table has a schema version and a
   validator; invalid/missing data fails closed with a specific log.
4. No deferred module owns ladder tables; no module duplicates another's data.
5. One slice at a time: data/schema + implementation + tests + evidence before the next.

## 3. Layers

| Layer | Purpose | Exit condition |
|---|---|---|
| L0 Foundation | ladder fixes, data pipeline, records, co-save, tests | ruler is authoritative and data-driven |
| L1 Combat core | threat protection, race classification, enemy packages, actor enhancement | first staged enemy fight works |
| L2 World/encounters | world bands, spawns, hunts, bosses, anomaly content | enemies appear in the world correctly |
| L3 Loot/economy/crafting | ladder-driven loot, materials, crafting, gear, rune economy | full loot-to-craft loop |
| L4 Player/UI | player rank/attributes/skills, UI pages, inspect | playable end-to-end with UI |
| L5 Release | compatibility, save/upgrade, balance, packaging | release gate passes |

## 4. Slice table

| ID | Layer | Slice | Deliverables | Acceptance | Depends |
|---|---|---|---|---|---|
| F0.1 | L0 | **Ladder audit fixes (first slice)** | fix default difficulty; complete RequestStage ABI; load curve/legacy tables; validate tier/band/difficulty; wire StageEngine into loot/rank; co-save v3 ladder state; retire old tier keywords; generator validator; host tests/benchmark | audit blockers closed; batch PASS >=20 self-tests; no dual 5-rank system; no crash | current |
| F0.2 | L0 | Data pipeline v2 | per-table schemas; content source dirs; pack overlay merge; deterministic generator; DataStore descriptors; validator pipeline | base + a test pack load/validate; no code change for a data row | F0.1 |
| F0.3 | L0 | Record/form pipeline | FormCatalog coverage; ESL FormID windows; generated records from data; tier keywords; pack records | catalog gate; every data-referenced EditorID resolves | F0.2 |
| F0.4 | L0 | Co-save v3 + migration | schema registry; v2->v3 migration; caps/truncation; downgrade/uninstall behavior; test saves | old save loads; corrupt/truncated rejection; no save bloat | F0.1 |
| F0.5 | L0 | Host tests + diagnostics | RunefallCoreHost/Catch2; load/lookup benchmark; no-allocation gate; read-only snapshot/log | host CI PASS; <=25 ms load; <=100 ns lookup | F0.2-F0.4 |
| C1.1 | L1 | Threat protection spike | minimal ESP+DLL hook proof; damage-category matrix; apply-once; pipeline position; perf | spike report covers melee/ranged/spell/DoT/explosion; no crash | F0.5 |
| C1.2 | L1 | Race classification | race/template/keyword classifier; host/anomaly/provisional; unknown report; patch table | all vanilla/DLC races mapped; synthetic unknown auto-defaults; no content gap | F0.3 |
| C1.3 | L1 | Threat protection | protection formula/penetration; player/anomaly policy; categories; compatibility hooks | staged fights respect gap; apply-once; KYE2 coexistence smoke | C1.1/C1.2 |
| C1.4 | L1 | Enemy packages | per-tier stats/perk/spell/ability packages from data; rank/elite/boss overlays | actors receive one package; no double scaling; budgets respected | C1.3 |
| C1.5 | L1 | Actor enhancement/equipment | humanoid gear/perk distribution; random apply tables; toggles/cleanup; tag/report | selected actors get player-like gear/perks; toggle does not corrupt saves | F0.3/C1.4 |
| C1.6 | L1 | Attacker/player interface | player rank==stage input; attacker StageProfile; damage channel/penetration contract | mage and warrior both get penetration; no player protection by default | C1.3 |
| W2.1 | L2 | World bands / level design | stage_content_bands consumption; world-band data; hold/dungeon bands; quest exclusions | data drives area difficulty; no vanilla record edits | C1.4 |
| W2.2 | L2 | Encounter/spawn system | encounter classes; spawn tables; caps/cooldowns; identity/cleanup; safe locations | spawns follow data; duplicate/reload safe; no city/quest leaks | W2.1 |
| W2.3 | L2 | Call of the Hunt / manifestations | hunt contract; rare manifestations; reward/identity; travel/safety | player-triggered and rare encounters work | W2.2 |
| W2.4 | L2 | Bosses/quest bands | fixed-stage boss placement; questBand->stage; unique trophy/reward; IDs | bosses spawn at authored stage; quest actors excluded | W2.2 |
| W2.5 | L2 | Anomaly ladder/content | anomaly families/curves/tier labels; anomaly encounters; cores/trophies; Anomaly Codex feed | anomaly fights scale without host protection; rewards in data | W2.2/C1.2 |
| A3.1 | L3 | Ladder-driven loot | loot budgets/ilvl bands; rarity/drop tables by stage; containers/quests/bosses | loot follows stage budget; no hardcoded values | W2.2/C1.3 |
| A3.2 | L3 | Loot authority/compat | additive/takeover modes; OWL/other loot mods; pricing | authority config documented/tested; no crash | A3.1 |
| A3.3 | L3 | Material items/tokens | real MISC items, fallback tokens, family/tier parts; material ledger from inventory | materials are items; unknown races get tokens; no currency-only | C1.2 |
| A3.4 | L3 | Crafting system | recipes/stations; salvage/reroll/upgrade/socket; transaction safety | full loop material->craft->use; fail-closed | A3.3 |
| A3.5 | L3 | Monster Hunter gear tree | weapon/armor trees from monster parts; set bonuses; trophy gear | gear tree data-driven; end-to-end craft | A3.4 |
| A3.6 | L3 | Rune economy/attunement | rune drops/wallet/spend; attunement gates; respec; glob mirrors | rank progression works with ladder; save-safe | A3.1 |
| P4.1 | L4 | Player Rune Rank/attributes | player rank 1-100; attributes; derived stats; UI data | player power curve works; no protection default | A3.6 |
| P4.2 | L4 | Skills/trees/active skills | 18 skills, trees, active abilities, cost/cooldown, data | skill build playable; no Papyrus polling | P4.1 |
| P4.3 | L4 | UI redesign | SKSE Menu Framework pages: character, forge, hunt, anomaly codex, settings | pages usable; no debug pages in release | P4.1/P4.2 |
| P4.4 | L4 | Inspect/tooltip/item data | native item detail/tooltip; resistance/inspect view; snapshots | item card reads registry; no name parsing | A3.5/P4.3 |
| R5.1 | L5 | Compatibility matrix | OWL, Worlds Dawn, Fortunate Foes, KYE2, Grid/Summermyst where available; load-order profile | target-loaded smoke PASS; conflicts documented | all |
| R5.2 | L5 | Save/upgrade/uninstall | old save migration; pack disable; uninstall; downgrade; version negotiation | all uninstall modes PASS | R5.1 |
| R5.3 | L5 | Balance and calibration | data-driven balance report; TTK/encounter budgets; playtest passes | playtest targets met or documented | P4.x |
| R5.4 | L5 | Packaging/release | version, FOMOD, Nexus assets/media, archive verify, release gate | ship gate PASS; archive verified | R5.2/R5.3 |

## 5. First slice F0.1 - detailed tasks

1. StageRequest default difficulty = none (255 or explicit flag); fix tests.
2. Complete RequestStage: source precedence, ruleId, category, player-rank value, jitter/clamp
   order, golden vectors; reject unknown values.
3. Load curve_profiles and legacy_rf_tier_map; validate maxStage/curve vs stage table.
4. Validate all ladder tables: tier names/keywords/ranges, content-band coverage/overlap,
   difficulty profiles, rule coverage, capacity formulas.
5. Wire StageEngine into actor rank/loot paths: replace the POC 5-rank decision with stage-derived
   rank/tier or a compatibility shim; remove dual scaling.
6. Co-save v3 ladder section (player rank/stage, difficulty, overrides cap 256) + v2 migration.
7. Retire old RF_KW_Tier_0..5 and old rank tables from new generation; map old data.
8. Generator validator + host core/Catch2 + benchmark; add gates to run-static-gates.
9. Batch checks for stage load and no dual system; no crash.
10. Update VALIDATION/STATE/CHANGELOG and commit.

Acceptance: all audit blockers closed; 20+ self-tests; batch PASS; static gates PASS; no crash;
one authoritative stage/tier path.

## 6. Deferred feature clarification backlog

No detailed spec is written for a deferred feature until its clarification slice is answered.
Each row is a gate for the later slice.

| Gate | Feature | Clarify before spec |
|---|---|---|
| G1 | Data pipeline | Where do content definitions live (CSV/JSON source dir), how do packs overlay/override, schema versioning, who validates? |
| G2 | Threat protection | Which damage categories/DoT/penetration channels, player/anomaly target policy, hook spike evidence, external-mod stacking. |
| G3 | Race classification | Host/anomaly table ownership, unknown provisional default, KYE2 seed import/credit, patch workflow for modded races. |
| G4 | World/encounters | World-band source, spawn modes (called/rare/ambient), caps/cooldowns, city/quest exclusions, difficulty presets. |
| G5 | Enemy packages/abilities | Per-tier stat/ability budget, package size, generated records, elite/boss overlays. |
| G6 | Actor enhancement/equipment | Humanoid gear/perk model, random apply chance, toggle/cleanup semantics, player-like equipment scope. |
| G7 | Anomaly ladder | Family list, curves/tier labels, Cores/trophies, Anomaly Codex, no host protection. |
| G8 | Crafting/materials | Real item designs vs token fallback, material grades, recipes/stations, Monster Hunter gear trees. |
| G9 | Player progression | Rune Rank economy, attunement, attributes/skills/trees/active skills, respec/NG+. |
| G10 | UI redesign | Page inventory, interaction model, tooltip/item card, settings. |
| G11 | Compatibility/release | Supported target mods, uninstall policy, versioning, Nexus assets. |

## 7. Execution rules

- Work one slice at a time; do not start the next until the current slice has evidence.
- Every slice must add tests and update the relevant spec/data doc.
- No feature may hardcode content or balance values; add data + validator instead.
- No Papyrus gameplay; no per-frame work; fail closed; no crash.
- If a slice reveals a missing design, stop and write a short clarification note; do not invent.
- The plan may be reordered by the human, but no slice is deleted without recording why.
