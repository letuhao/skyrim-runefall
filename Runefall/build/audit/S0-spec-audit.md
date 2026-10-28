# S0 specification audit — independent review (revision 2)

Auditor: delegated specification auditor (fresh; no S0 implementation history).
Date: 2026-10-28, Asia/Bangkok. Mode: read-only.
Subject: `build/slices/S0/spec.md` — DRAFT FOR INDEPENDENT SPEC AUDIT, 5,064 bytes,
SHA-256 `d6b0e8cdc5e62b2b6f30431057ae48f486410a8e48146412b81e52174d024810` (as read at 15:16 +07).
Task: audit completeness, testability, consistency and scope of the S0 contract. No remediation plan.

Provenance: an earlier audit existed at this path (author `tool-inventory`, mtime
2026-10-28T08:28:49Z, also REJECTED). It was read before this write and is superseded by this
revision; every finding below is independently re-derived from the sources, not copied.
Snapshot note: the repository changed during this audit. `Runefall/PROGRAM.md` appeared at
2026-10-28T08:30:20Z (untracked, `git status` = `?? Runefall/PROGRAM.md`); it is assessed in §2.1,
§2.3 and §4 (X1). Spec findings are against the spec hash above.

## 1. Verdict

**REJECTED.**

The S0 intent is correct and well-chosen: retract an unsupported 48/48 PASS, make status
gate-derived, and force a three-role audit trail before feature work. The scope exclusion
("no gameplay/source/data changes") and the decision not to rebuild the DLL are right, and the
7/30/11 baseline arithmetic is exactly reproducible (see §2.2). The contract is rejected because
it is not yet a contract: four criteria are structurally unsatisfiable as scoped (A2, A4, A8, and
the S0-record half of A3), two are unfalsifiable (A3, A5), two are mistargeted or bypassable
(A6, A7), and one is self-attesting (A1). Ten distinct defects are blocking:

| # | Blocking defect | Criteria |
|---|---|---|
| B1 | Normative source: criteria cite "goal §N"; no numbered goal document existed at spec freeze, and the `PROGRAM.md` that appeared mid-audit is untracked, unnumbered and uncited | all |
| B2 | Every required S0 artifact is gitignored and has never been tracked — including this audit file | A1, A3, A4, A8 |
| B3 | S0 has no record in the 48-slice state, yet A3 requires per-slice audit/commit fields for the current slice | A3, and counts in A1/A5 |
| B4 | A4's rejection rules ("non-PASS slice", "missing slice audit") make the gate unable to PASS on any truthful state; 41 slices are non-PASS and 0 slice audits exist | A2, A4 |
| B5 | State schema v2 has no field types, enums, version field, or migration rule for 371 free-text checklist values | A3, A4 |
| B6 | A6 forbids strings that do not occur in the docs it names, while the real claims (VALIDATION.md, STATE.md, EXECUTION-EVIDENCE.json, CHANGELOG.txt:17) are outside its scope | A6 |
| B7 | A5 conflates 48 plan rows with ~108 acceptance clauses; "no requirement omitted" has no mechanical oracle | A5 |
| B8 | A7's `git diff --name-only` cannot see untracked or ignored files; baseline commit unnamed; required `.gitignore` edit is outside its allowlist | A7 |
| B9 | A8 requires `S0-audit.md` to exist before the audit that produces it, and the auditor must run the gate that requires that same file | A8 |
| B10 | No integrity/independence mechanism: spec not frozen, audits not hashed, no actor/role record, no history schema | A3, A8 |

During this audit `Runefall/PROGRAM.md` appeared (mtime 2026-10-28T08:30:20Z, 2,871 B, untracked).
It supplies the baseline truth with the C1.2 normalisation (:30-39), role separation and gate order
(:14-26), remediation slices S0-S15 (:43-47) and constraints (:53-62). It narrows B1 but does not
close it: the spec cites numbered "goal" sections that `PROGRAM.md` does not have, the file is not
committed, and the spec does not name it. B3 likewise gains precision: the program has its own
slice set (S0-S15), separate from the 48 plan slices.

Minimum amendments for approval: §9 (M-01 … M-12). All are contract/spec text changes plus one
repository tracking decision; none requires gameplay work.

## 2. Facts established by this audit

### 2.1 Inputs read (all read-only)

`build/slices/S0/spec.md`; `SPECS/ULTIMATE-PLAN.md`; `SPECS/` (directory inventory: 30 files, no
goal/program document); `build/audit/plan-gaps.md`; `build/audit/runtime-gaps.md`;
`EXECUTION-STATE.json` (all 48 records + top-level); `EXECUTION-EVIDENCE.json` (147 entries);
`STATE.md`; `VALIDATION.md` (120 headings); `RELEASE-READINESS.md`; `CHANGELOG.txt` (head);
`build/execution/state.json`; `build/execution/evidence-index.json`; `tools/run-static-gates.ps1`;
`tools/test-content-pipeline.ps1`; `tools/run-host-tests.ps1`; `.gitignore`; `fomod/info.xml`;
`CURRENT.txt`; `Runefall/PROGRAM.md` (appeared 08:30:20Z, read after the spec); git queries at HEAD
`a50b2b6`.

Baseline report hashes measured now:

- `build/audit/plan-gaps.md` — 21,576 B — `4117ec7c861635412f1b260cfd0e5b0208c1c2d4c70aef4da8828cf3fc0b3c17`
- `build/audit/runtime-gaps.md` — 20,484 B — `4aee2879fe7090096306d4548721b233df4aab655e0ae5bd9fd85711caa8ae6a`

Neither is pinned in `spec.md`; neither is tracked by git.

### 2.2 Baseline arithmetic (A1 is factually well-founded)

Verdicts parsed from the two per-slice tables: **7 PASS, 29 PARTIAL, 11 FAIL, 1
"PARTIAL/UNVERIFIED" (C1.2)** = 48. The spec's 7/30/11 is correct **only if C1.2 is normalised to
PARTIAL**. That rule is not stated anywhere. FAIL IDs, not enumerated in the spec:
F0.7, F0.9, W2.2, A3.3, A3.7, P4.2, P4.4, P4.5, P4.6, P4.7, R5.3.
PASS IDs: F0.1, F0.4, C1.4, C1.5 (plan-gaps); R5.1, R5.4, R5.6 (runtime-gaps).
`PROGRAM.md:32-35` states this exact split and the normalisation, which confirms the arithmetic but
does not make `spec.md` self-contained.

### 2.3 State, docs and repository facts

- `EXECUTION-STATE.json`: 48/48 `status: PASS`, `overallStatus: PASS` (:4), `currentSlice: COMPLETE`
  (:5), `release.verified: true` / `published: false`, 142 evidence refs (1 missing on disk:
  `J:/SkyrimTest/1.6.1170/Data/Runefall/runtime`), 371 checklist entries with **116 distinct values**:
  207 exactly `PASS`, 148 `PASS <suffix>`, 16 explicit non-PASS tokens (`DEFERRED …` 4, `NOT
  VERIFIED …` 3, `NOT RUN …` 2, `PENDING C1.8`, `DATA false pending anomaly module`, `data seam
  false`, `session-scoped (co-save deferred)`, `future-only by design …`, `NOT PUBLISHED …`,
  `1939 bytes observed`).
- No `schemaVersion` field exists; slice keys are `id, layer, slice, status, acceptance, depends,
  evidence, notes, checklist` — none of `state/contract/specAudit/implementationAudit/commit/
  blockers/followups` required by A3.
- A second, contradictory state exists: `build/execution/state.json` (`overallStatus:
  IN_PROGRESS`, `currentSlice: F0.2`, all other slices `PENDING`, 2026-10-27T22:11Z), plus
  `build/execution/evidence-index.json` (5 entries, F0.1 only). The spec does not say which file is
  authoritative.
- `Runefall/PROGRAM.md` (2,871 B, mtime 2026-10-28T08:30:20Z) is **untracked** (`?? Runefall/PROGRAM.md`).
  It names baseline commit `a50b2b6` (:5), the 7/11/30 baseline and C1.2 normalisation (:30-39), the
  roles and gate order (:14-26), the remediation slice set S0-S15 (:43-47) and the non-negotiable
  constraints (:53-62). It has no §1-§12 numbering and `spec.md` never cites it.
- `EXECUTION-EVIDENCE.json`: 147 entries, **142 `result: PASS`, 5 `PARTIAL`** — a second false
  claim registry that A6 does not touch.
- `STATE.md`: 31 `PASS` mentions, including "L0 foundation complete" (:97), "L1 combat core
  C1.1-C1.8 complete" (:106), "L2 world complete. Next: A3.1" (:113); last-updated 2026-10-27 (:3);
  UI framework still listed as a blocking decision (:137).
- `VALIDATION.md`: 120 headings, **40 containing PASS**, 142 lines containing `PASS` (e.g. "R5.5
  endgame progression - PASS", "R6.4 release gate - PASS"), plus 8 NOT VERIFIED/UNTESTED lines.
- `CHANGELOG.txt:17`: "0.1.0 - 2026-10-27 (first public release …)", contradicted by
  `RELEASE-READINESS.md:3` ("Internal candidate only") and by the same file's addendum (:4, :15).
- Literal `48/48` occurs in exactly three files: `build/audit/runtime-gaps.md:7`, this spec's
  A5 row, and the prior audit copy. **It does not occur in STATE.md, RELEASE-READINESS.md or
  CHANGELOG.txt**, the three files A6 names.
- Git: branch `main`, HEAD `a50b2b6`, `git status` clean. `.gitignore:75` = `Runefall/build/`;
  `:56` = `*.log`; `:87-88` re-include only `Runefall/evidence/**`. `git ls-files Runefall/build` is
  empty; 1,278 files exist under `build/`; `git log -- Runefall/build` is empty. `git
  check-ignore` confirms `build/audit/*`, `build/slices/*`, `build/evidence/*`,
  `build/TRACEABILITY.json` and `build/audit/HISTORY.jsonl` are ignored.
- `tools/audit-gate.ps1`, `build/TRACEABILITY.json`, `build/audit/HISTORY.jsonl`,
  `build/evidence/S0/` do not exist yet (as expected).
- Environment: PowerShell 7.6.9, Pester 3.4.0, Python 3.13.12 with `jsonschema` 4.26.0, Node 25.9.0,
  git 2.49.0. All machine paths used by `tools/run-static-gates.ps1:4-6` exist here
  (`C:\Users\NeneScarlet\.cache\build-runefall.bat`, `E:\…\PapyrusCompiler.exe`,
  `VS2022 Enterprise`, `C:\Program Files\dotnet\dotnet.exe`), but the first two are outside the repo.

## 3. Criterion-by-criterion findings

| ID | Contract verdict | Blocking? |
|---|---|---|
| S0-A1 | Factually correct baseline, but not falsifiable as written (self-attested hash; undeclared C1.2 normalisation; FAIL IDs absent; source blobs untracked) | Yes |
| S0-A2 | Sound idea, unsatisfiable scope: the "mutate a slice to PASS" test requires audit records that do not exist for any slice | Yes |
| S0-A3 | Unfalsifiable: no schema, no enums, no nullability, no version field, no migration map, and no record for S0 itself | Yes |
| S0-A4 | Unsatisfiable as scoped and internally circular (global non-PASS rejection vs expected PASS; clean-tree vs pre-commit evidence) | Yes |
| S0-A5 | Ambiguous unit of "requirement" (48 rows vs ~108 clauses); omission cannot be detected without verbatim text/hash | Yes |
| S0-A6 | Mistargeted literals and incomplete scope; passes by grep while the false claims remain | Yes |
| S0-A7 | Bypassable command (`git diff` misses untracked/ignored), unnamed baseline, incomplete allowlist | Yes |
| S0-A8 | Circular (requires the audit it is audited by), undefined placeholder policy, history and command-log formats | Yes |

### S0-A1 — Baseline reconciliation (spec.md:31)

*Sound:* the expected multiset is achievable and the criterion correctly forbids retaining PASS
for baseline FAIL/PARTIAL slices. Recomputing from the report tables gives exactly 7/30/11 once
C1.2 is normalised (§2.2).

*Blocking defects:*

1. **Normalisation is undeclared in the spec.** A literal parser of the tables sees 7/29/11 + one
   `PARTIAL/UNVERIFIED`. A faithful gate would report a mismatch; an implementer would then either
   edit the frozen baseline or hardcode the expected string. `PROGRAM.md:35` now states
   `PARTIAL/UNVERIFIED → PARTIAL`, but `spec.md` neither cites it nor repeats it, so the criterion
   still has no in-spec rule and no guaranteed source.
2. **The hash check is self-attesting.** The spec says the baseline "path and SHA-256 [are]
   recorded" but never pins an expected value. The implementer records the hash of the file they
   just read, and the gate recomputes the same hash — a tautology. Pin both digests in `spec.md`
   (§2.1) and require the gate to compare against the pinned constants.
3. **"Known FAIL IDs" are inputs but are not listed.** The verification cell says the script
   "compares status multiset and known FAIL IDs"; the 11 IDs appear nowhere in the spec.
4. **The baseline is not durable.** Both reports are ignored and untracked (§6); a hash of an
   uncommitted working-tree file anchors nothing. A clean clone has no baseline at all.

*Required amendment:* M-06 (pin digests + 7/30/11 table + FAIL IDs + normalisation), M-02
(tracking).

### S0-A2 — Truthful aggregate (spec.md:32)

*Sound:* deriving the aggregate and rejecting a hand-edited PASS is the correct remedy; a
mutate-to-PASS fixture is the right test shape.

*Blocking defects:*

1. **No derivation rule.** "Derived status" is undefined: worst-of? count-based? blocked if any
   blocker? The aggregate vocabulary is also undefined and collides with slice vocabulary — `PASS`
   is used for both `overallStatus` and `status` today (the root cause of audit finding C1 in
   `runtime-gaps.md`). A gate can implement "derived" in mutually exclusive ways and still claim
   compliance.
2. **The negative test has no oracle.** "Mutate a slice to PASS and confirm the gate rejects"
   presupposes that the gate knows the slice's true verdict. That knowledge can only come from a
   per-slice audit record, and none exists for any of the 48 slices (`build/audit/` contains two
   aggregate reports only). The only implementable alternative — an allowlist of the 7 PASS IDs —
   is a hardcoded exception, not a derived status, and would also reject the 7 legitimate PASS
   slices.
3. **Manual-PASS prevention is unspecified.** Removing `overallStatus` from the file and computing
   it in a separate verdict artifact is materially different from keeping the field and having the
   gate ignore it; the spec must pick one.
4. **The `release` block escapes A2.** `EXECUTION-STATE.json` also asserts `release.verified:
   true` and `currentWork.slice: COMPLETE`; A2 speaks only of `overallStatus`.

*Required amendment:* M-04 (aggregate enum + derivation + PASS semantics + grandfathering).

### S0-A3 — State schema v2 (spec.md:33)

*Sound:* the field list captures the right concepts and "append-only history" is the right
property.

*Blocking defects:*

1. **No schema artifact and no types.** `program`, `baseline`, `state`, `contract`,
   `specAudit`, `implementationAudit`, `commit`, `blockers`, `followups` have no types, formats or
   allowed values. "All required fields present; invalid/missing fields rejected" therefore has no
   oracle: any file with the keys present passes, and nothing defines "invalid".
2. **No `schemaVersion`.** The criterion is titled "schema v2" but does not require a version field,
   so a v1 file and a v2 file are indistinguishable to the gate.
3. **No normalisation path for 371 legacy values.** The current checklist holds 116 distinct
   free-text values, including 16 explicit deferral/unverified/observation tokens. A4 requires
   rejecting `NOT VERIFIED / NOT RUN / DEFERRED`; a substring rule would also reject legitimate
   detail strings, and a strict enum would reject all 371 values unless a migration map is defined.
   Neither is specified.
4. **No nullability/model for unexecuted slices.** `commit`, `specAudit` and `implementationAudit`
   cannot exist for slices never executed under the new workflow. The schema must model
   "baseline-audited, not re-executed" as a first-class state, or the 41 non-PASS slices are
   schema-invalid.
5. **S0 itself has nowhere to live.** `slices{}` contains exactly the 48 ULTIMATE-PLAN IDs. A3
   requires the current slice's audit/commit/evidence; A1 and A5 require the count to stay 48.
   Adding S0 as a 49th record breaks both counts; omitting it leaves A8's S0 audit artifacts
   unmodelled. This is a hard contradiction, not a formatting gap.

*Required amendment:* M-03 (S0 remediation record), M-05 (schema + enums + migration).

### S0-A4 — audit-gate.ps1 rejection rules (spec.md:34)

*Sound:* the seven conditions are the right risk list, and a nonzero exit on each is testable.

*Blocking defects:*

1. **Global rejection makes PASS impossible.** "exits non-zero on … non-PASS slice [and] missing
   slice audit" applied to all 48 slices can never pass: 41 slices are non-PASS and zero slice
   audits exist. The criterion simultaneously expects "Gate PASS on valid audited state" — the two
   statements are incompatible. The gate needs scoped predicates: PASS *requires* a matching
   PASS audit; non-PASS is legal and simply keeps the aggregate IN_PROGRESS.
2. **Clean-tree vs evidence ordering is circular.** The gate must reject a "stale/dirty working
   tree", but S0's evidence (gate logs, this audit, the spec) is created in the working tree
   *before* the commit that would make it clean. Today the ignored `build/` tree hides 1,278 files
   from `git status`, so the check passes while the artifacts are untracked; after the tracking fix
   (§6) the same check fails on S0's own untracked outputs. Either way the predicate is wrong.
   Define dirty as tracked modifications only (`git status --porcelain --untracked-files=no`), or
   exclude declared S0 artifacts.
3. **"stale" is undefined.** No threshold, no reference time, no relationship to HEAD.
4. **"audit commit != slice commit" is ambiguous and probably unsatisfiable.** If "audit commit"
   means the commit that adds the audit file, it can never equal the commit being audited (a commit
   cannot contain its own audit). If it means the subject commit recorded by the auditor, the
   wording must say so. As written the gate can be implemented with opposite outcomes.
5. **No bootstrap rule.** The workflow (spec.md:45) has the implementation auditor run the gate and
   write `S0-audit.md`. The gate rejects a missing slice audit, so it must fail until the auditor
   writes the file; once written, the gate result no longer precedes the audit it validates.
6. **"criterion marked NOT VERIFIED/NOT RUN/DEFERRED" has no home.** A1-A8 are not represented in
   any state record, and checklist keys are slice-specific free text. There is no defined location
   for a criterion status.
7. **Reason specificity is unenforceable without codes.** "rejected with specific reason" has no
   vocabulary; fixtures cannot assert a message string robustly. Stable codes (GATE-E01…E09) are
   needed.
8. **Harness contamination risk.** If the gate invokes `tools/run-static-gates.ps1`, it inherits
   rebuilding the DLL and external paths (`run-static-gates.ps1:4-6`: `.cache/build-runefall.bat`,
   `E:\…\PapyrusCompiler.exe`), contradicting S0's "no rebuild" scope and clean-clone
   reproducibility.

*Required amendment:* M-04 (scoped rules, enums, dirty/stale, subject commit, bootstrap, codes,
no-rebuild), M-11 (fixtures incl. positive control).

### S0-A5 — Traceability registry (spec.md:35)

*Sound:* a requirement→test→evidence registry is the right artifact, and 48 is a checkable count.

*Blocking defects:*

1. **Unit ambiguity.** "Every original ULTIMATE-PLAN v2 slice requirement" vs "48/48 mapped". The
   plan has 48 rows but each Acceptance cell holds several clauses (F0.1 has ~6 under a `;`/`,`
   split; ~108 fragments across the table). Mapping 48 rows does not prove "no requirement
   omitted"; mapping 108 clauses breaks the stated count. Pick one unit and state it.
2. **No verbatim text or plan hash required.** A registry of 48 IDs with invented statuses passes
   a count and passes "counts reconcile". Omission and drift are undetectable unless each row
   carries the plan's acceptance text (or clause IDs) and the plan's SHA-256; the gate can then
   assert exact ID-set equality and hash equality.
3. **"Counts reconcile" has no target.** Presumably A1's 7/30/11, but the spec does not say the
   registry status multiset must equal the state's.
4. **"Audit baseline FAIL/PARTIAL remain visible" is not a field.** Which key carries it, and must
   it match the pinned baseline verdict per slice?

*Required amendment:* M-07.

### S0-A6 — Documentation truth reset (spec.md:36)

*Sound:* the goal — no unsupported PASS or publication claim — is exactly right, and naming the
three most visible documents is a reasonable start.

*Blocking defects:*

1. **The forbidden literals are strawmen.** `48/48` does not occur in `STATE.md`,
   `RELEASE-READINESS.md` or `CHANGELOG.txt` (verified by direct scan; the string exists only in
   `runtime-gaps.md:7` and in A5 of this spec). A grep gate would pass all three documents while
   they continue to assert per-slice PASS (`STATE.md` 31 mentions), "L0/L1/L2 complete", and
   "first public release" (`CHANGELOG.txt:17`).
2. **The largest claim registers are out of scope.** `VALIDATION.md` (40 PASS headings, 142 PASS
   lines, e.g. "R5.5 … PASS", "R6.4 release gate - PASS"), `EXECUTION-EVIDENCE.json` (142 `PASS`
   results out of 147), and `EXECUTION-STATE.json`'s `release.verified: true` would all survive.
   `docs/` and `MEMORY/` were not checked by the criterion at all.
3. **No allowed-claim rule.** "No unsupported PASS claim" is unbounded: a slice may legitimately
   be PASS (7 are), and `VALIDATION.md` legitimately reports command counts. Without a grammar —
   e.g. "PASS may appear only with the audit citation and the scope 'N/48 per the 2026-10-28
   audit'" — any doc either passes trivially or can never pass.
4. **No doc-consistency requirement.** The docs contradict each other today
   (`CHANGELOG.txt:17` public release vs `RELEASE-READINESS.md:3` internal candidate vs
   `STATE.md:3` stale date), and the FOMOD declares SKSE Menu Framework 3.18 (`fomod/info.xml:6`)
   while `ULTIMATE-PLAN.md:16` makes Prisma UI the hard dependency and `STATE.md:137` calls the UI
   framework an open blocking decision. A truth reset that leaves these unresolved is not a reset.
5. **"Limitations remain visible" is unspecified.** Which limitations, in which section, from which
   source (`RELEASE-READINESS.md:27-32` is the natural list) — otherwise the check is subjective.
6. **Harness caveat:** text search in this repo must not use ripgrep defaults; `rg` silently skips
   gitignored `build/` (observed: the grep tool returned 0 hits for `48/48` while a direct
   filesystem scan found 3 files). Any doc gate must use explicit file lists or `--no-ignore`.

*Required amendment:* M-08.

### S0-A7 — No gameplay/source/data changes (spec.md:37)

*Sound:* the exclusion itself is correct and the three protected paths are the right ones.

*Blocking defects:*

1. **The command misses the main failure mode.** `git diff --name-only` shows tracked
   modifications only. Adding a new file under `src/plugin` or `content/` (untracked) is invisible;
   changes under `build/dist`, `build/dist2`, `build/release` are invisible because the whole tree
   is ignored. The check must combine `git diff --name-only <base>..HEAD`,
   `git status --porcelain -uall`, and an explicit statement about ignored outputs.
2. **Baseline commit is not named.** "against baseline commit" — HEAD is `a50b2b6`; the two audit
   reports were written after it and are untracked, so the baseline for A7 must be pinned
   explicitly (and will change if M-02 adds a tracking commit).
3. **Allowlist incomplete and contradictory.** The only allowed artifacts are "state/docs/audit/
   gate", but S0 must change `.gitignore` to become durable (M-02), add fixtures, and possibly
   `VALIDATION.md`. None is in the allowlist. A gate built from A7 as written would flag the
   durability fix as a violation.
4. **Verification is manual-only.** "independent file inspection" has no artifact; the audit trail
   should record the exact command, its scope and the file list hash.

*Required amendment:* M-09.

### S0-A8 — Audit workflow artifacts and reproducibility (spec.md:38)

*Sound:* requiring spec audit, implementation audit, history, raw logs and exact commands is the
right shape for an auditable S0.

*Blocking defects:*

1. **Circular by construction.** A8 is an S0 acceptance criterion that requires
   `build/audit/S0-audit.md` to exist and to reference the exact commit. That file is produced by
   step 4 of the workflow, after implementation and after the gate run. S0 cannot satisfy A8 at
   the moment S0 is accepted for implementation, and the implementation auditor cannot run the
   gate (A4 rejects "missing slice audit") before writing the very audit that satisfies A8.
   `PROGRAM.md:16-26` fixes the roles and the audit verdicts but not this ordering.
2. **"Placeholder policy" is named but undefined.** What file, what content, when may it be empty,
   and who is allowed to author it?
3. **"Reproducible" is undefined and currently false.** From a clean clone, `run-static-gates.ps1`
   cannot run: it needs `C:\Users\NeneScarlet\.cache\build-runefall.bat` (outside the repo, ignored)
   and `E:\…\PapyrusCompiler.exe`. The spec must define reproducible as "from the committed tree
   using only committed files and documented tool paths", and the gate must not depend on those
   externals.
4. **History format unspecified.** `HISTORY.jsonl` has no required keys, no ordering rule, no
   integrity mechanism; "append-only" is a wish, not a checkable property. A file can be rewritten.
5. **Command-log format unspecified.** "commands and exit codes captured" does not say where
   (stdout vs file), in what structure, or that stderr and cwd are recorded.
6. **No actor/independence record.** Nothing records who audited, in what role, or that the
   auditor differs from the implementer; "fresh auditor" is undefined. `PROGRAM.md:17-26` now
   forbids self-audit and fixes the roles, but `spec.md` does not reference it and no artifact
   carries the actor id.
7. **Artifacts are untracked** (see §6), so "audit references exact commit" cannot be true: the
   audit file cannot be part of any commit as long as `build/` is ignored.

*Required amendment:* M-10 (split A8a/A8b, history schema, placeholder, reproducibility, actor),
M-02 (tracking).

## 4. Cross-document contradictions

| # | Spec / source A | Source B | Consequence | Blocking |
|---|---|---|---|---|
| X1 | Every criterion cites "goal §1…§12" (spec.md:31-38) | No numbered goal document existed at spec freeze; `PROGRAM.md` appeared during the audit (08:30:20Z) with baseline truth and roles but no §1-§12 numbering, and is untracked | Proposed criteria are still uncited; provenance cannot be audited until the spec cites a committed, numbered program document | Yes |
| X2 | S0 is a slice with `currentSlice`, specAudit, implementationAudit, commit (A3/A8) | `slices{}` is exactly the 48 plan IDs; `currentSlice` is `COMPLETE` (`EXECUTION-STATE.json:5`); `PROGRAM.md:43-47` defines a separate S0-S15 remediation set | S0 has no state record; adding it to `slices{}` breaks the 48-count checks in A1/A5, so two collections are needed | Yes |
| X3 | A4 rejects any non-PASS slice and missing audit | Baseline needs 41 non-PASS slices and has 0 slice audits | Gate can never PASS; "Gate PASS on valid audited state" is unsatisfiable | Yes |
| X4 | A4 requires a clean tree | S0's evidence is produced in the working tree before the commit; 1,278 files under `build/` are invisible to `git status` | Clean-tree check is either vacuous or self-defeating | Yes |
| X5 | A8 requires `S0-audit.md` before acceptance | `S0-audit.md` is produced only after S0 implementation, by the auditor who must first run the gate | Self-referential workflow; no legal ordering | Yes |
| X6 | A6 forbids "48/48 PASS" in three docs | The literal does not occur there; the claims live in `VALIDATION.md` (40 PASS headings), `STATE.md` (31 PASS mentions), `EXECUTION-EVIDENCE.json` (142 PASS) | A6 passes by grep while the false status remains | Yes |
| X7 | Purpose: "retract the invalid 48/48 PASS claim" | `EXECUTION-STATE.json` 48/48 PASS **and** `build/execution/state.json` IN_PROGRESS/F0.2 **and** `EXECUTION-EVIDENCE.json` 142 PASS coexist | Three competing truths; no declared authority | Yes |
| X8 | "Program: Runefall v0.2 remediation" (spec.md:4) | `CURRENT.txt` = "Runefall 0.1.0"; `release.version` = 0.1.0; version folders 0.1.x and 0.2.0-dev | Program identity/version ambiguous for the audit record | No |
| X9 | ULTIMATE-PLAN:16 "Prisma UI is the hard UI dependency" | `fomod/info.xml:6` requires SKSE Menu Framework 3.18; `STATE.md:137` lists UI framework as an open blocking decision | Truth reset would preserve a contradiction about the shipped dependency | No (must be recorded in A6 inventory) |
| X10 | S0 claims "no gameplay/source/data changes" | Required durability fix edits `.gitignore` (a config file), absent from scope and from A7's allowlist | Scope as written forbids its own enabling change | Yes (scope text) |
| X11 | A1 expects "baseline report path and SHA-256" | Baseline reports are untracked/ignored and were created 30 min before the spec | Baseline can change or vanish without detection | Yes |

## 5. Testability gaps — commands/checks that cannot prove the criterion

1. **`git diff --name-only` (A7)** — misses untracked additions and every ignored path; cannot
   prove "no gameplay/source/data changes".
2. **Hash-of-the-file-you-just-read (A1)** — a self-comparison, not an independent check, unless
   the expected digest is pinned in the spec.
3. **"Run gate on valid S0 state" (A4)** — no valid state can exist before the S0 audit exists
   (X5), and no state with 41 non-PASS slices can pass under the written rules (X3). The positive
   control is therefore unconstructible.
4. **"Mutate a slice to PASS" (A2)** — no per-slice audit oracle exists for any slice; the gate
   cannot distinguish "legitimate PASS" from "mutated PASS" without one.
5. **"Grep for forbidden claims" (A6)** — the literals do not occur; and ripgrep defaults skip
   `build/` (observed 0 hits vs 3 files by direct scan). Neither false negative nor false positive
   is controlled.
6. **"48/48 mapped; no requirement omitted" (A5)** — count equality proves row coverage, not
   requirement coverage; ~108 acceptance clauses are not enumerated anywhere.
7. **"All required fields present; invalid/missing fields rejected" (A3)** — no schema, so
   "invalid" has no definition and no fixture can be derived.
8. **"Rejected with specific reason" (A4)** — no reason codes; fixture assertions would depend on
   prose.
9. **"Reproducible" (A8)** — undefined (same machine? clean clone?); the existing static-gate
   harness is not reproducible from the repo (`run-static-gates.ps1:4-6`).
10. **"Stale/dirty tree" (A4)** — "stale" has no definition; "dirty" is ambiguous between tracked
    modifications and untracked artifacts, and the two readings give opposite results for S0.
11. **"Criterion marked NOT VERIFIED/NOT RUN/DEFERRED" (A4)** — no criterion registry exists; the
    371 checklist values are slice-specific free text with 116 distinct forms.
12. **"A8 artifacts exist"** — the artifacts are untracked, so existence today does not prove
    existence at the audited commit; and A8's own file is the audit.

## 6. Source control and durability (decisive structural defect)

### 6.1 Current state (verified)

- `.gitignore:75` ignores `Runefall/build/`; `:56` ignores `*.log`; only `Runefall/evidence/**` is
  re-included (`:87-88`). `Runefall/evidence/` holds 3 tracked Phase-0 logs.
- `git ls-files Runefall/build` → empty. `git log -- Runefall/build` → empty. 1,278 files exist under
  `build/`; 0 have ever been tracked.
- Ignored today: the S0 spec itself, both accepted baseline reports, this audit, the future
  `S0-audit.md`, `HISTORY.jsonl`, `TRACEABILITY.json`, and all `build/evidence/S0/*.log` files
  (double-ignored by the directory rule and `*.log`).
- Because git never descends into an excluded directory, adding `!Runefall/build/audit/` alone
  cannot work while `Runefall/build/` is ignored. I verified this and the working alternative in a
  throwaway repo outside the project (removed afterwards).

### 6.2 Consequence for the criteria

- **A1**: a SHA-256 of an untracked file is not a durable baseline; a clean clone has no reports.
- **A8**: "audit references the exact commit" is impossible — none of the audit artifacts can be in
  any commit.
- **A4**: "audit commit != slice commit" and "missing evidence file" cannot be evaluated against a
  committed state.
- **A3**: the per-slice `commit`/audit fields would record subjects whose audit artifacts are
  outside version control.
- This audit file, the deliverable I was asked to produce, is itself ignored and therefore cannot
  be preserved by the repository. That alone blocks approval.

### 6.3 Required decision (either is acceptable; the spec must choose one)

**Option A — narrow re-inclusion (verified working).** Replace `.gitignore:75` and add, in this
order:

```gitignore
Runefall/build/*
!Runefall/build/audit/
!Runefall/build/audit/**
!Runefall/build/slices/
!Runefall/build/slices/**
!Runefall/build/evidence/
Runefall/build/evidence/*
!Runefall/build/evidence/S0/
!Runefall/build/evidence/S0/**
!Runefall/build/TRACEABILITY.json
```

Verified in a temp repository with the same layout: `build/audit/*.md`,
`build/slices/**`, `build/evidence/S0/*.log` and `build/TRACEABILITY.json` become trackable;
`build/evidence/<other>/` and `build/dist/` stay ignored; `*.log` is overridden for S0 because the
later negation wins. Note the two intermediate directory negations (`evidence/`, `evidence/S0/`)
are required; a single `!Runefall/build/evidence/**` does **not** re-include the log (verified).

**Option B — relocate governance artifacts** to a non-ignored root (e.g. `Runefall/governance/`:
`spec/`, `audit/`, `history/`, `traceability.json`, `evidence/S0/`), keeping `build/` for
regenerable outputs only. Smaller .gitignore surface, but changes every path in the spec, the gate
and the evidence references.

Either way the spec must: add the change to `In scope`; add the path to A7's allowlist; and make
the gate assert that every governance artifact is tracked (`git ls-files --error-unmatch`) and not
ignored (`git check-ignore` returns empty). Without that assertion the tracking fix is itself
unverified.

## 7. Is the S0 boundary small enough for a reliable independent audit?

**Not as written.** S0 currently bundles six coupled deliverables:

1. a frozen-input reconciliation of two reports (A1),
2. a schema-v2 migration of 48 records with 371 free-text criteria (A3),
3. a new gate with 7 rejection predicates, reason codes and fixtures (A4),
4. a 48-row (or 108-clause) traceability registry (A5),
5. a claim reset across at least four large documents plus a machine-readable evidence registry (A6),
6. a repository tracking-policy change that every other item depends on (absent from scope).

Items 1-3 are mutually dependent (the gate validates the schema; the schema encodes the audit
rules; the baseline feeds both), and item 6 is a precondition for all of them. A single auditor
cannot validate the gate before the schema is stable, nor the schema before the tracking decision
is made. Two further scope facts make one-pass audit unreliable:

- 41 of 48 slices have no implementation under the new workflow; any schema that requires
  `commit`/`implementationAudit` for them is unsatisfiable, so the boundary implicitly includes
  retrofitting 41 unexecuted slices.
- S0 itself is not in the 48-slice model (X2), so the object being audited has no record in the
  artifact that is supposed to describe it.

**Recommendation (contract level, not a work plan):** either (a) split S0 into S0a (tracking
decision + authoritative state + remediation record + schema + gate + fixtures) and S0b (baseline
reconciliation + traceability + documentation reset), with S0a audited first; or (b) keep one S0
but explicitly scope A1/A5/A6 to S0b and require S0a's gate to validate only the remediation
record plus the 48 slices' baseline statuses. With M-02/M-03/M-04 applied, a single S0 is
auditable, but the audit will still be mostly bookkeeping and will not exercise gameplay evidence
at all — which is appropriate, and should be stated.

## 8. Requirements that cannot be verified in this environment

1. **Dirty-tree and commit-mismatch fixtures** require mutating the repository and creating
   commits. The spec auditor is read-only; only the implementer or a clone-based harness can run
   them. If a clone is used, it will not reproduce ignored `build/` content unless copied.
2. **The positive control** ("gate PASS on valid audited state") cannot be constructed before
   `S0-audit.md` exists, and that file cannot exist before the audit. Circular (X5).
3. **"No requirement omitted" (A5)** is a semantic judgment; a count and an ID-set equality can be
   automated, omission of a clause cannot.
4. **"No unsupported PASS claim" (A6)** requires an agreed claim inventory; without it the check is
   a judgment call that different auditors will score differently.
5. **Clean-clone reproducibility (A8)** fails today: `run-static-gates.ps1` needs
   `%USERPROFILE%\.cache\build-runefall.bat` (outside the repo) and an `E:` Papyrus path. They exist
   on this machine but not in a clone.
6. **48 per-slice contracts and audits** do not exist and are not in S0 scope, yet A4's wording
   requires them; no auditor can verify a condition whose inputs are absent by design.
7. **Runtime/game evidence** is not required for S0 and should stay out of scope — this is correct
   in the draft and should be stated as an explicit non-goal so no reviewer demands a game run.

## 9. Minimum amendments required for approval

Each item is a contract-text change (plus one repository decision). Numbering is stable for
re-audit. "Closes" lists the blocking findings.

**M-01 — Make the normative source committed, numbered and cited.** Commit `Runefall/PROGRAM.md`
(currently untracked) and either number its sections §1-§12 or replace every "goal §N" in
`spec.md` with `PROGRAM.md:<section>` or `:line`. `spec.md`, `PROGRAM.md` and the gate must agree
on the baseline set, the C1.2 normalisation, the role rules and the gate ordering. No criterion may
cite an uncommitted or unnumbered document. *Closes B1 (X1).*

**M-02 — Decide tracking and put it in scope.** Choose Option A or B from §6.3; add the exact
change (pattern block or relocation) and its paths to `In scope`, to A7's allowlist and to the
evidence locations; require the gate to assert `git ls-files --error-unmatch` for every governance
artifact and `git check-ignore` empty. *Closes B2, B8 (X11), and unblocks A1/A8.*

**M-03 — Model both slice systems.** Add a top-level `program` object holding the remediation
slices S0-S15 (`PROGRAM.md:43-47`) with the fields A3 requires (contract, specAudit,
implementationAudit, commit, evidence, blockers, followups), and keep `slices{}` as exactly the 48
ULTIMATE-PLAN IDs with their baseline statuses. Define "current slice" as `program.slices`, not
`slices{}`. A1/A5 counts remain 48. *Closes B3 (X2).*

**M-04 — Make A4 satisfiable.** Rewrite the rejection rules as scoped predicates and define the
vocabulary:
`PROGRAM.md:20-22` already fixes the audit verdicts (PASS/PARTIAL/FAIL/BLOCKED); reuse them rather
than inventing a second vocabulary. Define:
- `overallStatus ∈ {IN_PROGRESS, BLOCKED, COMPLETE}`; `PASS` is forbidden at top level.
- `status ∈ {PASS, PARTIAL, FAIL, NOT_STARTED}` per slice; `criterionStatus ∈ {PASS, PARTIAL,
  FAIL, NOT_APPLICABLE}`; `NOT VERIFIED / NOT RUN / DEFERRED / PENDING` may appear only as
  `detail`, never as a status.
- A slice may be `PASS` only if `implementationAudit.verdict == PASS`, its `subjectCommit`
  equals `state.commit`, and its evidence files exist and hash-match. Non-PASS slices are legal;
  the gate's PASS means "the remediation slice is correctly audited", never "program complete".
- Define `subjectCommit` (commit inspected by the auditor) and do not compare it to the commit
  that adds the audit file.
- `dirty` = tracked modifications only (`git status --porcelain --untracked-files=no`);
  `stale` = a named, checkable condition (e.g. state `updatedAt` older than HEAD commit time, or
  spec hash mismatch).
- Bootstrap: an explicit `-Bootstrap` mode (or `--expect-missing-audit <slice>`) that permits the
  current slice's audit to be absent, used once by the implementation auditor and recorded in
  history; without it the workflow cannot start.
- Stable reason codes `GATE-E01…E09`; fixtures assert code and exit status.
- Forbid invoking `run-static-gates.ps1` or any rebuild from the gate.
*Closes B4, B5 (enum half), B9 (bootstrap half), B10 (codes).*

**M-05 — Make A3 falsifiable.** Commit `build/audit/state.schema.json` (JSON Schema 2020-12) as the
normative artifact: `schemaVersion: 2` required; types/enums for every field;
`additionalProperties: false`; exactly 48 slice IDs; nullability rules for `commit`,
`specAudit`, `implementationAudit` on baseline-only slices; evidence entries
`{path, sha256, kind, exists}`; `blockers[]` and `followups[]` object shapes. Provide the legacy
normalisation map for the 371 current checklist values → `criterionStatus` + `detail`. A3's
verification becomes "validate with `jsonschema` 4.26 plus one negative fixture per required
field". *Closes B5 (X3), and the "no record" half of B3 with M-03.*

**M-06 — Pin the baseline.** Embed in `spec.md`: both report paths and SHA-256 (values in §2.1),
the 48-row expected status table (or the 7 PASS / 30 PARTIAL / 11 FAIL sets), the 11 FAIL IDs, and
the normalisation `PARTIAL/UNVERIFIED → PARTIAL (C1.2)`. Require the reports to be tracked blobs
at a named commit and the gate to re-hash them. *Closes B2/A1.*

**M-07 — Fix the unit of A5.** Choose 48 slice rows **or** ~108 clause IDs (e.g. `F0.1-1…F0.1-6`);
state the exact expected count; require each registry row to embed the plan's verbatim acceptance
text (or clause IDs) and the plan's SHA-256; require the gate to assert exact ID-set equality, plan
hash equality, and that the registry status multiset equals the state's. *Closes B7.*

**M-08 — Replace the strawman claim test.** Freeze a claim inventory (file:line list captured when
the spec is approved) covering at least `STATE.md`, `RELEASE-READINESS.md`, `CHANGELOG.txt`,
`VALIDATION.md`, `EXECUTION-EVIDENCE.json` and the state `release` block; define an allowed-claim
grammar ("PASS only with audit citation and 'N/48 per 2026-10-28 audit'"; `COMPLETE`/`public
release` only with a release decision record); require one identical `STATUS:` line across all
docs; resolve or record the UI-dependency contradiction (`fomod/info.xml:6` vs
`ULTIMATE-PLAN.md:16` vs `STATE.md:137`); require search with explicit file lists or
`--no-ignore`. *Closes B6.*

**M-09 — Fix A7's command and allowlist.** Name the baseline commit (`a50b2b6`, or the commit
that establishes tracking); use `git diff --name-only <base>..HEAD` **plus**
`git status --porcelain -uall`; enumerate allowed paths: `EXECUTION-STATE.json`, `STATE.md`,
`RELEASE-READINESS.md`, `CHANGELOG.txt`, `VALIDATION.md` (if in scope), `.gitignore`,
`tools/audit-gate.ps1`, `tools/fixtures/**`, `build/audit/**`, `build/slices/**`,
`build/evidence/S0/**`, `build/TRACEABILITY.json`; state that ignored `build/dist*`/`release` are
out of the diff and whether that is acceptable (or hash them). *Closes B8 (X10).*

**M-10 — Split A8 and define the artifacts.** A8a (pre-audit): spec audit, spec SHA-256, a history
record of the spec audit, gate + fixtures, raw logs, exact commands with cwd/exit code/output
hash. A8b (closure, verified by the auditor): `S0-audit.md` exists, names the subject commit and
the gate result, and its SHA-256 is recorded in history. Define the `HISTORY.jsonl` record schema
(`seq, ts, actor, role, action, slice, subjectCommit, artifacts[{path,sha256}], verdict,
prevHash, hash`) with either a hash chain or one commit per record; define the placeholder policy
file (may be absent, never may be empty-and-counted); define "reproducible" as "from the committed
tree, no external scripts"; define "fresh auditor" and require actor/role fields. *Closes B9, B10
(X5).*

**M-11 — Specify the fixture matrix.** At least one negative fixture per rejection code, plus the
forged-PASS case (slice marked PASS with fabricated or empty evidence), plus one positive control;
each fixture is (input state/commit, expected exit, expected reason code), stored under
`build/audit/fixtures/S0/` and tracked. "At least 5 fixtures" is below the 7+ conditions A4 lists.
*Closes A4's fixture gap.*

**M-12 — Require evidence integrity.** Each evidence entry must exist, be non-empty, carry a
SHA-256, and declare a kind (`log`/`report`/`code`/`data`) — `EXECUTION-STATE` F0.2 currently lists
a `.py` source as evidence. The gate verifies hash and non-emptiness, so touching an empty file
cannot satisfy it. *Closes the existence-only weakness in A4.*

**Re-audit condition:** re-audit the amended spec against M-01…M-12. Approval additionally requires
that the amended spec's SHA-256 is recorded; any later edit voids the approval unless it is
recorded as an approved amendment.

## 10. Nonblocking findings

1. **Version identity is ambiguous.** `spec.md:4` says "Runefall v0.2 remediation" and
   `PROGRAM.md` is "Runefall v0.2 Remediation Program", while `CURRENT.txt` and `release.version` are
   0.1.0 and version folders include 0.1.x and 0.2.0-dev. State that v0.2 is the remediation
   program, not the shipped mod version, so the audit record cannot be misread.
2. **Duplicate state/evidence artifacts.** `build/execution/state.json` (IN_PROGRESS, F0.2) and
   `build/execution/evidence-index.json` compete with `EXECUTION-STATE.json` and
   `EXECUTION-EVIDENCE.json`. Declare one authority and mark the others superseded (the gate can
   then reject multiple state files).
3. **Evidence refs mix code and logs.** F0.2's evidence includes `content/schema/validate_content.py`;
   F0.7's includes a runtime directory that no longer exists. M-12's `kind` field fixes the class.
4. **One missing evidence ref.** Of 142 state evidence paths, 1 does not exist on disk
   (`J:/SkyrimTest/1.6.1170/Data/Runefall/runtime`). A4's "missing evidence file" check would flag it
   if applied to the baseline state; decide whether baseline refs are grandfathered.
5. **Checklist key names are slice-local.** 371 keys with no global registry; A4's criterion
   rejection needs a canonical key set (M-04/M-05) or a per-slice allowlist.
6. **STATUS.md-style single source absent.** The repo has no one-line status artifact; A6's
   consistency goal is easier if one exists and the docs point to it.
7. **Spec freeze.** Freeze `spec.md` by hash at approval (M-10) so a post-approval edit cannot
   silently change the contract the auditor approved.
8. **Audit authorship.** The workflow says the auditor writes `S0-spec-audit.md` but not who appends
   the corresponding `HISTORY.jsonl` record. If the Lead does it, independence depends on the hash
   in the record; that must be stated.
9. **Timebox and concurrency.** Two auditors writing the same path is a real coordination hazard
   (see §11); the workflow should name one owner per audit path or use distinct paths.
10. **Pester 3.4.0 is ancient.** If fixtures are PowerShell-based, rely on plain assertions and exit
    codes, not Pester 5 syntax; the environment has Python `jsonschema` 4.26.0, which is the safer
    validator.

## 11. Auditor statement — independence, read-only compliance, limits

- **No implementation, data, config, test or state file was modified.** The only file written is
  this one. `EXECUTION-STATE.json`, `EXECUTION-EVIDENCE.json`, `STATE.md`, `VALIDATION.md`,
  `RELEASE-READINESS.md`, `CHANGELOG.txt`, `.gitignore`, `tools/*` and `src/**` are unchanged;
  I did not run builds, generators, tests, or the game.
- Read-only commands used: `git rev-parse/status/log/ls-files/check-ignore` (no index or ref
  changes), `Get-ChildItem`, `Test-Path`, file reads and SHA-256 computation, and a
  `python -c "import jsonschema"` capability check.
- To verify the ignore-pattern fix I created a throwaway git repository under
  `%TEMP%\s0-ignore-test*`, ran `git check-ignore` there, and deleted it. Nothing in the project
  tree was touched by that experiment.
- **Concurrency:** an earlier audit at this exact path (author `tool-inventory`,
  2026-10-28T08:28:49Z) was present when I started. I read it before writing; it reached the same
  verdict with a subset of these findings. This revision supersedes it. During the audit,
  `Runefall/PROGRAM.md` was created by another actor (08:30:20Z) and is untracked; the audit was
  updated to assess it. If two auditors (or an auditor and the Lead) write the same paths
  concurrently, the Lead should treat that as a workflow defect (see N-9) and keep one
  authoritative copy.
- Limits: no gate exists yet, so A2/A4/A8 were assessed for implementability, not executed. No
  runtime evidence was reviewed because S0 correctly excludes it. Findings are based on the files
  and git state listed in §2.1 at the times shown.

## Appendix A — measured inventories

| Measure | Value |
|---|---|
| ULTIMATE-PLAN slice rows | 48 |
| Acceptance clause fragments (simple `;`/`,` split) | ~108 (1-6 per row) |
| Baseline verdicts (raw) | 7 PASS, 29 PARTIAL, 11 FAIL, 1 PARTIAL/UNVERIFIED |
| Baseline verdicts (C1.2 → PARTIAL) | 7 / 30 / 11 |
| State slice records | 48, all `PASS` |
| State checklist entries / distinct values | 371 / 116 |
| Checklist values exactly `PASS` | 207 |
| Checklist explicit non-PASS tokens | 16 |
| State evidence references / missing on disk | 142 / 1 |
| EXECUTION-EVIDENCE entries / PASS / PARTIAL | 147 / 142 / 5 |
| VALIDATION.md headings / PASS headings / PASS lines | 120 / 40 / 142 |
| STATE.md `PASS` mentions | 31 |
| Files under `Runefall/build` / tracked | 1,278 / 0 |
| Tracked files in repo under `Runefall` | 362 |
| `Runefall/PROGRAM.md` | 2,871 B, untracked during audit, mtime 08:30:20Z |
| HEAD / branch / tree | `a50b2b6` / `main` / clean apart from `?? Runefall/PROGRAM.md` |

## Appendix B — verified tracking pattern (Option A)

Test repository layout identical to the project; result after applying the block in §6.3:

| Path | Result |
|---|---|
| `Runefall/build/audit/a.md` | trackable (negation matched) |
| `Runefall/build/slices/S0/spec.md` | trackable |
| `Runefall/build/evidence/S0/a.log` | trackable (overrides `*.log`) |
| `Runefall/build/TRACEABILITY.json` | trackable |
| `Runefall/build/evidence/phase0/old.md` | ignored (intended) |
| `Runefall/build/dist/b.txt` | ignored (intended) |

Confirmed failure mode: with the current single rule `Runefall/build/`, no negation inside the
directory can re-include anything (git does not descend into an excluded directory). A single
`!Runefall/build/evidence/**` without the two intermediate directory negations leaves the S0 log
ignored.
