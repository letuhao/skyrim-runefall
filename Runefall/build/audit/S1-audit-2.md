# S1 (co-save) implementation re-audit — subject commit 3e91f5b

Auditor: teammate `esp-explorer` (independent; did not implement S1, did not author the spec or the prior audit). Read-only: only this file was written. No build, no game launch.
Repo: D:/Works/source/skyrim-moddings/skyrim-runefall/Runefall. Date: 2026-10-28, timeboxed (~10 min including evidence inspection).
Subject: commit `3e91f5b` (fix(s1): correct fixtures, atomic load, range checks and evidence), supersedes `930a89c`. Spec: `build/slices/S1/spec.md` rev6, SHA-256 E94ECAEFF8375ADF282B93F348370AD6677733EE4EDF7F19D1D78F7E17EC9427 (matches remediation.S1.contract). Prior audit: `build/audit/S1-audit.md` (FAIL at 930a89c).

## 1. Commands actually run (read-only)

1. `git show 3e91f5b --stat` and `git diff --name-only 930a89c..3e91f5b` — 12 paths, all inside the S1 C1 allowlist (CoSave.cpp, tools/run-batch-test.ps1, S1 fixtures/tools, evidence/S1).
2. `python build/slices/S1/tools/verify_fixtures.py` (bundled Python; the `py` launcher is broken on this host, it points at a missing C:\Python313) -> PASS: v2 headers, tier5->rank100, tier2->rank40, baseline v3 header+ladder; exit 0.
3. `python build/slices/S1/tools/decode_cosave.py build/slices/S1/fixtures/baseline_v3.bin` -> schema 3, flags 0, saveVersion 3, sectionCount 10, section 11 len 16, trailing 0.
4. `Get-FileHash` on all five fixtures (hashes match evidence/S1/fixtures.log).
5. `pwsh tools/audit-gate.ps1 -StatePath EXECUTION-STATE.json -Mode truthfulness` -> exit 0, `{"ok":true,"code":"GATE-PASS",...,"active":"S1","overall":"IN_PROGRESS"}` (gate does not prove S1 closure).
6. Code reads: CoSave.cpp (Encode 223-372, Decode 374-646, OnSave 661-689, OnLoad 691-733, SelfTest 756-951), CoSave.h, prior audit, spec rev6, evidence/S1/*, expected_fields.json, HISTORY.jsonl, state.
7. Read crash log `C:/Users/NeneScarlet/Documents/My Games/Skyrim Special Edition/SKSE/crash-2026-10-28-10-37-27.log` (18,521 B) and the stack/register sections.
8. Did NOT rerun the native build, the batch, or the game (read-only audit; game launch is user-side here). Build/batch claims are inspected from committed evidence, not independently reproduced.

## 2. Criterion verdicts

| ID | Verdict | Evidence / reason |
|---|---|---|
| S1-A1 | PARTIAL | OnLoad no longer Resets before Decode: Reset()+ItemRegistry::Clear() only in the `if (!loaded)` tail (CoSave.cpp:726-732); Decode keeps failure purity (candidate, assign-on-success, :374-380; sentinel truncation/schema-0 SelfTest :821-835). But Progression/MaterialRegistry are not explicitly Clear()ed - they are rebuilt via `LoadFromCoSave()` after Reset (:729-730); equivalence is plausible, not proven by an explicit test. SelfTest covers truncated + schema-0 + record-version sentinel, still no semantic-invalid (range-reject) sentinel case as A1 requires. |
| S1-A2 | PASS | Unchanged from prior PASS: v1/v2 12-byte and v3 16-byte headers, flags=0, saveVersion mirror; SelfTest byte asserts :797-803; in-game hex `524E465303000000000003000A000000` (self-test.log:1) decodes to schema3/flags0/version3/count10; decode_cosave.py reproduces all fixtures; docs corrected in 930a89c. |
| S1-A3 | PARTIAL | Prior FAIL causes are fixed: baseline_v3.bin is now 192 B and its section-11 ladder payload is 16 B including `u16 skillCount=0` (decode shows len 16; verify asserts skillcount/endgame at :38-40) and matches the pre-S1 writer order; schema2_tier2.bin exists and verifies tier2->rank40; expected_fields.json is consumed by verify_fixtures.py (:27-41), which passes. Residual: schema1_legacy is never asserted (expected rank=stage=1 is in expected_fields.json but verify_fixtures.py checks only the two v2 files and baseline); "v3 baseline loads" is demonstrated only by the Python parser, not through the engine LoadRecord path; provenance is a Python mirror of the pre-S1 Encode at 37dfb68 ("pre-S1 writer format... mirrors CoSave.cpp Encode at 37dfb68 exactly", export_fixtures.py:2-3), not a save/build export, and the spec A3 wording ("from the pre-S1 build at 37dfb68 (one-off exporter or current runtime save)") is only partially met; exporter and verifier are self-referential (same author). Hashes + derivation command are committed (fixtures.log, commands.log). |
| S1-A4 | PARTIAL | Unchanged by this commit: LoadRecord version>3 and payload schema>3 paths exist with distinct tokens (:386, :411); SelfTest covers LoadRecord v4, recordVersion 0 accepted, Decode schema0, but still no payload-schema=4 case and no assertion of the log token text; in-game newer-save simulation BLOCKED. |
| S1-A5 | PARTIAL | Canonical encode is order-deterministic (sorted maps/containers/quests/items/overrides, min-FormID eviction :187-215, :225-290; SelfTest deterministic reorder :866-890, eviction :942-947). Still missing the required committed golden `canonical_v3.bin` (+ SHA-256) and any golden-byte comparison; duplicate-key policy for map sections is not documented in evidence. |
| S1-A6 | PARTIAL | Prior FAIL's central asymmetry is largely fixed: Encode now validates caps for every collection (:233-238), keys/lengths (:239-247), player ranges/difficulty/curve (:248-251), item rarity/ilvl/socketCount/enhancementCount/socketRunes/enhancement category+tier, duplicate registryIds (:252-270), overrides (:263-264). Remaining gaps: item.tags mask is written (:329) and read (:508) with no known-mask validation on either side although the spec row requires `co-save: reject item.tags`; Decode rejects sectionCount==0 without logging the fixed `co-save: reject section-count` token (:422; token is used only for >64 at :424); SelfTest still has only a subset of per-row negatives (skill cap, item cap, rank, socket, enhancement, override, sectionCount 0, oversized record) - not one per cap/range table row (rarity/ilvl/tags/hunt/material/pack/container/quest/string/duplicate-section/section-trailing/item.ilvl etc.). |
| S1-A7 | PARTIAL | Constants are correct (kMaxEncodedBytes=1,048,576; kMaxRecordBytes=2,097,152; CoSave.h) and OnSave checks empty encode + budget before OpenRecord (:669-681); OnLoad rejects record-size > cap before allocation (:701-704); SelfTest 1000-item legal case passes (:903-917). Still missing: no oversize-save refusal test (SelfTest only feeds an oversized record to LoadRecord :936-938, which is the decode side) and no committed budget-derivation table/evidence that a >budget state leaves no partial write; docs 1 MiB statement not re-verified in this pass. |
| S1-A8 | PARTIAL | Re-run batch at 17:41:24 exits 0 / RESULT PASS with 38 self-tests, schema-3 write 1685 B, reload equal (batch-20261028-174124.md; commands.log). But the required pre/post field list is still incomplete: the "Co-save fields round-trip" row compares only rank/stage/xp/class/endgame/containers count/quests count/items count/materials count/configHash - missing difficulty, curve, one skill rank, generatedContainers member, rewardedQuests member, overrides count, item registryId. "Co-save header hex" row passes but prints no hex in the batch file (hex only in self-test.log). Evidence file has no embedded cwd/command/exit/sha256 (only commands.log carries cwd/command/exit). |
 
## 3. Prior-audit findings re-check

| Prior finding | State at 3e91f5b |
|---|---|
| A3 baseline ladder missing skillCount | FIXED (16-byte ladder, skillcount parsed and asserted) |
| A3 tier2 fixture absent | FIXED (schema2_tier2.bin, verify PASS) |
| A3 expected_fields unconsumed | FIXED (verify_fixtures.py reads it) |
| A3 false provenance | IMPROVED/honest: docstring now states it mirrors the pre-S1 writer; still not a build/save export, and schema1 case unasserted |
| A6 encode/decode asymmetry | LARGELY FIXED; tags mask and one token remain |
| A1 Reset-before-Decode / clears | FIXED in structure (Reset only on failure/no-record; ItemRegistry cleared; Progression/MaterialRegistry reloaded from defaults) |
| A5 golden canonical_v3.bin | STILL MISSING |
| A7 oversize save test / budget evidence | STILL MISSING |
| A8 field assertions / header hex / evidence metadata | PARTIAL, same gaps |
| Governance: state S1 not advanced; HISTORY lacks implementation audit | STILL OPEN: remediation.S1 remains `state=IMPLEMENTING, status=NOT_STARTED, implementationAudit=null, commit=null, evidence=[]`; HISTORY.jsonl has only the spec-audit record; C1 hash not recorded. The truthfulness gate exit 0 says nothing about S1. |

## 4. Crash evidence: 17:37 BSWin32KeyboardDevice crash

- Log: `crash-2026-10-28-10-37-27.log` (CRASH TIME 2026-10-28 17:37:27, Skyrim SSE v1.6.1170, CrashLoggerSSE v1-16-0-0, exception ACCESS_VIOLATION at SkyrimSE.exe+0CDE0A2, process uptime 67 s, cwd J:/SkyrimTest/1.6.1170).
- PROBABLE CALL STACK (lines 37-45): all frames are SkyrimSE.exe (offsets +0CDE0A2, +0CD8F74, +0CF7888, +0CF7E51, +0CF61DA, +0CD0DBD) then KERNEL32/ntdll. No Runefall.dll frame anywhere in the stack; Runefall.dll appears only in the loaded-modules list. RBX/RSP show a `BSWin32KeyboardDevice*` and the faulting read is `[r14+rbp]` with non-pointer values (r14=0x743E736D) - an engine input/device path, not co-save code.
- Assessment: not evidenced as a Runefall or S1 regression. However, the 17:37 attempt has no committed batch report (only 17:17 and 17:41 files exist); the crash is a new crash since baseline and is treated as "no new crash" by the 17:41 batch row (it predates that run's start). That is an evidence-handling gap, not a code defect: the crash should be committed as environment evidence with its stack, or the batch report should cite "0 new crashes since batch start" explicitly. Marked as an A8 evidence gap.

## 5. Overall verdict

**FAIL** for S1 PASS (unchanged in overall status, materially improved in substance). Only S1-A2 is PASS; A1, A3, A4, A5, A6, A7, A8 remain PARTIAL. The two prior hard FAILs are substantially repaired: the v3 baseline fixture is now format-correct including skillCount, the tier2 fixture exists and is verified, expected_fields is consumed, and encoder caps/ranges are close to decode symmetry. S1 cannot be closed until at least: canonical_v3.bin + golden comparison (A5), item.tags mask validation plus remaining fixed-token/per-row negatives (A6), state/HISTORY entry recording C1=3e91f5b with implementationAudit request (governance/spec section 4), the A8 assertion list/metadata, and either a real pre-S1 build/save export or an explicit spec amendment accepting the writer-mirror provenance (A3).

No repairs were made; no files other than this audit were written.
