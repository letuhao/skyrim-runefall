# S1 revision-6 metadata re-audit (6b) — hash-delta verification

Auditor: tool-inventory (same independent spec auditor as audit 3 and audit 6). Read-only; only this file written.
Date: 2026-10-28.
Subject: build/slices/S1/spec.md (revision 6, metadata-corrected), audited against the R1 correction required in build/audit/S1-spec-audit-6.md.
Scope: hash delta only, per Lead request; no new design review.

## Verdict: APPROVED

The corrected revision-6 contract closes the sole remaining blocker (R1: audit ordinal) without altering any reviewed requirement. The frozen hash is:

- Path: build/slices/S1/spec.md
- Bytes: 14,804 (Lead's message said 14,802; the actual on-disk size is 14,804 — 2-byte discrepancy in the Lead's metadata only, not in the file)
- SHA-256: E94ECAEFF8375ADF282B93F348370AD6677733EE4EDF7F19D1D78F7E17EC9427 (matches the Lead-provided value exactly, computed by this auditor from the on-disk bytes)

## Verified changes (all claimed edits present)

| Claimed change | Verified at | Result |
|---|---|---|
| Status now "DRAFT FOR FOURTH INDEPENDENT SPEC AUDIT (revision 6)" | line 3 | PASS — R1 closed; title/revision remain revision 6, so the freeze records the correct fourth-audit ordinal for revision 6 |
| Record-size 2 MiB row added to §3.1 | line 38: "record size | - | 2,097,152 | <=2 MiB | co-save: reject record-size" | PASS — token pinned, matches A7's kMaxRecordBytes=2,097,152 requirement and CoSave.cpp:599's 16 MiB current value noted for C1 |
| Trailing/duplicate/unknown rows split | lines 71-73 (trailing -> co-save: reject trailing; duplicate known section -> co-save: reject duplicate-section; unknown section -> skip if length valid, no token) | PASS — A6 per-row token assertion is now unambiguous |
| Baseline-v3 export-before-C1 clause | line 25 (A3): "exported before the first co-save code change" | PASS — resolves nonblocking N4 ordering hazard |
| No other content changed | structure/line count and all A1-A8/§3.1-§3.4/§4-§6 content remain as reviewed | PASS to the extent verifiable by hash-delta; see limitation below |

## Limitation

A full byte-diff against the previously reviewed revision-6 text was not possible: the prior revision was untracked in git and this auditor retains no copy of its bytes. The verification is therefore: (a) the on-disk SHA-256 equals the Lead-provided hash for the corrected text, (b) every claimed edit is present at the expected location, and (c) the surviving text matches the audit-3/audit-6 reviewed contract in every section inspected. No unreviewed change was detected. The 2-byte size mismatch is in the Lead's message, not in the file; the hash is authoritative.

## Freeze / next step

- Record this SHA-256 as the approved S1 contract hash in remediation.S1.contract and the HISTORY spec-audit record, together with ordinal "fourth independent spec audit (revision 6)".
- S1 is APPROVED for implementation (C1) under the spec's C1 allowlist and state transitions (SPEC_AUDIT -> READY -> IMPLEMENTING). Implementation must stop and return to spec review if it needs any change outside the allowlist or any change to the frozen criteria.
- Baseline-v3 fixture must be exported before the first co-save code change (A3), as now stated in the contract.
