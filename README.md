# skyrim-runefall

Runefall - a Torchlight/Diablo-style four-pillar RPG overhaul for Skyrim SE/AE 1.6.1170.

The project lives in [Runefall/](Runefall/):

- [Runefall/GAME-DESIGN.md](Runefall/GAME-DESIGN.md) - product and gameplay pillars
- [Runefall/SPECS/](Runefall/SPECS/) - layered design specs (L1-L5) plus mining/drop-table studies
- [Runefall/src/plugin/](Runefall/src/plugin/) - native CommonLibSSE-NG plugin (C++, CMake + vcpkg)
- [Runefall/src/papyrus/](Runefall/src/papyrus/) - Papyrus native bridge and quest probe
- [Runefall/tools/DataMiner/](Runefall/tools/DataMiner/) - official-master data miner (C#, Mutagen)
- [Runefall/tools/Runefall.Build/](Runefall/tools/Runefall.Build/) - ESL + CSV generator/validator (C#, Mutagen)
- [Runefall/ARCHITECTURE.md](Runefall/ARCHITECTURE.md) - architecture baseline for the real mod
- [Runefall/STATE.md](Runefall/STATE.md), [Runefall/DECISIONS.md](Runefall/DECISIONS.md), [Runefall/VALIDATION.md](Runefall/VALIDATION.md)

## Requirements (hard)

Runefall **will not work without all four**:

1. **Skyrim SE/AE 1.6.1170** - the only supported runtime. Skyrim 1.7.x is currently deferred
   (the SKSE/Address Library/plugin ecosystem is not ready); 1.5.97 and consoles are unsupported.
2. **SKSE 1.6.1170** - the SKSE build matching 1.6.1170 (skse.silverlock.org).
3. **Address Library for SKSE Plugins** - the 1.6.1170 database
   (`versionlib-1-6-1170-0.bin` and `-0-1.bin`) placed in `Data\SKSE\Plugins`.
4. **SKSE Menu Framework 3.18+** (Nexus mod 120352) - release build for player installs.
   Copy its `SKSE` folder into `Data\` (DLL, fonts, ini, strings and themes). The DEV build is
   only for debug DLL builds and must not be used with the release DLL.

Not required: SkyUI, MCM Helper, JContainers, PapyrusUtil, .NET runtime, or any Creation Club
content beyond what Skyrim itself ships.

## Runtime policy: DLL-first, no gameplay Papyrus

Runefall gameplay, loot, ranks, inventory mutation, UI and quest-stage handling are implemented in
the native SKSE DLL. Papyrus is limited to the native declaration stub, debug/probe helpers gated
off in release, and an explicitly approved compatibility shim. Polling loops and gameplay logic in
Papyrus are prohibited; `tools/check-papyrus-policy.ps1` enforces this in the static gates
(decision D-064).

## Installation (v1 test build)

1. Install Skyrim 1.6.1170 + SKSE 1.6.1170 + Address Library 1.6.1170.
2. Install SKSE Menu Framework 3.18 (release) over `Data\`.
3. Install the Runefall files:
   - `Data\Runefall.esp` (base ESL)
   - `Data\Runefall - Unique.esp` (ESL, optional)
   - `Data\Runefall - Hunt.esp` (ESL, optional)
   - `Data\Runefall\Data\` (data tables, manifest, ini)
   - `Data\Scripts\RunefallNative.pex`, `RunefallProbe.pex`
   - `Data\SKSE\Plugins\Runefall.dll`
4. Load order: Skyrim.esm and DLC/CC masters first, then `Runefall.esp`, then its packs. All
   Runefall plugins are ESL-flagged; keep the base before the packs.
5. Launch through SKSE. In debug mode the plugin writes `Data\SKSE\Plugins\RunefallProbe.log`
   and `Runefall.log` under `Documents\My Games\Skyrim Special Edition\SKSE`.

### Test-environment install (developer)

    Runefall\tools\setup-test-env.ps1 -TestEnv "J:\SkyrimTest\1.6.1170"

This copies the compiled framework, generated content, Papyrus PEX files and the DLL into the
test environment. It never touches the Steam install or the Mantras MO2 list.

## Build

Prerequisites (Windows): Visual Studio 2022 C++ desktop, CMake, Ninja, vcpkg, .NET 8 SDK, and the
CommonLibSSE-NG template dependency tree under `Runefall/src/plugin/lib` (ignored by git; restore
from https://github.com/codepuncher/CommonLibSSE-NG-template with CommonLibSSE-NG alandtse
v14.0.1; see Runefall/src/plugin/README.md).

    # native plugin
    Runefall\src\plugin> cmake --preset release-windows
    Runefall\src\plugin> cmake --build --preset release-windows

    # content generator (writes Runefall\build\dist)
    dotnet run --project Runefall\tools\Runefall.Build -- --out Runefall\build\dist

    # Papyrus scripts (needs the Steam Papyrus Compiler + original sources)
    Runefall\tools\build-papyrus.ps1

Target runtime: Skyrim SE/AE 1.6.1170 only (D-062). Do not advertise 1.7.x compatibility.