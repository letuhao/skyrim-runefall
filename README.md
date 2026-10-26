# skyrim-runefall

Runefall - a Torchlight/Diablo-style four-pillar RPG overhaul for Skyrim SE/AE 1.6.1170.

The project lives in [Runefall/](Runefall/):

- [Runefall/GAME-DESIGN.md](Runefall/GAME-DESIGN.md) - product and gameplay pillars
- [Runefall/SPECS/](Runefall/SPECS/) - layered design specs (L1 gameplay, L2 mapping, L3 content/data, L4 runtime, L5 release) plus the mining/drop-table studies
- [Runefall/src/plugin/](Runefall/src/plugin/) - native CommonLibSSE-NG plugin (C++, CMake + vcpkg)
- [Runefall/tools/DataMiner/](Runefall/tools/DataMiner/) - official-master data miner (C#, Mutagen)
- [Runefall/tools/Runefall.Build/](Runefall/tools/Runefall.Build/) - ESL + CSV content generator and validator (C#, Mutagen)
- [Runefall/SPECS/data/](Runefall/SPECS/data/) - mining and design evidence
- [Runefall/VALIDATION.md](Runefall/VALIDATION.md), [Runefall/STATE.md](Runefall/STATE.md), [Runefall/DECISIONS.md](Runefall/DECISIONS.md) - project state

## Current status

Design layers L1-L5 complete. Phase 0 bootstrapped and runtime-verified on Skyrim 1.6.1170
(plugin load, Papyrus/co-save registration, kDataLoaded). Implementation follows the
implement-first / batch-test strategy (D-063): native core (I1) builds, content generator
(I2 foundation) PASSes and produces three ESL plugins plus data tables.

## Build

Prerequisites (Windows): Visual Studio 2022 with C++ desktop, CMake, Ninja, vcpkg, .NET 8 SDK,
and the CommonLibSSE-NG template dependency tree under Runefall/src/plugin/lib (ignored by git;
restore from https://github.com/codepuncher/CommonLibSSE-NG-template with the pinned
CommonLibSSE-NG alandtse v14.0.1; see Runefall/src/plugin/README.md).

    # native plugin
    Runefall\src\plugin> cmake --preset release-windows
    Runefall\src\plugin> cmake --build --preset release-windows

    # content generator (writes Runefall\build\dist)
    dotnet run --project Runefall\tools\Runefall.Build -- --out Runefall\build\dist

Target runtime: Skyrim SE/AE 1.6.1170 with SKSE 1.6.1170 and Address Library 1.6.1170.
Skyrim 1.7.x is not supported in v1.
