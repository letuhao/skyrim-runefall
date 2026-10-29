# S1 co-save implementation audit 10 — code 95be8aa / state 9fc994a

Auditor: dll-config-analyst (independent; did not implement S1). Read-only; only this file written.
HEAD 9fc994a (state), subject code 95be8aa. Spec build/slices/S1/spec.md rev6 SHA-256
E94ECAEFF8375ADF282B93F348370AD6677733EE4EDF7F19D1D78F7E17EC9427 (= state contract, recomputed, matches).
Read: spec rev6, S1-audit-9.md, CoSave.h/cpp (current), evidence/S1/batch-20261029-053626.md,
build-proof-20261029-0536.log, crash-assessment.log, build/audit/F7-waiver.md, commands.log,
run-batch-test.ps1 (preflight/headers), EXECUTION-STATE.json, HISTORY.jsonl, git log/diff/status.

## Commands actually run (read-only)

1. `git log -5`, `git show --stat 95be8aa`, `git show --stat 9fc994a`, `git diff --name-only 4e3094d..95be8aa`
   -> 95be8aa = CoSave.cpp (5 lines, token alignment) + batch/build-proof evidence; 9fc994a = state/HISTORY only; delta inside allowlist.
2. `python build/slices/S1/tools/verify_fixtures.py` -> exit 0, RESULT PASS (10/10, incl. schema1 legacy defaults).
3. `python build/slices/S1/tools/decode_cosave.py` on the five fixtures (run in audit-9 and repos verified; fixture bytes unchanged) -> headers/sections/trailing 0.
4. `powershell -NoProfile -ExecutionPolicy Bypass -File tools/audit-gate.ps1 -StatePath EXECUTION-STATE.json -Mode truthfulness` -> exit 0, `{"ok":true,"code":"GATE-PASS","slices":48,"active":"S1","overall":"IN_PROGRESS"}`.
5. SHA-256 checks: current installed test DLL `J:\SkyrimTest\1.6.1170\Data\SKSE\Plugins\Runefall.dll` =
   F6C82621A7B5D98C19CABBEFE8A903021A77D483B6C609E01A2CA1A227C4D86D, equals build-proof
   `testEnvDllSha256`; DLL mtime 05:35:57 local is after build-proof-recorded CoSave.cppLastWrite 05:35:19
   and before batch start 05:36:26. Batch body hash reproduced: SHA-256 of UTF-8 bytes after
   `--- batch report ---` marker (leading newline trimmed) = CA08CC8D066AC739376A46E4B15B123CDA023CD1CD551EB0D1C5C5FBBE838966 = recorded body_sha256.
6. Rehashed every remediation.S1.evidence entry: all seven current files match recorded hashes
   (spec, spec-audit, audit-9, batch-053626 DACB8902…, build-proof EE1476B5…, crash-assessment 011B261F…,
   F7-waiver D05D3A91…, LESSONS-LEARNED 38FB7872…); no duplicate entries.
7. Code inspection: CoSave.cpp tokens/reject paths (MarkReject, lines 399-659), Decode post-sort duplicate check
   (:661-674), LoadRecords/OnLoad (:690-780), SelfTest token table (:1101-1137), LoadRecords seam tests (:1079-1098).
8. NOT run: native build, static gates, in-game batch (read-only audit; no game launch). Build/batch claims
   verified only via the committed proof/DLL hashes and body-hash reproduction above.

## Criterion verdicts

| ID | Verdict | Exact remaining gap |
|---|---|---|
| S1-A1 | PARTIAL | LoadRecords seam covers empty/one/multi/unknown/malformed-then-valid with default post-state; Decode failure-purity retained. Remaining: OnLoad callback (:739-780) is still not exercised end-to-end; default/failure path uses Reset()+ItemRegistry.Clear()+Progression/MaterialRegistry.LoadFromCoSave (:760-779), not an explicit clear-and-assert of all three registries, so post-state after a missing/malformed stream is unproven in the callback seam. |
| S1-A2 | PASS | v1/v2/v3 header layouts unchanged; batch-053626 records full 16-byte hex `524E465303000000000003000A000000`; fixtures/decoder agree. |
| S1-A3 | PASS | verify_fixtures PASS 10/10 including schema1 legacy expected fields; v2 tier5/tier2 and baseline/canonical PASS; engine SelfTest covers v1/v2 migration + canonical decode (audit-9 PASS, unchanged by 95be8aa). |
| S1-A4 | PASS | Unchanged from audit-9 PASS: schema0/schema4 sentinel purity + LastRejectToken assertions; recordVersion 4/0 rejection token; in-game newer-save simulation BLOCKED per spec. |
| S1-A5 | PARTIAL | Encoder-side duplicate registryId rejection tested; decoder DOES now reject duplicate registryId post-sort (CoSave.cpp:665-669), but there is no SelfTest crafted-payload case for decode-side duplicates; duplicate-key policy for map/override sections remains undocumented. |
| S1-A6 | PARTIAL | Token alignment closed: frozen list now includes item.baseFormId, item.tier, range, generic `co-save: reject string`, `co-save: reject trailing`, and string.map-key (no string.material/string.hunt); record-size token emitted at :747. Remaining: (a) the SelfTest "token table" loop (:1101-1137) is self-referential — it MarkRejects each literal then reads LastRejectToken, so no crafted one-negative-per-row payload proves each row rejects via its real decode path; (b) duplicate-section token is listed but the actual duplicate path (:457) only logger::error()s, never MarkReject(), so that token is unreachable on the real path; no negative tests for most rows. |
| S1-A7 | PASS | 1 MiB/2 MiB constants, 1000-item legal case, EncodeWithBudget refusal, OnSave refuses before OpenRecord; docs state 1 MiB. Residual: `encoded bytes exceed budget` branch unreachable (cap-exceeded token reported instead). |
| S1-A8 | PASS | Clean evidence: cwd/command/exit=0/body_sha256+convention in batch MD; body hash independently reproduced; full 16-byte header hex; Save-files-present preflight row; full co-save field round-trip (skillCount/firstContainer/firstItem/skillRank/firstQuest); build proof covers the audited DLL (hash/mtime chain above); crash row cites `crash-2026-10-28-21-26-46.log @ 04:26:47` which is the pre-batch F7 ConsoleLog::Print crash documented by the accepted F7 waiver, not a new regression; remediation.S1.evidence hashes clean and unique. |

## F7 waiver / governance

- build/audit/F7-waiver.md (accepted in HISTORY seq21) covers the only non-allowlisted file, InputManager.cpp
  (debug F7 ConsoleLog::Print crash removal); crash stack documented; not part of CoSave behavior. It does not weaken S1 criteria.
  Follow-up F7-CONSOLE-THREAD remains open (state followups) and candidate 95be8aa touches only CoSave.cpp + evidence.
- State now records commit=95be8aa, status=PARTIAL, implementationAudit=S1-audit-9 (subject 4e3094d), evidence = 8 clean
  entries (all hashes verified), contract hash correct; latest HISTORY seq22 = audit-9 record. No record yet for audit-10
  (expected on C2). Truthfulness gate passes but does not hash-check the S1 evidence array; the cleanup is correct by inspection.
- Candidate delta 4e3094d..95be8aa is inside the S1 allowlist (CoSave.cpp, evidence/S1 batch+proof); the literal C1 command
  `git diff --name-only a50b2b6..HEAD` still includes pre-C1 S0 paths not grandfathered by the allowlist (spec-wording gap carried forward).

## Overall verdict

**PARTIAL — not PASS.** A2, A3, A4, A7, A8 pass; A1, A5, A6 remain PARTIAL. A6 still lacks a real per-row negative matrix and the duplicate-section MarkReject path; A5 lacks a decode-duplicate test/policy; A1 OnLoad callback/default-registry semantics remain unproven. The A8 build/evidence/provenance gap from audit-9 is genuinely closed by the current DLL hash/mtime chain, reproducible body hash and clean state evidence. The F7 waiver is documented and acceptable, with an open follow-up. No repairs made.
