# S1 spec amendment 2 audit (revision 2) — B1-B4 re-check

Auditor: tool-inventory (independent spec auditor; did not implement S1/C1). Read-only; only this file written.
Date: 2026-10-29. Timeboxed.
Subject: build/slices/S1/spec-amendment-2.md revision 2, 3,191 B, SHA-256 4C13708CEB07B3C5432813CDD2771BE2BECF8341A1495DD6409F0DB0C73ECCD0 (base contract rev8 FADE63AC...).
Read: amendment rev2; spec rev8; prior audit S1-spec-amendment-2-audit.md; CMakePresets.json; CMakeLists.txt; vendor directory; evidence/S1/vcpkg-failure-20261029-1445.log; C1-allowlist.txt; git status.

## Verdict: APPROVED (B1-B4 closed; no blocking findings). Four nonblocking corrections and the freeze record remain.

The revised amendment now moves the test harness into a conforming, reversible form: a separate vendored-Catch2 preset, the vcpkg preset restored, a CMake switch for both paths, vendor license/provenance retained, a real (not misattributed) vcpkg-failure transcript, and explicit §4 supersession wording. No acceptance criterion or runtime requirement is weakened.

## B1-B4 closure

| Item | Status | Verification |
|---|---|---|
| B1 preset/§4 | **CLOSED** | CMakePresets now has configurePresets test-windows-vendored (inherits _ninja,_msvc; binaryDir build/test-windows-vendored; CMAKE_BUILD_TYPE=Release, PLUGIN_TESTS_ONLY=ON, RUNEFALL_VENDORED_CATCH2=ON; no _vcpkg), plus matching buildPresets.test-windows-vendored and testPresets.test-windows-vendored. test-windows is restored to inherits [_vcpkg,_ninja,_msvc] with VCPKG_MANIFEST_FEATURES=tests, so the vcpkg path is preserved. CMakeLists PLUGIN_TESTS_ONLY branch switches on RUNEFALL_VENDORED_CATCH2 (vendored: CoSaveCodecTests.cpp + CoSaveCodec.cpp + vendor/catch_amalgamated.cpp; vcpkg: find_package(Catch2 3 CONFIG REQUIRED) + Catch2::Catch2WithMain). §4 commands in the amendment use the test-windows-vendored preset and match the preset names. |
| B2 vendor license/provenance | **CLOSED** | src/plugin/test/vendor/LICENSE.txt exists (1,338 B, SHA C9BFF757...AD566, matches the amendment) and is the Boost Software License 1.0 text; README.md exists (868 B) with source URL/tag v3.11.0, fetch date, purposes, file hashes and an explicit offline "UNVERIFIED-AUTHENTICITY" note. Both are covered by the §5/allowlist entry src/plugin/test/vendor/** (C1-allowlist.txt line present). The .hpp/.cpp hashes still match on disk (0646DF67..., 79B8D94F...). |
| B3 vcpkg evidence | **CLOSED (one wording correction)** | evidence/S1/vcpkg-failure-20261029-1445.log exists (1,961 B): it shows the CMake preset variables including the vcpkg toolchain, the "error: ... lib/vcpkg/.git", "fatal: not a git repository", "checking out port catch2", the vcpkg install failure and VCPKG_PROBE_EXIT=1. Root cause independently re-verified: src/plugin/lib/vcpkg/.git is a gitdir file pointing at ../../.git/modules/lib/vcpkg, which does not exist, and git submodule status lists no vcpkg entry. The false S1-audit-12/S1-batch-blocker citations are gone; the amendment cites the log and states repair is out of S1 scope. Correction: the log has no cwd/command/exit-1 header (it begins at CMake preset output and ends at VCPKG_PROBE_EXIT=1), so the amendment's parenthetical "(cwd Runefall/src/plugin; command documented there; exit 1)" overstates the file; prepend 'cwd=', 'command=', 'exit=' lines to the log or correct the wording. |
| B4 §4 supersession | **CLOSED** | §2 states the CMake branch supports both paths and "explicitly supersedes rev8 §4's single 'Catch2 package, existing test-windows preset' assumption"; the preamble limits the supersession to rev8 §4 test-harness wording and the §5 allowlist entries. This is an explicit precedence statement, not a silent weakening. |

## No-weakening check

No A1-A8 criterion, token, cap, migration, codec or runtime requirement changes. Production presets that inherit _vcpkg remain intact (release-windows, debug-windows, tidy-windows, test-windows); the only preset semantics changed are test-only. ItemMetadata.h is a header the amendment restricts to "only includes CoSaveCodec.h; type move, no logic"; the on-disk file is currently just #pragma once + #include "CoSaveCodec.h" (C1 review must confirm the candidate diff has no residual definitions). No src/ file references test/vendor or catch_amalgamated (checked), so the vendor cannot reach Runefall.dll.

## Nonblocking corrections

1. B3 wording/log header as above: add a provenance header (cwd, exact command, exit code) to evidence/S1/vcpkg-failure-20261029-1445.log, or soften the amendment parenthetical to match what the file records (preset output + VCPKG_PROBE_EXIT=1).
2. §4 integrity list omits the README.md hash; add README.md SHA-256 F82D63C684E9B3C8... or state that the README is provenance-only and not hash-pinned.
3. test-linux is unchanged by the amendment but CMakeLists' non-vendored branch now also compiles CoSaveCodecTests.cpp and requires find_package(Catch2 3 CONFIG); the preset still describes "against the system Catch2" — note that a Catch2 CONFIG package is required, or add RUNEFALL_VENDORED_CATCH2=ON to test-linux as well (test-only, optional).
4. Freeze/record: amendment rev2 is DRAFT and uncommitted (git status shows the C1-allowlist, CMakeLists, CMakePresets and sources modified; the amendment itself untracked). On approval, set remediation.S1.contract.amendments[] (path + SHA 4C13708C... + base rev8 FADE63AC...) with a precedence line, update HISTORY, and commit C1-allowlist.txt with the amendment.

No source, data, state or history was modified by this audit.
