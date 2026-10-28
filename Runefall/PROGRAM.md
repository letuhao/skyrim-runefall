# Runefall v0.2 Remediation Program

Status: ACTIVE — S0 truth reset and audit harness
Owner: Lead (Runefall project)
Baseline commit: a50b2b6
Baseline audit: 2026-10-28 independent audit reports

## 1. Mission

Remediate the invalidated 48/48 PASS claim and complete the missing Runefall v0.2 work one
independently audited slice at a time. Every completion claim must be reproducible from the
actual repository, the exact audited commit, executable checks, and raw evidence.

## 2. Mandatory roles and gates

For every slice Sx:
1. A fresh specification auditor (no implementer history) approves build/slices/Sx/spec.md.
2. An implementer changes the smallest complete unit and commits a candidate.
3. A fresh independent implementation auditor (no implementer history) examines that exact
   candidate, runs independent checks, and writes build/audit/Sx-audit.md with PASS, PARTIAL,
   FAIL, or BLOCKED.
4. The Lead may mark Sx PASS only when the auditor returns PASS for the candidate and
   tools/audit-gate.ps1 accepts the state.

The implementer may not audit its own slice. The Lead may not author an audit on an auditor's
behalf. Missing evidence, unrun commands, and successful builds alone are not PASS.

## 3. Baseline truth

The previous overallStatus=PASS is retracted. The accepted audit baseline is:

- PASS (7): F0.1, F0.4, C1.4, C1.5, R5.1, R5.4, R5.6
- FAIL (11): F0.7, F0.9, W2.2, A3.3, A3.7, P4.2, P4.4, P4.5, P4.6, P4.7, R5.3
- PARTIAL (30): every other slice
- Normalisation: any audit label "PARTIAL/UNVERIFIED" is PARTIAL.

Source reports (read-only baseline):
- build/audit/plan-gaps.md
- build/audit/runtime-gaps.md

## 4. Program slices

S0 truth reset + audit harness; S1 co-save correctness; S2 hook safety; S3 damage categories;
S4 fail-closed data/JSON-only; S5 StageEngine unification; S6 resolver/ratings wiring;
S7 item instance identity; S8 nine classes; S9 Prisma integration; S10 HUD indicators;
S11 menus/codex; S12 world integration; S13 compatibility; S14 regression/balance;
S15 release audit.

Dependencies: S0 before feature work; S1-S3 safety gates; S4-S7 foundations; S8-S13 consume
foundations; S14 cross-system; S15 release readiness. Splitting a slice is allowed only with
preserved requirements and independent review.

## 5. Non-negotiable constraints

- JSON-only authored content and runtime content; no CSV authoring.
- Stable content keys; no authored FormIDs except explicit master/unique references.
- Fail-closed content/data handling; no swallowed failures.
- Co-save migration and malformed-save safety.
- No publish, upload, remote push, or destructive external action without authorization.
- No false PASS, no aggregate PASS without audit artifacts, no deletion of failing criteria.
- User-side tests (full MO2, 40-hour playtest, Prisma visual host) are BLOCKED until evidence
  exists; they may not be claimed complete.
