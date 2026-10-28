# Runefall runtime/release audit — ULTIMATE-PLAN v2 slices A3.1–R6.4

Date: 2026-10-28. Auditor: dll-config-analyst (read-only). No source, data, config or tests were modified.
Scope: ULTIMATE-PLAN v2 slices A3.1–A3.7, P4.1–P4.7, R5.1–R5.6, R6.1–R6.4 plus native DLL safety/lifecycle,
co-save, UI bridge, compatibility and release claims.

Method: inspected current repo files, git state (clean, branch main, 48/48 PASS recorded), EXECUTION-STATE.json,
EXECUTION-EVIDENCE.json (147 entries), STATE.md, VALIDATION.md, RELEASE-READINESS.md, CHANGELOG.txt, FOMOD,
build/evidence/*, build/release/Runefall-0.1.0.zip (member list + SHA-256), build/dist tree, content/ tree,
skills/, tools/, and native source (Plugin.cpp, HookManager.cpp, DamagePipeline.cpp, CoSave.cpp/h, PrismaUIBridge.cpp,
LootService.cpp, Progression.cpp, CMakeLists.txt). No game launch, no build, no test execution.

## Per-slice verdicts

| Slice | Verdict | Reason / key evidence |
|---|---|---|
| A3.1 Ladder-driven loot | PARTIAL | Data tables (600 affixes/6 rarities/36 weights/36 drops) and batch loot checks exist, but most loot paths still use legacy TierEngine/RuneTier instead of StageEngine (LootService.cpp:212,364,439,464,511,670,722 vs single StageEngine call :445). "Loot follows stage" not demonstrated. |
| A3.2 Materials and tokens | PARTIAL | 31 material definitions + data-driven salvage (a3.2-materials evidence) and batch PASS. "Unknown roles get tokens" and "materials are real MISC items" are not evidenced (JSON itemEditorId only; no form_ids/record check). |
| A3.3 Runtime item generation | FAIL | Acceptance "generated item persists; visuals/name correct" unsupported: item metadata is still not instance-keyed (ARCHITECTURE.md:87,224 P0 debt open; no ExtraUniqueID/ItemInstanceKey in src). Evidence file is crafting/reroll (build/evidence/a3.3-crafting-20261028.md), not item generation. |
| A3.4 Crafting system | PARTIAL | RuneForge/8 recipes/self-test + batch exist. "MH/Last Epoch destroy-and-create; transaction-safe" not tested (no rollback/atomicity test); evidence file is rune gear (a3.4-rune-gear). |
| A3.5 Rune equipment and inventory | PARTIAL | Evidence is economy (a3.5-economy), not rune gear/inventory; no runeword table, no inventory-data stability test. |
| A3.6 Rune economy and attunement | PARTIAL | EconomyEngine/self-test exists, but attunement remains legacy 5-rank logic (Progression.cpp:379-399; LootService RuneTier uses). Evidence file is FormID catalog/compat (a3.6-compatibility), not ladder/attunement persistence. |
| A3.7 Loot authority compat | FAIL | No OWL/takeover/pricing test. Only static compat-profile JSON (additive); plan acceptance "OWL and major loot mods; pricing" unmet. Evidence a3.7 is a general end-to-end session, external loot mods deferred to R5/R6. |
| P4.1 Rune Level progression | PARTIAL | Rank/XP/co-save and +10 XP kill are shown (p4.1 evidence, batch 33/33). Breakthrough is listed only as "PASS code"; no in-game breakthrough or 40-hour runtime check. |
| P4.2 9 classes | FAIL | Plan requires 9 classes (ULTIMATE-PLAN.md:70); only 3 exist (classes.json rows warrior/rogue/mage; build/evidence/p4.2-classes-20261028.md; EXECUTION-STATE.json:770 class-data "PASS 3") yet slice is marked PASS. |
| P4.3 Skills and passives | PARTIAL | 6 perks + self-test; "builds playable; no NPC skill contamination" not tested (no build/play evidence). |
| P4.4 Prisma UI foundation | FAIL | PrismaUIBridge only serialises JSON to Data/Runefall/ui (PrismaUIBridge.cpp:32-130); no PrismaUI API integration. UI shell (ui/prisma/*.html) is not in build/dist nor in the 65-member release ZIP. Batch "Prisma view model PASS" only proves file write. |
| P4.5 Character/HUD/indicators | FAIL | Contract-only. EXECUTION-STATE.json:847 visual-host "NOT VERIFIED"; RELEASE-READINESS.md:30 says JSON contracts/shell only. "Ratings display correct" unverified. |
| P4.6 Breakthrough/class menus | FAIL | Contract-only (EXECUTION-STATE.json:872 visual-host NOT VERIFIED). "Menus complete" unverified; no screenshot/host evidence. |
| P4.7 Codex/inspect/tooltip | FAIL | Contract-only (EXECUTION-STATE.json:895 NOT VERIFIED). "Pages work" unverified. |
| R5.1 Pack tooling | PASS | Format-1 manifests, base/hunt/unique packs, PackManager validation, batch/static evidence agree. |
| R5.2 Base content packs | PARTIAL | Only base.json/hunt.json/unique.json manifests; no separate Skyrim/DLC foundation packs. "Base world fully staged; no gaps" unsupported (r5.2 evidence is manifest validation only). |
| R5.3 Famous-mod packs | FAIL | Only compat_profiles JSON (vanilla/owl/spid_kid); no pack per supported mod; detection selected vanilla in test env; plan acceptance "pack per supported mod; supported mod smoke PASS" unmet. |
| R5.4 Agent skills and criteria | PASS | skills/runefall-content-authoring, skills/runefall-pack-authoring, tools/validate-pack-manifests.py exist and validator PASS recorded. |
| R5.5 Endgame, raids, quests | PARTIAL | Endgame profile + self-test + co-save flag; EXECUTION-STATE.json:1017 scripted-final-kill "NOT RUN"; "endgame loop complete" not runtime-proven. |
| R5.6 Localization structure | PASS | 262 English keys, validate-strings.py, static gate 10/10 recorded. |
| R6.1 Compatibility matrix | PARTIAL | Only bounded DiabloRim+SPID/KID/SkyPatcher smoke (r6.1-compat evidence:12 limitation; EXECUTION-STATE.json:1068 full-mo2-list NOT RUN). Plan (ULTIMATE-PLAN.md:82) requires OWL, Worlds Dawn, Fortunate Foes, KYE2, loot/combat mods + documented conflicts. |
| R6.2 Save/upgrade/uninstall | PARTIAL | Three uninstall modes tested as load/no-crash (uninstall-*.md); no downgrade-refusal or in-game old-save migration test; batch covers only same-build v3 save/reload. |
| R6.3 Balance calibration | PARTIAL | Offline model only: 37.4h assumes 600 XP/h (r6.3 evidence); no playtest/TTK measurement; acceptance "targets met or documented" only documented. |
| R6.4 Release packaging | PARTIAL | Archive hash/members/FOMOD version verified (release-verify 25/25, hash matches 27241B9D…F2), but package lacks the Prisma UI shell; FOMOD info.xml:6 declares SKSE Menu Framework while ULTIMATE-PLAN.md:16 makes Prisma UI the hard dependency; CHANGELOG claims "first public release" (CHANGELOG.txt:17) vs internal-only addendum (:4/:7) and RELEASE-READINESS.md:3. |

## Severity-ranked gaps

### Critical

C1. **Overall PASS contradicts per-slice NOT VERIFIED / NOT RUN / DEFERRED items.**
Evidence: EXECUTION-STATE.json:4 overallStatus PASS, all 48 slices PASS; same file records P4.5/P4.6/P4.7 visual-host "NOT VERIFIED" (:847,:872,:895), R5.5 scripted-final-kill "NOT RUN" (:1017), R6.1 full-mo2-list "NOT RUN" (:1068), F0.1 wire-into-loot-rank "PASS_BRIDGE", C1.3 anomaly-policy "DATA false", C1.8 consumable-channel "data seam false". Impact: release/acceptance status is not a truthful aggregate; downstream users trust PASS. Missing evidence: an acceptance matrix where every checklist value is PASS or explicitly N/A/deferred, backed by raw logs.

C2. **P4.2 "9 classes" not met (3 implemented) but marked PASS.**
Evidence: ULTIMATE-PLAN.md:70; build/dist/Data/Runefall/data/classes.json rows=warrior,rogue,mage; p4.2 evidence names 3 classes; EXECUTION-STATE.json:770 "class-data: PASS 3". Impact: acceptance criterion failed; "class build differs" unverified. Missing: 9 class definitions, class-build comparisons, respec/class-quest/runtime test.

C3. **No working/shipped Prisma UI; UI slices are contract-only.**
Evidence: PrismaUIBridge.cpp writes only view-model/menu-model JSON (PrismaUIBridge.cpp:32-130; no PrismaUI API symbol), release ZIP has 65 members and no ui/ or Data/Runefall/ui entry (build/release/Runefall-0.1.0.zip member list; build/dist has no ui dir); batch "Prisma view model PASS" checks file text only (batch-test-20261028-113956.md:27,28,77,78). Impact: P4.4–P4.7 deliverables are absent; game UI cannot render the advertised HUD/menus. Missing evidence: PrismaUI host session (load, render, screenshots/logs), packaged shell files, declared PrismaUI dependency.

C4. **Dual ladder system still live.**
Evidence: LootService.cpp:14-15 includes both TierEngine and StageEngine; legacy paths at :212 TierEngine::RollRank and :364,439,464,511,670,722 Progression::Get().RuneTier(); only :445 uses StageEngine::RequestStage; Progression.cpp:379-399 still runs 5-rank attunement (++runeTier). Contradicts F0.1 acceptance "no dual system" (EXECUTION-STATE F0.1) and A3.1/P4.1/A3.6 acceptance. Impact: ladder does not actually drive most loot/economy/attunement; balance claims invalid. Missing: stage-driven loot/attunement matrix covering all call sites; removal/retirement of TierEngine paths; co-save proof that only ladder state drives outcomes.

C5. **Co-save v3 fail-closed violation.**
Evidence: CoSave.cpp:615-622 calls Decode(payload, CoSave::Get().State()) directly; on false it logs "state reset" but never resets state — partially decoded live state remains and is written by the next save. Impact: malformed/interrupted co-save can leave hybrid progression/items and persist it; acceptance "malformed data rejected" not met. Missing: decode-to-temp-then-swap implementation and an in-game malformed-record test asserting full reset + no state change.

C6. **v2→v3 co-save migration unproven.**
Evidence: EXECUTION-EVIDENCE.json:86 F0.5 "schema1 migrated to v3"; ULTIMATE F0.5 acceptance "migration v2 to v3; old save loads"; batch only exercises same-build schema 3 write/load (batch-test-20261028-113956.md co-save checks). No POC v2 save fixture (runeTier 0–5, no ladder section). Impact: existing POC saves may mis-migrate without detection. Missing: v2 fixture test covering all v2 sections, legacy runeTier 0–5→rank stages, and a downgrade-refusal test.

C7. **Release/status docs contradict the release claim.**
Evidence: CHANGELOG.txt:17 "0.1.0 - 2026-10-27 (first public release …)" and :22 "SKSE Menu Framework pages" vs CHANGELOG.txt:4/7 internal candidate addendum + "Prisma UI contracts"; RELEASE-READINESS.md:3 internal candidate only, no upload/tag/push, :29 bounded smoke, :30 Prisma unverified; STATE.md:3 last updated 2026-10-27 and :113 "Next: A3.1", :133 blocking UI decision unresolved. Impact: no single trusted release status; public-release statements unsupported and FOMOD dependency disagrees with plan. Missing evidence: reconciled version/changelog/readiness/state set and a doc-consistency gate.

### High

H1. **Partial Load3D hook install can break container rendering.**
Evidence: HookManager.cpp:227-240 — after installing the TESObjectREFR Load3D vfunc, a failed Character write sets g_originalRefrLoad3D = nullptr (:238) but leaves the refr vtable slot pointing at HookRefrLoad3D, which then returns nullptr for every container (:191-207) because the original is null. Impact: containers/objects missing 3D or engine crash on hook failure; silent partial degradation. Missing: fault-injection test for each hook write; vtable restore on failure; per-hook health reporting.

H2. **DoT/reflected/environmental damage classification not applied by the live damage hook.**
Evidence: DamagePipeline.cpp:29-33 thunk flags only set Environmental (attacker==0) and Reflected (attacker==target); kDamageFlagDot is never set by the HandleHealthDamage thunks; HitSink records fallback damage with kDamageFlagNone (HookManager.cpp HitSink). DoT 50% protection exists only in ComputeMultiplier self-test (DamagePipeline.cpp SelfTest). Impact: C1.1/C1.3 "direct/DoT/reflected/environmental" and "DoT 50%" acceptance is self-test-only; runtime DoT/reflected balance not applied. Missing: live sample counts per category after scripted direct/DoT/reflected/environmental hits; DoT source identification.

H3. **Hook coverage accepted if any vtable write succeeds.**
Evidence: DamagePipeline.cpp:69-101 detects any=true and logs "installed" if Actor OR Character OR PlayerCharacter vtable write returns non-null; no assertion that all required vtables were hooked. Impact: NPC or player damage may silently bypass protection while batch reports "Damage hook installed PASS". Missing: per-vtable install assertions, expected coverage matrix, failure log per missing vtable.

H4. **Co-save header layout contradicts the authoritative spec.**
Evidence: CoSave.cpp:225-227 writes magic, u16 schema, u32 flags, u16 saveVersion (kSaveVersion), while POWER-LADDER.md:178 and L4-RUNTIME.md:117 specify magic, u16 schema, u16 serialization version, u32 flags, u32 section count. Decoder (:358) matches the code, so round-trip/self-tests cannot catch the divergence. Impact: external readers/migration/validator built from spec misparse v3 saves; the documented format is not the written format. Missing: independent format decoder test, spec/code reconciliation.

H5. **Crash/regression coverage is weak and self-referential.**
Evidence: batch-test-20261028-113956.md:47 "No new crash log | PASS | crash-2026-10-27-23-38-01.log" (an old file shown as detail), compat/uninstall evidence repeats the same check; no crash-log scan/capture tied to each run, no per-slice regression matrix, no CrashLogger output attached. Impact: "no crash" claims are not independently evidenced. Missing: crash-log baseline+diff per run, CrashLogger artifacts, post-conditions (game still running, no new log) with timestamps.

H6. **Compatibility matrix is bounded, not the claimed matrix.**
Evidence: r6.1-compat-20261028.md:12 "bounded external-mod smoke test, not full 3,700-mod"; EXECUTION-STATE.json:1068; plan ULTIMATE-PLAN.md:82 requires OWL, Worlds Dawn, Fortunate Foes, KYE2, loot/combat mods and documented conflicts. 2026-10-27 compat logs exist for OWL/WorldsDawn/FF but only assert plugin active + paths run, are not cited by R6.1, and KYE2 is untested. Impact: compatibility claim over-scoped; conflict documentation missing. Missing: conflict tests/load-order profiles per named mod, OWL loot interaction tests, documented conflict table.

H7. **A3 slice evidence does not match A3 acceptance criteria.**
Evidence: build/evidence/a3.3-crafting-20261028.md (crafting) for A3.3 Runtime item generation; a3.4-rune-gear for A3.4 Crafting; a3.5-economy for A3.5 Rune equipment/inventory; a3.6-compatibility for A3.6 Rune economy/attunement; a3.7-loot-acceptance for A3.7 Loot authority. EXECUTION-STATE checklists repeat the shifted content. Impact: persistence/visual, transaction-safe, inventory-stability, attunement/save, OWL/takeover acceptance are not covered. Missing: per-slice acceptance matrix with the exact checks from ULTIMATE-PLAN rows 62-68.

H8. **Runtime item identity (A3.3) remains P0 debt.**
Evidence: ARCHITECTURE.md:87 ("item metadata not instance-keyed") and :224 P0 debt row still open; grep found no ExtraUniqueID/ItemInstanceKey usage in src/plugin/src; CoSave stores a flat items vector keyed by registryId (CoSave.h/ItemRegistry), not by item instance. Impact: generated loot metadata can detach from the item instance on save/load; "generated item persists; visuals/name correct" untested. Missing: instance-keyed registry, ExtraUniqueID marker, save->reload metadata survival test, tooltip check.

H9. **UI/Prisma hard dependency is not declared or packaged.**
Evidence: fomod/info.xml:6 requires "SKSE Menu Framework 3.18"; fomod/ModuleConfig.xml has no dependencies section and does not install ui/prisma; ULTIMATE-PLAN.md:16 says "Prisma UI is the hard UI dependency"; release archive has no ui/ members; CHANGELOG.txt:22 still advertises SKSE Menu Framework pages. Impact: package does not declare/contain the planned UI; users cannot know the true dependency. Missing: FOMOD dependency XML, packaged Prisma shell, release verifier check for UI assets.

### Medium

M1. **Hardcoded vtable slots without ownership/version verification.** HookManager.cpp:227,235 (Load3D slot 0x6A) and DamagePipeline.cpp:69 (0x104/0x106) write vfuncs directly; no module/bytes/relocation verification or runtime-version guard beyond SKSE load. Impact: wrong slot on any non-target runtime = crash; target 1.6.1170 pinned but no guard. Missing: vtable/slot verification test, abort/log evidence when the expected function identity is not proven.

M2. **Co-save encoding nondeterminism and cap mismatch.** CoSave ladder section caps skill count at 256 while iterating the full map; unordered_map iteration order changes written bytes; save-size acceptance is "1939 bytes observed" (EXECUTION-STATE.json:110) for a minimal override set, with no cap-256 test (256×10 B ≈ 2.5 KB exceeds any 1 KB budget claim). Impact: non-reproducible save bytes; save-size budget untested at cap. Missing: deterministic ordered encoding, cap-256 size test, documented budget.

M3. **Plugin ignores data-load failure and proceeds.** Plugin.cpp:127 calls DataStore::LoadAll without inspecting its result; all engines are then Load()ed and hooks remain active. Impact: missing/corrupt JSON may leave features running on empty tables with only partial logs; fail-closed claim not met. Missing: gate features on load status + negative test with missing/corrupt tables.

M4. **Per-slice evidence files are claim summaries, not raw evidence.** build/evidence/a3.*, p4.*, r5.*, r6.* are 600–970-byte Markdown summaries that restate checklist values; the only raw runtime artifacts are batch/static md and a few screenshots from 2026-10-26/27. Impact: cannot independently re-verify most slice claims. Missing: per-slice raw logs/console captures/screenshots committed with an index.

M5. **UI bridge writes files during gameplay path without gating.** PrismaUIBridge::WriteViewModel/WriteMenuModel write JSON to Data/Runefall/ui on F10/probe/self-test (PrismaUIBridge.cpp:32-130); a read-only Data tree only logs failure; no gameplay gating or rate limit. Impact: no user-visible UI; file I/O claims conflated with UI evidence. Missing: PrismaUI host test and explicit no-op/disabled behavior evidence.

### Low

L1. Batch "Prisma view model/menu model PASS" checks JSON text only (PrismaUIBridge.SelfTest) — conflates contract with rendering; should be labelled contract-only.

L2. Release tree contains stale build/release/Runefall-0.2.0-dev.zip beside the 0.1.0 candidate; version selection ambiguity.

L3. Release ZIP has no README/CHANGELOG/VALIDATION inside; acceptable for internal candidate but not for a public package if publication proceeds.

## Slice/evidence coverage note

- 24 requested slices: 3 PASS (R5.1, R5.4, R5.6), 13 PARTIAL, 8 FAIL (A3.3, A3.7, P4.2, P4.4, P4.5, P4.6, P4.7, R5.3).
- EXECUTION-STATE currently records 48/48 PASS; this audit finds no slice that fails to produce some artifact, but 20 of the 24 scoped slices fall short of their own ULTIMATE-PLAN acceptance text.
- Native safety: 2 Critical/High runtime-safety defects found in reviewed code paths (partial hook install, co-save decode-to-live-state); damage classification and item identity are high-risk gaps for the release claim.

## What is missing to close (evidence only, no remediation plan)

1. Truthful acceptance matrix: every slice PASS justified by a raw artifact and a matching ULTIMATE-PLAN acceptance criterion; N/A/deferred values not counted as PASS.
2. 9-class data and runtime class-build evidence (P4.2).
3. Working PrismaUI integration + packaged UI shell + host screenshots/logs + FOMOD dependency (P4.4–P4.7).
4. StageEngine actually driving loot/economy/attunement across all call sites; TierEngine/RuneTier retirement proof (A3.1/A3.6/P4.1).
5. Co-save: decode-to-temp-then-swap, malformed-save in-game test, v2 POC save fixture migration test, downgrade-refusal test, header-layout spec test.
6. Fault-injection tests for each vtable hook write (Load3D, damage, partial failure restore), plus per-vtable install assertions.
7. Live damage-category samples (direct/DoT/reflected/environmental) with counters/log evidence; DoT source classification evidence.
8. Compatibility evidence for OWL/Worlds Dawn/Fortunate Foes/KYE2/loot mods and documented conflict matrix; full-MO2 run or an explicit bounded-scope decision.
9. Item instance identity + save/reload item identity/tooltip test (A3.3).
10. Crash/regression evidence: CrashLogger output per run, crash-log baseline diff, per-slice regression matrix.
11. Reconciled docs: STATE.md, CHANGELOG.txt, RELEASE-READINESS.md, VALIDATION.md, FOMOD dependency and version/changelog consistency gate.
12. Cap-256 co-save size test and deterministic encoding; data-load failure negative test.
