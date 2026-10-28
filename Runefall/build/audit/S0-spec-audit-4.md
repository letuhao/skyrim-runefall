# Fourth independent spec audit — S0 truth reset and audit harness (revision 4)

Auditor: teammate papyrus-analyst (fresh; did not author S0, PROGRAM.md, or audits 1-3; not the implementer). Date: 2026-10-28, Asia/Bangkok. Mode: read-only; only this file written.
Subject: build/slices/S0/spec.md revision 4, 13,809 B, SHA-256 F66E9EBC76A582034D261BE710655C6F339AE0D421DB9581E47052DD06E6EF1E.
Also read: PROGRAM.md revision (2,886 B, SHA-256 58168D368DF12791F359A3241205B29D738944F2D7F75011DF8267660DCFEB87), build/audit/S0-spec-audit-3.md, EXECUTION-STATE.json (36,376 B, SHA-256 32A186F708B26A277362CA99641142FC599A54B9C17D99AA6E51A523501B6837), repo-root .gitignore (939 B, SHA-256 9BC63D904E17D18DC9EA7FBBED33D2B6A96D4FFC26F40C6EDD0D2EB71EA61D71). git HEAD a50b2b6; working tree only untracked Runefall/PROGRAM.md.

Scope: only audit-3 blockers B-1/B-2 and cheap items N1/N3/N4/N7. No broadening, no implementation.

## 1. Verdict

**REJECTED** — B-2 is resolved and B-1's nullability/scoping/hash-scoping clauses are correct, but B-1 is not fully closed: the schema acceptance sentence in 3.7 and acceptance criterion S0-A2 are arithmetically impossible as written ("sixteen PENDING remediation records and exactly one active record"), since remediation has exactly sixteen records S0..S15 (3.4). Two legacy N items (N1, N3) are only partially fixed and are cheap same-revision text changes.

## 2. Checklist results

| Item | Required | Revision 4 | Verdict |
|---|---|---|---|
| B-1a contract/specAudit nullable before SPEC_AUDIT | 3.4: "Contract and specAudit are null until the slice reaches SPEC_AUDIT; future S1-S15 records may exist with null contract/specAudit/audits and state PENDING"; 3.7 nullability list (contract, specAudit, commit, implementationAudit, recordCommit); A2 requires null-acceptance | FIXED |
| B-1b 16 PENDING records valid | 3.7: "The schema MUST accept a truthful state with sixteen PENDING remediation records and exactly one active record"; A2 repeats "sixteen PENDING records". 3.4 fixes remediation = S0..S15 (16 records), so 16 PENDING + 1 active = 17 records - impossible. The truthful S0 state is 1 active (currentSlice) + 15 PENDING, or an all-PENDING pre-start state; the sentence matches neither. | **NOT FIXED (blocking)** |
| B-1c bootstrap scoped to currentSlice | 4.2: "-Bootstrap -CurrentSlice S0 permits exactly one missing implementationAudit, scoped to the named currentSlice only, while that record state is in {IMPLEMENTING, AUDIT_PENDING} ..." | FIXED |
| B-1d gate hash checks only non-null >= SPEC_AUDIT | 5.2: "Contract/spec hash checks apply only to non-null records whose state is at or beyond SPEC_AUDIT"; 5.4 keeps rejection for real mismatches | FIXED |
| B-2 exact A7 allowlist | A7 now enumerates: .gitignore, PROGRAM.md, EXECUTION-STATE.json, EXECUTION-EVIDENCE.json, STATE.md, RELEASE-READINESS.md, VALIDATION.md, CHANGELOG.txt, DECISIONS.md, build/slices/**, build/audit/**, build/evidence/S0/**, build/TRACEABILITY.json, build/state.schema.json, tools/audit-gate.ps1, tools/audit-validators/**. No "state/docs"; DECISIONS.md present (needed by 6.4); EXECUTION-EVIDENCE.json present (named by 7.1); consistent with 6.4 and 7.1. | RESOLVED (two nits, section 4) |
| N1 PROGRAM numbering citation | PROGRAM.md now numbered 1-5 (verified: "## 1. Mission" ... "## 5. Non-negotiable constraints"); 1.1 correctly says "sections are numbered 1-5", but the spec header still says "Program: PROGRAM.md sections 1-8", the title line still says "revision 3", and "Spec revision: 3" is stale against Revision: 4. | PARTIAL (cheap fix) |
| N3 normaliser variant prefixes | 3.8 now states case-insensitive prefix matching and covers PASS/BLOCKED/NOT_RUN/NOT_VERIFIED/DEFERRED/N/A/unknown, but "beginning PARTIAL/FAIL/BLOCKED/NOT_RUN/NOT_VERIFIED/DEFERRED -> matching criterionStatus" is undefined: criterionStatus enum is PASS, PARTIAL, FAIL, NOT_APPLICABLE, so four of those six prefixes have no matching enum member. Determinism required by A2 is therefore not yet falsifiable. | PARTIAL (cheap fix) |
| N4 audit filename namespace | 10.1 uses build/audit/S0-spec-audit.md, -2.md, -3.md (next revision number); on-disk set matches; this audit correctly lands as -4. | RESOLVED |
| N7 baseline precedence | 3.6: "Where both baseline reports cover a slice, runtime-gaps is authoritative and plan-gaps is recorded as the secondary report." Matches audit-3 N7 required one-sentence rule. | RESOLVED |

Independently confirmed context: PROGRAM.md has 5 numbered sections; EXECUTION-STATE.json still has no schemaVersion/remediation (overallStatus=PASS, currentSlice=COMPLETE) so it is the invalid data 1.4 describes; build/execution/state.json exists (see nit B); repo-root .gitignore already contains build/, Runefall/build/ and the evidence negation.

## 3. Minimum text changes for approval

1. 3.7 — replace "The schema MUST accept a truthful state with sixteen PENDING remediation records and exactly one active record." with: "The schema MUST accept a truthful state in which remediation contains exactly the sixteen records S0..S15: every non-active record is PENDING with null contract, specAudit, implementationAudit and commit, and at most one record (the currentSlice) is active. An all-PENDING pre-start state with sixteen PENDING records is also valid."
2. S0-A2 — replace "sixteen PENDING records with null contracts/audits validate" with: "all sixteen remediation records S0..S15 validate (fifteen PENDING plus one active, or all sixteen PENDING); PENDING records have null contract/specAudit/implementationAudit/commit".
3. 3.8 — replace "beginning PARTIAL/FAIL/BLOCKED/NOT_RUN/NOT_VERIFIED/DEFERRED -> matching criterionStatus" with an explicit mapping to the four-member enum, e.g. "beginning PARTIAL -> PARTIAL; beginning FAIL/BLOCKED/NOT_RUN/NOT_VERIFIED/DEFERRED -> FAIL; N/A or NOT_APPLICABLE -> NOT_APPLICABLE; unknown -> FAIL". Add one normaliser fixture per mapped prefix.
4. Header — change "Program: PROGRAM.md sections 1-8" to "sections 1-5"; change title "revision 3" and "Spec revision: 3" to 4.

## 4. Nonblocking nits (not gating)

- A. A7 entries are Runefall-relative (build/..., PROGRAM.md) except ".gitignore", which exists only at repo root (Runefall/.gitignore does not exist); state the path base ("relative to Runefall/, plus repo-root .gitignore") so git diff --name-only a50b2b6..HEAD output is directly comparable.
- B. Section 11 says build/execution/state.json is superseded and "gate rejects multiple authoritative state files", but A7 does not allow deleting that existing tracked file. Add either "removal of build/execution/state.json is allowed in C1" to A7 or scope "multiple authoritative state files" to EXECUTION-STATE.json variants only.
- C. 3.7 requires contract/specAudit/implementationAudit/commit once a remediation slice is PASS; keep that conditional wording explicit so A2 null-acceptance and PASS acceptance are unambiguous.

No other blocking issue found. B-1's nullability/bootstrapping/hash-scoping intent is correct; only the record-count sentence and two cheap legacy nits block approval.

## 5. Auditor statement

Read-only verification. Only this file was written; no state, spec, PROGRAM.md, gate, fixture, git, build or game action. Commands: file reads, SHA-256 over the four inputs, git rev-parse/status. Independence: not the implementer; did not author S0, PROGRAM.md, or audits 1-3.
