# Third independent spec audit — S0 truth reset and audit harness (revision 3)

Auditor: teammate `environment-analyst` (fresh; no S0 implementation history; did not author S0,
PROGRAM.md or the first two audits). Date: 2026-10-28, Asia/Bangkok. Mode: read-only.
Subject: `build/slices/S0/spec.md` revision 3, 12695 B, SHA-256 `7E24899CA570CABE592211937936631CD6B8067E6BB6E006BC62B45A03CFABBF`, status
"DRAFT FOR THIRD INDEPENDENT SPEC AUDIT". Also read: `PROGRAM.md` (62 lines), `build/audit/S0-spec-audit.md`
(rev 2, 50,374 B, M-01..M-12 and B1-B10), `build/audit/S0-spec-audit-2.md` (R1-R5),
`EXECUTION-STATE.json` (48 slices, 371 checklist values), repo `.gitignore`, git status (HEAD a50b2b6;
only `?? Runefall/PROGRAM.md`).

## 1. Verdict

**REJECTED** — two blocking amendments. Revision 3 genuinely resolves all five second-audit findings
(R1-R5) and the material M-01..M-12 items: baseline hashes are pinned and measurable, the S0 record
exists as a separate remediation system, bootstrap is a scoped mode, schema v2/enums/evidence shape
and the 371-value normaliser are specified, and HISTORY carries actor/role/subjectCommit/prevHash/hash
plus the spec hash. The two blockers are new precision holes created by specifying future records and
the allowlist: the schema cannot validate a truthful state that contains S1-S15 records for slices that
have no contract/audit files yet, and A7's "exactly" allowlist is not exact, which makes the
no-source-scope gate both ambiguous and self-contradictory with section 6.4.

## 2. Independently re-measured facts (all match the spec)

| Claim | Measured | Spec |
|---|---|---|
| plan-gaps.md SHA-256 / size | 4117EC7C861635412F1B260CFD0E5B0208C1C2D4C70AEF4DA8828CF3FC0B3C17 / 21,576 B | 1.2 |
| runtime-gaps.md SHA-256 / size | 4AEE2879FE7090096306D4548721B233DF4AAB655E0AE5BD9FD85711CAA8AE6A / 20,484 B | 1.2 |
| Baseline multiset | PASS 7 IDs, FAIL 11 IDs, remainder 30; no overlap; all IDs present in state | 1.3 |
| slices{} count and ID set | exactly 48; ID set equals ULTIMATE-PLAN (no state-only / plan-only IDs) | 3.6 |
| Legacy checklist values | 371 entries, 116 distinct, 148 PASS-suffix, 16 explicit non-PASS | 3.8 / A2 |
| Overall current state | overallStatus=PASS, currentSlice=COMPLETE, no schemaVersion/remediation, release verified=true — correctly treated as invalid data to reset | 1.4 / 7.4 |

## 3. Resolution of R1-R5 (second audit)

| # | Required | Revision 3 | Verdict |
|---|---|---|---|
| R1 | S0 record in state; new remediation object S0-S15 | Section 3.4/4.1: `remediation` object S0..S15 with contract/specAudit/implementationAudit/commit/evidence/blockers/followups; `slices{}` stays the 48 plan IDs; `currentSlice` refers to remediation | RESOLVED |
| R2 | Bootstrap `-Bootstrap` mode | 4.2 fixes the predicate (state in IMPLEMENTING/AUDIT_PENDING, audit=null, recordCommit=null, only before C2, records GATE-BOOTSTRAP); 5.2 forbids requiring audits for baseline/NOT_STARTED slices; 5.4 keeps audit-missing rejection for claimed PASS | RESOLVED (one wording residue, N2) |
| R3 | schemaVersion 2, enums, evidence shape, 371-value normaliser | 3.1 JSON Schema 2020-12 schemaVersion=2 additionalProperties=false; 3.3/3.5 enums; 3.9 evidence {path,sha256,kind,exists}; 3.8 explicit normalisation rules + normalize_legacy.py and its output hash; A2 requires the map | RESOLVED (N3) |
| R4 | pinned exact SHA-256s | 1.2 pins both report hashes and sizes; re-measured equal (section 2) | RESOLVED |
| R5 | HISTORY actor/role/subjectCommit/prevHash/hash + spec hash | 3.10 full line schema incl. prevHash/hash and artifacts[{path,sha256}]; 10.2 spec approval records spec SHA-256; 5.4 E10 + hash-mismatch rejection | RESOLVED |

## 4. Material M-01..M-12 (first audit)

| # | Item | Revision 3 | Status |
|---|---|---|---|
| M-01 | commit/number/cite PROGRAM.md | 1.1, 6.3 commit PROGRAM.md in C1 | PARTIAL — 1.1 says "numbered sections 1-8"; actual PROGRAM.md has 5 unnumbered headings (Mission, Mandatory roles, Baseline truth, Program slices, Non-negotiable constraints) (N1) |
| M-02 | tracking decision + exact negation block | 6.1 exact 11-line block; 6.2 check-ignore/add -n/ls-files verification; 6.4 fallback | RESOLVED (N5) |
| M-03 | model both slice systems | 2 in-scope, 3.4, 4.1 | RESOLVED |
| M-04 | A4 satisfiable / scoped predicates | 4.2/5.2/5.3/5.4 + staleAuditedCommit 4.4 | RESOLVED (N2) |
| M-05 | falsifiable schema | 3.1/3.8/3.9 + A2 | RESOLVED (N3) |
| M-06 | pin baseline | 1.2 | RESOLVED |
| M-07 | unit of A5 | 8.1/8.2: 48 rows + acceptanceClauses[] + status multiset equality | RESOLVED |
| M-08 | real claim test | 7.3 forbidden current claims + retraction; A6 grep/claim inventory | RESOLVED |
| M-09 | A7 command + allowlist + baseline commit | A7 uses a50b2b6..HEAD, git status -uall, allowlist | PARTIAL — allowlist contains fuzzy "state/docs" and omits DECISIONS.md needed by 6.4 (B-2) |
| M-10 | A8 artifacts + pre-audit split | 10.2/10.3, 3.10, A8 (audit placeholder null, no file) | RESOLVED |
| M-11 | fixture matrix | 5.6: positive control + one negative per E01-E10 + forged-PASS + missing + empty = 13 negatives | RESOLVED |
| M-12 | evidence integrity | 3.9 + 5.4 | RESOLVED |

## 5. Blocking findings

**B-1 — Remediation schema, nullability and bootstrap scope are inconsistent for future slices.**
Evidence: 3.4 makes `contract{path,sha256}` and `specAudit{path,sha256,verdict,recordCommit}` required
on every S0..S15 record; 3.1 requires `additionalProperties=false`; 3.7 grants nullability only to
`commit`, `implementationAudit`, `recordCommit`. S1-S15 specs and audits do not exist yet, so either
the state must carry placeholder hashes for absent files (5.4 rejects "contract hash mismatch" and A2
requires a valid schema) or A2 cannot pass; additionally 4.2 says "exactly one missing
implementationAudit", which reads globally although 15 future slices have none (5.2 says they are not
required, but 4.2 does not scope the predicate to `currentSlice`). Fix (minimum): in 3.4/3.7 mark
`contract`, `contract.sha256`, `specAudit` nullable/null until the slice reaches SPEC_AUDIT; in 4.2
scope the "exactly one missing audit" predicate to `currentSlice`; add to 5.2 "gate verifies contract
and spec hashes only for non-null records whose state is at or beyond SPEC_AUDIT".

**B-2 — A7's "allowlist exactly" is not exact and contradicts 6.4/7.1.**
Evidence: A7 allows "state/docs" (undefined file set) and omits `DECISIONS.md`, which 6.4 requires for
the fallback decision and 11 implies for open decisions; 7.1 names `EXECUTION-EVIDENCE.json`, which is
not in A7; 2 says "docs truth reset" without a file list. As written, implementing 6.4 either violates
A7 or is skipped, and an independent scope audit cannot reproduce the allowlist. Fix (minimum): replace
"state/docs" in A7 with the exact paths: `EXECUTION-STATE.json`, `EXECUTION-EVIDENCE.json`,
`STATE.md`, `RELEASE-READINESS.md`, `VALIDATION.md`, `CHANGELOG.txt`, `DECISIONS.md` (plus already
listed `.gitignore`, `PROGRAM.md`, `build/slices/**`, `build/audit/**`, `build/evidence/S0/**`,
`build/TRACEABILITY.json`, `build/state.schema.json`, `tools/audit-gate.ps1`, `tools/audit-validators/**`).

## 6. Nonblocking findings

- N1 Section 1.1 cites "PROGRAM.md sections 1-8"; the file has five unnumbered headings. Fix the citation
  or number the headings.
- N2 Section 4.2 "a missing audit is rejected after the spec-audit stage" is still stage-scoped, not
  slice-scoped; align with 5.2 (see B-1).
- N3 Section 3.8's explicit token list does not cover the real variants in the 371 values
  ("NOT RUN (user-side)", "NOT VERIFIED (no Prisma host screenshot)", "DEFERRED R6.1",
  "PASS (schema1 observed)", "PASS 1939 bytes observed"). State the matching rule (case-insensitive
  prefix match; unknown -> FAIL) and prove it with the normaliser fixture; the 371/116/148/16 counts
  themselves are verified.
- N4 Section 10.1 names spec audits `S0-spec-audit-rN.md`; the accepted namespace on disk is
  `S0-spec-audit.md`, `S0-spec-audit-2.md`, third `S0-spec-audit-3.md`. Align the pattern.
- N5 Durability block: repo `.gitignore` already has `build/` (line 42), `Runefall/build/` (line 75)
  and an evidence negation. The 6.1 appended recipe (negate directory, ignore children, negate each
  subdirectory) is the standard order and should work, but A8 must be executed before C1 and 6.4's
  relocation fallback kept; record the actual `check-ignore` output as evidence.
- N6 Section 2 says "fixtures, validators" while A7 names `tools/audit-validators/**` and 3.8 homes the
  normaliser under `build/audit/`; confirm the final paths are mutually consistent.
- N7 Baseline provenance: plan-gaps.md (48 plan rows) and runtime-gaps.md (A3.1-A3.7, P4.1-P4.7,
  R5.1-R5.6, R6.1-R6.4 plus native) overlap for 20 slices; 3.6 has one `baselineAudit.report` per slice
  with no precedence/merge rule. Statuses are fixed by 1.3, so this is provenance-only; add one
  sentence ("runtime-gaps is authoritative where it reports; plan-gaps otherwise") or make `report`
  an array.

## 7. Minimum amendments for approval

1. B-1: future-record nullability + scoped bootstrap/verification (3.4, 3.7, 4.2, 5.2).
2. B-2: exact A7 allowlist including DECISIONS.md and EXECUTION-EVIDENCE.json.
3. Nonblocking, same revision if cheap: N1 citation, N2 wording, N3 normaliser variants, N4 audit
   filename pattern, N7 baseline precedence.

Everything else in revision 3 is buildable and falsifiable as written. No other blocking issue found:
baseline hashes and sizes match; the 48/7/30/11 mapping is exact; the 371-value normaliser scope is
real; fixtures, reason codes, gate modes, HISTORY chain, traceability, docs reset and the no-source
scope are otherwise coherent.

## 8. Auditor statement

Only this file was written in the repository; a throwaway counting script lived in %TEMP% and is not
part of the project. No code, data, state, gate, fixture or git change; no builds, generators, gate
runs or game launches. Commands used: file reads, Get-FileHash, git status, and a read-only Python
count over EXECUTION-STATE.json/ULTIMATE-PLAN.md. Independence: I am not the S0 implementer and did not
author the spec, PROGRAM.md, or either earlier audit.
