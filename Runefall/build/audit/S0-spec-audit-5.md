# Fifth independent spec audit — S0 truth reset and audit harness (revision 5)

Auditor: same independent spec auditor as S0-spec-audit-4.md (did not author S0, PROGRAM.md, or audits 1-4; not the implementer). Date: 2026-10-28, Asia/Bangkok. Mode: read-only; only this file written.
Subject: build/slices/S0/spec.md revision 5, 14,502 B, SHA-256 D543CCEC5EB18013245DF9F0F55C3A3CDF1DA4ECBBCA164E318CA7EB1D7359A0, status "DRAFT FOR FIFTH INDEPENDENT SPEC AUDIT".
Also checked: build/audit/S0-spec-audit-4.md residue list; unchanged inputs re-hashed — PROGRAM.md 58168D368DF12791F359A3241205B29D738944F2D7F75011DF8267660DCFEB87, EXECUTION-STATE.json 32A186F708B26A277362CA99641142FC599A54B9C17D99AA6E51A523501B6837, repo-root .gitignore 9BC63D904E17D18DC9EA7FBBED33D2B6A96D4FFC26F40C6EDD0D2EB71EA61D71 (all identical to the fourth audit); git HEAD a50b2b6, working tree only untracked Runefall/PROGRAM.md.

Scope: only the residue from S0-spec-audit-4.md, plus a no-regression spot check. No broadening, no implementation.

## 1. Verdict

**APPROVED** — all five residue items are fixed as requested, and no other clause regressed. No blocking or nonblocking defect remains.

## 2. Residue verification

| # | Fourth-audit residue | Revision 5 | Result |
|---|---|---|---|
| 1 | 3.7/A2 arithmetic ("sixteen PENDING + one active" impossible) | 3.7 now accepts both (a) all sixteen remediation records PENDING before S0 starts and (b) fifteen PENDING plus exactly one active S0 record, and requires count text to say "fifteen PENDING + one active"; S0-A2 now reads "all-16-PENDING and 15-PENDING+1-active remediation states validate (null contracts/audits)". Total is 16 in both cases, matching 3.4 remediation S0..S15. | FIXED |
| 2 | Header revision / PROGRAM citation | Title "(revision 5, after four independent spec audits)", "Revision: 5", "Program: PROGRAM.md sections 1-5", "Spec revision: 5" — matches PROGRAM.md's five numbered headings and 1.1. | FIXED |
| 3 | 3.8 normaliser prefixes undefined for BLOCKED/NOT_RUN/NOT_VERIFIED/DEFERRED | criterionStatus enum now PASS, PARTIAL, FAIL, BLOCKED, NOT_RUN, NOT_VERIFIED, DEFERRED, NOT_APPLICABLE; explicit case-insensitive prefix mapping PASS/PARTIAL/FAIL/BLOCKED/NOT RUN/NOT VERIFIED+NOT_VERIFIED/DEFERRED/N/A/unknown -> FAIL; requirement added that the fixture cover each prefix including space forms "NOT RUN (...)" and "NOT VERIFIED (...)". Matches 5.2/5.4 "criterionStatus in enum". | FIXED |
| 4 | A7 paths not repo-relative / .gitignore base ambiguous | S0-A7 now states "exact allowlist (paths relative to repo root)" and lists .gitignore plus Runefall/PROGRAM.md, Runefall/EXECUTION-STATE.json, Runefall/EXECUTION-EVIDENCE.json, Runefall/STATE.md, Runefall/RELEASE-READINESS.md, Runefall/VALIDATION.md, Runefall/CHANGELOG.txt, Runefall/DECISIONS.md, Runefall/build/slices/**, Runefall/build/audit/**, Runefall/build/evidence/S0/**, Runefall/build/TRACEABILITY.json, Runefall/build/state.schema.json, Runefall/tools/audit-gate.ps1, Runefall/tools/audit-validators/**. Directly comparable to git diff --name-only a50b2b6..HEAD. | FIXED |
| 5 | Section 11 "gate rejects multiple authoritative state files" contradicted A7 (no deletion allowed) | 11 now says build/execution/state.json is a legacy snapshot superseded by EXECUTION-STATE.json, "archived in place and not deleted in S0"; gate reads only the stated -StatePath, so no deletion or out-of-allowlist change is required. No contradiction with A7. | FIXED |

## 3. No-regression spot check (revision 4 -> 5)

Unchanged and consistent: 1.2 baseline report hashes/sizes; 1.3 baseline 7 PASS / 30 PARTIAL / 11 FAIL IDs and normalisation; 1.4 PASS retraction; 3.1-3.6 (including 3.6 runtime-gaps precedence from N7); 3.9 evidence shape; 3.10 HISTORY schema; 4.1-4.4 (4.2 bootstrap scoped to currentSlice); 5.1-5.7 (5.2 hash scoping only non-null >= SPEC_AUDIT); 6.1-6.4 durability block and fallback; 7.1-7.5 docs truth reset; 8.1-8.3 traceability; S0-A1 and A3-A8 (A7 only path-base change); 10.1 audit filename pattern (next = S0-spec-audit-5.md, which this file is); 11 remaining open decisions.
No new contradiction found between the expanded criterionStatus enum and gate semantics: 5.2/5.4 validate membership, while 5.3 release requires slice/remediation PASS and never promotes a checklist criterionStatus. The state enum (3.5) and criterionStatus enum (3.8) remain distinct fields.

Optional nit (non-gating): read 11's "archived in place" strictly as left in place unchanged; if it ever means a rename/move, build/execution/state.json must be added to S0-A7. No change required for approval.

## 4. Auditor statement

Read-only verification: file reads, SHA-256 over spec/PROGRAM.md/EXECUTION-STATE.json/.gitignore, git rev-parse/status. Only this file was written; no state, spec, PROGRAM.md, gate, fixture, git, build or game action. Independence: not the implementer; did not author S0, PROGRAM.md, or audits 1-4.
