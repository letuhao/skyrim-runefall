# S1 co-save implementation audit 9 (fresh) — candidate 4e3094d / state cfb75d1

Auditor: independent implementation auditor; did not implement S1. Read-only: only this file written; no build, no game launch.
Repo/HEAD: Runefall @ cfb75d1. Subject code: 4e3094d. Verified delta 4e3094d..cfb75d1 = EXECUTION-STATE.json + HISTORY.jsonl only.
Spec: build/slices/S1/spec.md rev6, SHA-256 E94ECAEFF8375ADF282B93F348370AD6677733EE4EDF7F19D1D78F7E17EC9427 (= state contract hash, verified).
Read: spec, S1-audit-8.md, CoSave.h/cpp, F7-waiver.md, evidence/S1/batch-20261029-052516.md, build-proof-20261029-0525.log, commands.log, crash-assessment.log, run-batch-test.ps1, HISTORY.jsonl, EXECUTION-STATE.json.

## 1. Commands actually run (read-only)

1. git log/status/cat-file/diff: HEAD cfb75d1; 4e3094d and cfb75d1 exist; delta 4e3094d..cfb75d1 = state+HISTORY only; non-allowlist paths in diff a50b2b6..4e3094d = CoSave.cpp, CoSave.h, InputManager.cpp (F7).
2. SHA-256 of J:/SkyrimTest/1.6.1170/Data/SKSE/Plugins/Runefall.dll = B003E1D268D120D1418A41F8F8072827E179C302361D2DA214C662EB2D07BFEC (mtime 10/29 05:24:49); CoSave.cpp mtime 05:23:48; batch start 05:25:16; build-proof log mtime 05:26:44.
3. Independently reproduced batch body_sha256: marker "--- batch report ---", SHA-256 of following UTF-8 bytes = 85F1E03215407CBF8ADAFA62F5CDBD30E012D0584C68D298AEB46506A9C4AD90 = recorded value.
4. python build/slices/S1/tools/verify_fixtures.py -> exit 0, RESULT PASS, 10 checks (incl. new schema1 v1 rank/stage/difficulty/curve assertions).
5. Recomputed every remediation.S1.evidence hash: one mismatch + duplicate entries (see §4). Code inspection CoSave.cpp: MarkReject/Decode/LoadRecords/OnLoad/SelfTest; grep tokens/duplicates.
6. NOT run: native build, static gates, game/batch (read-only audit; runtime claims inspected from committed evidence, not re-executed).

## 2. Criterion verdicts

| ID | Verdict | Evidence / exact remaining gap |
|---|---|---|
| S1-A1 | PARTIAL | LoadRecords unit seam now tests zero/one/multi/unknown and malformed-then-valid, with empty post-state == default (CoSave.cpp:1010-1015, :1079-1098); Decode failure-purity/semantic sentinel retained. Remaining: OnLoad callback seam (:739-796) is still never exercised end-to-end; failure path is Reset()+ItemRegistry.Clear()+Progression/MaterialRegistry.LoadFromCoSave (:773-783), not an explicit clear/test of all three registries, so default post-state after a missing/malformed stream is not proven. |
| S1-A2 | PASS | Header layouts and flags unchanged; batch-052516 row records full 16-byte hex 524E465303000000000003000A000000; decode_cosave/verify fixtures PASS. |
| S1-A3 | PASS | verify_fixtures.py now asserts schema1 legacy expected fields (rank/stage=1, difficulty=255, curve=1) and 'no runeTier/bossKillTier' (10/10 PASS); v2 tier5/tier2 and baseline_v3 ladder fields still PASS; hashes/provenance unchanged and honest. |
| S1-A4 | PASS | Unchanged from audit-8 PASS: Decode schema0/schema4 sentinels with post-state equality; LoadRecord recordVersion 4/0 rejection + token assertions; in-game newer-save simulation BLOCKED accepted by spec. |
| S1-A5 | PARTIAL | Deterministic encode, canonical golden bytes embedded/checked in SelfTest (canonicalHex -> Encode equality + Decode, :984-999), eviction/duplicate registryId on encode. Remaining: decode-side duplicate item.registryId is still not rejected or tested (items loop :502-531 validates only registryId==0 and ranges; duplicate check exists only for encode, :1006-1009); duplicate-key policy for map/override sections still undocumented. |
| S1-A6 | PARTIAL (was FAIL) | Closed: record-size row now uses fixed token 'co-save: reject record-size' (MarkReject :747; frozen list :1104); Decode string lengths use fixed tokens. Remaining: (a) spec row characterClass token 'co-save: reject string' is never emitted (code uses string.class etc.), so the generic token is absent from the frozen table; (b) frozen list adds 'reject string.material'/'reject string.hunt', but the descriptor rows for those keys specify 'reject string.map-key', and those new tokens are unreachable in code (:1118-1122); (c) the 30-token loop MarkReject()s then reads LastRejectToken (self-referential accessor check), so no one-negative-per-row crafted payload matrix proves each row rejects via its own path. |
| S1-A7 | PASS | Unchanged from audit-8 PASS: 1 MiB/2 MiB constants, 1000-item legal case, EncodeWithBudget refusal, OnSave refuses before OpenRecord; docs state 1 MiB. |
| S1-A8 | PARTIAL (build-proof gap CLOSED) | Build proof now covers the audited DLL: proof testEnvDllSha256 B003E1D2... equals the installed DLL I hashed; CoSave.cpp mtime 05:23:48 < DLL 05:24:49 < batch 05:25:16; buildExit=0 and linking output recorded. Batch evidence has cwd/command/exit/body_sha256 + convention, reproducible body hash, full header hex, full co-save field round-trip (skillCount/firstContainer/firstItem/skillRank/firstQuest), Save-files preflight, crash row. Remaining: remediation.S1.evidence still contains a stale crash-assessment.log hash (FFBCCD6D..., current file 011B261F...) plus duplicate evidence entries (crash-assessment x2, LESSONS-LEARNED x3, build-proof-0443 x2); evidence integrity is not gate-enforced, so the S1 record is not clean. Also the 'No new crash log' row cites crash-2026-10-28-21-26-46.log @ 04:26:47 (before batch start, so wording is correct but relies on an old crash file). |

## 3. Audit-8 gap closure

| Audit-8 gap | State at 4e3094d |
|---|---|
| A8 build proof does not cover audited DLL | CLOSED (DLL B003E1D2 matches proof and precedes batch; body_sha256 reproducible) |
| A6 record-size token missing / string tokens | record-size CLOSED; string tokens improved but generic 'reject string' + unreachable material/hunt tokens + per-row matrix remain |
| A1 OnLoad seam / explicit registry clear | PARTIAL (LoadRecords seam added; OnLoad still untested end-to-end, reload-not-clear pattern) |
| A5 decode-side duplicate item + duplicate-key policy | OPEN (not addressed by 4e3094d) |
| State crash-assessment.log hash stale (+ duplicates) | OPEN (still FFBCCD6D vs current 011B261F; duplicate evidence entries) |
| A3 schema1 expected fields unasserted | CLOSED (verify_fixtures 10/10) |

## 4. F7 waiver and governance

- F7-waiver.md (build/audit/F7-waiver.md, sha D05D3A91...) documents the only non-allowlisted change, InputManager.cpp (4-line debug-handler change removing crash-producing ConsoleLog::Print; crash stack Runefall.dll ConsoleLog::Print -> SkyrimSE.exe). HISTORY seq21 = lead/gate/waiver/4e3094d/ACCEPTED. On AGENTS.md 3.1 crash-safety grounds the waiver is acceptable and does not weaken any S1 acceptance criterion; it remains an allowlist exception recorded as follow-up F7-CONSOLE-THREAD.
- State: S1 state=AUDIT_PENDING, status=PARTIAL, commit=4e3094d, implementationAudit still audit-8 (subject e819ecd, PARTIAL) — correct pending this audit. Evidence duplicate/stale entries above must be refreshed before a clean PASS record; truthfulness gate does not hash-check S1 evidence.
- No files were modified by this audit; git status clean before and after.

## 5. Overall verdict

**PARTIAL — not passable for S1 PASS at 4e3094d.** A2, A3, A4, A7 PASS; A1, A5, A6, A8 PARTIAL. Since audit-8, the blocking build-proof gap is genuinely closed (proof/DLL/batch/hash chain verified by independent hash and mtime checks), the schema1 A3 assertions and record-size token are closed, and the F7 waiver is documented and accepted; S1 remains blocked on A5 decode-side duplicate rejection/policy, A6 generic-string token + unreachable frozen tokens + per-row negatives, A1 OnLoad end-to-end/default-post-state proof, and A8 state evidence hash/duplicate cleanup.