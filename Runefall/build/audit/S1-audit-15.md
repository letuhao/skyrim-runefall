# S1 implementation audit 15 — code candidate d8bcf79 / state child 9c7d40f

Auditor: dll-config-analyst (independent; did not implement S1). Read-only; only this file written.
Repo: D:\Works\source\skyrim-moddings\skyrim-runefall\Runefall. Date: 2026-10-29.
HEAD 9c7d40f (state), candidate code d8bcf79, clean tracked tree.
Contract: spec rev8 SHA-256 FADE63AC4AE10973E9746F8E54CE86D17ED967C725E7E2AE941DDAD57E5744C1 (= state contract, recomputed) + amendment 2 E5456397... (approved by S1-spec-amendment-2-audit-rev2.md, APPROVED).

## Commands actually run (read-only; no build/game launch)

1. git rev-parse HEAD -> 9c7d40f; git status clean; git show --stat d8bcf79 -> CoSaveCodec.cpp (+4/-1), CoSaveCodecTests.cpp (+56), evidence/S1/{batch-152924, batch-152924-raw, build-1529, host-tests-1529}; 9c7d40f = EXECUTION-STATE only.
2. Ran host test exe: `src/plugin/build/test-windows-vendored/RunefallTests.exe` -> **exit 0, "All tests passed (94 assertions in 10 test cases)"** (matches evidence/S1/host-tests-20261029-1529.txt).
3. Hash verification: batch header body_sha256 **9CAAA1B97BB05A24BFEE90E94FB7500F15D269524CD2F87AC6FEC2249A4B5F7E** = SHA-256 of tracked raw copy `evidence/S1/batch-20261029-152924-raw.md` (reproduced); build evidence DLL_SHA256 **CF9094CE045CC051A60DD77BFEBB7DFFED302DFCCB5F56B7810D141468DBADE7** = SHA-256 of rig DLL J:\SkyrimTest\1.6.1170\Data\SKSE\Plugins\Runefall.dll, mtime 08:29:01Z; batch 15:29:24 after DLL. Build evidence `build-20261029-1529.txt`: BUILD_EXIT=0.
4. Hash-checked **all 13** remediation.S1.evidence entries against current files: **all MATCH** (previously stale C1-allowlist/batch-144557/host-1432 entries removed/replaced). Allowlist current CBB6AF7E... matches state.
5. Code/test inspection: CoSaveCodec.cpp duplicate paths (:582-584 override tuple; :450/:508/:524/:606 map keys; :635-638 item.registryId token; :646 missing-section; :623-628 trailing), CoSave.cpp A4 warn path (:200), complete CoSaveCodecTests.cpp; C1-allowlist; batch raw/body; host metadata.
6. NOT run: native build/static gates/game batch (read-only; no game launch). Batch/build claims inspected via committed evidence and hash reproduction.

## Criterion verdicts

| ID | Verdict | Exact gap at d8bcf79 |
|---|---|---|
| S1-A1 | PARTIAL | Codec failure purity and LoadRecords zero/one/malformed-then-valid/unknown/multi are host-covered. Adapter OnLoad publish vs ClearLoadedState + three-registry reset is still not exercised by host tests; in-game F11/batch is regression evidence, not the spec's adapter-seam evidence split. Unchanged by this candidate. |
| S1-A2 | PASS | v1/v2/v3 header layouts/flags; host header asserts; batch full 16-byte hex `524E465303000000000003000A000000`; docs/fixtures agree. |
| S1-A3 | PASS | Migration fixtures + assertions unchanged (audit-10..14 PASS); candidate only touches token/tests/evidence. |
| S1-A4 | **PASS** | `co-save: warn newer-save version={}` present and reachable in CoSave.cpp:200 (OnLoad loop, version > kSaveVersion), then fail-closed; on-screen warning BLOCKED per spec. Verified by grep; host record-version tests unchanged. |
| S1-A5 | **PASS** | Duplicate item.registryId now MarkReject("co-save: reject item.registryId") in post-sort check (CoSaveCodec.cpp:635-638) **and crafted host test extracts the item payload and duplicates it** (tests "A5 duplicate registryId"), asserting the token. Map duplicate-key crafted test exists (materials, "reject duplicate-key"); override duplicate tuple crafted test exists ("A5 override duplicate"); hunt/pack/skill same token path. Minor residual: same-key/different-stage override semantics remain undocumented (exact-tuple duplicates rejected). |
| S1-A6 | PARTIAL | Host suite 94 assertions/10 cases covers ~25 of 37 A6.1 rejection classes: record-version, flags, version-mirror, section-count 0/65, empty-section, missing-section, duplicate-section, count-remaining, duplicate-key (material), record-size, budget, item-count, hunt-count, material-count, pack-count, container-count, quest-count, override-count, string.map-key, string.pack, string.class, range (3 variants), trailing, duplicate registryId/override. **Still missing mandatory rows:** 1a schema0/4 with "reject schema" token (only sentinel purity, no token assertion), 3a truncated body/trailing-body token, 5h skill-count cap, 7x string.affix/string.skill rows, 8b-8j item field rows (baseFormId, zero registryId, rarity, ilvl, socketCount, enhancementCount, socketRunes, category, tier, tags), 8a xp/endgame range variants, map duplicate-key tests for hunt/pack/skill (only materials), and A6.2 accepted classes 13a at-cap, 13b max-size, 13c missing-optional-section, plus container/quest dedupe (only unknown-drain + multi-record accepted). |
| S1-A7 | PASS | 1 MiB/2 MiB constants, 1000-item legal case, EncodeWithBudget(8) refusal + token, OnSave budget check; unchanged. |
| S1-A8 | **PASS** | Batch evidence header cwd/command/exit=0/body_sha256; body_sha256 = SHA-256 of tracked raw copy (verified); 38/38 self-tests incl schema3 save/reload 1861 B, full 16-byte header, full field round-trip, Save-files preflight; build DLL hash == rig DLL; host evidence has cwd/command/exit; all 13 state evidence hashes current; allowlist current. Minor: host-tests file has no hash field (cwd/command/exit only). |

## Allowlist / amendment

- Candidate paths: src/plugin/src/CoSaveCodec.cpp, src/plugin/test/CoSaveCodecTests.cpp, evidence/S1/** (incl. tracked raw copy) — all in C1-allowlist (current SHA CBB6AF7E...); amendment-2 paths (test/vendor/**, CMakePresets, ItemMetadata.h, evidence/S1/**) already allowed. No production file outside allowlist touched.
- Amendment 2 approved (rev2); no criterion weakening observed.

## Evidence/hash ledger

- Candidate d8bcf79; state child 9c7d40f; clean tree.
- batch-152924 raw SHA 9CAAA1B9...; batch evidence file F61FAE52...; build-1529 9872ECC1... (DLL CF9094CE...); host-1529 52EDDBA0...; state all MATCH.
- 94 host assertions / 10 cases, exit 0; host build command in evidence: cmake --build --preset test-windows-vendored --target RunefallTests && build/test-windows-vendored/RunefallTests.exe.

## Verdict

**PARTIAL — not PASS at d8bcf79.** A2/A3/A4/A5/A7/A8 PASS; **A6 PARTIAL** (≈25/37 rejection + 2/6 accepted classes; missing schema-token, truncated-token, item-field rows, skill-count/string.skill, hunt/pack/skill dup-key, and A6.2 at-cap/max-size/optional-section/dedupe accepted classes); **A1 PARTIAL** (adapter OnLoad publish/clear seam still host-untested). No repairs made.
