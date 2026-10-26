# Runefall native plugin

CommonLibSSE-NG SKSE plugin for Runefall v1 (Skyrim SE/AE 1.6.1170 only).

## Runtime requirements

- Skyrim SE/AE **1.6.1170** (1.7.x unsupported, D-062)
- SKSE 1.6.1170
- Address Library for SKSE Plugins (1.6.1170 bins)
- SKSE Menu Framework **3.18 release** (Nexus 120352; DEV build only for debug DLLs)

No SkyUI/MCM Helper/JContainers/PapyrusUtil dependency.

## Build

Pinned toolchain: CommonLibSSE-NG alandtse v14.0.1 (commit 723cfda480ae), vcpkg baseline
ee12231b20c95013c6638d845d04c91559a1d1ff, triplet x64-windows-static-md, VS2022 MSVC 14.44,
CMake + Ninja.

```powershell
cmake --preset release-windows
cmake --build --preset release-windows
# debug build for crash symbols:
cmake --preset debug-windows
cmake --build --preset debug-windows
```

Output: `build/msvc/Runefall.dll`. Content generation and packaging live in
`tools/Runefall.Build` and `tools/build-release.ps1`.

## Safety contract

A player CTD is never acceptable. Runtime game forms use `InventoryChanges::EnchantObject`;
every game pointer/lookup is null-checked; event sinks capture FormIDs/NiPointers and enqueue
main-thread tasks; runtime form creation waits for a gameplay-ready gate. See
`Runefall/SAFETY-AUDIT.md` and workspace `AGENTS.md` §3.1.

## Diagnostics

Debug builds emit PDBs; CrashLogger is required in the test environment. `RunefallProbe`
writes `Data/SKSE/Plugins/RunefallProbe.log`; set `bDebugSelfTest=1` in
`Data/Runefall/runefall.ini`.
