# S1 spec amendment 2 — test-harness and allowlist extension (revision 2)

Status: DRAFT for independent spec audit. Scope: test-only dependency/preset/allowlist; no acceptance weakening.
Base contract: revision 8, SHA FADE63AC4AE10973E9746F8E54CE86D17ED967C725E7E2AE941DDAD57E5744C1.
This amendment supersedes only rev8 §4 test-harness wording and the §5 allowlist entries listed below.

## 1. Why

The frozen §4/§5 text assumed the existing vcpkg Catch2 package. On this host vcpkg manifest
install fails before CMake configure with:

  fatal: not a git repository: ../../.git/modules/lib/vcpkg
  note: while checking out port catch2 with git tree 5f738523...
  vcpkg install failed

Durable transcript: evidence/S1/vcpkg-failure-20261029-1445.log (cwd
Runefall/src/plugin; command documented there; exit 1). Root cause: lib/vcpkg/.git points at
../../.git/modules/lib/vcpkg, which does not exist, and no vcpkg submodule is registered.
Repair is out of scope for S1 (would require a network/bootstrap change to the test host).

## 2. §4 amendment — test presets

Keep test-windows unchanged for vcpkg-capable environments. Add:

- configure preset test-windows-vendored: inherits _ninja,_msvc; binaryDir build/test-windows-vendored;
  cacheVariables CMAKE_BUILD_TYPE=Release, PLUGIN_TESTS_ONLY=ON, RUNEFALL_VENDORED_CATCH2=ON;
  no _vcpkg inheritance.
- matching build preset and test preset with the same name.

The CMake PLUGIN_TESTS_ONLY branch now supports both paths: RUNEFALL_VENDORED_CATCH2=ON builds
CoSaveCodecTests.cpp + CoSaveCodec.cpp + test/vendor/catch_amalgamated.cpp; OFF uses
find_package(Catch2 3 CONFIG REQUIRED) and the vcpkg-provided package. This explicitly supersedes
rev8 §4's single "Catch2 package, existing test-windows preset" assumption.

Commands for the frozen host-test evidence:
  cmake --preset test-windows-vendored
  cmake --build --preset test-windows-vendored --target RunefallTests
  ctest --preset test-windows-vendored --output-on-failure

## 3. §5 allowlist additions

- src/plugin/src/ItemMetadata.h (now only includes CoSaveCodec.h; type move, no logic)
- src/plugin/CMakePresets.json (test-only vendored preset; production presets unchanged)
- src/plugin/test/vendor/** including catch_amalgamated.hpp, catch_amalgamated.cpp, LICENSE.txt, README.md
- evidence/S1/** (tracked as Runefall/evidence/S1/)
C1-allowlist.txt is updated with these paths.

## 4. Integrity / provenance

- catch_amalgamated.hpp 0646DF67296AD02FA48F06DE56B2DB05CC02A1F687085DAE1D6F7D85676BB53F
- catch_amalgamated.cpp 79B8D94F696929D99AEFD567BB51DD3F1EE7221E0891F65276DFD8369FB25BE4
- LICENSE.txt C9BFF75738922193E67FA726FA225535870D2AA1059F91452C411736284AD566
- README.md F82D63C684E9B3C86948665D9F58C00DBED879B45E0ACEBDE51CD651B8591AD1
- Source: https://github.com/catchorg/Catch2 tag v3.11.0, extras/ amalgamated files; fetched 2026-10-29.
- Official-release byte comparison is offline UNVERIFIED; version macros report 3.11.0.
- Test-only: no src/ file includes test/vendor; Runefall.dll target does not compile or link it.

## 5. Requested decision

Approve amendment 2 so candidate C1 can be audited against a conforming test harness/allowlist.
No A1-A8 criterion, token, cap, migration, runtime requirement, or production file is weakened.
