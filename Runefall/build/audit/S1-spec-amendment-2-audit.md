# S1 spec amendment 2 audit — test-harness/allowlist extension

Auditor: tool-inventory (independent spec auditor; did not implement S1/C1). Read-only; only this file written.
Date: 2026-10-29. Timeboxed.
Subject: build/slices/S1/spec-amendment-2.md (DRAFT, 2,209 B; base contract rev8 SHA FADE63AC...).
Read: amendment 2; spec.md rev8; S1-audit-12.md; C1-allowlist.txt; src/plugin/test/vendor/*; CMakeLists.txt tests branch; CMakePresets.json; ItemMetadata.h; vcpkg submodule state; git status.

## Verdict: REJECTED — no acceptance weakening, but four blocking corrections are needed before the allowlist extension is conforming

The amendment is directionally right and does not weaken any acceptance criterion or runtime requirement: it adds only a test-only vendored Catch2, the already-required test sources, ItemMetadata.h (type move) and CMakePresets.json to the C1 allowlist. The blocking problems are that it does not amend §4 (which forbids the preset edit it relies on), it lacks vendor license/provenance artifacts, its vcpkg-failure evidence citation is false, and it does not explicitly supersede §4's Catch2-package assumption.

## Answers to the four review questions

### Q1 — Does it avoid weakening any acceptance criterion/runtime requirement? YES

Scope is test-only: vendored Catch2, test sources, CMake test branch, ItemMetadata.h type move, CMakePresets test preset, evidence additions. No A1-A8 criterion, token, cap, migration, codec or runtime requirement changes. Production presets are untouched (all four presets that inherit _vcpkg remain; only the test-only test-windows preset drops it). Caveat: ItemMetadata.h is a production header; the amendment says "type move only", and on disk it is now a single include of CoSaveCodec.h. Confirm in the candidate diff that no struct definition/logic remains there, and that CoSave.h/CoSaveCodec.h aliasing is unchanged; that is a review check for C1, not a spec blocker.

### Q2 — Are vendor paths/hashes, ItemMetadata.h, CMakePresets, evidence/S1 additions explicit and sufficient for §5? PARTLY

- Vendor hashes are correct against the on-disk files: catch_amalgamated.hpp 0646DF67...BB53F (532,998 B), catch_amalgamated.cpp 79B8D94F...5BE4 (435,326 B); Catch2 version macros report 3.11.0. Good.
- CMakeLists tests branch already builds RunefallTests from CoSaveCodecTests.cpp + CoSaveCodec.cpp + vendor/catch_amalgamated.cpp with the vendor include dir and add_test; commands stay cmake --preset test-windows / --build ... --target RunefallTests. Good.
- ItemMetadata.h and CMakePresets.json are added to §5 and are in the (modified) C1-allowlist.txt. evidence/S1/** is already in rev8 §5; the amendment's repeat is redundant but harmless (normalise "evidence/S1/**" vs tracked "Runefall/evidence/S1/" prefix, carried N3).
- **Missing:** no vendor/LICENSE.txt and no vendor/README.md. Catch2 is BSL-1.0; vendoring requires the license text and provenance (official source URL, release/tag, fetch date, SHA-256s) to be retained and allowlisted. Neither file exists, and §2 lists only the .hpp/.cpp. This is a blocking correction.
- Authenticity vs the official release is UNVERIFIED offline: hashes are pinned and self-consistent, version macros are 3.11.0, but the auditor could not fetch catchorg/Catch2 to compare; record the comparison method (release archive hash) or explicitly mark authenticity UNVERIFIED in the amendment.

### Q3 — Is changing existing test-windows to no-vcpkg acceptable, or must it be a new preset?

Changing the existing test-windows preset is **not conforming as written**: rev8 §4 says CMakePresets.json may be edited "only if a new preset is added", and the amendment adds the file to §5 without amending that §4 rule — a direct contradiction with the frozen contract (also flagged by S1-audit-12 Q5). Also test-linux still describes itself as building "against the system Catch2", which is no longer true once the CMake tests branch uses vendored Catch2 for every preset.

Recommendation: **add a new dedicated preset** and keep test-windows for vcpkg-capable environments. Exact wording to insert in §4/§2:

"Add preset test-windows-vendored (inherits _ninja, _msvc; binaryDir build/test-windows-vendored; cacheVariables: CMAKE_BUILD_TYPE=Release, PLUGIN_TESTS_ONLY=ON, RUNEFALL_VENDORED_CATCH2=ON, no _vcpkg inheritance). Add a matching build preset and test preset. Update the §4 commands to: cmake --preset test-windows-vendored; cmake --build --preset test-windows-vendored --target RunefallTests; ctest --preset test-windows-vendored --output-on-failure. Leave test-windows unchanged for environments with a working vcpkg; update its and test-linux's descriptions to state which Catch2 they use."

If the Lead prefers a single preset, the alternative exact wording is:

"Amend §4: replace 'CMakePresets.json only if a new preset is added' with 'CMakePresets.json may also be edited to remove _vcpkg inheritance from the test-only test-windows preset when the vcpkg submodule is unavailable; no production/plugin preset may change.' Update test-linux's description to 'vendored Catch2'."

Either is acceptable; the new-preset route better matches the frozen §4 rule and preserves the vcpkg path.

### Q4 — Is the vcpkg failure evidence sufficient to justify a test-only vendored dependency? ROOT CAUSE REAL, BUT THE CITED EVIDENCE IS INSUFFICIENT AND MISATTRIBUTED

- The quoted failure is independently reproducible on this host: src/plugin/lib/vcpkg/.git is a gitdir file containing "gitdir: ../../.git/modules/lib/vcpkg", and Runefall/.git/modules/lib/vcpkg does not exist; git submodule status lists no vcpkg entry. The fatal error is therefore real.
- However the amendment says the evidence is "recorded in build/audit/S1-audit-12.md and S1-batch-blocker.md" — false: S1-audit-12 contains no transcript (it discusses the allowlist), and S1-batch-blocker.md contains no vcpkg/Catch2 text at all. There is no durable transcript of the failing command, cwd, exit code and stderr anywhere in the repo.
- Minimum to justify the test-only vendor: (a) record the exact command(s) and stderr (e.g., cmake --preset test-windows before the change, and vcpkg install/manifest failure) with date and cwd in Runefall/evidence/S1/vcpkg-failure-<stamp>.log; (b) state why repair is not used (vcpkg is not a registered submodule and .git/modules/lib/vcpkg is absent; no network/bootstrap path on the test host); (c) confirm the vendored dependency is test-only and cannot reach the shipped DLL (no include of test/vendor from src/, CMake target separate); (d) pin the version and hashes (already done) plus the license/provenance files; (e) mark Catch2 release-authenticity verification as offline-UNVERIFIED or add the release-archive comparison.

## Blocking corrections (exact)

B1. Amend §4's preset rule, not just §5: add test-windows-vendored (recommended) or explicitly authorize the test-windows edit and correct test-linux's description; update §4 commands accordingly. Without this the amendment contradicts the frozen rev8 contract.

B2. Add src/plugin/test/vendor/LICENSE.txt (Catch2 BSL-1.0 text) and src/plugin/test/vendor/README.md (source URL https://github.com/catchorg/Catch2, version 3.11.0, fetch date, both SHA-256s, "test-only, not linked into the plugin", authenticity status) and allowlist both in §2/§5; note the .hpp/.cpp hashes match on disk but official-release comparison remains UNVERIFIED offline.

B3. Record the durable vcpkg-failure transcript under Runefall/evidence/S1/ (command, cwd, exit, stderr), correct the false citation to S1-audit-12.md/S1-batch-blocker.md, and state why repair is not used (missing .git/modules/lib/vcpkg; no registered vcpkg submodule).

B4. Explicitly supersede §4's "Catch2 package, existing test-windows preset" wording: state that the CMake tests branch no longer uses find_package(Catch2) and builds the vendored amalgamated Catch2 for host-test presets; this is a §4 amendment, not only an allowlist addition.

## Nonblocking

- N1 Normalise evidence path spelling ("evidence/S1/" vs Runefall/evidence/S1/) in §5 and C1-allowlist.
- N2 Record amendment 2 (path + SHA + base rev8 SHA FADE63AC...) in remediation.S1.contract/HISTORY with an explicit precedence line (rev8 + amendment 2 supersedes rev8 §4/§5 only for listed test-harness paths).
- N3 C1-allowlist.txt is currently modified-uncommitted; commit it with the amendment and note the added vendor/ItemMetadata/CMakePresets lines.
- N4 If a single preset is chosen, add RUNEFALL_VENDORED_CATCH2 cache variable (or comment) so future readers know the preset intentionally bypasses vcpkg.

No source, data, state or history was modified by this audit.
