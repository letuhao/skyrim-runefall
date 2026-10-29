# S1 co-save implementation audit 11 — code a8fe10e / state 78119f2

Auditor: teammate `environment-analyst` (independent; did not implement S1; no S1 implementation
history). Date: 2026-10-29, Asia/Bangkok. Mode: read-only; only this file written.
Subject code **a8fe10e** ("explicit clear-state seam, duplicate-section token, green self-tests"),
state **78119f2** (HEAD). Spec `build/slices/S1/spec.md` rev6 SHA-256
`E94ECAEFF8375ADF282B93F348370AD6677733EE4EDF7F19D1D78F7E17EC9427` = state contract (recomputed).
Read: spec rev6; `S1-audit-10.md`; `CoSave.h/.cpp` (whole file regions incl. 183-222, 400-480,
690-790, 1002-1165); candidate diff `git show a8fe10e`; `evidence/S1/batch-20261029-055759.md`;
`evidence/S1/build-proof-20261029-0557.log`; `build/audit/F7-waiver.md`; `EXECUTION-STATE.json`;
git log/show/diff/status.

## Read-only checks actually run
1. `git rev-parse/log`: HEAD 78119f2, subject a8fe10e (parent chain 95be8aa->9fc994a->a8fe10e);
   `git diff a8fe10e..HEAD` empty for CoSave.h/.cpp; `git show --stat a8fe10e` = CoSave.cpp/h +
   batch/build-proof evidence only; delta 95be8aa..a8fe10e also touches EXECUTION-STATE.json and
   HISTORY.jsonl (intermediate state commit 9fc994a), all inside the S1 allowlist.
2. Spec contract hash recomputed = state `remediation.S1.contract.sha256` (match).
3. Rehashed all 7 `remediation.S1.evidence` entries: every file exists and SHA-256 matches (spec,
   spec-audit-6b, S1-audit-10, batch-055759, build-proof-0557, crash-assessment, F7-waiver).
4. Build proof: buildExit=0; testEnvDllSha256 `969DA4895E7F74782BD1F326B394BD004B8D587F880CAD6CE894578818020527`
   equals the installed `J:\...\Runefall.dll` hash re-computed now; DLL mtime 05:57:53 is after
   CoSave.cppLastWrite 05:57:12 and before batch start 05:57:59.
5. Batch body hash reproduced: SHA-256 of bytes after `--- batch report ---` =
   `0C9189C92CDBF5A780873892B5BBAFD9E1ADBBE5BE19B6373386B863055B6593` = recorded body_sha256.
6. Code inspection of ClearLoadedState/OnLoad/decode/MarkReject/SelfTest regions (citations below).
NOT run: native build, static gates, game/batch, audit-gate.py (read-only audit; build/batch claims
verified only through committed proof/DLL hashes, body hash and code).

## Criterion verdicts

| ID | Verdict | Basis / exact remaining gap |
|---|---|---|
| S1-A1 | **PARTIAL** | Closed: `ClearLoadedState()` now exists (CoSave.cpp:189-195: Reset + ItemRegistry::Clear + Progression/MaterialRegistry::LoadFromCoSave), OnLoad failure path calls it (:777-781), and success path publishes only after LoadRecords returns true (:765-769); LoadRecords decodes into a local candidate and resets `a_out=CoSaveState{}` on zero valid records (:698-711); seam tests cover empty/one/multi/unknown/malformed-then-valid (:1084-1104). Remaining: the new ClearLoadedState SelfTest (:1143-1151) asserts only CoSave state.playerRank==1 and ItemRegistry empty — Progression and MaterialRegistry post-state are called but never asserted, so A1(b)'s "clears ItemRegistry/Progression/MaterialRegistry" is proven for one of three registries; the OnLoad callback loop itself (GetNextRecordInfo/ReadRecordData, :747-783) is still only code-inspected, not executed by any test (spec allows code inspection, so this is a residual evidence gap, not a functional defect). |
| S1-A2 | **PASS** | Unchanged from audit-10; batch-055759 records full 16-byte header hex 524E465303000000000003000A000000; encoder/decoder v1/v2/v3 layouts unchanged (:313, :431-445); fixtures/decoder unchanged (no fixture edits in candidate diff). |
| S1-A3 | **PASS** | Unchanged from audit-10 (fixtures and verify log untouched by a8fe10e; audit-10 verified 10/10 + v1/v2 expected fields). No new evidence required. |
| S1-A4 | **PASS** | Unchanged: schema 0 and schema 4 sentinel purity + LastRejectToken assertions (:888-904), recordVersion 4/0 token; in-game newer-save simulation BLOCKED per spec. |
| S1-A5 | **PARTIAL** | Decode-side duplicate registryId check exists (post-sort, CoSave.cpp:669-676) but the only duplicate SelfTest is encode-side: `Encode(duplicateItems).empty()` (:1011-1014) — no crafted payload exercises the decode path/rejection token, exactly as audit-10 recorded. Also duplicate-key policy for map/override sections is still neither rejected nor documented (decoder inserts into std::map/std::set; spec A5 says "duplicate keys rejected or documented"). Eviction-at-cap test was added (:1153-1161, smallest FormID) — that gap is closed. |
| S1-A6 | **PARTIAL** | Duplicate-section real path is fixed: `:465` now calls `MarkReject("co-save: reject duplicate-section")` before returning false (audit-10 gap (b) closed), and the token list includes it (:1138). But the token matrix loop (:1140-1141) is unchanged: it calls `MarkReject(token)` then reads `LastRejectToken()`, so it proves only setter/getter equality — no crafted one-negative-per-row payload exercises each row's real decode path (audit-10 gap (a) remains). Real-path negative coverage exists only for a subset (schema0/schema4/record-version/oversized/count-caps/encode ranges). |
| S1-A7 | **PASS** | 1 MiB/2 MiB constants, 1000-item legal case (:1031-1047), EncodeWithBudget refusal (:1069) and OnSave budget check before OpenRecord (:726-729) present; docs state 1 MiB. Residual: with encode caps the >1 MiB "encoded bytes exceed budget" branch is practically unreachable (cap-exceeded path :722-725 fires first); not a criterion defect but evidence for A7's budget branch is synthetic. |
| S1-A8 | **PASS** | Clean, reproducible evidence: batch MD has cwd/command/exit=0/body_sha256 convention; body hash independently reproduced; full 16-byte header hex; Save-files-present preflight; full co-save field round-trip (rank/stage/xp/class/endgame/containers/quests/items/materials/configHash/difficulty/curve/overrides/skillCount/firstContainer/firstItem/skillRank/firstQuest, batch :55); no-new-crash row cites the pre-batch crash-2026-10-28-21-26-46 @04:26:47 covered by the F7 waiver; build proof hash/mtime chain matches the audited DLL; all state evidence hashes verified. |

## Audit-10 gap closure
| Gap | Status |
|---|---|
| A1 ClearLoadedState seam / OnLoad failure | CLEARED functionally (seam added+used); test assertion still covers 1/3 registries → A1 PARTIAL |
| A6 duplicate-section MarkReject | CLEARED (:465) |
| A6 token matrix per-row real-path matrices | OPEN (self-referential loop :1140) |
| A5 decode-side duplicate | OPEN (only encode-side test :1011) |
| A8 build/evidence/provenance | CLEARED (hash/mtime/body-hash/reproduced) |
| F7 waiver | ACCEPTED (see below) |

## F7 waiver / governance
- `build/audit/F7-waiver.md` (hash in state evidence) covers the only out-of-allowlist file,
  `src/plugin/src/InputManager.cpp` (debug F7 ConsoleLog::Print crash removal; hotfix cda5dbe, before
  this candidate). It does not weaken an S1 criterion; follow-up `F7-CONSOLE-THREAD` is recorded in
  `remediation.S1.followups`. Candidate a8fe10e delta is otherwise inside the S1 allowlist.
- Carried-forward governance gap (also in audit-10): spec 4 says C1 enforcement is
  `git diff --name-only a50b2b6..HEAD` inside the allowlist, but that literal command now lists 106
  files incl. pre-C1 S0 paths; the delta from S0 closure (2d606c2..a8fe10e) is 60 files and still
  includes STATE.md, RELEASE-READINESS.md, F7-hotfix.md, LESSONS-LEARNED, S1-batch-blocker.md and the
  waived InputManager.cpp — outside the frozen list. Non-blocking for A1-A8 (audit-10 also recorded
  it) but the C1 control needs a grandfathered manifest or a baseline reset before S1 closure.
- State: S1 state=AUDIT_PENDING, status=PARTIAL, implementationAudit still S1-audit-10 (subject
  95be8aa), commit a8fe10e, blocker list empty; this audit is the record that should replace the
  audit-10 pointer on C2. State evidence manifest is clean (7 unique entries, hashes verified); no
  duplicate entries.

## Overall verdict
**PARTIAL — not PASS.** A2, A3, A4, A7, A8 PASS. A1 closes the ClearLoadedState/OnLoad-failure gap
functionally but its test asserts only the CoSave state + ItemRegistry post-state; A5 still lacks a
crafted decode-side duplicate test and a duplicate-map-key policy; A6 still has the self-referential
token table with no per-row crafted payloads (duplicate-section real path is fixed). No source,
data, state, fixture, gate, build or game action was performed; only this file was written.
