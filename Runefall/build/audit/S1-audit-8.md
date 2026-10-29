# S1 co-save implementation audit 8 (fresh) — candidate e819ecd / state 35eecc1

Auditor: tool-inventory-independent (fresh implementation auditor; read-only; only this file written).
Date: 2026-10-29. Subject code: e819ecd; state: 35eecc1; baseline a50b2b6. Spec: build/slices/S1/spec.md rev6, SHA-256 E94ECAEFF8375ADF282B93F348370AD6677733EE4EDF7F19D1D78F7E17EC9427 (= state contract hash, verified).
Read: spec; S1-audit-7.md; CoSave.h/cpp (current, 1109 lines region); evidence/S1/batch-20261029-050933.md, build-proof-20261029-0443.log, crash-assessment.log, commands.log, fixtures.log, verify-fixtures.log; run-batch-test.ps1; fixtures/PROVENANCE; HISTORY.jsonl; EXECUTION-STATE.json; F7-hotfix.md.
Read-only commands run: git show e819ecd diff; git log/status; body-hash recomputation; state evidence hash check; python build/slices/S1/tools/verify_fixtures.py (exit 0, RESULT: PASS); stat mtimes of CoSave.cpp/DLL/evidence. NOT run: native build, static gates, game/batch.

## 1. Verdict: NOT PASS — PARTIAL (closure still blocked at e819ecd)

e819ecd closes three of audit-7's four evidence gaps and materially improves the token table, but A8's build proof does not cover the audited candidate DLL, A6's per-row negative/token evidence is still incomplete and partly self-referential, and A1/A5 narrow seams remain. State evidence remains stale for crash-assessment.log.

## 2. Per-criterion verdicts

| ID | Verdict | Evidence / exact remaining gap |
|---|---|---|
| S1-A1 | **PARTIAL** | Decode failure-purity and record-loop semantics covered (LoadRecords zero/one/multi/unknown/malformed-then-valid; empty post-state equals default; CoSave.cpp ~1090-1098, 992-997). **Remaining:** the OnLoad seam is still not exercised end-to-end (only callback registration at CoSave.cpp:739/796); no test that a missing/malformed stream leaves CoSave + ItemRegistry + Progression + MaterialRegistry at the exact default post-state, and the failure path still does Reset() + ItemRegistry.Clear() + Progression/MaterialRegistry.LoadFromCoSave() rather than an explicit clear of all three registries. |
| S1-A2 | **PASS** | Header layouts match spec/code/docs; batch-050933 row "Co-save header hex" records full 16 bytes 524E465303000000000003000A000000 (schema3/flags0/ver3/count10); decode_cosave fixtures PASS (verify_fixtures exit 0). |
| S1-A3 | **PASS** | Fixtures + PROVENANCE + hashes committed; verify_fixtures.py PASS (10/10); engine SelfTest covers v2 tier5->100, v2 tier2->rank/stage 40, v1->rank/stage1 (audit-7 verified; this candidate does not touch those paths). |
| S1-A4 | **PASS** | e819ecd adds Decode-level sentinel assertions for schema 0 and schema 4 (prefill playerRank=77, Encode-equality after reject; CoSave.cpp:878-897) and LoadRecord(recordVersion 4/0) rejection with token assertions. In-game newer-save simulation BLOCKED is spec-accepted. OnLoad default post-state remains under A1. |
| S1-A5 | **PARTIAL** | Deterministic encode/decode, canonical golden (192 B, SHA DC78DF40...), unordered-map reorder determinism, smallest-FormID eviction, and encode-side duplicate-registryId rejection (~1006-1009) covered. **Remaining:** decode-side duplicate item.registryId is not rejected/tested (decoder checks registryId==0 only; grep shows duplicate handling for sections/records but not items); duplicate-key policy for map/override sections still undocumented. |
| S1-A6 | **PARTIAL (was FAIL)** | e819ecd adds a 30-token frozen table with MarkReject/LastRejectToken assertions; direct validation paths do use static fixed tokens (CoSave.cpp:399-509 etc.). **Remaining:** (a) the `3.1 row "record size -> co-save: reject record-size" has no MarkReject fixed token in the code (record-size refusal is formatted logger output only) and is absent from the SelfTest frozen list; (b) characterClass's spec token "co-save: reject string" is not a frozen literal (code uses string.class/string.skill/... variants); (c) the 30-token loop calls MarkReject then reads LastRejectToken (self-referential accessor test), so it does not prove each of the 37 `3.1 rows rejects via its own validation path; no one-negative-per-row crafted-payload matrix exists. |
| S1-A7 | **PASS** | kMaxEncodedBytes=1,048,576 / kMaxRecordBytes=2,097,152; 1000-item legal case under budget; EncodeWithBudget refusal; OnSave refuses before OpenRecord. Carried minor residual (unreachable budget-branch) unchanged and nonblocking. |
| S1-A8 | **PARTIAL** | Closed: explicit "Save files present" preflight row (batch-050933 row 1, before MCP), reproducible body_sha256 (verified: F5100176... = SHA-256 of the report body from "# Runefall batch test" through EOF; convention line present in the file), full 16-byte header hex row, CleanSave/not-Prisoner row, full co-save field round-trip row, crash row. **Remaining (blocking):** the only committed build proof does not cover the audited candidate DLL — build-proof-20261029-0443.log (mtime 04:46 local) records testEnvDll SHA E4C84757...C7934 and CoSave.cpp last write 04:41:15, but the source CoSave.cpp mtime is 04:56:25 and the test-env DLL is SHA 47FD3A95... (mtime 04:57:06); batch-050933 (05:09) therefore ran on a DLL rebuilt after the proof, with no build log for that build. This violates the "build proof before runtime claim" rule (AGENTS.md 3.1.12) and A8's build evidence requirement. |

## 3. Audit-7 gap closure

Closed: A4 Decode-level sentinel post-state (schema0/schema4); A6 static token table/frozen list (30 tokens) and direct fixed-token usage; A8 save-file preflight row and reproducible body_sha256 (verified by independent recomputation); A3 tier2/v1 tests unchanged and still PASS.
Open: A8 build proof for the actual candidate DLL (above); A6 record-size token + per-row negative matrix + generic string token; A1 OnLoad seam/explicit registry clear; A5 decode-side duplicate item + duplicate-key policy; state crash-assessment.log hash stale.

## 4. Governance

- **State evidence hash mismatch persists:** remediation.S1.evidence records evidence/S1/crash-assessment.log SHA FFBCCD6D2F3B... but the current file is E6BA766644A6... (recorded in 19e0abc-era state; 35eecc1 did not refresh it). The audit-gate truthfulness mode passes because it only hash-checks S0 evidence, so this staleness is silent. Blocking for a clean S1 PASS record.
- HISTORY: seq17 audit-6 (FAIL, f765235), seq18 audit-7 (PARTIAL, 19e0abc), seq19 candidate e819ecd — chain prevHash links are consistent, and the audit-6/audit-7 records now exist (audit-7's HISTORY gap is closed). This audit-8 file must be appended as seq20 with its SHA.
- State contract hash matches spec rev6 (verified E94ECAE...); specAudit 6b APPROVED sha matches (3070A6A6...); implementationAudit points at audit-7 PARTIAL — correct pending audit-8.
- **F7 hotfix scope:** the InputManager.cpp F7 console-print removal (cda5dbe) is not in the S1 allowlist and is already in the candidate's ancestry. On safety grounds it is acceptable: minimal 2-line removal in a debug-only handler, no co-save path touched, crash-assessment shows F7 retested with no new crash, and state records follow-up F7-CONSOLE-THREAD. However it remains a candidate-level allowlist exception: it needs an explicit Lead waiver recorded in state/HISTORY (or a documented spec-4 exception), and it must not be retro-fitted into the co-save contract. Literal enforcement of spec section 4 currently makes the candidate nonconforming.

## 5. Required minimum before S1 PASS

1. Produce a foreground build proof for the exact DLL used by the evidence batch: build log with exit 0, source mtimes < DLL mtime, copied test-env DLL SHA matching the batch evidence, and re-run batch-050933-class evidence after that build; store as build-proof-<stamp>.log referenced by the A8 evidence.
2. A6: add the record-size row to the frozen token mechanism (static token + MarkReject) and to the SelfTest list; align characterClass/generic string token; add one negative crafted payload per `3.1 row (or an explicit, justified grouping matrix) with token-text assertions.
3. A1: add an OnLoad-level seam test (empty/malformed stream -> exact default state incl. registries) and explicit clear of ItemRegistry/Progression/MaterialRegistry.
4. A5: decode-side duplicate item.registryId rejection + test; document duplicate-key policy for map/override sections.
5. Refresh the stale crash-assessment.log hash in state; record the F7 waiver; append this audit-8 record to HISTORY (seq20) and set implementationAudit to its verdict.

No source, data, state, history, or build artifacts were modified by this audit.
