# Runefall S1 — independent SPEC audit (co-save correctness)

Auditor: independent spec auditor (no S1 implementation history)
Date: 2026-10-28
Verdict: **REJECTED — revise and resubmit as revision 2. No implementation should start.**

## 0. Inputs audited

- Draft build/slices/S1/spec.md — sha256 B1A6754ACCD4F2F2762C275CB7B7C6200DAD9F1C301B4392B78BA405368EC79C, 4,611 B, UNTRACKED at audit time.
- Repo: skyrim-runefall, HEAD 37dfb68f642569ef01edf38e7510899c0334a4d8 (docs: record S0 PASS and next slice S1); git status --porcelain = one entry, ?? Runefall/build/slices/S1/.
- State: EXECUTION-STATE.json remediation.S0 state=PASS, currentSlice=S1, remediation.S1 PENDING with all contract/specAudit/audit fields null. S0 PASS accepted from state; S0's own audit was not re-run for this spec audit.
- Read: build/slices/S1/spec.md; PROGRAM.md; EXECUTION-STATE.json (S0/S1 records); build/audit/plan-gaps.md (F0.5 row 18, items 16/22/23); build/audit/runtime-gaps.md (C5 lines 58-59, C6 61-62, H4 78-79, M2 100, missing-items 128/135); src/plugin/src/CoSave.h and CoSave.cpp (whole file); src/plugin/src/Core.h (kSaveVersion=3); src/plugin/src/ItemRegistry.h/.cpp (registryId uniqueness); src/plugin/src/DebugHarness.cpp (CoSave::SelfTest registered at line 146); SPECS/POWER-LADDER.md section 9 (line 176+); SPECS/L4-RUNTIME.md section 5 (line 117+); tools/run-batch-test.ps1 co-save block (lines 246-266); S0 spec for project acceptance conventions (sections 3.4, 3.9, 3.10, 9, 10); historical CoSave.cpp at e0b9dfa (POC base, kSaveVersion=2) for v2 provenance.

Timebox: spec audit only; no build, no game run, no implementation claims evaluated.

## 1. Verdict summary

The contract direction matches the gap reports (C5/C6/H4/M2) and the eight criteria are the right areas. As written, however, the contract is not falsifiable and contains internal contradictions. Nine blocking defects follow; each criterion requires amendment before approval.

## 2. Blocking findings (criterion-level)

### F1 — S1-A1 atomic decode is not testable and contradicts load semantics

- Decode unconditionally clears the destination before any validation: a_out = CoSaveState{} at CoSave.cpp:340. The S1-A1 pass condition ("destination equals prefilled sentinel after every rejected input") can never pass against this API unless the API changes.
- OnLoad resets the live state at the start (CoSave.cpp:593) and decodes directly into the live state (CoSave.cpp:615-620). "live state unchanged" is therefore ambiguous and untestable: before OnLoad the state is deliberately reset; after any partial decode, parsed fields remain in live state.
- There is no seam to test the OnLoad path in SelfTest (SerializationInterface cannot be faked).
- Required amendment: Decode must be failure-pure (decode into local candidate; assign only on success). Add a testable seam such as LoadRecord(bytes, recordVersion, out) used by OnLoad; SelfTest covers it. Define fail-closed OnLoad semantics exactly: any rejected co-save record results in empty/default CoSaveState, ItemRegistry cleared, Progression/MaterialRegistry reset, one fixed log line; registries must never keep hybrid state. Define zero-record / multiple-RNF2-record / unknown-record behavior.

### F2 — S1-A2 header layout contradicts the authoritative docs; two version fields undefined

- Actual v3 bytes (CoSave.cpp:224-227 and 332): 0-3 magic RNFS; 4-5 u16 schema (kSaveVersion=3); 6-9 u32 flags (=0); 10-11 u16 saveVersion (=kSaveVersion); 12-15 u32 sectionCount.
- SPECS/POWER-LADDER.md:178 states "magic, u16 schema, u16 serialization version, u32 flags, u32 section count" — wrong order and wrong widths. SPECS/L4-RUNTIME.md:117 and :134 state the same wrong layout. So the A2 pass condition "POWER-LADDER/L4 docs match code" is currently false, and the criterion does not say which document wins.
- v1/v2 header is different and unspecified: 12 bytes = magic, u16 schema, u16 saveVersion, u32 sectionCount (verified from POC base e0b9dfa encoder/decoder; no flags field). A2 mentions "first 16 bytes" and "u32 flags (schema>=3)" but gives no v1/v2 layout, no minimum payload size, no per-version rule.
- The inner saveVersion field is read and then never checked or used (CoSave.cpp:358-361); flags is read but never validated. "u32 flags (schema>=3)" is the only statement.
- Required amendment: one normative header table for v1/v2/v3 with byte offsets, widths, values and semantics; declare the code order canonical (do not change format: baseline-written v3 saves must remain readable) and update POWER-LADDER section 9 and L4-RUNTIME section 5 to the identical table; define flags as reserved must be 0 (or forward-compat preserve) and saveVersion as must-equal-schema or as the SKSE record-version mirror; add mismatch tests; SelfTest must log the 16-byte header hex with a fixed prefix and the S1 slice must ship a standalone decoder (for example build/slices/S1/tools/decode_cosave.py) so an independent auditor can decode without narration.

### F3 — S1-A3 v2 fixture is self-referential; S1-A3/A4 migration mapping is contradictory

- "synthetic schema-2 payload" built by the implementer from the same assumptions the new decoder encodes is a tautology: the fixture must prove compatibility with the historical writer, not with the new decoder.
- Historical v2 writer provenance exists: commit e0b9dfa (POC base) has kSaveVersion=2, 12-byte header, sections 1-10, no ladder section, progression payload 13 * i32 including runeTier and bossKillTier, item entries byte-identical to v3. The fixture must be derived from that revision (git show e0b9dfa:Runefall/src/plugin/src/CoSave.cpp), committed as binary/hex with sha256 and derivation command.
- Migration mapping is contradictory. Code (CoSave.cpp:552-566): hard-coded LUT [1,20,40,60,80,100] for both playerRank and playerStage, difficultyProfile=255, curveId=1, bossKillTier untouched/ignored. POWER-LADDER.md:188 says "runeTier 0-5 -> playerRankStage [1,20,40,60,80,100]" (single ambiguous field name); POWER-LADDER.md:211 says legacy_rf_tier_map.csv maps to stage ranges 0:1-15, 1:16-30, 2:31-45, 3:46-60, 4:61-80, 5:81-100; POWER-LADDER.md:102 declares that CSV as an authoritative table. These are two different mappings, and the code uses a third (hard-coded LUT, not the CSV).
- A4 claims schema 1/2/3 accepted "as designed", but only a v2 fixture is required. v1 (progression without runeTier/bossKillTier; CoSave.cpp:382-384 handles it) is untested even though runtime logs show a real schema-1 migration.
- Required amendment: pin exact v2/v1 fixture files, field-by-field expected values after migration, and the authoritative mapping table; reconcile LUT vs CSV (state which applies to save migration and update the other documents); state whether runeTier/bossKillTier persist after migration; add a baseline-generated v3 fixture (real 1772-byte-era artifact or pre-S1 build output) proving old v3 saves still load; add v1 fixture or remove schema-1 acceptance and mark it BLOCKED/NOT_VERIFIED.

### F4 — S1-A4 downgrade: two different version gates conflated, update path untestable, silent rejection

- Gate 1: OnLoad record version check, CoSave.cpp:611 ("newer than supported", continue). Gate 2: internal payload schema check, CoSave.cpp:356, returns false with no log. The spec treats both as "schema > kSaveVersion rejected with a log".
- "In-game log for newer save simulation if possible (log assertion acceptable)" is not a verification method: with an older DLL the engine will not hand the plugin a v4 record; a SelfTest cannot observe OnLoad logging.
- "destination unchanged" conflicts with A1/F1 and with the reset-at-start behavior.
- Required amendment: name both gates explicitly (SKSE record version vs payload schema); require distinct fixed log tokens; make both testable through the LoadRecord seam (recordVersion=4 and schema=4 cases, each rejecting with log and sentinel preserved); define post-reject state (empty baseline) and record-loop behavior; remove the vague in-game downgrade claim or mark it BLOCKED user-side.

### F5 — S1-A5 deterministic encoding: no canonical keys, no decode canonicalization, wrong hash choice

- Encode output depends on unordered_map iteration for huntFirstKill, materials, packVersions and skillRanks (CoSave.cpp:245-260, 326-330) and on raw vector order for items, containers, quests, overrides. The spec demands sorted output but names no sort keys, tie-breakers, duplicate policy, or whether decode canonicalizes.
- Without decode canonicalization, an old unsorted v3 save decodes then re-encodes with different bytes; round-trip byte stability is not implied.
- Sorting generatedContainers/rewardedQuests changes eviction order: RememberGeneratedContainer erases begin() at cap (CoSave.cpp:185-215). The spec must declare persisted order non-semantic or define a deterministic eviction policy over canonical order.
- SHA-256 is required by A5 but the plugin has no crypto dependency; adding one for a test is out of proportion to "smallest complete unit" and adds risk. Byte equality is sufficient.
- Item sort key is available: registryId is unique by construction (ItemRegistry.cpp:18, 44-45); duplicate registryIds in a payload must be rejected or normalized with a rule.
- Required amendment: canonical ordering table (map keys byte-lexicographic ascending; containers/quests ascending u32; overrides by key, then stage, then flags; items by registryId then baseFormId; duplicate keys policy); require decode canonicalization and idempotent re-encode; deterministic tests: two states built with reverse insertion orders encode equal; decode(encode(x)) re-encode equal; old unsorted v3 payload decodes and re-encodes canonically. Use byte comparison (if a hash is used, specify algorithm and a known vector without a new dependency).

### F6 — S1-A6 bounds: encode-side caps missing; decode allocates before validating; actual defects to fix

- Encode: ladderOverrides count is cast to u16 with no cap (CoSave.cpp:316); skillRanks writes count = min(size,256) but then iterates the whole map (CoSave.cpp:324-330) — with >256 skills the record declares 256 and writes extra entries, silently corrupting the save on next load. Items, generatedContainers, rewardedQuests, materials, hunts and packs have no encode-side cap enforcement.
- Decode: items count cap 100000 then a_out.items.resize(count) before any minimum-size check (CoSave.cpp:416-418) — a 4-byte count can force a multi-megabyte allocation. Several string caps are checked after assignment; hunt and pack strings are uncapped; duplicate or unknown section ids are silently skipped; trailing bytes after all sections are ignored; unknown fields are tolerated without a policy.
- "semantically invalid" is undefined and no enum/range table exists: playerRank/stage 1-100, difficultyProfile valid set (255 sentinel), curveId valid set, skill rank 1-100, playerXp >= 0, material count >= 0, hunt flag 0/1, endgame byte 0/1, item rarity 0-5, socketCount 0-3, enhancementCount 0-5, enhancement category/tier ranges, tags mask, registryId > 0, baseFormId validity.
- Required amendment: one spec table with every field/collection, encode cap, decode cap, allowed range, and fixed log token; validate-before-allocate using minimum entry size; exact-consume rules (or explicitly documented optional tails); duplicate/unknown section policy; encode caps equal to decode caps; SelfTest one negative case per row (items count 100000 with 4-byte payload, rarity 6, socketCount 4, enhancementCount 6, container id 0, container count 4097, quest count 1025, override count 257, override key/stage invalid, oversized strings, trailing garbage, duplicate section) each asserting rejection plus sentinel preservation plus no crash.

### F7 — S1-A7 64 KiB budget is arithmetically incompatible with the A6 caps; nothing is enforced

- Worst-case arithmetic with the declared caps already exceeds 64 KiB: materials 1000 entries with strings up to 128 bytes is about 134 KB alone; hunts allow 10,000 entries; skills 256 entries; containers 4096 * 4 B = 16 KB; quests 1024 * 4 B = 4 KB; overrides 256 * 10 B = 2.5 KB. "cap-sized legal state is either under budget or rejected" is therefore not a pass condition: a cap-sized legal state cannot be under 64 KiB.
- OnSave performs no size check at all before OpenRecord/WriteRecordData (CoSave.cpp:577-583); OnLoad accepts records up to 16 MiB (CoSave.cpp:600-604). The budget is enforced nowhere, and "refused with error" is undefined (skip record? partial write? player-visible consequence?).
- HashConfig is a hard-coded placeholder (CoSave.cpp:670; plan-gaps item 16). S1 must declare configHash opaque/preserved and out of scope, otherwise the audit will flag a placeholder as a defect.
- Required amendment: define kMaxEncodedBytes and kMaxRecordBytes; put the cap/budget worst-case arithmetic table in the spec and reconcile caps to it (smaller caps or a larger budget); define a realistic "legal worst case" state and require Encode <= budget; define and test the oversize refusal path (fixed log token; explicit save write behavior and accepted data-loss consequence, or a documented deterministic fallback); SelfTest covers both budget pass and oversize refusal; auditor checks constant, table and test.

### F8 — S1-A8 in-game persistence is not falsifiable from the spec

- Existing batch checks only assert log strings and one count: run-batch-test.ps1:246-266 waits for "co-save: wrote .*schema 3", "co-save: loaded schema 3", and generatedContainers > 0 from an F11 probe (Tap-Key 68). They do not compare any field across reload.
- The spec says "save/reload preserves rank/XP/class/skill/endgame/containers" but names no field list, no probe format, no expected values, no exact command, no test environment, no crash-log check, and no evidence file format.
- Required amendment: cite the exact batch command and environment (test root, save name, SkyLink MCP dependency), name the required pre/post fields (playerRank, playerStage, playerXp, characterClass, skillRanks count and a sample entry, difficultyProfile, curveId, endgameComplete, generatedContainers and rewardedQuests counts plus a member from each set, ladderOverrides count, items count plus one registryId, materials count, configHash), require Add-Check assertions with expected values, require F11 per-case PASS log tokens plus header hex, require evidence file under build/evidence/S1/ with command, cwd, exit code and sha256 (S0 section 3.9 style), and a crash-log absence check. Mark any user-side-only step BLOCKED if unavailable.

### F9 — Governance and reproducibility clauses are missing (S0 precedent)

- The S1 draft is untracked and unfrozen, so there is no stable contract hash. S0 sections 3.4, 3.9, 3.10 and 10 require: remediation.contract/specAudit objects with sha256, ARTIFACT evidence objects with sha256 and exists, a HISTORY.jsonl hash-chained record per audit action, and explicit audit-file ownership.
- S1 lacks: an exact allowed-file list for C1 (S0-A7 style), the exact build command and toolchain, the audit-gate invocation and expected exit/reason codes, state-transition owners and order, C1 commit capture and clean-tree check, spec-audit commit/record process, and evidence naming.
- Required amendment: freeze the approved revision by commit and record its sha256 in remediation.S1.contract plus a HISTORY spec-audit record; define the C1 allowlist precisely (expected: src/plugin/src/CoSave.h, CoSave.cpp; DebugHarness.cpp only if registration changes; SPECS/POWER-LADDER.md; SPECS/L4-RUNTIME.md; build/slices/S1/**; build/audit/S1-spec-audit.md; EXECUTION-STATE.json; build/audit/HISTORY.jsonl; build/evidence/S1/**; nothing else); define state transitions SPEC_AUDIT -> READY -> IMPLEMENTING -> LOCAL_CHECKS -> AUDIT_PENDING -> PASS with actor per transition; require git rev-parse HEAD and git status --porcelain recorded; audit-gate command pwsh tools/audit-gate.ps1 -StatePath Runefall/EXECUTION-STATE.json -Mode truthfulness expected exit 0 (no truthfulness bootstrap); C1 recorded with hash; S1-audit.md committed at C2.

## 3. Criterion-level verdicts

| ID | Falsifiable at exact commit? | Blocked by | Required amendments |
|---|---|---|---|
| S1-A1 | NO | F1 | M1, M2, M11 |
| S1-A2 | NO | F2 | M3, M4 |
| S1-A3 | NO | F3 | M5, M6 |
| S1-A4 | NO | F3, F4 | M6, M7 |
| S1-A5 | NO | F5 | M8 |
| S1-A6 | NO | F6 | M9 |
| S1-A7 | NO | F7 | M10 |
| S1-A8 | NO | F8, F9 | M11, M12 |
| (slice-wide) | NO | F9 | M13, M14 |

## 4. Minimum amendment list for revision 2

M1. Failure-pure Decode plus LoadRecord(bytes, recordVersion, out) seam; exact fail-closed semantics and registry reset rules; SelfTest covers sentinel and registry state.
M2. Define the OnLoad record loop (zero, one, multiple RNF2 records; unknown record types; oversized record) and the exact reset point.
M3. One normative v1/v2/v3 header byte table with offsets, widths, semantics; SelfTest header-hex log; standalone decoder artifact.
M4. Correct SPECS/POWER-LADDER.md section 9 and SPECS/L4-RUNTIME.md section 5 to that table; state the single normative source.
M5. Golden fixtures with provenance: schema2.bin/schema1.bin (from historical writers/real saves), baseline-v3.bin (pre-S1 build or real save), each with hex, sha256, derivation command, expected decoded state.
M6. Exact migration mapping table (runeTier -> rank/stage, difficultyProfile, curveId, XP, endgame, runeTier/bossKillTier retention) and resolution of LUT vs legacy_rf_tier_map.csv.
M7. Distinct record-version and inner-schema rejection gates, fixed log tokens, testable rejection cases, defined post-reject state.
M8. Canonical ordering and canonicalization rules; determinism tests with reversed insertion orders and idempotent re-encode; byte equality instead of SHA-256.
M9. Full field/collection cap/range table; validate-before-allocate; encode-side cap enforcement fixing the skillRanks count/loop mismatch; per-row negative tests.
M10. Budget/caps reconciliation table; kMaxEncodedBytes and kMaxRecordBytes; defined oversize-save refusal behavior and test; configHash declared opaque/out of scope.
M11. Exact build/batch/F11/selftest commands, log tokens, field-level before/after comparisons, crash-log check, evidence file format and hashes.
M12. Per-criterion acceptance mapping: criterion -> SelfTest case name(s) or script -> evidence artifact -> expected exit/token, so the implementation auditor never needs narration.
M13. Governance: frozen spec sha256 in remediation.S1.contract; specAudit record and HISTORY entry; state transition table; audit-gate command/exit; C1 hash capture; C2 audit commit; single-writer rule.
M14. Exact C1 allowed-file list (S0-A7 style) and out-of-scope statement enforcement (git diff --name-only plus allowlist), including docs and evidence paths.

## 5. Is the slice small enough to audit reliably?

The code scope (CoSave.h/.cpp plus docs and a test harness) is small enough for one audit pass: expected delta is roughly 100-250 lines plus fixtures. The acceptance scope is not: A8 depends on an MO2/SkyLink runtime environment and cannot be rerun by an auditor from the repository alone, so it currently blocks the S2-S15 safety gate on environment availability.

Recommendation: split S1 into S1a (codec contract: A1-A7, SelfTest + standalone decoder + fixtures, fully repo-runnable) and S1b (runtime persistence evidence: A8, batch/save-reload, environment-gated, may be BLOCKED without invalidating S1a). PROGRAM.md section 4 permits splitting when requirements are preserved and independently reviewed; submit the split as part of revision 2, or keep one slice only if A8 is explicitly environment-gated and does not gate S2-S15.

After revision 2 answers M1-M14, an independent auditor can run every check from the repository: F11 SelfTest log, standalone decoder script against fixture hashes, golden fixtures, build log, audit-gate, and batch evidence file. Until then the contract is not auditable without implementer narration.

## 6. Re-audit conditions

APPROVED only when revision 2 includes M1-M14 (or an explicitly audited equivalent), the criterion table shows a concrete falsification method for each of S1-A1..S1-A8, the governance clauses are present, and the draft is committed and frozen with a sha256. Until then the verdict stays REJECTED.

## 7. Audit limits

- Spec audit only: no build was run, no game/batch was run, and no implementation claim was evaluated.
- S0 PASS was taken from EXECUTION-STATE (state=PASS, currentSlice=S1); S0 audit evidence was not re-verified except where this contract depends on S0 conventions.
- The draft changed only by being read; this file is the only write.
