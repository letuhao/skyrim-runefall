# Second independent spec audit — S0 truth reset and audit harness

Auditor: teammate `esp-explorer` (fresh; no S0 implementation history; did not author the spec,
PROGRAM.md or the first audit). Date: 2026-10-28, Asia/Bangkok. Mode: read-only.
Subject: `build/slices/S0/spec.md`, 8,289 B, SHA-256 `3630890d...aada5f9c892` (amended after the
first rejection; `S0-spec-audit.md` rev.2 reviewed the 5,064 B draft). Also read: `PROGRAM.md`
(2,871 B, untracked), `S0-spec-audit.md` (720 lines, first rejection), `EXECUTION-STATE.json`
(48 slices all PASS, overallStatus PASS, updatedAt 2026-10-28T04:45:49Z, no `schemaVersion` or
`remediation` key), `.gitignore`, git state (HEAD `a50b2b6`, clean except `?? Runefall/PROGRAM.md`).

## 1. Verdict

**REJECTED.** The four headline defects named in the handoff are materially addressed (section 2)
and the 7/30/11 baseline is now exactly reproducible. The contract is still not satisfiable and not
fully falsifiable: the audited object (S0) has no record in the state it governs, the pre-audit
bootstrap remains self-contradictory, and the state-schema v2 requirement lacks the
version/migration/evidence shape needed to validate the existing 371 checklist values.

## 2. The four handoff findings

| finding | amended spec | verdict |
|---|---|---|
| B1 gate truthfulness vs release unsatisfiable | `Gate semantics` (:51-58) defines `-Mode truthfulness|release`; truthfulness PASSes a truthful but incomplete program, release rejects; A4 (:37) aligned | RESOLVED at mode level; one contradiction remains (R2) |
| B2 audit-after-commit impossible | `Audit commit semantics` (:43-49): slice.commit=C1, audit.auditedCommit=C1, audit.recordCommit=C2 descendant, C3 optional; gate checks C1/C2 ancestors of HEAD | RESOLVED |
| B3 PASS_* false-pass hole | Allowed tokens: exactly PASS or PARTIAL/FAIL/BLOCKED/NOT_RUN/NOT_VERIFIED/DEFERRED/N/A; any PASS_* rejected (:56, A4) | RESOLVED for gate tokens; not for state migration (R3) |
| D1 gitignored artifacts | In scope (:15) amend .gitignore; A8 (:41) requires unignoring the exact S0 paths, check-ignore/ls-files/add -n durability, committed artifacts; A7 allowlist covers .gitignore | RESOLVED at contract level; exact path list/negation block unstated and PROGRAM.md is not named as a required tracked artifact |

Also verified: baseline 7/30/11 (:25-28) - PASS list has exactly 7 IDs, FAIL list exactly 11 IDs
matching the first audit's section 2.2, C1.2 -> PARTIAL normalisation stated; docs reset A6 (:39)
names the five real files and appends rather than rewrites; no-rebuild requirement present (:19,
:37, :57); nullability mentioned in A3 (:36).

## 3. Remaining blocking findings

| # | Blocking finding | Evidence |
|---|---|---|
| R1 | S0 has no record in the state. A3 (:36) requires per-slice commit/audit/nullability fields but `slices{}` is the 48 plan IDs; the S0 lifecycle implied by A4/A8 (`state audit=null`, workflow steps 2-4) has no defined object, so "Lead updates state to PASS" (step 4, :75) is unaddressable. | first audit B3/M-03, unaddressed; state has no `remediation`/`schemaVersion` key |
| R2 | Bootstrap contradiction. A4 (:37) still lists "audit missing" as a gate rejection while A4 also requires truthfulness to accept "before/after S0 audit" and A8 (:41) fixes `state audit=null` with no placeholder file. The same state is both rejectable and required to pass; no explicit pre-audit mode or scoped predicate. | A4 criterion text + A8 |
| R3 | `state.schema.json` is underspecified. A3 names required fields/enums/nullability but not `schemaVersion: 2`, `additionalProperties:false`, exactly-48-plus-remediation, evidence object shape `{path,sha256,kind,exists}`, or a normalisation/migration map for the current 371 checklist values (116 distinct, incl. 148 `PASS <suffix>` and 16 explicit non-PASS). Schema v2 therefore cannot validate the existing state and A3's "valid state passes" is not constructible. | A3 (:36); first audit B5/M-05 |
| R4 | Baseline hashes not pinned. :25-28 fixes counts and IDs and A1 (:34) requires recomputation, but the spec embeds no expected hashes, so the check is self-attested. Re-measured: plan-gaps.md 21,576 B `4117ec7c...fb3c17`; runtime-gaps.md 20,484 B `4aee2879...8ae6a`. | M-06 open |
| R5 | Audit integrity/independence still unverifiable. No spec.md/audit-file SHA-256 in HISTORY (A3 only id/timestamp/event), no actor/role fields, no prevHash chain, no requirement to commit PROGRAM.md or record its hash; spec freeze (first audit N-7) absent. | B10/M-10 partially open |

## 4. Minimum amendments for approval

1. Add a top-level `remediation` (S0) object to state and schema with contract/specAudit/
   implementationAudit/commit/evidence/blockers/followups; state `slices{}` exactly the 48 IDs;
   reconcile A2's overallStatus enum with workflow step 4's "state to PASS".
2. Define the bootstrap explicitly: pre-audit predicate or `-Bootstrap`/`-ExpectMissingAudit` mode
   (legal only while `audit=null` and no recordCommit exists); scope A4's "audit missing" rejection
   to post-audit states; define `staleAuditedCommit` mechanically.
3. Extend A3/schema: `schemaVersion=2` required; enums for overallStatus and slice status; exactly
   48 plus remediation; evidence `{path,sha256,kind,exists}`; `additionalProperties:false`; legacy
   normalisation map for the 371 checklist values (PASS suffix -> PASS + detail; PARTIAL/UNVERIFIED ->
   PARTIAL; explicit non-PASS tokens -> detail).
4. Embed the expected SHA-256 of both baseline reports in the spec; require PROGRAM.md and spec.md
   committed in C1 with their hashes recorded.
5. HISTORY schema: actor, role, subjectCommit, verdict, artifacts[{path,sha256}], prevHash/hash;
   specify who records the audit-file hash.

Nonblocking: enumerate the exact .gitignore negation block and tracked paths (first audit Appendix B
pattern); use `git status --porcelain -uall` for A7; A6 claim inventory/forbidden-pattern list and
one STATUS line; A5 plan SHA-256 plus verbatim acceptance text plus status-multiset equality;
GATE-E reason codes; fixture matrix path; declare `build/execution/state.json` superseded;
grandfather the one missing baseline evidence ref.

## 5. Auditor statement

Only this file was written. An earlier append of this section to `S0-spec-audit.md` was reverted
(unintended path collision with the owning auditor): only the appended section was removed from that
file; its pre-existing content was left as found. No code,
data, state, test or git changes; no builds, generators, gate runs or game launches. Commands: file
reads, Get-FileHash, git rev-parse/status/ls-files/check-ignore (read-only). Limits:
`tools/audit-gate.ps1` does not exist yet, so A2/A4/A8 were assessed for implementability, not
executed; independence: I am not the S0 implementer and did not author the spec, PROGRAM.md or the
first audit revision.
