# S1 implementation audit 12 — code candidate 3e34031 / state d18cf95...HEAD

Subject code: **3e34031fe19c955424e9baa15fc745d8b3769464** ("feat(s1): split pure co-save codec with host tests"), HEAD = 3e34031 (state record 1d5cc8b is its parent; no state update for this candidate yet).
Spec: **build/slices/S1/spec.md revision 8**, SHA-256 **FADE63AC4AE10973E9746F8E54CE86D17ED967C725E7E2AE941DDAD57E5744C1** (= Lead-provided FADE63AC…44C1; matches remediation.S1.contract). Approved by S1-spec-audit-8/8b.
Auditor: independent; read-only; only this file written. Prior context: S1-audit-11 (PARTIAL @a8fe10e), S1-spec-audit-8b.
Date: 2026-10-29. Timeboxed.

## 1. Read-only commands actually run (captured)

| Command / check | Result |
|---|---|
| `git rev-parse HEAD`; `git status --porcelain`; `git show --stat 3e34031` | HEAD 3e34031; **clean tracked tree**; commit = 10 files, +27,394/−758 (CoSaveCodec.h 110 / .cpp 660 new; CoSave.cpp/h reworked; ItemMetadata.h; CMakeLists; CMakePresets; test/CoSaveCodecTests.cpp 118; test/vendor/catch_amalgamated.{hpp,cpp} 26,397 lines) |
| `git ls-files` on candidate paths | tracked: CoSaveCodec.h/.cpp, test/CoSaveCodecTests.cpp, **test/vendor/catch_amalgamated.cpp/.hpp**, ItemMetadata.h, CMakePresets.json, CMakeLists.txt, C1-allowlist.txt |
| `git check-ignore build/evidence/…` | **all candidate host-test/batch evidence is IGNORED**: `.gitignore:98 Runefall/build/evidence/*` → s1-host-tests.txt and batch-test-20261029-140033.md are untracked/undurable |
| Source inspection | CoSaveCodec.h: pure (std only; defines CodecState/ItemMetadata/Enhancement/LadderOverride/CoSaveRecord/ItemTag + kSaveVersion=3, kMaxEncodedBytes=1,048,576, kMaxRecordBytes=2,097,152). CoSaveCodec.cpp includes only its header + std (`PCH.h`/`Core.h`/`RE::`/`SKSE::` = 0 hits). Adapter **calls production codec**: CoSaveCodec::Encode×2, Decode×1, LoadRecords×1, LoadRecord×2, LastRejectToken×1; adapter keeps SKSE/re/serialization (SKSE::×4, GetNextRecordInfo×1) |
| Host test evidence `build/evidence/s1-host-tests.txt` (SHA 751DDBE0EE3301B0B93A4940D43F5C89FDF2315F6EC065FFC8BD05E998E69951) | `All tests passed (31 assertions in 5 test cases)` |
| Batch evidence `build/evidence/batch-test-20261029-140033.md` (SHA 99C3CA9740FDEED1533695894B11E9BF1A092300D4AD767304E3F50BC7DD5954) | exit 0 implied by report; 38/38 PASS incl. save-files-present, load/player/cell, full 16-byte header `524E465303000000000003000A000000`, co-save field round-trip, no-new-crash (pre-batch crash 04:26:47 covered by F7 waiver). **No cwd/command/exit/body_sha256 metadata header** (starts directly at "# Runefall batch test"). |
| DLL on rig `J:\SkyrimTest\1.6.1170\Data\SKSE\Plugins\Runefall.dll` | 1,510,912 B; mtime 13:55:51; SHA-256 **2AD17285B82B1A9C73CA43E137B269CCC5667850CB30239E8A797DB3B78E3E02** — no committed build-proof log records build exit/DLL hash/source mtime for this candidate |
| State `remediation.S1` | state **READY / status NOT_STARTED / commit null / implementationAudit null**; evidence[] references batch-test-20261029-134621.md, not the 140033 batch; all 6 listed evidence hashes verified OK |
| NOT run | native build, CTest, static gates, game/batch (read-only audit). Host/batch claims verified only via committed/on-disk evidence + code inspection. |

## 2. Answers to the Lead's key questions

1. **Is the codec split genuine production code?** YES — CMake `RunefallTests` compiles `test/CoSaveCodecTests.cpp + src/CoSaveCodec.cpp + vendored Catch2` (real source, not a test copy), and the SKSE adapter `CoSave.cpp` delegates Encode/Decode/LoadRecord/LoadRecords/LastRejectToken to `CoSaveCodec`; the codec header is PCH/Core/RE/SKSE-free. Production path exercised by host tests = true for encode/decode/seam, but coverage is thin (Q3).
2. **Failure purity / LoadRecords semantics:** Decode path is candidate-then-publish (codec `Decode` into local; adapter `OnLoad` assigns only after `LoadRecords` true, else `ClearLoadedState()`); LoadRecords: zero → defaults (false), one → publish, multi → first valid wins (duplicate ignored token), malformed→later valid publishes, unknown type skipped. Correct per §2.5.4, but host tests cover zero/one/malformed/unknown only — **no multi-valid-first-wins test**, and unknown-record handling has **no bounded drain token `drain unknown-record`** (A6.2 class 10 gap).
3. **A6.1/A6.2 coverage:** **only a subset** — host tests 5 cases/31 assertions exercise: truncated body (3a), schema 0 (1a), record-version 0/newer (1b), failure purity, zero/one/malformed/unknown seam, deterministic encode, header/round-trip. **NOT covered (blocking):** flags, version-mirror, section-count, record-size, duplicate-section, all collection caps (items/hunt/materials/packs/containers/quests/overrides/skills), count-remaining, all string lengths, range, item rarity/ilvl/socketCount/enhancementCount/socketRunes/category/tier/tags/baseFormId/registryId, missing-section, empty-section, budget; and A6.2 accepted classes 10/12/13a/13b/13c have no host tests. Tokens for **missing-section, empty-section, budget, count-remaining, drain unknown-record** are absent from both codec and adapter (grep = false). This fails the A6 pass condition ("every rejection row has a host-crafted payload and exact token; every accepted row has a host test").
4. **Catch2 vendor vs §5 allowlist:** **outside the frozen allowlist** — §5 allows only `src/plugin/test/CoSaveCodecTests.cpp`; `test/vendor/**` (26,397 tracked lines) and the modified `ItemMetadata.h`/`CMakePresets.json` are not listed, and C1-allowlist.txt (SHA C3AFEE72749FB8C39FBFB6D8B5D668FC1A84113E2600F5F2BE3BDDF67EA49E33) omits all three. This is a documented-amendment requirement: either extend §5/C1-allowlist with a signed amendment/waiver (F7-waiver precedent) or vendor Catch2 outside the C1 commit (e.g., fetched test dependency).
5. **CMakePresets change vs contract:** **inconsistent** — §4 permits `CMakePresets.json` "only if a new preset is added"; the change drops `_vcpkg` inheritance from the existing `test-windows` preset (now `inherits: [_ninja,_msvc]`, + formatting churn) rather than adding a preset. No vcpkg.json change (ok), but the preset edit is outside §4 authority and outside §5 allowlist.
6. **Build proof / DLL / batch integrity:** build proof **missing** for this candidate (no buildExit/buildOutput/CoSave last-write/DLL-hash log; evidence dir has no build-proof-20261029-1*.log); rig DLL hash 2AD17285… not recorded anywhere in state/evidence; batch report has no metadata header and is gitignored; state still READY/NOT_STARTED with stale 134621 evidence and null audit. Clean save ran (CleanSave; save-files-present row PASS); no new crash (prior 04:26:47 crash covered by F7 waiver). A8 therefore not durable evidence.

## 3. Criterion verdicts (exact commit)

| ID | Verdict | Basis / exact remaining gap |
|---|---|---|
| S1-A1 | **PARTIAL** | Codec split is genuine; failure-purity + zero/one/malformed-then-valid/unknown seam tested; adapter publishes only after LoadRecords and clears via ClearLoadedState (CoSave.cpp:85-94, 192-235). Gaps: multi-valid-first-wins not host-tested; unknown-record drain token absent; registries (ItemRegistry/Progression/MaterialRegistry) post-state proven only by in-game F11 (batch 38/38), not host-layer; spec evidence split respected but thin. |
| S1-A2 | **PASS** | Header contract unchanged; batch records full 16-byte hex 524E465303000000000003000A000000; codec/adapter v1/v2/v3 layouts match spec §1; standalone decoder/fixtures unchanged and still reproduce (not modified by 3e34031). |
| S1-A3 | **PASS** | Fixtures/decoder untouched; adapter SelfTest retains v1/v2/tier2 migration + canonical-golden decode (adapter SelfTest: tier2×8, canonical×21, v1×45 refs; batch "Self-tests all pass pass=38"). verify_fixtures.py unchanged (prior 10/10). |
| S1-A4 | **FAIL** | Record-version/schema gates + destination-unchanged are tested (host + adapter). **Missing rev8 requirement:** fixed loud token **`co-save: warn newer-save`** (and on-screen-warning recording) is absent from codec and adapter (grep 0). |
| S1-A5 | **FAIL** | Ordering/eviction/determinism/encode(decode(encode)) constants exist, but rev8 A5 requires (a) crafted decode-side duplicate registryId payload + token, (b) duplicate-key policy **REJECT with `co-save: reject duplicate-key` + host test** — token absent, no duplicate-key code, no crafted decode test; only encode-side duplicate rejection exists. Duplicate-section token exists but is untested. |
| S1-A6 | **FAIL** | Host tests cover only a subset of A6.1/A6.2 (Q3 above): 5 cases/31 assertions vs 30+ rejection classes and 6 accepted classes with mandatory crafted payloads/tokens; 5 required tokens unimplemented. Mandatory pass condition unmet → blocking. |
| S1-A7 | **PASS** | kMaxEncodedBytes=1,048,576 / kMaxRecordBytes=2,097,152 in CoSaveCodec.h; adapter keeps 1000-item legal case + EncodeWithBudget refusal + OnSave-before-OpenRecord (SelfTest refs 1000×5, budget×8); batch passes. Residual unchanged: "encoded bytes exceed budget" branch practically unreachable (cap token first). |
| S1-A8 | **FAIL** | Batch report functional rows are good (38/38, full header, field round-trip, save-file/player/cell), but evidence is **not durable or contract-compliant**: report is gitignored (`build/evidence/*`), lacks required cwd/command/exit/body_sha256 metadata, no build-proof log/DLL-hash record for 3e34031 (rig DLL 2AD17285… unrecorded), state still READY/NOT_STARTED with stale 134621 batch evidence and null audit. Spec A8 requires `Runefall/evidence/S1/batch-<stamp>.md` + metadata + build proof. |

## 4. Allowlist / governance findings

- Candidate delta (3e34031 vs parent) **violates the frozen §5 allowlist** for three items: **`src/plugin/test/vendor/catch_amalgamated.{cpp,hpp}`** (26,397 lines, no waiver), **`src/plugin/src/ItemMetadata.h`** (not listed), **`src/plugin/CMakePresets.json`** (not listed; §4 only permits adding a new preset). F7-waiver covers only InputManager.cpp. A documented amendment or Lead-signed waiver is required before closure; otherwise C1 is nonconforming.
- `C1-allowlist.txt` (rev7 text, tracked, SHA C3AFEE72749FB8C39FBFB6D8B5D668FC1A84113E2600F5F2BE3BDDF67EA49E33) does not list any of the three files above → the enforcement manifest itself is stale relative to the candidate.
- State lifecycle has not advanced for this candidate: state READY/NOT_STARTED/commit null/implementationAudit null, evidence still batch-134621 (parent candidate). A submitted C1 should be AUDIT_PENDING with commit 3e34031 and candidate evidence.
- No build-proof artifact: none of build/evidence/{build-proof*, s1-*} records buildExit/DLL hash for 3e34031 (only test output + batch report); DLL freshness can be inferred only from mtime 13:55:51 < batch 14:00.
- Spec rev8 itself is consistent (hash FADE63AC…44C1 verified on disk); C1-allowlist.txt header still says "revision 7".

## 5. Evidence hashes (independently computed)

```
spec.md                    FADE63AC4AE10973E9746F8E54CE86D17ED967C725E7E2AE941DDAD57E5744C1
C1-allowlist.txt           C3AFEE72749FB8C39FBFB6D8B5D668FC1A84113E2600F5F2BE3BDDF67EA49E33
CoSaveCodec.h              F269B1EBF197C4B190EAE8DB5496F4FF838B7A88CF4475918D50649912EEA071
CoSaveCodec.cpp            4324FA0D0BEAE57B89850AE8C4D7AE2B1DF62293A4BD9F232D2D10948BBABAC1
CoSaveCodecTests.cpp       7F25641B31AD3EB6F2A306F9B109119AAFF1C5883ADF6092D4BA8C4850D5F887
CoSave.h / CoSave.cpp      41E857DB4CFC31B5A739ACE67640C6D0F9382D6A96B92805FF4E9AC61DCFCAA8 / ADB1FC8F1DFEA9189B6DEE76C0892A9E232178731C24B7F7615FA8D698CD3B3E
ItemMetadata.h             068E439E4D172C5F3D7CB72D1882F3A82090560490F3DC036BF5E69C265A8246
CMakePresets.json          7734E5B4BFDF3B94A6C6DA532E53E1179141D37F47F7BA655A15669FCF67023E
CMakeLists.txt             7EB8E474726CE8CC1FF6D8CC4EBA864762EF0C273A899899F0294148560ED8DA
vendor/catch_amalgamated.hpp 0646DF67296AD02FA48F06DE56B2DB05CC02A1F687085DAE1D6F7D85676BB53F
vendor/catch_amalgamated.cpp 79B8D94F696929D99AEFD567BB51DD3F1EE7221E0891F65276DFD8369FB25BE4
build/evidence/s1-host-tests.txt 751DDBE0EE3301B0B93A4940D43F5C89FDF2315F6EC065FFC8BD05E998E69951
build/evidence/batch-test-20261029-140033.md 99C3CA9740FDEED1533695894B11E9BF1A092300D4AD767304E3F50BC7DD5954
rig Runefall.dll           2AD17285B82B1A9C73CA43E137B269CCC5667850CB30239E8A797DB3B78E3E02
```

## 6. Overall verdict: **FAIL (not PASS) at 3e34031**

Geniune codec split (adapter→production codec, pure header, host target compiles real source) — the architectural step is real. But the frozen rev8 acceptance criteria are not met: **A4 FAIL** (newer-save warning token missing), **A5 FAIL** (duplicate-key rejection policy/token absent, no crafted decode duplicate test), **A6 FAIL** (host tests cover a small subset; several mandated tokens missing), **A8 FAIL** (ignored/non-metadata evidence, no build proof/DLL hash, stale state), **A1 PARTIAL**. Additionally the candidate commits out-of-allowlist Catch2 vendor + ItemMetadata.h + CMakePresets edits and weakens the §4 preset rule without amendment.

Blocking closure items: (1) implement A6.1/A6.2 token set and one crafted host test per class incl. accepted classes; (2) add `co-save: warn newer-save` (A4) and `co-save: reject duplicate-key` + decode duplicate tests (A5); (3) put A8 evidence under `evidence/S1/batch-<stamp>.md` with cwd/command/exit/body_sha256 + commit build proof and DLL hash; (4) signed allowlist amendment for Catch2 vendor/ItemMetadata.h/CMakePresets (or revert them) and update C1-allowlist.txt; (5) advance state to AUDIT_PENDING with commit 3e34031.

Auditor independence: read-only; no source/state/fixture/gate/build/game action; only this report written.
