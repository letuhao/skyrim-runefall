# Runefall - Development guide

Audience: contributors and developer/test environments. Player-facing install and
requirements live in [README.md](README.md).

This project is pre-release and runs on the audited-slice discipline of
[PROGRAM.md](Runefall/PROGRAM.md): no publish, upload, remote push or destructive
external action without authorization; completion claims require audit artifacts.

## Build prerequisites (Windows)

- Visual Studio 2022 C++ desktop workload, CMake, Ninja, vcpkg, .NET 8 SDK.
- The CommonLibSSE-NG template dependency tree under `Runefall/src/plugin/lib`
  (git-ignored; restore from https://github.com/codepuncher/CommonLibSSE-NG-template
  with CommonLibSSE-NG alandtse v14.0.1; see `Runefall/src/plugin/README.md`).

## Build commands

Native plugin:

    Runefall\src\plugin> cmake --preset release-windows
    Runefall\src\plugin> cmake --build --preset release-windows

Content generator (writes `Runefall\build\dist`; deterministic - same inputs, same bytes):

    dotnet run --project Runefall\tools\Runefall.Build -- --out Runefall\build\dist

Papyrus scripts (needs the Steam Papyrus Compiler + original sources):

    Runefall\tools\build-papyrus.ps1

Build proof rule: run the native build in the foreground, capture the exit code and
the last build lines, and confirm the produced DLL exists and is newer than the
edited sources before any test or runtime claim.

## Test environment

    Runefall\tools\setup-test-env.ps1 -TestEnv "J:\SkyrimTest\1.6.1170"

Copies the compiled framework, generated content, Papyrus PEX files and the DLL into
the test environment. It never touches the Steam install or any MO2 list. Target
runtime is Skyrim SE/AE 1.6.1170 only (D-062); 1.7.x is deferred, not supported.

## Content authoring rules (v0.2 program)

- JSON-only authored and runtime content; no CSV authoring.
- Stable content keys; no authored FormIDs except explicit master/unique references.
- Fail-closed data handling: malformed or unknown data is dropped with a log, never a
  crash; missing entries degrade to documented defaults.
- Every coverage and range violation is a build error from the Runefall.Build
  validator, not a review opinion.

## Runtime policy (D-064, DLL-first)

All gameplay, loot, rank, inventory mutation, UI and quest-stage logic lives in the
native SKSE DLL. Papyrus is restricted to the native declaration stub
(`RunefallNative`) and debug/probe scripts gated off in release. Polling loops and
gameplay logic in Papyrus are prohibited and enforced by the static gate:

    Runefall\tools\check-papyrus-policy.ps1

## Module map (ladder-first architecture)

One core, ten consumer modules. No module duplicates ladder tables; consumers call the
ladder API with their own data (full contracts: `Runefall/SPECS/DEFERRED-MODULES.md`).

1. POWER-LADDER (core: 100 stages, 10 tiers, curves, capacities, content bands,
   difficulty profiles, stage assignment)
2. THREAT-PROTECTION (damage categories, protection/penetration)
3. RACE-CLASSIFICATION (HOST / ANOMALY / PROVISIONAL tags)
4. LEVEL-DESIGN / ENCOUNTERS (world bands, spawn tables, placement, caps)
5. ENEMY-ABILITIES (per-tier ability packages)
6. ANOMALY-LADDER (anomaly curves, families, rewards)
7. EQUIPMENT / ACTOR-ENHANCEMENT (gear and perk distribution, enhancement tables)
8. PLAYER-PROGRESSION (Rune Rank 1-100 economy, attributes, skill trees, respec)
9. CRAFTING-MATERIALS (materials, recipes, stations, Monster Hunter gear trees)
10. UI-REDESIGN (Anomaly Codex, character pages, tooltips - read-only snapshots)
11. COMPATIBILITY / RELEASE (load-order tests, uninstall, packaging, versioning)

## Performance budgets

Data load 100 ms; loot 0.5 ms; kill 0.5 ms; spawn 0.2 ms; cell attach 2 ms; save 5 ms;
load 20 ms; co-save under 1 KB per 100 tracked items. No per-frame gameplay work;
logging is rate-limited. Loot/rank/crafting logic runs on the game thread; file
parsing happens at data load (D-055).

## Debug tooling

Debug builds carry PDBs and run with CrashLogger beside the test DLL. In debug mode:
`Data\SKSE\Plugins\RunefallProbe.log` (automated PASS/FAIL probe suite) and
`Runefall.log` under `Documents\My Games\Skyrim Special Edition\SKSE`.

## Where the truth lives

- [Runefall/PROGRAM.md](Runefall/PROGRAM.md) - active slice program (S0 PASS; S1 co-save
  correctness onward, S0-S15 to release)
- [Runefall/STATE.md](Runefall/STATE.md) - current status and proven evidence
- [Runefall/DECISIONS.md](Runefall/DECISIONS.md) - D-001..D-068 architecture decisions
- [Runefall/VALIDATION.md](Runefall/VALIDATION.md) - audit and build evidence
- [Runefall/ARCHITECTURE.md](Runefall/ARCHITECTURE.md) - architecture baseline
- [Runefall/SPECS/POWER-LADDER.md](Runefall/SPECS/POWER-LADDER.md) - ladder and balance truth
- [Runefall/SPECS/DEFERRED-MODULES.md](Runefall/SPECS/DEFERRED-MODULES.md) - module contracts

## License

GPL-2.0 - see [LICENSE](LICENSE).
