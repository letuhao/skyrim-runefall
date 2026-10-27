# STATE - Runefall

Last updated: 2026-10-27

## Goal

Build Runefall: a public Skyrim SE/AE mod with four pillars - RPG attribute progression,
Diablo/Torchlight-style loot, enemy tiers (elite / champion / mini-boss / boss), and
crafting with material hunting - unified by a single Rune Rank economy, shipped as a
native SKSE plugin plus ESL content, with maximum compatibility.

## Done

- Design context established (GAME-DESIGN.md, DECISIONS.md, VALIDATION.md, PLAN.md).
- Project scaffold + version baseline 0.2.0-dev; design layers L1-L5 complete.
- Repository initialized: project moved into D:\Works\source\skyrim-moddings\skyrim-runefall
  (GitHub letuhao/skyrim-runefall, branch main); project root is skyrim-runefall\Runefall.
  Third-party toolchain (vcpkg, CommonLibSSE-NG checkout) and build outputs are git-ignored.
- L2 Skyrim mapping drafted: SPECS\L2-SKYRIM-MAPPING.md (attribute channels, native hotkey
  cast model, loot/tier/salvage hooks, per-item metadata, co-save, UI, cut list).
- L1 gameplay model drafted and revised: SPECS\L1-GAMEPLAY-MODEL.md (rune economy,
  attributes, items, loot, enemy tiers, crafting, economy, pacing, L2 open questions).
  Economy is event-driven (E-00..E-07): per encounter / loot event / discovery, never per
  hour; no idle or passive generation.
- Evidence base for reference mods in _context\rpg-mechanism\.
- Actor value coverage note: SPECS\L2A-ACTOR-VALUE-COVERAGE.md (AV model comes from
  Mutagen/Synthesis; no custom dump or engine re-verification kept). Every attribute now has
  a non-combat anchor (L1 P-01/P-11).
- Target environment verified: Mantras of Mara MO2 list (1.6.1170) = release/test target; 1.7.104 deferred (D-062).
- Native toolchain verified: VS2022 Enterprise, MSVC 14.44, CMake, vcpkg, .NET 6-10.
- Content-size audit vs reference mods: SPECS\L3A-CONTENT-SIZE-COMPARISON.md.
- Math-derived content plan: SPECS\L3B-CONTENT-MATH.md (revision to 120 affixes / 600 rows /
  ~182 MGEF; coverage gates D-038/D-039).
- Multi-ESL architecture (base + packs, D-040) and unique-affix gap (L3D).
- DataMiner built and run over the five official masters; unique-affix count derived U=59
  (20 boss + 20 quest + 19 hunt); gaps G1-G12 closed. Mining outputs in SPECS\data\mining.
- General drop table + race hunt tables designed (SPECS\L3E-HUNT-AND-DROP-TABLES.md, D-043/D-044).
- Crafting Table reserved as Runefall - Crafting.esp expansion; v1 is Rune Forge basics + hunting
  primacy (D-045/D-046).
- Future gap register and v1 scope lock: SPECS\GAP-REGISTER-FUTURE.md (32 reserved/watched,
  10 now-hooks N-01..N-10, 8 risks); D-047/D-048.
- L4 runtime architecture drafted: SPECS\L4-RUNTIME.md (D-049..D-055): modules, hook map,
  co-save format, bridge API, UI, budgets, failure modes, Phase 0 checklist.
- L5 compatibility/release drafted: SPECS\L5-COMPAT-RELEASE.md (D-056..D-061): dependencies,
  compatibility passes, FOMOD, intended gates, test plan, release/Nexus policy.
- Test environment: J:\SkyrimTest\1.6.1170 (Stock Game + SKSE 1.6.1170 + AL 1170 + Runefall.dll).
  The 1.7.104 clone was removed after the target was deferred (D-062); no 1.7.104 support claim.
- **Phase 0 runtime verified on 1.6.1170**: SKSE loaded Runefall.dll; log shows plugin load,
  runtime AE 1.6.1170, co-save callbacks, Papyrus functions, kDataLoaded bootstrap complete.
  Evidence in Runefall\evidence\phase0. Game process stopped after test.
- 1.7.104 Steam build verified (1.7.104.0, buildid 24914197) but deferred/unsupported for v1
  (D-062); its clone was removed. Mantras 1.6.1170 is the sole release target.
- Phase 0 build SUCCESS: Runefall.dll 613,376 bytes, SHA-256 00F206D5...32B2A; exports
  SKSEPlugin_Load/Query/Version verified. Scaffold has logging, Papyrus stubs
  (RunefallNative.GetApiVersion/GetPluginVersion), co-save callbacks (RNFL/RNF1) and
  kDataLoaded message. Staged test mod under Runefall\build\phase0-mod. In-game test pending.
- L3 content/data architecture drafted: SPECS\L3-CONTENT-DATA.md (ESL record set, FormID
  ranges, CSV schemas, Mutagen build + validator, module split).
- Cut-list audited: dynamic loot and drop tables confirmed mandatory (dynamic per-instance
  generation; drop tables are data); pre-generated variant fallback removed.
- PROTEUS 3.4.0 analysed as the runtime actor/record editing reference
  (_context\rpg-mechanism\10-PROTEUS.md); L2 section 13 and decisions D-027/D-028 added.

## Strategy (D-063)

Implement v1 fully before manual game tests: static validation + an automated in-DLL probe
harness continuously; batch in-game sessions only at T1 (full probe + scripted walkthrough) and
T2 (fix confirmation). Goal: implement-first, batch-test (session goal, up to 30 rounds).

## Architecture baseline (2026-10-27)

- POC base code is frozen as the starting point for the real mod. `ARCHITECTURE.md` defines the
  layer/dependency rules, item identity, effect data model, hook lifecycle, co-save contract,
  POC debt register and migration phases A-F.
- `FormCatalog` + generator-emitted `form_ids.csv` are the single source of truth for Runefall
  record identity; hard-coded local FormID windows were removed from LootService/Progression and
  the archive verifier now asserts the catalog is packaged.
- Evidence: static gates 6/6 (2026-10-27 23:42), batch 33/33 PASS with 19/19 self-tests
  (2026-10-27 23:43), archive verifier PASS with 554 catalog rows.
- Papyrus avoidance is now a binding rule (D-064): packaged scripts are declaration/probe only,
  debug mutation natives are gated by `bEnableDebugNatives=0` by default, and the static gates
  include `tools/check-papyrus-policy.ps1`.
- Power-ladder scope narrowed to the **ruler only**: `SPECS/POWER-LADDER.md` (100 stages, 10 tiers,
  x15 curve, StageProfile/RequestStage). All other systems are registered in
  `SPECS/DEFERRED-MODULES.md` and must not be added to the ladder. D-065/D-066/D-067 record the
  decisions; ARCHITECTURE rule 10 makes balance data-only.
- F0.1 in progress (2026-10-28): StageEngine full ABI/validation, co-save v3 ladder section,
  old tier keyword retirement, generator validator; batch 20/20 PASS and static gates 6/6.
  F0.1 PASS. F0.2 PASS: 28 JSON schemas, base pack 7 entities, validator and static gate.
- F0.3 PASS: generator reads JSON pack, validates entity files, rejects invalid packs with paths,
  emits runtime tables/ESP; static gates 8/8 and batch 20/20.
- F0.4 PASS: DataStore reads runtime JSON only; F0.5 PASS: co-save v3 migration verified in-game.
- Next: F0.6 host core tests/benchmarks and static-gate wiring.

## Next step

Implementation phase I1 (native core) is in progress:

1. DONE: architecture baseline + FormCatalog (POC freeze; see ARCHITECTURE.md).
2. DONE: Config, DataStore, PackManager, FormResolver, CoSave, DebugHarness, PapyrusBridge,
   modular bootstrap. Build PASS (780,288 bytes, hash 7867ABEB...1ECB); deployed to the test env.
2. DONE (I2 foundation): Runefall.Build generator PASS; 3 ESL plugins (184/40/38 records) +
   18 data CSVs + manifests + ini generated and deployed to J:\SkyrimTest\1.6.1170.
3. BATCH SESSION PASS (2026-10-27): 17/17 core checks, 18/18 self-tests, Draugr first kill
   grants material + unique item, no crash (batch-test-20261027-054059.md). Runtime loot/probe/
   death drops verified; category FormIDs+validator; FOMOD SKSE payload; soft cap/rune units/
   native attunement compile; quest Papyrus scripts compile. Remaining: boss-kill attunement gate,
   synthetic hotkey harness, P3.3-P3.7 pipeline, P4.2/P4.12, P5 release gates.

Already proven and kept as evidence: DLL build, plugin load, Papyrus/co-save registration and
kDataLoaded on J:\SkyrimTest\1.6.1170 (Runefall\evidence\phase0).

## Blocking decisions

1. Public product name (Runefall collides with a 2018-2019 casual game series; alternatives
   Runegrasp / Runemark / Runesurge are unclaimed).
2. UI framework: SKSE Menu Framework vs Prisma UI vs MCM Helper only.

Closed: 1.7.104 support deferred; release target locked to 1.6.1170 (D-062).

## Last known-good version

0.1.0 POC base (runtime-verified; not a release).

## Build / run command

Pending Phase 0 (will be recorded in VALIDATION.md).

## Log path

Pending Phase 0 (planned: Data\SKSE\Plugins\Runefall.log).