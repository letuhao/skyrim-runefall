# S1 implementation audit 13 — code candidate 849f9d8 / state child 219cd1a

Auditor: dll-config-analyst (independent; did not implement S1). Read-only; only this file written.
Repo: D:\Works\source\skyrim-moddings\skyrim-runefall (Runefall). Date: 2026-10-29. Timeboxed.
HEAD 219cd1a (state commit), candidate code 849f9d8, clean tracked tree.
Contract: spec rev8 SHA-256 **FADE63AC4AE10973E9746F8E54CE86D17ED967C725E7E2AE941DDAD57E5744C1**
(= state contract, recomputed). Amendment 2 file SHA **E54563976FF17974E22CED1C71483086BCA1276EF6833C35177A2273009F2F95**
(approved by build/audit/S1-spec-amendment-2-audit-rev2.md, SHA F6519FBE473B18DE15EDA53F26A9C88083E2A5ED2AA4C6C09C943F53A7BF0B43; verdict APPROVED with nonblocking corrections).
Read: spec rev8, amendment 2 + rev2 audit, audit-12, CoSaveCodec.h/.cpp, CoSave.cpp/h, test/CoSaveCodecTests.cpp, CMakePresets/CMakeLists, evidence/S1/{host-tests-20261029-1432.txt,build-20261029-1432.txt,batch-20261029-144557.md}, C1-allowlist.txt, state/HISTORY, git.

## Commands actually run (read-only; no build/game launch)

1. `git rev-parse HEAD` -> 219cd1a; `git status --short` -> clean; `git show --stat 849f9d8` / `219cd1a` -> candidate changes exactly: CoSaveCodec.cpp, CoSave.cpp (1 line), tests, CMakeLists/Presets, vendor LICENSE/README, evidence batch/build/host; state commit 219cd1a = EXECUTION-STATE only.
2. `src/plugin/build/test-windows-vendored/RunefallTests.exe` (host test target) -> **exit 0, "All tests passed (60 assertions in 7 test cases)"** (matches evidence/S1/host-tests-20261029-1432.txt).
3. SHA-256 checks: spec FADE63AC…; amendment E5456397…; amendment audit F6519FBE…; evidence batch **8C7C6B23C194DF78898E451FD56838C7F0C9D83E287D49FBDEFAE3B096317380** (matches state); build evidence **DC82447178F8BD59B83788FB7550219966207724887939F40048F8F7D2E68AF3** (matches); host evidence **11B69C01D5B41DB022DBD254A75364C645A4A05A88BF7E1CA985DCC73C65D102** (matches); C1-allowlist current **CBB6AF7E16529F58B111DF13F09B8D807CC95CC5CF78DC571C81B04784EB3DD2** vs state-recorded **C3AFEE72749FB8C39FBFB6D8B5D668FC1A84113E2600F5F2BE3BDDF67EA49E33** -> **state evidence entry stale**.
4. Batch evidence header: cwd/command/exit=0/body_sha256 02D10AFE…/raw_report build/evidence/batch-test-20261029-144557.md; recorded body_sha256 equals SHA-256 of that raw report file (verified 02D10AFEBE5215DD5B1258CF09A0BE7DBF2662CB75A0244607049310800487D4); batch has 38/38, full 16-byte header `524E465303000000000003000A000000`, full co-save field round-trip incl. difficulty/curve/overrides/skillCount/firstContainer/firstItem/skillRank/firstQuest, Save-files-present preflight; crash row cites pre-batch F7 crash 04:26:47 (waiver-covered).
5. Build evidence tail: BUILD_EXIT=0, DLL_SHA256=**53969F07DAFD0D6617D6A8605B4B3A99F76AA284C57AAE54A74911AC1C6B2343**, DLL_MTIME 2026-10-29T14:29:45; rig DLL J:\SkyrimTest\1.6.1170\Data\SKSE\Plugins\Runefall.dll SHA-256 **53969F07…2343**, mtime 07:29:45Z -> **build-evidence DLL == rig DLL**; batch 14:45:57 after DLL mtime.
6. Code inspection: CoSaveCodec toggle/reject tokens, LoadRecords, Decode sections; CoSave.cpp adapter delegation; tests list/cases; CMakePresets test-windows + test-windows-vendored; C1-allowlist entries. NOT run: native build/static gates/game batch (read-only; game launch prohibited).

## Criterion verdicts

| ID | Verdict | Exact gap (for 849f9d8) |
|---|---|---|
| S1-A1 | PARTIAL | Codec split/host tests extend LoadRecords (zero/one/malformed-then-valid/unknown/multi first-wins). Remaining: adapter OnLoad callback (publish vs ClearLoadedState/three registries) still not exercised by host tests; malformed-in-game save load remains untested (adapter code inspected only, no seam test). |
| S1-A2 | PASS | Header v1/v2/v3 layouts codec-verified; host header test; batch full 16-byte hex; docs/fixtures consistent. Candidate does not alter layout. |
| S1-A3 | PASS | Rev8 migration/fixtures unchanged from audit-10/11 PASS; host round-trip + adapter load path intact (candidate only adds tokens/tests/evidence; no migration change). |
| S1-A4 | **FAIL** | Rev8 A4 requires the fixed token **`co-save: warn newer-save`** (newer-save downgrade safety). Codec/adapter contain only `co-save: reject record-version` / `reject schema`; no `warn newer-save` token or warn path exists (grep both files = none; host test asserts reject-record-version for versions 0/4, not warn). On-screen warning BLOCKED is allowed, but the fixed log token is mandatory and unreachable/absent. |
| S1-A5 | PARTIAL | Duplicate-key implemented for materials/hunt/pack/skillRanks maps and duplicate registryId post-sort; crafted host test exists only for **material** duplicate-key (plus duplicate-section). No crafted decode test for hunt/pack/skill duplicate keys or duplicate item registryId; overrides are a vector with no duplicate-key policy/rejection/test (CoSaveCodec.cpp ladder override loop allows duplicate keys). |
| S1-A6 | PARTIAL | A6.1/A6.2 token set now exists (flags, version-mirror, section-count 0/65, empty-section, missing-section, duplicate-section, count-remaining, duplicate-key, record-size, budget, drain unknown-record, multi-record) — but only **~12 rejection classes + 2 accepted classes** are tested by real crafted decode payloads, versus the spec's 30+ rejection classes and 6 accepted classes. Untested rows include: schema/newer (A4), top-level trailing, material/hunt/pack/container/quest/override counts, all item field rows (baseFormId, registryId, rarity, ilvl, socketCount, enhancementCount, socketRunes, string.affix, category, tier, tags), range (rank/stage/xp/difficulty/curve/endgame), string.class/skill/pack/map-key length rows, skill-count, duplicate-key for hunt/pack/skill/item-registryId, empty-record stream, bounded drain chunk limit. So the answer to the Lead's key question: **subset, not all mandatory A6 rows**. |
| S1-A7 | PASS | kMaxEncodedBytes/kMaxRecordBytes, 1000-item legal case, EncodeWithBudget(8-byte budget) refusal + token, OnSave budget check verified; test/evidence present. |
| S1-A8 | PARTIAL | Batch evidence is durable/exact on disk and metadata-complete (cwd/command/exit=0/body_sha256 = raw-report SHA, raw report on disk, 38/38, 16-byte header, full field round-trip, build DLL hash matches rig). Remaining: raw report path `build/evidence/batch-test-20261029-144557.md` is under .gitignore `Runefall/build/evidence/*` (not tracked); durable copy is the tracked evidence/S1 file, but the recorded body_sha256 is the untracked raw report's hash and cannot be re-derived from the embedded body as-written. Host-test evidence file has only 4 lines (no cwd/command/exit/hash), and state evidence still records the pre-amendment C1-allowlist hash (C3AF… vs current CBB6…). |

## Other checks (amendments, tokens, F7)

- **Amendment 2 / rev2 audit:** APPROVED; B1/B2/B4 closed, B3 closed with wording correction (vcpkg-failure log lacks cwd/command/exit header). Candidate 849f9d8 touches CMakeLists/CMakePresets/vendor which C1-allowlist now covers (allowlist includes src/plugin/CMakePresets.json, src/plugin/src/ItemMetadata.h, src/plugin/test/vendor/*, evidence/S1/*); git diff for 849f9d8 matches these paths.
- **CMake:** test-windows preset restored to vcpkg path (`_vcpkg` inheritance + MANIFEST_FEATURES=tests) and `test-windows-vendored` added; host exe present and passing (60 assertions/7 cases). Build evidence build-20261029-1432.txt is the production vcpkg build (BUILD_EXIT=0, DLL hash above); no separate CMake/CTest stdout for the vendored test build is committed (host-tests file contains only Catch output).
- **A4/A5/A6 explicit tokens requested by Lead:** `warn newer-save` missing (A4 fail); duplicate-key for **maps/overrides** only partially there (maps implemented, overrides not); crafted decode tested for material duplicate-key only. A6 tokens missing-section/empty-section/budget/count-remaining/drain unknown-record **are present and tested**, but only as part of the subset above.
- **F7 waiver:** remains separate accepted exception; not touched by 849f9d8 and not required for S1.

## Hashes / durability summary

- Candidate code 849f9d8; state child 219cd1a; tree clean.
- Spec FADE63AC…; amendment E5456397…; amendment audit F6519FBE…; C1-allowlist file CBB6AF7E… (state stale C3AFEE72…).
- Batch evidence 8C7C6B23…; body_sha256 02D10AFE… = raw report SHA; build evidence DC824471…; host evidence 11B69C01…; DLL 53969F07… (build = rig).

## Verdict

**PARTIAL — not PASS.** A2/A3/A7 PASS; A1/A5/A6/A8 PARTIAL; **A4 FAIL** because the mandatory `co-save: warn newer-save` token/path is absent. A6 remains a subset (≈12/30+ rejection classes, 2/6 accepted classes) despite the new token work; A5 duplicate-key/registryId/override coverage is incomplete (material only, overrides unhandled/untested); A8 evidence is durable on disk and hash-consistent with the rig/build, but the recorded body hash refers to an untracked raw report and the state's allowlist hash is stale. No repairs made.
