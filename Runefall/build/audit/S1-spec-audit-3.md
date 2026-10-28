# Third independent specification audit — S1 co-save correctness (revision 4)

Auditor: tool-inventory (fresh; did not implement S1, did not author the spec, PROGRAM.md, or audit 1/2). Read-only.
Date: 2026-10-28. Timeboxed.
Subject: build/slices/S1/spec.md revision 4 (13,032 B; title still says "revision 3"; status says "DRAFT FOR FOURTH INDEPENDENT SPEC AUDIT").
Read: S1 spec rev4; S1-spec-audit.md; S1-spec-audit-2.md; PROGRAM.md; CoSave.h/.cpp; POWER-LADDER.md section 9; L4-RUNTIME.md section 5; tools/run-batch-test.ps1; .gitignore; git status.
Limit: no implementation/execution; only this file written.

## Verdict: REJECTED (B3 and B4 closed; B1, B2, B5 remain open, plus one new header-table defect and one revision-identity defect)

Rev4 is a substantial improvement: it adds the cap/token table, budget arithmetic, record-loop semantics, pinned A8 command/environment, and the explicit "same game session" gating. It is still not approval-ready: B1 and B2 miss their own minimum amendments, B5's required evidence location is untracked (durability contradiction reintroduced for S1), and the normative header table contradicts its own precise paragraph.

## B1-B5 closure against the second audit

| Finding | Status | Evidence / remaining defect |
|---|---|---|
| B1 full cap/range/token table + schema0 + declared counts | **NOT CLOSED** | `3.1 table added with encode/decode caps, ranges and rejection tokens; schema 0/absent rejection is prose-only ("Schema 0 or absent schema is rejected with co-save: reject schema") and has no row; no schema in {1,2,3} row and no saveVersion-mirror/flag row despite audit-2's explicit minimum. Field rows required by `3 prose are missing: item.ilvl (0-1000), socketRunes (0-5), enhancement category/tier, tags mask, affix/effect string length 64, material key length. A6 demands "one negative per table row ... assert token text", but the table assigns shared family tokens (e.g. two item rows -> co-save: reject item-count / item-range), so per-row token assertion is not possible as written. Declared-count rule is present (`3, "skillRanks: write exactly min(size,256)...") and matches the real bug at CoSave.cpp:324-328 (declares min(size,256) then writes all entries); encoder-cap rule (`3) matches. |
| B2 ordering/eviction semantics | **NOT CLOSED** | `3.2 defines canonical order, sorting/dedupe and "at cap, the smallest FormID is evicted before insert". This contradicts current behaviour: RememberGeneratedContainer/RememberRewardedQuest evict FIFO with erase(begin()) (CoSave.cpp:189-191, 209-211), and the spec does not declare that eviction policy changes (or that this is a behaviour change for existing saves). A5's test list (reversed insertion, unsorted decode, idempotent re-encode, duplicate test, golden) contains no cap-eviction test, which audit-2's minimum required ("one clause + test"). Also `1 "do NOT reorder" vs `3.2 canonical sorting still needs the "header/section order only" clarification (audit-2 N3). |
| B3 budget arithmetic + oversize behaviour | **CLOSED** | `3.3 arithmetic is correct: 1024*787=805,888; sum with materials 17,920 + hunts 17,152 + packs 4,352 + containers 16,384 + quests 4,096 + overrides 2,560 + skills 17,920 = 886,272 B < 890 KiB < 1 MiB; the 1,024-item worst case covers the 1,000-item Phase-0 case. Oversize path is stated (encode > 1,048,576 -> log "co-save: encoded bytes exceed budget" -> return before OpenRecord; no partial write; documented data-loss consequence = defaults on reload). Minor residual (nonblocking): A7 also names kMaxRecordBytes=2,097,152 but `3.1/`3.3 never state the decode-side record cap/token; current code uses 16 MiB (CoSave.cpp:599) and is unchanged. |
| B4 OnLoad zero/multi/unknown/malformed | **CLOSED** | `3.4 specifies zero records (reset defaults + clear registries + "co-save: no record; defaults"), one valid (atomic publish), multiple (first valid wins, later "co-save: duplicate record ignored"), unknown (ignored), malformed/oversized (reject, defaults/clear once, fixed token, continue), recordVersion/schema > kSaveVersion (distinct tokens before Decode). A1's sentinel/fail-closed requirement is consistent. Residual ambiguity (nonblocking): sequence "invalid record then later valid record" is not defined (does the later record publish after a failure already published defaults?), and duplicate-section vs duplicate-record wording overlaps. |
| B5 exact A8 command/environment + consistent gating | **NOT CLOSED** | A8 now pins `powershell tools/run-batch-test.ps1`, game root J:\SkyrimTest\1.6.1170, SKSE 1.6.1170, CleanSave, evidence fields (cwd/command/exit/sha256), F11 header hex, crash absence, and states all A1-A7 run in the same game session; rig unavailable => S1 BLOCKED, not PASS. But: (a) the required evidence path build/evidence/S1/** is still gitignored — .gitignore:98 ignores Runefall/build/evidence/* with an exception only for S0 (lines 99-100); `git check-ignore` returns .gitignore:98 for build/evidence/S1/x.md; (b) .gitignore is not in the `4 C1 allowlist, so S1 cannot durably commit its own required evidence; (c) "default clean-save manifest" is not pinned: run-batch-test.ps1's param block defaults GameRoot but SaveName to '' (manifest selection implicit), and A8 does not pass -SaveName/-GameRoot explicitly; (d) "build C:/Users/.../.cache/build-runefall.bat" is an out-of-repo absolute path with no hash/recording rule (audit-2 N6). |

## New blocking defect: normative header table contradicts itself

`1's table is not usable as a normative byte table:
- Row at offset 6 says width u16 for v3, but the v3 field there is u32 flags (correct width is 4, spans offsets 6-9).
- Row at offset 8 mixes v1/v2 sectionCount with "u16 saveVersion at 10, u32 sectionCount at 12", so the Width column does not describe any single layout.

The "Precise" paragraph immediately below is correct and matches the encoder (CoSave.cpp:224-227: magic RNFS, u16 schema, u32 flags=0, u16 saveVersion; sectionCount at CoSave.cpp:332). Audit-2 N1 flagged this table in rev2; rev4 retains it unchanged. A2 requires "byte offsets asserted" and "docs corrected"; an internally contradictory table is a direct A2 hazard, and A1/A3 fixtures are derived from it. Minimum: replace with two clean tables (v1/v2 12-byte layout; v3 16-byte layout) or delete the table and make the Precise paragraph normative.

## New blocking defect: revision identity

The file's title says "(revision 3, after two independent spec audits)" while the metadata says "Revision: 4 (amends rev3 after second audit B1-B5)" and the status says "DRAFT FOR FOURTH INDEPENDENT SPEC AUDIT". This third audit reads revision 4, and `4 requires the approved revision SHA-256 in remediation.S1.contract and HISTORY. Ambiguous revision naming defeats the freeze/hash record. Minimum: title -> revision 4; state the audit ordinal explicitly (third independent spec audit) or define the numbering convention.

## A1-A8 falsifiability and required evidence

| Criterion | Falsifiable? | Note |
|---|---|---|
| A1 | Yes | Sentinel prefill + rejected-inputs comparison is mechanical; LoadRecord seam does not exist yet (CoSave.h) but is creatable within C1; needs explicit seam signature in the spec. |
| A2 | Partly | Header byte assertions are mechanical once the table is fixed; "docs match" needs the exact doc lines/hashes; the conflicting table undermines it today. |
| A3 | Yes | Expected mapping matches code: MigrateLegacy (CoSave.cpp:556-567) maps runeTier 5 -> 100/100, tier2 -> 40, difficulty 255, curve 1; schema1 (11 i32) -> tier0 -> rank/stage 1. Needs the pre-S1 baseline-v3 artifact provenance committed (audit-2 N4) — A3 says "derivation source/command and SHA-256 committed" but not where the pre-S1 build comes from; acceptable only if the command reproduces it. |
| A4 | Yes | LoadRecord(recordVersion=4)/LoadRecord(schema=4)/schema=0 with distinct tokens; in-game downgrade BLOCKED unless a real newer save exists (honest). |
| A5 | Yes, but incomplete | Golden canonical_v3.bin sha256 is committed; map/keys/items/skills/overrides canonical order defined; duplicate policy defined. Missing eviction-at-cap test (B2), and "encode(decode(encode(x))) byte-identical" should be complemented by encode(decode(old_v3_bytes)) == canonical golden to prove canonicalisation of legacy order. |
| A6 | Yes | One negative per table row + >256 skills/schema0/duplicate/trailing; token text asserted; but the table's shared tokens and missing rows (B1) make "per row" incomplete. |
| A7 | Yes | Legal 1000-item encode <= budget; oversize refused before OpenRecord; constants stated. Decode-side kMaxRecordBytes needs one sentence (nonblocking). |
| A8 | Yes | Batch exit 0 + log tokens + evidence fields + no new crash; requires the durability fix (B5) and explicit -GameRoot/-SaveName/manifest. |

## Fixtures/migration, downgrade, deterministic golden, governance/C1

- Fixtures/migration: A3 is well specified (two tier cases + schema1 + baseline v3; expected field table) but the baseline-v3 origin is not pinned to a commit/build hash; add the exact reproduction command and commit reference.
- Downgrade: A4 now has two distinct gates and tokens; schema 0 rejected; good. The spec's "in-game simulation BLOCKED unless real newer save exists" is honest and testable.
- Deterministic golden: A5 covers map ordering, container/quest ordering, dedupe, item order, overrides, idempotence and golden bytes; missing eviction test and legacy-order canonicalisation assertion (above).
- Governance/C1: `4 allowlist covers CoSave.h/cpp, DebugHarness SelfTest registration, POWER-LADDER/L4-RUNTIME, run-batch-test.ps1, build/slices/S1, build/audit/S1*, build/evidence/S1, EXECUTION-STATE.json, HISTORY; content/fomod/Runefall.Build excluded; state transitions and spec-hash recording present. Gaps: .gitignore (and build/evidence/S1 unignore) not allowed; the C1 enforcement command (`git diff --name-only a50b2b6..HEAD` + allowlist + clean-tree check) is still absent (audit-2 N5); "revert plan" exists but no executed-check evidence format.
- Dependency: S1 requires S0 PASS; S0 was rejected by spec audit (S0-spec-audit.md) and its amendments include .gitignore durability. S1 approval should be conditional on S0 actually landing the build/evidence/S1 unignore (or on S1 owning that edit).

## Minimum amendments to reach APPROVED

1. Complete `3.1: add rows for schema in {1,2,3}, saveVersion mirror, flags=0, item.ilvl, item.socketRunes, enhancement category/tier, tags mask, and string lengths; make schema0 a table row; either assign one token per row or change A6 to "one token per family, all rows exercised".
2. Replace the `1 header table with two correct layouts (v1/v2 12 B; v3 16 B) and keep the Precise paragraph as the single normative source.
3. B2: declare eviction a behaviour change vs current FIFO (CoSave.cpp:189-191, 209-211) or adopt FIFO; add a cap-eviction test to A5; clarify "do not reorder" = header/section order only.
4. B5: add .gitignore to the S1 C1 allowlist and unignore build/evidence/S1/** (or relocate S1 evidence to a tracked path); pin the exact batch invocation including -GameRoot J:\SkyrimTest\1.6.1170 and an explicit SaveName/manifest for CleanSave; record the build script hash and note clone-reproducibility limits.
5. Add the C1 enforcement command and the self-test seam signature (LoadRecord) to `4/A1.
6. Fix revision identity (title -> revision 4; audit ordinal); optionally one sentence for decode-side kMaxRecordBytes.

After these amendments the contract is testable and scoped; no implementation should start before a fourth audit (or Lead-accepted amendment review) confirms them.
