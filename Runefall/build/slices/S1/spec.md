# S1 — Co-save correctness (revision 6, after three independent spec audits)

Program: PROGRAM.md sections 1-5. Baseline: a50b2b6. Slice source: PROGRAM.md section 4 (S1) and ULTIMATE-PLAN.md F0.5 row; audit findings H4/H5/C5/C6/M2.
Status: DRAFT FOR FOURTH INDEPENDENT SPEC AUDIT (revision 6)
Revision: 6 (amends rev5 after third audit token/provenance precision)
Dependencies: S0 PASS. S1 is a safety gate; S2-S15 may not consume co-save changes until S1 PASS.

## 1. Normative header table (M3/M4)

Canonical v3 layout is the bytes the existing code writes; do NOT reorder (baseline v3 saves must remain readable). POWER-LADDER.md section 9 is normative; L4-RUNTIME.md section 5 must match it.

| Version | Header bytes | Fields (offset:width) | Rule |
|---|---|---|---|
| v1/v2 | 12 | magic 0:4; schema 4:u16; saveVersion 6:u16; sectionCount 8:u32 | schema 1 or 2; saveVersion equals schema |
| v3 | 16 | magic 0:4; schema 4:u16; flags 6:u32; saveVersion 10:u16; sectionCount 12:u32 | schema=3; flags=0; saveVersion=3 |

Precise: v1/v2 header 12 bytes: magic[0..3], u16 schema[4..5], u16 saveVersion[6..7], u32 sectionCount[8..11]. v3 header 16 bytes: magic[0..3], u16 schema[4..5], u32 flags[6..9]=0, u16 saveVersion[10..11]=3, u32 sectionCount[12..15]. Encoder writes schema 3 only; schemas 1/2 are read-only legacy. Exact doc edits: POWER-LADDER.md section 9 header bullet; L4-RUNTIME.md section 5 lines 117 and 134. Standalone decoder build/slices/S1/tools/decode_cosave.py must parse fixtures and print header fields/sections.

## 2. Frozen acceptance criteria

| ID | Criterion | Verification | Pass condition |
|---|---|---|---|
| S1-A1 | Decode destination atomicity and publish-only-on-success. (a) Decode decodes into a local candidate and assigns to caller only on success; every rejected input leaves caller state byte-for-byte unchanged. (b) OnLoad does not Reset or mutate registries until Decode succeeds; on failure it publishes empty/default state and clears ItemRegistry/Progression/MaterialRegistry once. Zero-record, one-record, multiple-RNF2, unknown-record and oversized-record behavior specified. | SelfTest: prefill sentinel CoSaveState, decode malformed/truncated/semantic-invalid inputs, compare every field; LoadRecord seam tests; code inspection of OnLoad; in-game F11 log. | Sentinel preserved on every reject; accepted record publishes atomically; fail-closed state exact; no crash. |
| S1-A2 | Canonical header: exact v1/v2/v3 table; flags=0; saveVersion mirror checked; byte offsets asserted; docs corrected; standalone decoder agrees. | SelfTest header bytes and hex log; decode_cosave.py on fixtures; doc diff evidence. | Header matches for all versions; docs match; decoder reproduces. |
| S1-A3 | Golden migration fixtures: schema2.bin with progression section only (header magic,u16 schema=2,u16 saveVersion=2,u32 sectionCount; no flags; payload 13 i32: attrs9, attrPoints, skillPoints, runeTier, bossKillTier; runeTier=5 and a second case runeTier=2; no ladder section) and schema1.bin without runeTier/bossKillTier; baseline-v3.bin from the pre-S1 build at 37dfb68 (one-off exporter or current runtime save), exported before the first co-save code change; derivation command and SHA-256 committed. Expected: tier5 -> rank=stage=100, tier2 -> 40, difficulty=255, curve=1, bossKillTier retained, XP0, class empty, endgame false; v1 -> rank=stage=1. | decode each fixture; assert field table; v3 baseline loads; standalone decoder; re-encode canonical. | Fixtures decode; all pinned fields exact; v3 baseline loads; docs/mapping consistent. |
| S1-A4 | Distinct downgrade gates: recordVersion > kSaveVersion and payload schema > kSaveVersion separately tested with distinct fixed log tokens, rejection, empty/default post-state, no crash; schema 0/absent rejected; in-game simulation BLOCKED unless real newer save exists. | LoadRecord(recordVersion=4), LoadRecord(schema=4), schema=0 tests; log capture. | Both reject with distinct tokens; empty state exact; no crash. |
| S1-A5 | Deterministic encoding/canonicalization: maps keys byte-lexicographic; containers/quests ascending u32 with dedupe policy; overrides key/stage/flags; items registryId then baseFormId; skills key; duplicate keys rejected or documented; decode canonicalizes; encode(decode(encode(x))) byte-identical; golden canonical_v3.bin with sha256 committed. | SelfTest reversed insertion, unsorted decode, idempotent re-encode, duplicate test, eviction-at-cap test (smallest FormID removed), compare against golden bytes. | All equal; golden matches; no unordered leak. |
| S1-A6 | Bounds/range table section 3.1 with encode cap=decode cap, fixed rejection token per row, validate-before-allocate and exact-consume; encoder writes exactly declared counts (fixes skillRanks min/loop bug); schema 0/absent rejected; no truncation. | SelfTest one negative per table row and >256 skills/schema0/duplicate/trailing bytes; assert token text; no crash. | Every case rejected with its token; no unbounded allocation; counts exact. |
| S1-A7 | Budget: kMaxEncodedBytes=1,048,576 and kMaxRecordBytes=2,097,152 (code currently accepts 16 MiB; C1 must lower it to 2 MiB); budget derivation table proves 1,000-item Phase-0 case fits; oversize save refused before OpenRecord with fixed error and no partial write; configHash opaque/preserved/out of scope. | SelfTest legal 1000-item state <= budget; oversize state refused; constants/docs audited. | Budget enforced; legal case passes; oversize refused; docs state number. |
| S1-A8 | Runtime persistence on the existing disposable rig: command `powershell -NoProfile -ExecutionPolicy Bypass -File tools/run-batch-test.ps1` from Runefall project root, default clean-save manifest (SaveName from _context/clean-save/CURRENT-CLEAN-SAVE.txt), game root J:\SkyrimTest\1.6.1170, SKSE 1.6.1170, CleanSave; batch must add pre/post field assertions (rank, stage, xp, class, one skill rank, difficulty, curve, endgame, generatedContainers count+member, rewardedQuests count+member, overrides count, items count+registryId, materials count, configHash) and F11 header hex; crash-log absence. Evidence file Runefall/evidence/S1/batch-<stamp>.md with cwd/command/exit/sha256. All A1-A7 self-tests run through the same game session (DebugHarness F11); if the rig is unavailable S1 is BLOCKED, not PASS. | Batch exit 0; log tokens; evidence file; no new crash. | All fields match pre/post; batch exit 0; no new crash; evidence complete. |


### 3.1 Field/collection cap, range and fixed token table

| Collection/field | Encode cap | Decode cap | Range/rule | Rejection token |
|---|---|---|---|---|
| sectionCount | 64 | 64 | >=1, <=64 | co-save: reject section-count |
| record size | - | 2,097,152 | <=2 MiB | co-save: reject record-size |
| items | 1024 | 1024 | registryId>0 unique; sorted | co-save: reject item-count |
| item.rarity | - | 5 | 0..5 | co-save: reject item.rarity |
| item.socketCount | - | 3 | 0..3 | co-save: reject item.socketCount |
| item.enhancementCount | - | 5 | 0..5 | co-save: reject item.enhancementCount |
| item.registryId | - | - | >0 unique | co-save: reject item.registryId |
| item.baseFormId | - | - | low32 | co-save: reject item.baseFormId |
| huntFirstKill | 256 | 256 | key<=64, flag 0/1 | co-save: reject hunt-count |
| materials | 256 | 256 | key<=64, count>=0 | co-save: reject material-count |
| packVersions | 64 | 64 | key/version<=32 | co-save: reject pack-count |
| generatedContainers | 4096 | 4096 | u32>0, sorted set | co-save: reject container-count |
| rewardedQuests | 1024 | 1024 | u32>0, sorted set | co-save: reject quest-count |
| ladderOverrides | 256 | 256 | key>0, stage1..100, flags 0/1 | co-save: reject override-count |
| skillRanks | 256 | 256 | key<=64, rank1..100 | co-save: reject skill-count |
| characterClass | 64 | 64 | string | co-save: reject string |
| playerRank/stage | - | - | 1..100 | co-save: reject range |
| playerXp | - | - | 0..10,000,000 | co-save: reject range |
| difficultyProfile | - | - | 255 or 1..5 | co-save: reject range |
| curveId | - | - | 1..5 | co-save: reject range |
| endgameComplete | - | - | 0/1 | co-save: reject range |
| schema | - | - | 1,2,3 only | co-save: reject schema |
| saveVersion | - | - | equals schema | co-save: reject version-mirror |
| flags | - | - | v3 must be 0 | co-save: reject flags |
| item.ilvl | - | 1000 | 0..1000 | co-save: reject item.ilvl |
| item.socketRunes | - | 3 entries | each 0..5 | co-save: reject item.socketRunes |
| item.enhancement category | - | 5 entries | 0..5 | co-save: reject item.category |
| item.enhancement tier | - | 5 entries | 0..5 | co-save: reject item.tier |
| item.tags | - | - | known mask only | co-save: reject item.tags |
| string.class | - | 64 | length | co-save: reject string.class |
| string.skill | - | 64 | length | co-save: reject string.skill |
| string.material/hunt | - | 64 | length | co-save: reject string.map-key |
| string.pack | - | 32 | length | co-save: reject string.pack |
| string.affix/effect | - | 64 | length | co-save: reject string.affix |
| trailing bytes | - | - | reject | co-save: reject trailing |
| duplicate known section | - | - | reject | co-save: reject duplicate-section |
| unknown section | - | - | skip if length valid | (none) |

Schema 0 or absent schema is rejected with co-save: reject schema. Every rejection leaves caller destination unchanged and logs its fixed token once.

## 3. Codec semantics, caps and ranges (M1/M2/M9/M10)

Caps (encode and decode identical): sectionCount 64; items 1024; hunt 256; materials 256; packs 64; containers 4096; quests 1024; ladderOverrides 256; skillRanks 256; string lengths: class 64, skill 64, material key 64, hunt key 64, pack key/version 32, affix/effect 64; item: rarity 0-5, ilvl 0-1000, socketCount 0-3, socketRunes 0-5, enhancementCount 0-5, category/tier 0-5, registryId>0 unique, baseFormId low 32-bit, tags mask known bits; playerRank/stage 1-100, playerXp<=10,000,000, difficultyProfile 255 or 1-5, curveId 1-5, material count>=0, hunt flag 0/1, endgame 0/1.

- Decode validates every length/range before resize/allocation; unknown sections skipped only after valid length; duplicate known sections rejected; trailing bytes rejected.
- Encoder enforces caps; if a collection exceeds cap, encode fails and OnSave logs co-save: encode cap exceeded and does not write. No silent truncation.
- skillRanks: write exactly min(size,256) and reject >256; loop bounded by declared count.
- Budget: kMaxEncodedBytes=1048576; kMaxRecordBytes=2097152. OnSave checks encoded.size() > budget and returns before OpenRecord with co-save: encoded bytes exceed budget. Docs: POWER-LADDER section 9, L4-RUNTIME section 5 and section 14 item 11 must state 1 MiB.
- configHash remains opaque/placeholder and is preserved; no fingerprint behavior in S1.


### 3.2 Ordering and eviction (B2)
Persisted order is canonical and non-semantic: containers/quests are sorted ascending u32 and deduped; at cap, the smallest FormID is evicted before insert (deterministic; the implementation must change the current FIFO erase(begin()) to smallest-FormID eviction); items sorted by registryId then baseFormId, duplicate registryId rejected; overrides by key/stage/flags; maps by byte-lexicographic key. Decode canonicalizes; encode(decode(encode)) byte-identical. Persisted order never affects gameplay outcome beyond the documented eviction rule.

### 3.3 Budget arithmetic and oversize behavior (B3)
Worst case: 1024 items * 787 B = 805,888 B; materials 256*70=17,920; hunts 256*67=17,152; packs 64*68=4,352; containers 4096*4=16,384; quests 1024*4=4,096; overrides 256*10=2,560; skills 256*70=17,920; item/map section overheads < 2 KiB; total < 890 KiB < 1 MiB. 1,000-item Phase-0 case is under budget. If encoded.size() > 1,048,576, OnSave logs co-save: encoded bytes exceed budget and returns before OpenRecord; the engine save continues without Runefall co-save data (documented data-loss consequence: that save reloads Runefall defaults; no in-save mutation or partial record).

### 3.4 OnLoad record loop (B4)
- Zero RNF2 records: reset state to defaults, clear ItemRegistry/Progression/MaterialRegistry, log co-save: no record; defaults.
- One valid RNF2: publish candidate atomically.
- Multiple RNF2: first valid record wins; later records ignored with co-save: duplicate record ignored.
- Unknown record type: ignored.
- Malformed/oversized RNF2: reject, publish defaults/clear once, log fixed token; continue loop. A malformed record followed by a valid RNF2 in the same stream must publish the later valid record (invalid does not poison the session).
- recordVersion > kSaveVersion or schema > kSaveVersion: reject before Decode with distinct tokens (co-save: reject record-version / co-save: reject schema), default state; no crash.

## 4. Evidence and governance (M11-M14)

- C1 allowed paths: src/plugin/src/CoSave.h, CoSave.cpp; src/plugin/src/DebugHarness.cpp only for SelfTest registration; SPECS/POWER-LADDER.md and SPECS/L4-RUNTIME.md; tools/run-batch-test.ps1; build/slices/S1/**; build/audit/S1-spec-audit*.md; build/audit/S1-audit.md; Runefall/evidence/S1/**; EXECUTION-STATE.json; build/audit/HISTORY.jsonl. Nothing under content/, fomod/, tools/Runefall.Build/**. Revert plan: git restore the listed code/docs and state if audit fails.
- Spec freeze: approved revision sha256 recorded in remediation.S1.contract and HISTORY spec-audit record.
- State transitions: SPEC_AUDIT->READY->IMPLEMENTING->LOCAL_CHECKS->AUDIT_PENDING->PASS; C1 hash recorded; C2 commits S1-audit.md; C3 state closure. Single writer per audit path.
- Commands: build C:/Users/NeneScarlet/.cache/build-runefall.bat; batch tools/run-batch-test.ps1; gate pwsh tools/audit-gate.ps1 -StatePath EXECUTION-STATE.json -Mode truthfulness exit 0; evidence under Runefall/evidence/S1 with cwd/command/exit/hash; C1 enforcement command git diff --name-only a50b2b6..HEAD plus git status -uall must stay inside the allowlist.

## 5. C1 acceptance mapping

| Criterion | Test/evidence |
|---|---|
| S1-A1 | SelfTest sentinel + LoadRecord zero/multi/unknown/oversized + F11 log |
| S1-A2 | header byte assertions + decode_cosave.py + doc diff |
| S1-A3 | schema1.bin/schema2.bin/baseline-v3.bin hashes, expected field table, derivation command |
| S1-A4 | LoadRecord version=4/schema=4/schema=0 tests with fixed log tokens |
| S1-A5 | reversed-order equality, canonical golden bytes, idempotent re-encode, duplicate test |
| S1-A6 | one negative per cap/range row + >256 skills |
| S1-A7 | 1000-item budget encode + oversize refusal |
| S1-A8 | batch evidence with field comparison + F11 header + crash check |

## 6. Out of scope

Item instance identity (S7), save schema v4, gameplay fields, UI, pack/content changes, configHash fingerprinting.
