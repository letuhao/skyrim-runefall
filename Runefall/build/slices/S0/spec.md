# S0 — Truth reset and audit harness (revision 5, after four independent spec audits)

Status: DRAFT FOR FIFTH INDEPENDENT SPEC AUDIT
Revision: 5 (amends revision 4 after fourth audit residue)
Program: PROGRAM.md sections 1-5 (Runefall v0.2 Remediation Program)
Baseline revision: a50b2b6
Spec revision: 5

## 1. Normative sources and provenance

1.1 This contract is subordinate to PROGRAM.md, which defines the mission, roles, baseline and slice sequence. PROGRAM.md sections are numbered 1-5 and it is committed in the S0 candidate commit C1.
1.2 The accepted baseline reports are committed in C1 with these exact SHA-256 values:
- build/audit/plan-gaps.md: 4117EC7C861635412F1B260CFD0E5B0208C1C2D4C70AEF4DA8828CF3FC0B3C17 (21,576 B)
- build/audit/runtime-gaps.md: 4AEE2879FE7090096306D4548721B233DF4AAB655E0AE5BD9FD85711CAA8AE6A (20,484 B)
1.3 Baseline is fixed: PASS=7, PARTIAL=30, FAIL=11. PASS IDs: F0.1, F0.4, C1.4, C1.5, R5.1, R5.4, R5.6. FAIL IDs: F0.7, F0.9, W2.2, A3.3, A3.7, P4.2, P4.4, P4.5, P4.6, P4.7, R5.3. All other 30 slices are PARTIAL. plan-gaps PARTIAL/UNVERIFIED (C1.2) normalises to PARTIAL; any UNVERIFIED label normalises to PARTIAL.
1.4 The previous overallStatus=PASS is retracted. A PASS at top level is forbidden; the gate derives COMPLETE only under release mode.

## 2. Scope

In scope:
- EXECUTION-STATE.json schema v2 and truth reset; build/state.schema.json.
- A top-level remediation object for S0-S15; slices{} remains exactly the 48 plan IDs.
- build/TRACEABILITY.json, build/audit/HISTORY.jsonl, tools/audit-gate.ps1, fixtures, validators.
- .gitignore amendments and durable artifacts; docs truth reset; S0 raw evidence logs.
Out of scope:
- src/plugin/**, content/**, tools/Runefall.Build/**, fomod/**, release archive/DLL rebuild, static-gates, gameplay code.

## 3. State schema v2 (resolves R1/R3/M-03/M-05)

3.1 build/state.schema.json is normative, JSON Schema 2020-12, schemaVersion=2, additionalProperties=false at every level.
3.2 Root fields: schemaVersion, program, sourceRevision, baseline, overallStatus, currentSlice, updatedAt, remediation, slices, auditHistory, release.
3.3 overallStatus enum: IN_PROGRESS | BLOCKED | COMPLETE. PASS is forbidden at root.
3.4 remediation is an object keyed S0..S15. Each record: id, state, status, contract{path,sha256} (nullable), specAudit{path,sha256,verdict,recordCommit} (nullable), implementationAudit{path,sha256,verdict,subjectCommit,recordCommit} (nullable), commit (nullable), evidence[], blockers[], followups[]. Contract and specAudit are null until the slice reaches SPEC_AUDIT; future S1-S15 records may exist with null contract/specAudit/audits and state PENDING.
3.5 state enum: PENDING | SPEC_AUDIT | READY | IMPLEMENTING | LOCAL_CHECKS | AUDIT_PENDING | PASS | PARTIAL | FAIL | BLOCKED. status enum: PASS | PARTIAL | FAIL | NOT_STARTED | BLOCKED.
3.6 slices is exactly the 48 ULTIMATE-PLAN IDs. Each record: id, layer, slice, status, baselineAudit{reports:[{report,verdict,sha256}], commit, evidence[], blockers[], followups[], checklist}. Where both baseline reports cover a slice, runtime-gaps is authoritative and plan-gaps is recorded as the secondary report. Baseline PASS slices use baselineAudit; they are not remediation PASS and never count toward release.
3.7 Nullability: contract, specAudit, commit, implementationAudit, recordCommit are null before their lifecycle stage; once a remediation slice is PASS, contract/specAudit/implementationAudit/commit are required. Baseline slices may have null remediation fields. The schema MUST accept both (a) all sixteen remediation records PENDING before S0 starts, and (b) fifteen PENDING records plus exactly one active S0 record. Any count text must say fifteen PENDING + one active when the active record exists.
3.8 checklist values: each entry is {criterionStatus, detail}. criterionStatus enum: PASS | PARTIAL | FAIL | BLOCKED | NOT_RUN | NOT_VERIFIED | DEFERRED | NOT_APPLICABLE. Legacy free-text values normalise with case-insensitive prefix matching: any value beginning PASS (PASS_BRIDGE, PASS_28, PASS_7_entities, PASS (schema1 observed), PASS 1939 bytes observed) -> PASS + detail=original; beginning PARTIAL -> PARTIAL + detail; beginning FAIL -> FAIL + detail; beginning BLOCKED -> BLOCKED + detail; beginning NOT RUN -> NOT_RUN + detail; beginning NOT VERIFIED/NOT_VERIFIED -> NOT_VERIFIED + detail; beginning DEFERRED -> DEFERRED + detail; N/A or NOT_APPLICABLE -> NOT_APPLICABLE; any unknown -> FAIL + detail. The fixture must cover each prefix, including space-form NOT RUN (...) and NOT VERIFIED (...). The normaliser is build/audit/normalize_legacy.py and its output hash is recorded in HISTORY.
3.9 Evidence entries are objects {path, sha256, kind (log|report|code|data), exists}; gate verifies non-empty, hash match, and exists=true.
3.10 HISTORY.jsonl line schema: seq (monotonic), ts, actor, role (spec-auditor|implementer|implementation-auditor|lead|gate), action, slice, subjectCommit, artifacts[{path,sha256}], verdict, prevHash, hash. Hash chain must verify; no rewrites. Auditor writes its own record; Lead appends gate/closure records.

## 4. Remediation slice records and bootstrap (resolves R1/R2/M-04)

4.1 S0 has a remediation record even though slices{} is 48 plan IDs. currentSlice refers to remediation, never slices.
4.2 Bootstrap: audit-gate.ps1 -Mode truthfulness -Bootstrap -CurrentSlice S0 permits exactly one missing implementationAudit, scoped to the named currentSlice only, while that record state is in {IMPLEMENTING, AUDIT_PENDING}, implementationAudit=null, recordCommit=null. All other remediation records may be PENDING with null audits. It is used only before C2 and records GATE-BOOTSTRAP. Without -Bootstrap, a PASS claim with a missing audit is rejected.
4.3 A remediation slice PASS requires implementationAudit.verdict=PASS, subjectCommit == remediation.commit, evidence exists/hash-matches, no blockers, state PASS. The audit file may be committed later at recordCommit (descendant of subjectCommit); gate checks ancestry.
4.4 staleAuditedCommit = subjectCommit is not an ancestor of HEAD, or spec/contract hash differs from the audited hash, or state updatedAt is older than the last relevant commit. Re-audit required.

## 5. Gate semantics and fixtures (resolves R2/M-04/M-11/M-12)

5.1 tools/audit-gate.ps1 -StatePath <path> -Mode truthfulness|release [-Bootstrap] [-SkipGitCheck].
5.2 truthfulness: validates schema, baseline counts/IDs/hashes, legacy normalisation, allowed criterionStatus, remediation records, audit ancestry, evidence integrity, blockers, and dirty tracked tree. It returns exit 0 when truthful even if many slices are PARTIAL/FAIL/BLOCKED; this is the only legal pre-release aggregate. It MUST NOT require implementation audits for baseline-only or NOT_STARTED/PENDING slices. Contract/spec hash checks apply only to non-null records whose state is at or beyond SPEC_AUDIT.
5.3 release: exit 0 only if all 48 slices PASS with valid baseline/implementation audits, all S0-S15 PASS with implementation audits, currentSlice COMPLETE, overallStatus COMPLETE, clean tracked tree, RELEASE-audit.md PASS, archive verification PASS; otherwise exit non-zero.
5.4 Reject false PASS: any slice status PASS without valid audit; any remediation PASS without audit; any checklist criterionStatus not in enum; any PASS suffix; any unresolved blocker; any evidence missing/empty/hash mismatch; any audit subjectCommit mismatch/not ancestor; any baseline count/ID/hash mismatch; any doc forbidden claim; any state/spec/contract hash mismatch.
5.5 Reason codes: GATE-E01 schema, E02 baseline, E03 false-PASS, E04 audit missing/mismatch, E05 evidence, E06 stale commit, E07 dirty, E08 blocker, E09 doc claim, E10 history, GATE-BOOTSTRAP.
5.6 Fixtures under build/audit/fixtures/S0: positive control + one negative per E01-E10 + forged-PASS + missing-evidence + empty-evidence. Each fixture records expected exit and code; tracked.
5.7 Gate must not invoke rebuild, run-static-gates.ps1, game, or absolute external paths. Dirty means git status --porcelain --untracked-files=no.

## 6. Durability (resolves D1/M-02/M-06)

6.1 .gitignore must add exactly:
!Runefall/build/
Runefall/build/*
!Runefall/build/audit/
!Runefall/build/audit/**
!Runefall/build/slices/
!Runefall/build/slices/**
!Runefall/build/evidence/
!Runefall/build/evidence/**
!Runefall/build/TRACEABILITY.json
!Runefall/build/state.schema.json
!Runefall/build/evidence/S0/**
6.2 Verify: git check-ignore returns no match for spec/reports/audit/slices/evidence/S0/traceability/schema; git add -n succeeds; git ls-files lists them after commit.
6.3 PROGRAM.md, this spec, both baseline reports, spec audit report, state schema, traceability, history, gate and fixtures are committed in C1. S0-audit.md is committed in C2; state closure in C3 if separate.
6.4 If the negation block cannot be made to work in this repository, relocate the governance set to Runefall/evidence/v02/ and update paths, with C1 committing the relocation and a migration mapping. This is a decision point recorded in DECISIONS.md.

## 7. Documentation truth reset and claim inventory (resolves R5/M-08)

7.1 Files in scope: STATE.md, RELEASE-READINESS.md, VALIDATION.md, CHANGELOG.txt, EXECUTION-EVIDENCE.json, EXECUTION-STATE.json release block, PROGRAM.md.
7.2 A single STATUS line must appear identically at top of STATE.md and RELEASE-READINESS.md: STATUS: v0.2 remediation — S0 truth reset — audit in progress — NOT RELEASED.
7.3 Forbidden current claims: 48/48 PASS, all gates pass, all 48 slices PASS, published release, first public release (unless prefixed HISTORICAL and date-bound), ship gate PASS. Historical changelog entries remain but a new appended retraction section states the 0.1.0 candidate was invalidated by the audit.
7.4 Release block in state: version 0.1.0, archive null, verified false, published false, retracted true, reason audit baseline.
7.5 UI dependency contradiction must be recorded as unresolved followup S9 (FOMOD SKSE Menu Framework vs Prisma hard dependency), not silently changed.

## 8. Traceability (resolves M-07)

8.1 build/TRACEABILITY.json rows: sliceId, planRow (verbatim), planSha256, baselineStatus, acceptanceClauses[], verification, evidence[], followupSlice, currentStatus.
8.2 Exactly 48 slice rows, exact ID set equal to ULTIMATE-PLAN table; plan file SHA-256 recorded. Registry status multiset must equal state slices status multiset.
8.3 Baseline FAIL/PARTIAL visible; no row may be hidden or collapsed.

## 9. Acceptance criteria (frozen)

| ID | Criterion | Verification | Pass condition |
|---|---|---|---|
| S0-A1 | Baseline and provenance reconciled and committed: baseline hashes match, PROGRAM.md/spec/reports tracked, 7/30/11 mapping exact. | Independent hash/ID/count script; git ls-files. | Hashes match; 48 IDs exact; counts 7/30/11; FAIL IDs exact; all governance files tracked. |
| S0-A2 | State schema v2 and both slice systems exist; all-16-PENDING and 15-PENDING+1-active remediation states validate (null contracts/audits); legacy 371 checklist values normalised with explicit map; HISTORY hash-chain schema. | jsonschema validation; normaliser fixture; history verifier. | Schema valid; 48 slices + S0 remediation; normaliser deterministic; history chain verifies. |
| S0-A3 | Truthful aggregate: overallStatus non-PASS; truthfulness gate exit 0 on truthful state; release gate exit non-zero; forged overall PASS rejected. | Gate runs on state; negative fixtures. | Truthfulness 0; release non-zero; forged PASS rejected. |
| S0-A4 | Gate rejection matrix and reason codes with positive control and >=12 negative fixtures, including E01-E10, forged PASS, empty/missing evidence. | Run every fixture; capture exit/code. | All expected outcomes; no rebuild invoked. |
| S0-A5 | Traceability exact 48 rows, plan hash, verbatim rows, status multiset equals state. | Independent script. | Exact match. |
| S0-A6 | Documentation truth reset with STATUS line, forbidden-claim inventory, appended changelog retraction, retracted release block, UI contradiction followup. | Grep/claim inventory; independent read. | No forbidden current claim; retraction visible; STATUS identical; followup recorded. |
| S0-A7 | No source/data/feature changes; exact allowlist (paths relative to repo root): .gitignore, Runefall/PROGRAM.md, Runefall/EXECUTION-STATE.json, Runefall/EXECUTION-EVIDENCE.json, Runefall/STATE.md, Runefall/RELEASE-READINESS.md, Runefall/VALIDATION.md, Runefall/CHANGELOG.txt, Runefall/DECISIONS.md, Runefall/build/slices/**, Runefall/build/audit/**, Runefall/build/evidence/S0/**, Runefall/build/TRACEABILITY.json, Runefall/build/state.schema.json, Runefall/tools/audit-gate.ps1, Runefall/tools/audit-validators/**. | git diff --name-only a50b2b6..HEAD + git status -uall + allowlist. | No out-of-scope paths. |
| S0-A8 | Durable, reproducible evidence: check-ignore empty, files committed, raw logs command/cwd/exit/output hash, history records, no external path dependency, audit placeholder policy state audit=null and no file. | Independent commands; log/history format check. | All checks pass on a clean clone. |

## 10. Workflow and audit ownership

10.1 One owner per audit path. Spec audits use build/audit/S0-spec-audit.md, S0-spec-audit-2.md, S0-spec-audit-3.md (next revision number); implementation audit writes build/audit/S0-audit.md. No concurrent writers.
10.2 Spec approval records spec SHA-256 in HISTORY by the spec auditor; later edits void approval unless an amendment is audited.
10.3 C1 candidate includes implementation; auditor inspects exact C1 and writes S0-audit.md; C2 commits audit report; C3 state closure. Gate checks ancestry.
10.4 The implementation auditor does not modify implementation files, state, or fixtures; it may write only S0-audit.md and its HISTORY record.

## 11. Open decisions recorded

- v0.2 is the remediation program label, not the shipped mod version (mod version remains 0.1.0 internal candidate; invalidated release claim).
- build/execution/state.json is a legacy snapshot superseded by EXECUTION-STATE.json; it is archived in place and not deleted in S0. The gate reads only the stated -StatePath; if an operator passes the wrong state file, the validation fails naturally.
- One legacy baseline evidence path may be grandfathered if it is a runtime directory; all new evidence must exist.
- S0 is not split; if audit scope proves unmanageable, split into S0a/S0b by amendment before implementation.
