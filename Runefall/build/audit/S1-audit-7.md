# S1 co-save implementation audit 7 — candidate 19e0abc / state d18cf95

Subject code: **19e0abc** (includes f765235, cda5dbe F7 hotfix, 19e0abc); HEAD **d18cf95** (state record). Baseline a50b2b6.
Spec: build/slices/S1/spec.md **rev 6**, SHA-256 E94ECAEFF8375ADF282B93F348370AD6677733EE4EDF7F19D1D78F7E17EC9427 (= remediation.S1.contract).
Auditor: independent; read-only; only this file written. Prior context: S1-audit-5/6, S1-spec-audit-6b (APPROVED).
Files read: CoSave.h/cpp (1109 lines), build/audit/F7-hotfix.md, evidence/S1/batch-20261029-044245.md, build-proof-20261029-0443.log, crash-assessment.log, run-batch-test.ps1, fixtures+PROVENANCE, state, HISTORY.

## 1. Read-only commands actually run (captured)

| Command (cwd Runefall/) | Actual result |
|---|---|
| `python build/slices/S1/tools/verify_fixtures.py` | exit 0, RESULT PASS, 10/10 checks |
| `python build/slices/S1/tools/decode_cosave.py` (5 fixtures) | exit 0; schema1/ver1/count9/trailing0; schema2(×2)/ver2/count9; schema3 flags0/ver3/count10/section11=16B/trailing0 |
| `powershell tools/audit-gate.ps1 -StatePath EXECUTION-STATE.json -Mode truthfulness` | exit 0, `{"ok":true,"code":"GATE-PASS","slices":48,"active":"S1","overall":"IN_PROGRESS"}` |
| `git rev-parse HEAD` / `git diff --name-only 576a840..19e0abc` / `git status --porcelain` | HEAD d18cf95; candidate files: CoSave.cpp, **InputManager.cpp (F7)**, run-batch-test.ps1, evidence/S1/**, build/audit/{F7-hotfix,S1-batch-blocker,LESSONS-LEARNED}, state, HISTORY; tree clean except prior untracked S1-audit*.md |
| Static inspection | MarkReject tokens 33 calls / 30 unique; SelfTest 1109 lines; 27/27 §3.1 token rows present in code; only 2 distinct token texts asserted via LastRejectToken in SelfTest |
| Evidence checks | batch-044245 body_sha256 704099DF… not reproducible from file (whole/body/trim/LF variants tested); state evidence crash-assessment.log hash MISMATCH; HISTORY last seq 16 (audit-5), no audit-6 record |
| NOT run | native build, static gates, game/batch (read-only audit; no game launch). Build/batch claims inspected from committed evidence only. |

## 2. Criterion verdicts

| ID | Verdict | Evidence / exact remaining gap |
|---|---|---|
| S1-A1 | **PARTIAL** | LoadRecords zero/one/multi/unknown/malformed-then-valid covered; empty-records post-state now asserted equal to default (`Encode(emptyOut)==Encode(CoSaveState{})`, CoSave.cpp ~1090-1098); Decode failure-pure via candidate+assign and sentinel tests. **Gaps:** OnLoad seam itself not exercised (no test that absent/malformed stream leaves CoSave+ItemRegistry+Progression+MaterialRegistry at exact defaults); failure path still `Reset()+ItemRegistry.Clear()+Progression/MaterialRegistry.LoadFromCoSave()` (~773-778), not an explicit clear of all three. |
| S1-A2 | **PASS** | v1/v2 12-byte and v3 16-byte layouts match spec/code/docs (POWER-LADDER:178, L4-RUNTIME:117/134); SelfTest header byte assertions + full 16-byte hex; batch row now records **full 16-byte** header `524E465303000000000003000A000000` (schema3/flags0/ver3/count10); decoder reproduces all five fixtures. |
| S1-A3 | **PASS** | Fixtures+hashes committed, PROVENANCE honest (one-off pre-S1 writer mirror, allowed); verify 10/10; engine SelfTest now covers v2 tier5→rank100, **v2 tier2→rank/stage 40** (memcpy runeTier at payload offset 62) and **v1→rank/stage1, diff255, curve1**; canonical/baseline bytes engine-decoded (rank42/stage42). Minor note: engine tests use synthetic byte layouts equivalent to the v1/v2 fixture files, not the .bin files themselves; verify_fixtures' schema1 "expected migration" check remains a JSON-literal check (now backstopped by the engine v1 test). |
| S1-A4 | **PARTIAL** | Distinct tokens asserted by exact text (schema vs record-version); recordVersion 4/0 and payload schema 0/4 rejected; **LoadRecord rejection post-state now asserted unchanged** (`Encode` equality, ~992-997). **Gaps:** Decode-level sentinel post-state for schema0/schema4 not asserted; OnLoad-level default post-state untested; in-game newer-save simulation BLOCKED (spec-accepted). |
| S1-A5 | **PARTIAL** | encode(decode(encode)) byte-identity; canonical golden encode+decode (embedded hex == canonical_v3.bin, 192 B, SHA **DC78DF40…**); unordered-map reorder determinism; smallest-FormID eviction; **encode now rejects duplicate registryId** (~998-1001). **Gaps:** no decode-side duplicate test (crafted payload with duplicate registryId/duplicate known section only exercised by code path); duplicate-key policy for map/override sections still undocumented. |
| S1-A6 | **PARTIAL (was FAIL)** | 27/27 §3.1 rows now have a `co-save: reject <row>` token in code, incl. **item.baseFormId**, **item.tier**, **item.category/socketRunes/affix strings**, per-collection counts, `co-save: reject range`, and trailing wording fixed to `co-save: reject trailing`; MarkReject=33 calls/30 unique. **Gaps:** no one-negative-per-row SelfTest matrix and no per-row token-text assertions — only schema and record-version tokens are asserted (LastRejectToken used 3×); decode-side negative cases for the 27 rows are not evidenced. |
| S1-A7 | **PASS** | kMaxEncodedBytes=1,048,576 / kMaxRecordBytes=2,097,152; 1000-item legal case under budget; EncodeWithBudget(…,100) refusal; OnSave refuses before OpenRecord. Carried minor residual: "encoded bytes exceed budget" branch unreachable because Encode enforces the cap (cap-exceeded token reported instead). |
| S1-A8 | **PARTIAL** | Closed: full 16-byte header row; not-Prisoner assertion ("Player loaded in game" name=CleanSave); cell query row; **build-proof log** (foreground build exit 0, DLL copied to J:\SkyrimTest, SHA E4C84757…C7934, CoSave.cpp mtime 04:41:15 < batch 04:42); crash assessment now covers the 04:26 F7 crash + input-device crash + batch 04:42 no-new-crash; field round-trip intact (rank/stage/xp/class/skillRank/difficulty/curve/endgame/containers+member/quests+member/overrides/items+registryId/materials/configHash). **Gaps:** save-file (.ess/.skse) existence check is a silent preflight gate (no explicit report row; failure path exits 2) — Lead-requested save-file preflight row absent; recorded body_sha256 remains unreproducible; no independent in-game re-run (game/user-side). |

## 3. Audit-6 gap closure

Closed: A6 missing baseFormId/tier/range tokens + trailing wording (27/27 rows); A3 engine tier2 (rank40) and v1 migration tests; A4 LoadRecord rejection post-state; A5 duplicate-registryId encode rejection; A8 full 16-byte header, not-Prisoner/cell rows, foreground build proof, crash assessment including F7 crash.
Open: A6 per-row negative/token evidence matrix; A1 OnLoad seam + explicit clear; A4 Decode/OnLoad post-state; A5 decode-side duplicate + policy doc; A8 save-file row + body_sha256 convention.

## 4. F7 hotfix scope (out-of-S1-allowlist change)

- Commit cda5dbe `fix: disable unsafe F7 console print` changes only **InputManager.cpp** F7 handler: removes `RE::ConsoleLog::Print("[Runefall] diagnostics: %s", …)` for the multi-kB diagnostics JSON (crash 2026-10-29 04:26:47, stack Runefall.dll!ConsoleLog::Print → SkyrimSE.exe); full JSON is still logged. Minimal (2-line deletion + comment), no co-save path touched.
- Scope is documented in build/audit/F7-hotfix.md as follow-up **F7-CONSOLE-THREAD**, and crash-assessment.log records F7 retest with no new crash. InputManager.cpp is **outside the frozen S1 allowlist** (spec §4 names only CoSave.h/.cpp + DebugHarness.cpp): the candidate therefore relies on a crash-safety hotfix exception. Auditor position: acceptable on safety grounds (prevents a player CTD, AGENTS.md §3.1 "CTD never acceptable"), but it requires explicit Lead/spec sign-off and must not be retro-fitted into the S1 co-save contract; if the S1 allowlist is enforced literally, candidate 19e0abc is allowlist-nonconforming.

## 5. Governance observations

- remediation.S1: state AUDIT_PENDING, status PARTIAL, commit **19e0abc** ✓, implementationAudit = S1-audit-6 FAIL @f765235 (stale until this report is recorded); evidence[] now includes batch-044245, build-proof, F7-hotfix, LESSONS-LEARNED.
- **state evidence hash mismatch:** `evidence/S1/crash-assessment.log` recorded in state but current file SHA-256 ≠ recorded value (file edited at 19e0abc; state evidence entry stale). Gate truthfulness still passes because the gate only hash-checks S0 audit evidence, not S1 evidence — silent staleness.
- **HISTORY gap:** last line seq 16 = S1-audit-5 (576a840); no HISTORY record for S1-audit-6 or for candidate 19e0abc — the audit-6 referenced in state is not in the hash chain.
- Candidate diff 576a840..19e0abc also contains out-of-allowlist InputManager.cpp (§4 above). The literal C1 command `git diff --name-only a50b2b6..HEAD` still includes pre-C1 S0 paths not grandfathered by the S1 allowlist (carried spec-wording gap from audits 4-6).
- This report written to S1-audit-7.md per Lead direction; spec §4 names S1-audit.md — path/record reconciliation still required at C2.

## 6. Overall verdict

**NOT PASS — PARTIAL (closure blocked) at 19e0abc.** A2, A3 and A7 PASS. A1, A4, A5, A8 are PARTIAL (narrow seams) and A6 is PARTIAL: tokens are now complete in code but the mandated per-row negative/token-text evidence matrix is still missing. A8 closed most of audit-6 (full header, preflight rows, build proof, crash assessment) but still lacks an explicit save-file preflight row and reproducible body_sha256. F7 hotfix is a justified crash-safety fix but lies outside the frozen S1 allowlist and needs explicit sign-off.

Remaining closure work: (1) A6 SelfTest one-negative-per-row matrix with token-text assertions (27 rows); (2) A1 OnLoad seam/default-registry test + explicit clear semantics; (3) A4 Decode/OnLoad rejected post-state; (4) A5 decode-side duplicate test + map/override duplicate policy; (5) A8 explicit save-file preflight row + document body_sha256 convention; (6) governance: record this audit + audit-6 in HISTORY, refresh crash-assessment.log evidence hash, decide F7 allowlist exception.

Auditor independence: read-only; no source/state/fixture/build/game action; only this report written.
