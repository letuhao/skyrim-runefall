# S1 revision-8 editorial re-audit (8b) — hash-delta verification

Auditor: tool-inventory (same independent spec auditor as rev6/rev7/rev8). Read-only; only this file written.
Date: 2026-10-29. Timebox: 2 minutes. Scope: hash-delta confirmation against the DEF1AFB8 approval only; no new content review.

## Verdict: APPROVED

The requested editorial correction is present and nothing outside the 8g split is visible in the audited region.

- Path: build/slices/S1/spec.md
- On-disk SHA-256: FADE63AC4AE10973E9746F8E54CE86D17ED967C725E7E2AE941DDAD57E5744C1 (matches the Lead value exactly, computed independently from the file)
- On-disk size: 17,619 B (Lead's message stated 17,594 B; the file hash is authoritative and matches, so only the Lead's size metadata is off by 25 B)
- Previous audited revision: DEF1AFB85D35878D36A456E743069B8C9EAC0A0B2B90EF1AD8F85D09A97881B1 (17,610 B)

## Verified delta (only the requested change detected)

| Item | Verified |
|---|---|
| Row 8g split | Now "| 8g | category | reject item.category |" and "| 8h | tier | reject item.tier |" — the combined row "reject item.category / reject item.tier" is gone. |
| Renumbering | Rows now read 8i tags mask -> reject item.tags and 8j baseFormId zero -> reject item.baseFormId; subsequent rows (9, 11a/11b, 14a/14b) unchanged and still well-formed. |
| A6.1 table contract | "one exact token each" is now satisfied: all 8x rows carry exactly one concrete token; no placeholder or slash-combined token remains in A6.1. |
| Other regions | No change detected in the sections inspected for consistency (heading map, §2.5 table, A1 seam text, A4 warning token, A6.2 accepted classes, A8 evidence path). |

## Limitation

A full byte-diff against DEF1AFB8 was not possible (the prior revision bytes are not retained by the auditor). Verification is the independent hash match to the Lead-published corrected hash, the presence of the exact requested split/renumbering, and spot-checks that the surrounding normative text is unchanged. Proxy check: row count increased by exactly one and total size changed by +9 B relative to the auditor's measured rev8 (17,610 -> 17,619), consistent with splitting one row and renumbering.

## Freeze note

Approval carries to hash FADE63AC4AE10973E9746F8E54CE86D17ED967C725E7E2AE941DDAD57E5744C1. Before implementation, the Lead must still complete the rev8-freeze actions from S1-spec-audit-8.md: commit the corrected spec, set remediation.S1.contract.sha256 to FADE63AC..., append the spec-audit record to HISTORY, and commit build/slices/S1/C1-allowlist.txt. The remaining nonblocking editorial notes (pass-condition lines A2-A8, allowlist path spelling, F7 waiver citation) are unchanged from the rev8 approval and do not block implementation.
