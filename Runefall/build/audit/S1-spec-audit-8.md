# S1 specification delta re-audit — revision 8 (closes R1-R4)

Auditor: tool-inventory (audited S1 rev6/rev7; did not implement S1). Read-only; only this file written.
Date: 2026-10-29. Timeboxed delta audit (6 min). Subject: build/slices/S1/spec.md revision 8, 17,610 B, SHA-256 DEF1AFB85D35878D36A456E743069B8C9EAC0A0B2B90EF1AD8F85D09A97881B1 (working tree, modified; not yet committed/frozen).
Read: rev8 spec; rev7 audit; amendment audit BA1-BA10; METHODOLOGY.md; ADR-001; REFERENCE-REVIEW/MEO-REVIEW; AGENTS 3.2; state/HISTORY/git.

## Verdict: APPROVED (R1-R4 closed; no blocking findings). Freeze steps and four small editorial corrections required before implementation.

Rev8 is a self-contained merged contract: the retained data contract is back in the document, the host seam is frozen with a defined Record type and per-record version, the pure-header constraint is explicit, and A6 is split into rejection vs accepted classes with concrete tokens. Remaining items are editorial/process, not contract blockers.

## R1-R4 delta verification

| Item | Verdict | Evidence |
|---|---|---|
| R1: 2.5.1 bounds table, 2.5.2 ordering, 2.5.3 budget arithmetic, 2.5.4 OnLoad semantics; no dangling §3.1 refs | **CLOSED** | §2.5 is present and self-contained: 2.5.1 33-row bounds/record table (sectionCount..unknown-section, incl. record size 2,097,152; item rarity/ilvl/socket/enhancement/category/tier/tags/registryId/baseFormId; string limits; ranges), 2.5.2 ordering+smallest-FormID eviction, 2.5.3 arithmetic (<890 KiB <1 MiB, Encode/Decode caps), 2.5.4 record-loop semantics. Grep finds no §3.1 references; remaining §3.2 mention is explicitly historical ("rev6 §3.2 retained") and the §2.5.x/§4/§5 references resolve. |
| R2: frozen seam with Record definition + recordVersion; unknown-drain vs A1-default contradiction resolved; zero/one/multi/unknown/malformed semantics | **CLOSED** | A1(b) freezes "struct Record { uint32 type; uint16 version; vector<uint8_t> bytes; }" and "static bool LoadRecords(const std::vector<Record>& records, std::uint16_t recordVersion, CodecState& out)"; per-record version checked by codec; 2.5.4 defines zero (defaults once, registries cleared)/one/multi (first valid wins, duplicate token)/unknown (bounded drain chunks <=4096, stream continues, later valid publishes)/malformed (reject that record, continue)/no-valid-record (defaults once). A1(b) no longer claims unknown stream collapses to defaults; contradiction with A6 class 10 is gone. |
| R3: pure CoSaveCodec.h types; no PCH/Core/RE/SKSE; registry defaults F11 | **CLOSED** | A1 freezes: CoSaveCodec.h defines CodecState, ItemMetadata, Enhancement, LadderOverride, Record, ItemTag with no PCH.h/Core.h/RE/SKSE includes; CoSave.h includes CoSaveCodec.h and aliases types; "no Core.h dependency may leak into the codec header". Registry defaults (CodecState{}, ItemRegistry empty, Progression defaults, MaterialRegistry map) assigned to in-game F11 because registries include Core.h/RE/SKSE. Feasibility confirmed against the tree: PCH.h includes RE/SKSE, Core.h includes PCH.h, so the relocation is required and now specified; §4 authorizes the files and the existing test-windows preset/target. |
| R4: A6 rejection vs accepted split, exact class->token rows, no placeholder tokens, no decode=false on accepted paths | **CLOSED (one editorial row to split)** | A6.1 lists rejection classes with concrete tokens (1a-1d, 2, 3a/3b, 4, 5a-5h, 6, 7a-7e, 8a-8i, 9, 11a/11b, 14a/14b) — placeholders "<collection>-count"/"string.*" gone. A6.2 lists accepted classes (10 drain, 12 dedupe, 13a/13b/13c accept) with decode true and "no rejection", so decode=false applies only to A6.1. Editorial: row 8g lists two tokens ("reject item.category / reject item.tier") under a table headed "one exact token each"; split into two rows (renumber 8h/8i/8j) or state the subfield convention in the header. |

## Nonblocking items

- N1 Pass-condition sentences: only A1 (:127) and A6 (:225) have explicit "Pass condition:" lines. Add one sentence each to A2, A3, A4, A5, A7, A8 (A8 has sub-item statuses but no single pass line). Editorial, not blocking.
- N2 A4 warning: addressed — fixed token "co-save: warn newer-save" pinned (:150), on-screen verification recorded BLOCKED and explicitly not required for A4 PASS (:151-152). CLOSED.
- N3 Evidence path: A8 now says Runefall/evidence/S1/batch-<stamp>.md (:249) which matches the tracked location; §5 allowlist still writes "evidence/S1/**" (:284) — state both are repo-relative to Runefall or normalize the allowlist spelling. Editorial.
- N4 ADR citation: present at :25 ("ADR-001 implements this methodology"). CLOSED.
- N5 Governance: rev8 is uncommitted (git status M Runefall/build/slices/S1/spec.md), state remediation.S1.contract still holds the rev6 hash E94ECAE...; C1-allowlist.txt exists but is untracked. Required after approval: commit rev8, set remediation.S1.contract.sha256 to the final rev8 hash (DEF1AFB8..., or the corrected hash after the 8g split), append the spec-audit record to HISTORY, and commit C1-allowlist.txt with the spec.
- N6 F7 waiver: §5 pre-approves src/plugin/src/InputManager.cpp as a crash-safety exception; add the reference to build/audit/S1-audit-8.md / F7-hotfix.md in the waiver parenthetical. Editorial.

## Consistency / no-weakening check

No weakening found: §1 header table unchanged; 2.5.1 bounds equal or clearer than rev6 §3.1; 2.5.2-2.5.4 equal to rev6 §3.2-3.4 with the Calamity bounded-drain addition; A2/A3/A4/A5/A7/A8 retain rev6 requirements (A3 pinned assertions intact; A4 distinct gates + newer-save consequence; A5 exact duplicate/eviction; A7 constants; A8 full batch evidence, BLOCKED definitions). A6 change is the ADR-001/METHODOLOGY-approved class-based evidence model, with the rev6 per-row matrix explicitly retired. A1's host/F11 evidence split resolves BA2/BA3 feasibility. No partial publish, fail-closed defaults, and registry-clearing requirements are preserved.

## Approval and next steps

APPROVED for implementation once the Lead completes the freeze actions (N5) and applies the editorial corrections above (8g row split, A2-A8 pass-condition lines, allowlist path spelling, F7 waiver citation). If the corrections are applied before the freeze commit, the auditor should confirm the hash delta only; if rev8 is frozen as-is, the corrections may be recorded as accepted editorial notes, but the 8g row must still be read as two sub-classes (category and tier) with their two named tokens. Implementation may begin under the §4/§5 allowlist after the contract hash is recorded.
