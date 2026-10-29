# S1 — Co-save correctness (revision 8)

Program: PROGRAM.md sections 1-5. Baseline commit: a50b2b6.
Status: DRAFT FOR INDEPENDENT SPEC AUDIT (revision 8 supersedes revision 6 and amendment-1).
This revision merges amendment-1 and resolves the rev7 audit R1-R4 and BA1-BA10.
Dependencies: S0 PASS. S1 is a safety gate; S2-S15 may not consume co-save changes until S1 PASS.

## 1. Normative header table (unchanged from rev6)

Canonical v3 layout is the bytes the existing code writes; do NOT reorder. POWER-LADDER.md
section 9 is normative; L4-RUNTIME.md section 5 must match.

| Version | Header bytes | Fields (offset:width) | Rule |
|---|---|---|---|
| v1/v2 | 12 | magic 0:4; schema 4:u16; saveVersion 6:u16; sectionCount 8:u32 | schema 1 or 2; saveVersion equals schema |
| v3 | 16 | magic 0:4; schema 4:u16; flags 6:u32; saveVersion 10:u16; sectionCount 12:u32 | schema=3; flags=0; saveVersion=3 |

Precise: v1/v2 = magic[0..3], u16 schema[4..5], u16 saveVersion[6..7], u32 sectionCount[8..11].
v3 = magic[0..3], u16 schema[4..5], u32 flags[6..9]=0, u16 saveVersion[10..11]=3,
u32 sectionCount[12..15]. Encoder writes schema 3 only; schemas 1/2 are read-only legacy.
Standalone decoder build/slices/S1/tools/decode_cosave.py must reproduce the table.

## 2. Reference review (new, required by METHODOLOGY §4)

Sources indexed and read (ADR-001 implements this methodology): CodexOfPowerNG, calamity-reactive-loot-affixes, isekai-hero-skyrim,
MEO, Adaptive Attributes SKSE. Details: _context/code-index/REFERENCE-REVIEW.md,
MEO-REVIEW.md. Adopted patterns:

- Split pure codec/state-store from the SKSE callback (Codex); host C++ tests with CMake/CTest.
- Stage a snapshot and publish only after the record loop; do not reset live state before decode.
- Bounded drain for unknown/unsupported records (Calamity), with a fixed drain token.
- Versioned POD structs and explicit migration functions (Codex/Adaptive).
- Stable identity strings, never enum indexes or raw runtime FormIDs without ResolveFormID (MEO).
- P0-style premise gate before a large crafting feature; not part of S1 implementation.

Rejected from references: MEO's reset-then-read directly into global state; Adaptive Attributes'
unchecked ReadRecordData results; Codex's source-grep atomicity test (behavioral test required).
Useful but out of S1 scope: MEO's generated catalog and item-instance identity (future crafting slice).

## 2.5 Normative data contract (retained from rev6; self-contained)

### 2.5.1 Field/collection bounds and record contract

| Collection/field | Encode cap | Decode cap | Range/rule | Token |
|---|---|---|---|---|
| sectionCount | 64 | 64 | >=1, <=64 | reject section-count |
| record size | - | 2,097,152 | <=2 MiB | reject record-size |
| items | 1024 | 1024 | registryId>0 unique | reject item-count |
| item.rarity | - | 5 | 0..5 | reject item.rarity |
| item.socketCount | - | 3 | 0..3 | reject item.socketCount |
| item.enhancementCount | - | 5 | 0..5 | reject item.enhancementCount |
| item.registryId | - | - | >0, unique | reject item.registryId |
| item.baseFormId | - | - | nonzero low32 | reject item.baseFormId |
| item.ilvl | - | 1000 | 0..1000 | reject item.ilvl |
| item.socketRunes | - | 3 entries | each 0..5 | reject item.socketRunes |
| item.category | - | 5 entries | 0..5 | reject item.category |
| item.tier | - | 5 entries | 0..5 | reject item.tier |
| item.tags | - | - | known mask 0x3F | reject item.tags |
| huntFirstKill | 256 | 256 | key<=64, flag 0/1 | reject hunt-count |
| materials | 256 | 256 | key<=64, count>=0 | reject material-count |
| packVersions | 64 | 64 | key/version<=32 | reject pack-count |
| generatedContainers | 4096 | 4096 | u32>0, sorted set | reject container-count |
| rewardedQuests | 1024 | 1024 | u32>0, sorted set | reject quest-count |
| ladderOverrides | 256 | 256 | key>0, stage 1..100, flags 0/1 | reject override-count |
| skillRanks | 256 | 256 | key<=64, rank 1..100 | reject skill-count |
| string.class | - | 64 | length | reject string.class |
| string.skill | - | 64 | length | reject string.skill |
| string.material/hunt | - | 64 | length | reject string.map-key |
| string.pack | - | 32 | length | reject string.pack |
| string.affix/effect | - | 64 | length | reject string.affix |
| playerRank/stage | - | - | 1..100 | reject range |
| playerXp | - | - | 0..10,000,000 | reject range |
| difficultyProfile | - | - | 255 or 1..5 | reject range |
| curveId | - | - | 1..5 | reject range |
| endgameComplete | - | - | 0/1 | reject range |
| trailing bytes | - | - | reject | reject trailing |
| duplicate known section | - | - | reject | reject duplicate-section |
| unknown section | - | - | skip if length valid | drain unknown-record |

### 2.5.2 Ordering and eviction

Maps sorted by byte-lexicographic key; containers/quests ascending u32 and deduped; items by
registryId then baseFormId; overrides by key/stage/flags. At cap, the smallest FormID is evicted
before insert (not FIFO). Persisted order is canonical and non-semantic. Decode canonicalizes;
encode(decode(encode)) is byte-identical.

### 2.5.3 Budget arithmetic

1024 items * 787 B = 805,888 B; materials 17,920; hunts 17,152; packs 4,352; containers 16,384;
quests 4,096; overrides 2,560; skills 17,920; overhead <2 KiB; total <890 KiB <1 MiB. 1,000-item
Phase-0 case is under budget. kMaxEncodedBytes=1,048,576; kMaxRecordBytes=2,097,152.
OnSave refuses >1 MiB before OpenRecord; Decode rejects >2 MiB.

### 2.5.4 OnLoad record-loop semantics

Zero RNF2 records: publish defaults once and clear all registries. One valid: publish it. Multiple:
first valid wins; later records ignored with a duplicate token. Unknown record type: drain bounded
chunks and continue; it does not collapse the stream. Malformed/oversized: reject that record,
continue; a later valid record publishes. recordVersion > kSaveVersion or schema > kSaveVersion:
reject before decode with distinct tokens. No record at all leaves defaults.

## 3. Frozen acceptance criteria

### A1 — Failure-pure codec and fail-closed load

Two levels, explicitly separated.

(a) Codec/pure layer: pure codec Decode(bytes) returns an optional/candidate. A rejected input
returns no candidate and leaves the caller destination byte-for-byte unchanged. Host-tested.

(b) SKSE adapter: frozen seam:
  struct Record { std::uint32_t type; std::uint16_t version; std::vector<std::uint8_t> bytes; };
  static bool LoadRecords(const std::vector<Record>& records, std::uint16_t recordVersion, CodecState& out);
recordVersion is the SKSE record version for the stream; each Record.version is the per-record
version and is checked by the codec. The seam selects the first valid RNF2 record. Zero/one/multi/
unknown/malformed-then-valid expectations: see 2.5.4; unknown records are drained and later valid
records publish; a stream with no valid record publishes defaults once and clears CoSave state,
ItemRegistry, Progression and MaterialRegistry.

Pure header: CoSaveCodec.h defines CodecState, ItemMetadata, Enhancement, LadderOverride, Record and
ItemTag with no PCH.h/Core.h/RE/SKSE includes. CoSave.h includes CoSaveCodec.h and aliases the types
into Runefall; no Core.h dependency may leak into the codec header.

Registry defaults: assert equality against a freshly constructed default instance (CodecState{},
ItemRegistry empty, Progression defaults, MaterialRegistry default map), not hardcoded data values.
Evidence: codec/publish/token via host test; ItemRegistry/Progression/MaterialRegistry via in-game F11
because they include Core.h/RE/SKSE. Pass condition: all seam cases and both evidence layers pass.

### A2 — Header contract (unchanged)

Exact v1/v2/v3 table; flags=0; version mirror; byte assertions; full 16-byte header hex in batch;
standalone decoder agrees. PASS evidence from rev6 remains valid for unchanged code.

### A3 — Migration behavior (rev6 assertions retained)

Fixtures with committed SHA-256 and derivation: schema1_legacy.bin, schema2_tier5.bin,
schema2_tier2.bin, baseline_v3.bin, canonical_v3.bin. Pinned expected table:
- v2 tier5 -> rank=stage=100; v2 tier2 -> rank=stage=40; v1 -> rank=stage=1.
- difficultyProfile=255, curveId=1, bossKillTier retained, XP=0, class empty, endgame=false.
- baseline v3 loads and decodes rank/stage 42; canonical bytes equal baseline.
- Engine SelfTest covers v1/v2/migration + canonical decode; verify_fixtures covers fixtures.
Data values are samples; these migration assertions are behavior, not data enumeration.

### A4 — Downgrade and newer-save safety (rev6 retained)

Distinct gates/tokens: recordVersion > kSaveVersion and payload schema > kSaveVersion rejected
before decode, destination unchanged; schema 0/absent rejected; LoadRecord post-state asserted.
Newer save consequence (MEO-derived): log a loud fixed token and on-screen warning that the build
cannot read those records and SKSE does not preserve unread co-save data, so saving with this version
would destroy them. Fixed log token: co-save: warn newer-save; the on-screen warning is user-side
and its verification is recorded BLOCKED (not required for A4 PASS). In-game newer-save simulation
is BLOCKED per spec.

### A5 — Canonical order and duplicate policy (exact)

- Items: decode rejects duplicate registryId post-sort; crafted duplicate payload required; token
  co-save: reject item.registryId; destination unchanged.
- Known sections: duplicate section rejected with co-save: reject duplicate-section.
- Map/override duplicate keys: policy is REJECT with token co-save: reject duplicate-key; host test.
- Containers/quests: dedupe on decode and encode; sorted ascending; eviction at cap removes the
  smallest FormID (rev6 §3.2 retained).
- Encode/decode determinism: byte equality across insertion order; encode(decode(encode)) identical;
  smallest-FormID eviction host-tested.

### A6 — Failure classes, not per-row data

The rev6 per-row token matrix and §5 C1 "one negative per table row" are RETIRED. Section 2.5.1 is
the normative data contract/bounds but is not the acceptance matrix. Host codec tests exercise one
representative crafted payload per failure class through the real decode path. Rejection classes
must assert the exact token, decode false, destination unchanged, no partial publish, no unbounded
allocation. Accepted/normalized classes assert decode true and the expected normalized result; they
must not be forced to reject. In-game F11 is regression evidence only, not the codec oracle.

#### A6.1 Rejection classes (one exact token each)

| Class | Failure | Exact token |
|---|---|---|
| 1a | schema 0/4 | reject schema |
| 1b | record version 0/newer | reject record-version |
| 1c | v3 flags nonzero | reject flags |
| 1d | saveVersion != schema | reject version-mirror |
| 2 | sectionCount 0 or 65 | reject section-count |
| 3a | truncated body | reject trailing |
| 3b | record >2 MiB | reject record-size |
| 4 | duplicate known section | reject duplicate-section |
| 5a | items >1024 | reject item-count |
| 5b | hunt >256 | reject hunt-count |
| 5c | materials >256 | reject material-count |
| 5d | packs >64 | reject pack-count |
| 5e | containers >4096 | reject container-count |
| 5f | quests >1024 | reject quest-count |
| 5g | overrides >256 | reject override-count |
| 5h | skills >256 | reject skill-count |
| 6 | count within cap but >remaining bytes | reject count-remaining |
| 7a | class >64 | reject string.class |
| 7b | skill >64 | reject string.skill |
| 7c | material/hunt key >64 | reject string.map-key |
| 7d | pack >32 | reject string.pack |
| 7e | affix/effect >64 | reject string.affix |
| 8a | rank/stage/xp/diff/curve/endgame | reject range |
| 8b | rarity | reject item.rarity |
| 8c | ilvl | reject item.ilvl |
| 8d | socketCount | reject item.socketCount |
| 8e | enhancementCount | reject item.enhancementCount |
| 8f | socketRunes | reject item.socketRunes |
| 8g | category | reject item.category |
| 8h | tier | reject item.tier |
| 8i | tags mask | reject item.tags |
| 8j | baseFormId zero | reject item.baseFormId |
| 9 | duplicate/zero registryId | reject item.registryId |
| 11a | absent required section | reject missing-section |
| 11b | zero-length section payload | reject empty-section |
| 14a | encoded >1 MiB | reject budget |
| 14b | record >2 MiB decode | reject record-size |

#### A6.2 Accepted/normalized classes (decode true)

| Class | Behavior | Expected result |
|---|---|---|
| 10 | unknown record type | bounded drain (chunk <=4096), stream continues, later valid record publishes; log token drain unknown-record |
| 12 | duplicate raw container/quest IDs | dedupe and sort; no rejection |
| 13a | collection exactly at cap | accepted |
| 13b | record at exact max size | accepted |
| 13c | missing optional section | accepted with defaults |

Pass condition: every rejection row has a host-crafted payload and exact token; every accepted row
has a host test; no row is both; all destination/post-state assertions pass.

### A7 — Budget/caps (unchanged)

kMaxEncodedBytes=1,048,576; kMaxRecordBytes=2,097,152; 1000-item legal case under budget;
EncodeWithBudget refusal; OnSave refuses before OpenRecord; docs state 1 MiB.
§2.5.1 bounds remain normative: sectionCount 64; items 1024; hunt/materials 256; packs 64;
containers 4096; quests 1024; overrides/skills 256; string lengths 64/64/64/64/32/64;
item ranges; playerRank/stage 1-100; xp<=10M; difficulty 255/1-5; curve 1-5.
§2.5.2 ordering/eviction, §2.5.3 budget arithmetic and §2.5.4 OnLoad record-loop semantics are retained.

### A8 — Runtime integration (restated verbatim + BLOCKED definitions)

Exact command: powershell -NoProfile -ExecutionPolicy Bypass -File tools/run-batch-test.ps1
from Runefall project root, default clean-save manifest, game root J:\SkyrimTest\1.6.1170,
SKSE 1.6.1170, SaveName from _context/clean-save/CURRENT-CLEAN-SAVE.txt.

Preflight: save .ess and .skse exist; MCP connected; load_save accepted; player name not Prisoner;
cell formId. F11 runs the amended self-test suite (not a per-row matrix).

Pre/post fields: rank, stage, xp, class, endgame, containers count+member, quests count+member,
overrides count, items count+registryId, materials count, configHash, difficulty, curve, skillCount,
firstContainer, firstItem, skillRank, firstQuest. Full 16-byte header hex. Crash-log check
(no new crash after batch start). Evidence file Runefall/evidence/S1/batch-<stamp>.md with cwd/command/exit=0/body_sha256.

Sub-item status: save/load/field/header/preflight are mandatory for A8 PASS. Corrupt/truncated
co-save injection is BLOCKED unless a documented safe injection seam is implemented; if BLOCKED it
does not by itself fail A8 but must be recorded, and no release claim may cite it. If the rig is
unavailable, A8 is BLOCKED, not PASS.

## 4. Codec architecture and host test target (authorized paths)

NEW pure codec/state-store: src/plugin/src/CoSaveCodec.h, CoSaveCodec.cpp. Constraints: no
PCH.h/Core.h/RE/SKSE includes; fixed-width types; optional/candidate return; token callback;
bounded reads/writes. CoSave.h/CoSave.cpp remain the SKSE adapter and call the pure codec.

NEW host test: src/plugin/test/CoSaveCodecTests.cpp (Catch2, existing test-windows preset).
EDIT: src/plugin/CMakeLists.txt to add CoSaveCodec.cpp to RUNEFALL_CORE_SOURCES and the test to
PLUGIN_TESTS_ONLY target (or dedicated EXCLUDE_FROM_ALL target modeled on tests/ladder_host.cpp).
CMakePresets.json only if a new preset is added; vcpkg.json unchanged.

Commands:
  cmake --preset test-windows
  cmake --build --preset test-windows --target RunefallTests
  ctest --preset test-windows --output-on-failure
Then the normal build: C:/Users/NeneScarlet/.cache/build-runefall.bat; then batch.

## 5. C1 allowlist and enforcement

Allowed in C1:
- src/plugin/src/CoSave.h, CoSave.cpp, CoSaveCodec.h, CoSaveCodec.cpp
- src/plugin/src/DebugHarness.cpp only for SelfTest registration
- src/plugin/test/CoSaveCodecTests.cpp
- src/plugin/CMakeLists.txt
- SPECS/POWER-LADDER.md, SPECS/L4-RUNTIME.md
- tools/run-batch-test.ps1
- build/slices/S1/**
- build/audit/S1-spec-audit*, S1-audit*
- evidence/S1/**
- EXECUTION-STATE.json, build/audit/HISTORY.jsonl
- F7 waiver: src/plugin/src/InputManager.cpp (pre-approved crash-safety exception; not S1 scope)

Enforcement must use a grandfathered manifest for pre-C1 S0 paths, not the literal
a50b2b6..HEAD command. Add build/slices/S1/C1-allowlist.txt listing the exact allowed paths and
use: git diff --name-only <C1-base> plus git status -uall filtered against that manifest.
No content/, fomod/, tools/Runefall.Build/** changes.

## 6. State and audit rules

- Spec freeze: approved revision 8 SHA-256 recorded in remediation.S1.contract + HISTORY.
- Lifecycle: SPEC_AUDIT -> READY -> IMPLEMENTING -> LOCAL_CHECKS -> AUDIT_PENDING -> PASS/PARTIAL/FAIL/BLOCKED.
- C1 candidate code commit; C2 commits independent S1-audit.md; C3 state closure.
- A PASS audit must be against the exact C1 commit and must include host test output, build proof,
  batch evidence, and criterion-by-criterion verdicts.
- No false PASS; missing evidence is PARTIAL/BLOCKED.

## 7. Out of scope

Item instance identity/crafting (future slice, MEO reference), save schema v4, gameplay fields, UI,
pack/content changes, configHash fingerprinting, Prisma host integration.
