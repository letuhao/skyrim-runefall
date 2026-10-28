# Sixth-round S1 spec re-audit — revision 6 (same independent auditor as audit 3)

Auditor: tool-inventory. Read-only; only this file written. Date: 2026-10-28.
Subject: build/slices/S1/spec.md revision 6 (14,656 B, SHA-256 EB536809FD80208A2D9FBB24200011268A7F00C70CB15789C5BAC7ED22844591).
Inputs: spec rev6; build/audit/S1-spec-audit-3.md (prior findings); CoSave.h/.cpp; ItemMetadata.h; tools/run-batch-test.ps1; .gitignore; git state.
Scope: verify only the third-audit blockers and check for regressions.

## Verdict: REJECTED — one blocking metadata defect remains; every functional blocker from audit 3 is closed

The contract content is now complete and testable. The only remaining blocker is the audit-ordinal identity line, which must be corrected before the revision-6 hash is frozen into remediation.S1.contract/HISTORY. No implementation should start until that one line is fixed and the corrected bytes are re-hashed.

## Third-audit blocker closure

| Prior blocker | Status | Verification |
|---|---|---|
| Unique per-row tokens; add schema/saveVersion/flags/item/string rows | **CLOSED** | `3.1 now has per-row tokens (co-save: reject item.rarity / socketCount / enhancementCount / registryId / baseFormId / ilvl / socketRunes / category / tier / tags / string.class / string.skill / string.map-key / string.pack / string.affix) and added rows for schema (1,2,3 only), saveVersion (equals schema), flags (v3=0). Row values match code shapes: ItemMetadata.h has socketRunes array<3>, enhancements array<5>, tags u16; `3 prose "socketRunes 0-5" is now clarified by the row as 3 entries each 0..5. A6 can now assert one token per row. |
| Eviction smallest-FormID policy + A5 eviction test | **CLOSED** | `3.2 explicitly requires changing the current FIFO erase(begin()) (CoSave.cpp:189-191, 209-211) to smallest-FormID eviction; A5 verification now includes "eviction-at-cap test (smallest FormID removed)". |
| Runefall/evidence/S1 path + exact batch command/SaveName provenance | **CLOSED** | A8 pins `powershell -NoProfile -ExecutionPolicy Bypass -File tools/run-batch-test.ps1` from Runefall project root, game root J:\SkyrimTest\1.6.1170, SaveName from _context/clean-save/CURRENT-CLEAN-SAVE.txt, and evidence Runefall/evidence/S1/batch-<stamp>.md. Verified: manifest exists with saveName=Save3_...; run-batch-test.ps1:95-102 reads that manifest when -SaveName is empty (throw if absent); GameRoot default matches (:15). git check-ignore confirms Runefall/evidence/** is unignored (.gitignore:88), and tracked Runefall/evidence/phase0 files prove the path is durable. |
| kMaxRecordBytes 2 MiB code requirement | **CLOSED** | A7 now states kMaxRecordBytes=2,097,152 and "code currently accepts 16 MiB; C1 must lower it to 2 MiB" — matches CoSave.cpp:599 (16u*1024u*1024u). |
| Header table / revision identity | **PARTIAL** | Header table is now correct: two-row layout v1/v2 12 B (magic 0:4, schema 4:u16, saveVersion 6:u16, sectionCount 8:u32) and v3 16 B (flags 6:u32, saveVersion 10:u16, sectionCount 12:u32), consistent with the Precise paragraph and CoSave.cpp:224-227/332. Revision number is now consistent (title/metadata revision 6). Remaining defect: Status says "DRAFT FOR SIXTH INDEPENDENT SPEC AUDIT" while the title correctly says "after three independent spec audits" — this is the FOURTH independent spec audit (audit 1 rev2, audit 2 rev2, audit 3 rev4, this audit rev6). Revision number 6 is not the audit ordinal. |
| Invalid-then-valid record sequence | **CLOSED** | `3.4 now states "A malformed record followed by a valid RNF2 in the same stream must publish the later valid record (invalid does not poison the session)", consistent with "first valid record wins; later records ignored". |

## Remaining defect (blocking)

**R1 — Audit ordinal in the frozen contract is false.** Status line: "DRAFT FOR SIXTH INDEPENDENT SPEC AUDIT"; actual ordinal = fourth (the title itself says "after three independent spec audits"). The freeze step records the revision SHA in remediation.S1.contract and a HISTORY spec-audit record, so this line becomes a false governance record for the audited artifact. Minimum amendment: change the status line to "DRAFT FOR FOURTH INDEPENDENT SPEC AUDIT (revision 6)" (or define the numbering convention explicitly, e.g. "audit round = revision number"), re-hash the file, and ensure HISTORY records the fourth spec audit for revision 6. No other content changes are needed for approval.

## Nonblocking findings (do not block approval)

- N1 A7 names kMaxRecordBytes=2,097,152 but no fixed decode-side token for payload > 2 MiB is pinned (`3.1/`3.4 say "fixed token" generically). Add e.g. co-save: reject record-size when implementing.
- N2 `3.1 last row combines trailing-bytes and duplicate-section into one row with two tokens ("reject trailing / duplicate-section"); split the row or accept explicitly that this family has two tokens.
- N3 A5 still does not assert legacy-order canonicalisation directly (encode(decode(pre-S1 v3 bytes)) == canonical golden); current tests (unsorted decode, idempotence, eviction) cover it indirectly. Optional strengthen.
- N4 A3 pins "the pre-S1 build at 37dfb68" — verified: commit 37dfb68 exists and is HEAD. Add one clause that baseline-v3.bin must be exported before the first co-save code change (ordering hazard), since after C1 the pre-S1 build is no longer reproducible from source.
- N5 run-batch-test.ps1's manifest path is absolute (D:\Works\...\_context\clean-save\CURRENT-CLEAN-SAVE.txt); A8's provenance statement is accurate but evidence should record that host-bound path/hash.

## Regression check (rev4 -> rev6)

No regression found: all eight criteria A1-A8 remain present with the same pass conditions; `3.1-`3.4 expanded, not weakened; A1 atomicity/OnLoad rules, A2 doc-correction mandate, A3 fixture expectations (tier5->100, tier2->40, v1->1), A4 two downgrade gates, A6 caps/declared-counts, A7 budget, A8 gating all intact. Governance gained the C1 enforcement command (`git diff --name-only a50b2b6..HEAD plus git status -uall) and the tracked Runefall/evidence/S1 allowlist. `5 mapping remains consistent (A5 row omits the new eviction test in the mapping table but A5's own verification lists it; purely editorial).

## Recommendation

Apply R1 (one-line status correction), re-hash the spec, and record the fourth-audit ordinal. If the Lead applies exactly that editorial amendment, this auditor confirms the corrected revision-6 text is APPROVED without further content review (auditor may verify the hash delta only). No implementation before that.
