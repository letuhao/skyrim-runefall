# Runefall plan-compliance audit - ULTIMATE-PLAN v2 slices F0.1-F0.9, C1.1-C1.8, W2.1-W2.7

Auditor: tool-inventory (read-only). Date: 2026-10-28.
Repo: D:\Works\source\skyrim-moddings\skyrim-runefall (git branch main, HEAD a50b2b6, clean working tree).
Target: Runefall\ against SPECS/ULTIMATE-PLAN.md v2 slice rows and acceptance criteria, EXECUTION-STATE.json, EXECUTION-EVIDENCE.json, STATE.md, VALIDATION.md.
Method: static verification of src/plugin/src, tools/*, content/** and build/evidence/**; cross-checked against the machine's latest runtime artefacts (%USERPROFILE%\Documents\My Games\Skyrim Special Edition\SKSE\Runefall.log, and J:\SkyrimTest\1.6.1170\Data\SKSE\Plugins\RunefallProbe.log). No rebuild/re-test was performed (audit restricted to writing this file). No source, data, config or test files were modified.

Baseline: EXECUTION-STATE.json reports overallStatus PASS with all 24 in-scope slices marked PASS and zero blockers. That is not supported: of the 24 in-scope slices, only 4 are PASS (F0.1, F0.4, C1.4, C1.5); 17 are PARTIAL or UNVERIFIED; 3 are FAIL (F0.7, F0.9, W2.2).

## Per-slice verdict table

| Slice | Verdict | Reason / decisive evidence |
|---|---|---|
| F0.1 Ladder audit fixes | PASS | Host ladder test (100 stages, curves, 0 alloc, 1.53 ns) plus in-game batch 20/20 and co-save schema 3 verified; plan explicitly allows legacy rank shim until A3.1 (build/evidence/f0.1-evidence-20261028.md). Caveat: checklist item is PASS_BRIDGE (loot rarity still selected by POC rank tables, LootService.cpp:439-464). |
| F0.2 Freeze JSON schema | PARTIAL | 49 schemas parse and 29 pack entity files validate (build/evidence/json-schemas-20261028-112802.log; content/schema/validate_content.py:74-93), but the 20 core tables (affixes, rarity, drops, materials, recipes, skills, attributes, unique, strings, form_ids) have no authored JSON instance and are not schema-validated; they are still authored in C# ContentFactory.cs and emitted as CSV (tools/Runefall.Build/Program.cs:79-107). Acceptance "all current content represented; no CSV authoring" is not met. |
| F0.3 JSON content pipeline | PARTIAL | Pack builds from JSON via LadderContent.WriteFromPack (tools/Runefall.Build/LadderContent.cs:300-340), but most runtime tables come from C# ContentFactory, and the negative test asserts only the field name "powerIndex" - no path/line assertion (tools/test-content-pipeline.ps1:27-40). Acceptance "invalid pack fails with path/line" not proven. |
| F0.4 JSON runtime loader | PASS | DataStore loads only .json with schemaVersion 1; runtime log "loaded 49 tables, 2120 rows"; malformed file returns Fail (DataStore.cpp:120-171). Note: table keys retain .csv names (DataStore.cpp:11-60) although no CSV parser exists; missing files degrade to Warn with per-feature fail-closed logging. |
| F0.5 Co-save v3 | PARTIAL | Sections, caps (kMaxPersistedRefs/Quests/Overrides) and truncation self-test exist (CoSave.cpp:26-28, 338-553, 647-706). Real migration observed: Runefall.log 11:40 "migrated schema 1 runeTier 0 ... loaded schema 1 (128 bytes)" then "wrote 1772 bytes (schema 3)". But no v2 fixture is tested, and "save size budget" is only an observation (1939 bytes) with no defined/asserted budget. CoSave::HashConfig is a hardcoded placeholder (CoSave.cpp:215-218), so config/pack change detection in the save is not real. |
| F0.6 Host core and tests | PARTIAL | RunefallCoreHost exists and passes (src/plugin/tests/ladder_host.cpp:36-160; CMakeLists.txt:148-150) with allocation gate and benchmark. But it is a custom main, not Catch2; Catch2 is wired only to the template test/ExampleTests.cpp under PLUGIN_TESTS_ONLY (CMakeLists.txt:13-28), no static-schema test in the host target, and the repo has no CI workflow (no .github under Runefall). "host CI PASS" is unverified. |
| F0.7 Runtime content resolver | FAIL | RuntimeContentResolver::ResolveRace (RuntimeContentResolver.cpp:271) has zero call sites outside its own SelfTest (lines 331,332,348); LootService uses RaceClassifier instead (LootService.cpp:416-425). Latest runtime log shows resolver activity only from the probe and rescan ("resolved RFTest.esp:2048:RF_UnknownRace", "resolved Skyrim.esm:0:NordRace"), never from gameplay. Acceptance "unknown race resolves; cache reload; no crash" is self-test-only; the resolver is dead code from the game's perspective. |
| F0.8 Diagnostics and rescan | PARTIAL | F6 manual rescan and delayed startup scan are real (log: "manual rescan removed 3 cached profile(s)"; "delayed startup scan complete (31s/4s)"; hotkeys registered InputManager.cpp:139). But "automatic rescan on pack/version change" is not implemented: dataVersion is computed once at Load (RuntimeContentResolver.cpp:107,211-214) and no file/version watcher exists; a pack change while running is never detected. |
| F0.9 POC JSON migration | FAIL | Runtime is JSON-only, but the generator still authors CSV: Runefall.Build/Program.cs:79-107 calls WriteCsv for affixes, rarity, drops, materials, recipes, skills, attributes, unique and strings, and RuntimeJson.WriteAll converts CSV to JSON then RemoveCsv deletes it (tools/Runefall.Build/RuntimeJson.cs:8-46; Program.cs:48,62). The data is authored in C# (ContentFactory.cs, e.g. AffixRows at :10/:268), not JSON. Acceptance "generator and runtime both use JSON only" is false for the generator; build/dist2 also still carries 19 CSV files (build/dist2/Data/Runefall/data/*.csv). |
| C1.1 Protection spike | PARTIAL | Hook install at vtable slot 0x104 for Actor/Character/PlayerCharacter verified in log; direct damage samples and no-crash verified. But the category matrix is a classifier self-test only; DoT/reflected/environmental in-game observation is explicitly deferred in the slice's own evidence (build/evidence/c1.1-spike-20261028.md: "deferred to C1.3"), so acceptance "spike covers direct/DoT/reflected/environmental" is not met. |
| C1.2 Roles and race classification | PARTIAL/UNVERIFIED | 15 roles, 19 explicit race profiles and 7 keyword rules load (content/base/data/roles.json, race_profiles.json, race_classification_rules.json); Nord plus in-game Draugr verified. No enumeration or regression test covers all vanilla/DLC races, no test for modded-race defaults; "no content gap" is unproven. |
| C1.3 Threat protection | PARTIAL | Cap/floor/per-stage formula, player exemption and one NPC sample (13.00 -> 11.70) are verified. But kDamageFlagDot is never set anywhere in production code (DamagePipeline.cpp:29-30 only sets environmental/reflected flags; kDamageFlagDot appears only at :200, :237, :262, :274, i.e. self-test and multiplier), so the planned 50% DoT protection can never apply. Reflected/environmental are attacker==target / attacker==0 heuristics, not real category detection. Penetration sources are still 0 (checklist says PENDING C1.8). Acceptance only partially demonstrated. |
| C1.4 NPC packages | PASS | 20 packages load; in-game log shows "package humanoid_1_20 applied once to 00013483 (stage 8)" with spell/perk application and budget validation; batch 24/24 (build/evidence/batch-test-20261028-073859.md). Caveat: apply-once is enforced per call by HasSpell for spells, but AddPerk is called without a HasPerk guard (ActorPackageEngine.cpp:170-177), and the log line "applied once" (:180) is not a duplicate-count assertion. |
| C1.5 Actor equipment distribution | PASS | 15 equipment sets; log shows 5 items + 1 perk resolved and applied; toggle-off run suppresses equipment (batch-test-20261028-081054.md "Equipment suppressed"), toggle-on applies once (batch-test-20261028-081220.md "Equipment applied once"). Vanilla equipment persists by design; save/reload duplicate check is not isolated in the evidence. |
| C1.6 Player attributes | PARTIAL | Nine attributes, 3 points/stage, breakthrough awards 9/12/15, soft cap and respec verified in Progression.SelfTest (Progression.cpp:403-438), attributes sync to globals (Progression.cpp:133-157). Acceptance "PCR follows gear/class" is not implemented: PCR/NCR/RCR consume only offenseDPS/effectiveHP/utility/stage (CombatRatingEngine.h:15-17) and have no gear/class input path. |
| C1.7 Combat ratings | PARTIAL | Formulas, two pace profiles and display-profile data exist and self-test baseline PCR=100 (CombatRatingEngine.cpp:121-190, content/base/data/rating_*.json). But PCR/NCR/RCR/TTK/TTD/ThreatLabel have zero call sites outside the engine and its SelfTest (grep), so no rating is ever displayed or consumed; "display profiles" is data-only and number+label rendering is unverified until P4.5. |
| C1.8 Player attacker/penetration | PARTIAL | Player stage input and penetration applied in AdjustDamage (DamagePipeline.cpp:182-187, 224) with self-test vectors. But mageChannel/weaponChannel/consumableChannel are loaded and then never used (DamagePipeline.cpp:175-179); penetration is one scalar regardless of attack channel, so "attack channel; mage channel" does not exist in code. |
| W2.1 World bands | PARTIAL | Matcher supports worldspace/cell/zone/default and actor stage is wired in-game (log: "world-band: actor 00035534 cell 00009731 -> stage 7"). Data has only 2 bands, both stage 1-20 (content/base/data/world_bands.json: default + Tamriel, no exclusions), and every observed cell maps into the same 1-20 range. "Data drives area stage; vanilla zones respected" is only minimally true; no vanilla encounter-zone/cell overrides or exclusions exist. |
| W2.2 Spawn and encounter system | FAIL | EncounterManager has only DebugSpawn (EncounterManager.cpp:128; no OnCellLoad hook anywhere in src), no city/quest filtering, and the shipped ini has bEnableEncounters=0 (Program.cs:222). Batch/evidence uses only the F5 debug spawn (build/evidence/w2.2-encounters-20261028.md admits ambient triggering and city/quest filtering deferred). Acceptance "spawns follow data; reload-safe; no city/quest leaks" is unmet. |
| W2.3 Rare high-stage spawns | PARTIAL | Rate table, high-stage override and deterministic selection exist (rare_spawns.json: 2 rows; EncounterManager.cpp:143-148 uses RareSpawnEngine then BossRegistry fixed stage); condition tags fail closed. Only exercised through the debug encounter path and self-test seeds; no ambient rare spawn. |
| W2.4 Hunt contracts | PARTIAL | Start/track/complete/reward verified in-game (batch-test-20261028-091456.md: "Hunt contract started/completed", 2 runes; HuntContractManager.cpp:83-133). Duplicate-reward risk is bounded by one active contract + cooldown + processedActors, but contract state is session-scoped and co-save persistence is deferred (evidence file says so), so "hunts work end-to-end" is not save/load end-to-end. |
| W2.5 Bosses and unique actors | PARTIAL | 5 boss definitions with fixedStage/unique/questBand load (bosses.json); fixed stage override is wired through EncounterManager.cpp:148 and unique refs resolve via FormResolver self-test. No author-placed boss spawns exist in the world; only the debug encounter path can produce a boss, so "authored bosses at designed stage" is debug-only. |
| W2.6 Anomaly ladder | PARTIAL | 5 families with stage offsets and tier labels, core/trophy corpse rewards, anomaly-protection exemption wired (LootService IsAnomalyFormID used by DamagePipeline.cpp:210), debug F3 spawn registered (InputManager.cpp:52). No ambient anomaly encounters; codex feed is data-only pending P4.6; no in-game anomaly kill was evidenced in this audit. |
| W2.7 Runtime cache lifecycle | PARTIAL | Cache key is plugin:localFormId:editorId (RuntimeContentResolver.cpp:135-137), manifest writes schemaVersion/dataVersion/counts (:152-170), F6 rescan works. But invalidation is weak: dataVersion = "roles=N;races=M;plugin=0.1.0" (:211-214) changes only when table row counts or plugin version change; RescanIfNeeded does a substring check (:216-233). A pack edit that keeps counts is never invalidated. Acceptance "cache correct after pack change" has no test and no content-hash evidence. |

## Severity-ranked gaps

### Critical

1. **JSON-first architecture is not what the repo implements (F0.2/F0.9 false PASS).**
   - Evidence: tools/Runefall.Build/Program.cs:79-107 (WriteCsv for 19 core tables), :48 and :62 (RuntimeJson.WriteAll/RemoveCsv), tools/Runefall.Build/RuntimeJson.cs:8-46 (CSV->JSON conversion), tools/Runefall.Build/ContentFactory.cs (data authored as C# rows, e.g. AffixRows :10/:268).
   - Impact: the plan's rule 1 ("JSON is the only authored and runtime content format") and F0.9's "convert all current CSV content/balance to JSON; deprecate CSV" are not satisfied; most content is not reviewable, diffable or schema-validated as data, and any future pack tooling (R5.1) inherits a two-format pipeline.
   - Missing evidence: authored JSON instances + schema validation for affix/rarity/drop/material/recipe/skill/attribute/unique/string/form_id tables, and a generator run that reads only JSON.

2. **F0.7 runtime content resolver is dead code.**
   - Evidence: RuntimeContentResolver.cpp:271 definition; only calls are its own SelfTest (:331,:332,:348). No consumer in Plugin.cpp or any engine; latest Runefall.log shows resolver lines only from probe/rescan, while gameplay classification is RaceClassifier (RaceClassifier.cpp Classify; LootService.cpp:416-425).
   - Impact: unknown/modded races never get the provisional role/stage/cache path at runtime; F0.7 acceptance (and the resolver's role in C1.2 unknown-race behavior) is self-test-only.
   - Missing evidence: a gameplay call site, plus a log showing an unknown modded race resolved by the resolver during play.

3. **C1.3 DoT protection can never fire.**
   - Evidence: DamagePipeline.cpp:29-30 sets only environmental (attacker==0) and reflected (attacker==target) flags; kDamageFlagDot is referenced only at :200 (multiplier), :237 (classifier) and :262/:274 (self-test). No production code path sets kDamageFlagDot.
   - Impact: the plan's balance rule "DoT 50 percent protection" is unenforceable; C1.3/C1.1 category coverage claims are classifier-only.
   - Missing evidence: a runtime damage event classified as DoT (e.g. DOT spell/burn effect) with the 50% reduction observed, and a code path that sets the flag.

### High

4. **W2.2 spawn system has no world integration.**
   - Evidence: EncounterManager.cpp:128 DebugSpawn only; grep for OnCellLoad in src returns nothing; shipped config bEnableEncounters=0 (Program.cs:222); evidence file itself admits "ambient cell-load triggering, quest/city location filtering ... remain ... W2.3/W2.5".
   - Impact: acceptance "spawns follow data; reload-safe; no city/quest leaks" cannot be met; no reload/persistence test for spawned refs; no city/quest exclusion code at all.
   - Missing evidence: ambient trigger hook, city/quest filter implementation, reload/cleanup test.

5. **Rating engines (C1.6 PCR-follows-gear/class, C1.7 display) are unwired.**
   - Evidence: CombatRatingEngine.cpp:121-163 PCR/NCR/RCR/TTK/TTD/ThreatLabel have no call sites outside the file and SelfTest (grep); Progression.cpp:133-157 only syncs attribute values to globals.
   - Impact: "PCR follows gear/class" is not implemented; ratings are never shown in-game; C1.7 "both number+label" unverified.
   - Missing evidence: a caller feeding player gear/class/attributes into PCR and a UI/log showing the rating and label.

6. **W2.1 vanilla zone mapping absent.**
   - Evidence: content/base/data/world_bands.json has 2 rows only (default, Tamriel 1-20), no exclusions; runtime log shows all sampled cells (00009731, 00009712) mapping into the same band. No encounter-zone/cell table content.
   - Impact: "vanilla zones respected" unproven; every outdoor fight is 1-20 regardless of hold/zone danger.

7. **Cache invalidation cannot detect pack-content edits.**
   - Evidence: RuntimeContentResolver.cpp:211-214 (dataVersion = counts + plugin version), :216-233 (substring match); no content hash and no file watcher; W2.7 acceptance "cache correct after pack change" has no corresponding test.
   - Impact: edited balance/role values with unchanged counts leave stale runtime profiles/manifests forever.

8. **F0.8 "automatic rescan on pack/version change" is missing.**
   - Evidence: dataVersion computed once at Load (RuntimeContentResolver.cpp:107); rescan paths are delayed startup (:235-250) and manual hotkey (InputManager.cpp:67, Batch evidence).
   - Impact: the acceptance wording implies runtime change detection; only startup/manual rescan exists.

9. **C1.2 "no content gap" unproven.**
   - Evidence: content/base/data/race_profiles.json has 19 profiles, race_classification_rules.json 7 keyword rules; tests/examples cover NordRace and Draugr only (C1.2 evidence; RaceClassifier SelfTest requires >=5 rules/>=10 profiles, not full coverage).
   - Impact: many vanilla/DLC/modded races may fall to Unknown() with no explicit policy; a coverage enumeration and fallback-behaviour test is missing.

### Medium

10. **F0.3 invalid-pack rejection is not asserted at path/line level.** tools/test-content-pipeline.ps1:27-40 checks only "CONTENT PACK FAIL" and the "powerIndex" token; no file/line assertion despite acceptance.
11. **F0.6 has no Catch2 host suite and no CI.** src/plugin/CMakeLists.txt:13-28 wires Catch2 only for the template test/ExampleTests.cpp in PLUGIN_TESTS_ONLY mode; RunefallCoreHost is a custom main (tests/ladder_host.cpp); no .github directory in the repo. "host CI PASS" cannot be reproduced from the repo.
12. **C1.8 channel flags are dead data.** penetrationMage/Weapon/Consumable are read (DamagePipeline.cpp:175-177) and only logged (:178-179); PlayerPenetration (:182-187) ignores them. Mage/weapon channel distinction is nominal.
13. **Apply-once evidence for packages is not a duplicate assertion.** ActorPackageEngine.cpp:176-177 adds perks unconditionally (no HasPerk guard); the "applied once" line (:180) just reports that something was applied. Batch row "Equipment applied once" exists for equipment but no equivalent duplicate-count assertion for perks.
14. **W2.4 contracts are session-scoped; persistence deferred.** build/evidence/w2.4-hunt-contracts-20261028.md: "Contract state is session-scoped; co-save persistence is deferred to A3.6/A3.7 content work." Acceptance "hunts work end-to-end" is not save/load complete.
15. **W2.5/W2.6 content is debug-only.** Boss fixed-stage and anomaly spawn paths are reachable only through debug hotkeys/F5 (EncounterManager.cpp:128; InputManager.cpp:52); nothing author-placed exists.
16. **CoSave config/pack fingerprint is a placeholder.** CoSave.cpp:215-218 returns constant 0x52464C31 ("until the real config/pack fingerprint lands"); save/pack mismatch detection is not implemented.
17. **Evidence reproducibility/external dependencies.** tools/run-static-gates.ps1:5-9 depends on C:\Users\<user>\.cache\build-runefall.bat (outside the repo) and an E: Steam PapyrusCompiler path; C1.1 evidence cites crash-2026-10-27-*.log files that are not in the repository. Static-gate PASS cannot be reproduced from the repo alone.
18. **Stale CSV deployment remains in the repo.** build/dist2/Data/Runefall/data contains 19 .csv tables, contradicting the "0 CSV" cleanliness implied by F0.9 evidence (content tree is clean, deployment artefact is not).
19. **Verdict semantic drift.** EXECUTION-STATE.json promotes slices to PASS while their own checklists carry DEFERRED/PENDING entries (C1.1 ranged/DoT/explosion; C1.3 penetration; C1.8 consumable channel; W2.2 ambient/city-quest filter; W2.4 persistence; W2.5 quest band). PASS should not be used with outstanding acceptance items.

### Low

20. **Table keys still use .csv names while files are .json** (DataStore.cpp:11-60); correct today because of replace_extension(".json") (DataStore.cpp:140-142), but confusing and error-prone for new code.
21. **Schema-count narration drift.** EXECUTION-STATE F0.2 checklist says "PASS_28"; static-gate logs evolved to "30 schemas parsed" then current "49 schemas parsed; ... 29 entity files validate" (build/evidence/json-schemas-20261028-114102.log). STATE should be updated rather than carrying stale counts.
22. **Probe report counts WARN as non-failure.** DebugHarness only tallies pass/warn/fail, and at least one PASS item is explicitly logic-only (RunefallProbe.log: "DynamicEnchantment.SelfTest: logic-only (runtime form creation deferred until in-game)").
23. **Stage layer logs do not match main evidence.** RunefallStage.log shows "DataStore loaded 18 tables, 1068 rows" and "co-save: wrote 275 bytes (schema 1)" while the main log shows 49 tables and schema 3; the layered Stage build/test configuration is undocumented against F0.1's co-save v3 claim (src/plugin/src/stage/StagePlugin.cpp not covered by this audit's slice list, but relevant to layered builds).

## Cross-checks used (runtime truth, not narration)

- Latest Runefall.log: startup 11:39, hook 0x104 installed, protection loaded cap=0.7/3%/5%/dotFactor 0.5, DataStore 49 tables/2120 rows, resolver loaded 15 roles/19 profiles, encounters 3 entries, hunts 2 contracts, anomalies 5 families, co-save migrated schema 1 -> schema 3 (1772 bytes), 0 warnings, 0 errors. This confirms F0.1/F0.4/C1.1 hook/C1.2 data load/C1.4-C1.5 application/W2.x data loads, and simultaneously confirms the resolver and encounter systems are not used by gameplay.
- J:\SkyrimTest\1.6.1170\Data\SKSE\Plugins\RunefallProbe.log: SUMMARY pass=38 warn=0 fail=0 - these are self-tests (DebugHarness.RunAll lists every engine SelfTest), which is why "26/26, 30/30, 38/38 PASS" evidence rows cannot be read as acceptance coverage for runtime behaviour.
