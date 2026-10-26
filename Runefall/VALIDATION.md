# VALIDATION - Runefall

Evidence discipline: a claim is only PASS with a command output, log line, file hash or
in-game observation recorded here. Structural validation is not runtime confirmation.

## Build gates (per milestone)

- DLL: CMake configure + build for each runtime target present in the single DLL.
  Record: command, compiler version, output path, hash.
- ESL content: generated plugin validated for new FormIDs only, ESL-safe ranges, no vanilla
  overrides, no leveled-list/container/cell edits. Tool: Mutagen validator (to be built in M2).
- Papyrus (if any): compile with PapyrusCompiler from the MO2 Stock Game; record warnings.
- Packaging: FOMOD structure and file list check; archive hash.

Commands: to be filled at M1/M2.

## Runtime gates

- DLL loads on 1.6.1170 test profile; version line in Data\SKSE\Plugins\Runefall.log.
- New game and existing save both load without crash.
- Save -> load -> revert cycle: co-save state rebuilt, no orphaned data.
- Uninstall: remove DLL + ESP, load save, verify no permanent record changes and no crash.
- One loot roll observed: item gets rarity, name and affixes; container contents sane.
- One tier spawn observed: rank keyword/package applied exactly once; no double application.
- Currency: runes gained and spent; counts survive save/load.

## Mining run (2026-10-26)

- Tool: Runefall\tools\DataMiner, built Release (net8.0, Mutagen 0.50.2).
- Command: dotnet DataMiner.dll --data-dir "H:\Skyrim\MantrasOfMara\Stock Game\Data"
  --out Runefall\SPECS\data\mining
- Exit 0. Counts: weapons 3,293; armors 3,835; enchantments 752; NPCs 6,525; quests 2,391;
  races 204. Candidates: HIGH items 1,453 (318 weapons, 1,135 armors); HIGH NPCs 31; MEDIUM
  quests 48; 161 merged race EditorIDs -> 19 qualifying hunt families (>=10 NPC actors).
- Outputs: candidates.csv (1.0 MB), unique-items.json (2.0 MB), unique-enchants.json (271 KB),
  unique-npcs.json (1.25 MB), quests.json (576 KB), MINING-REPORT.md.
- Note: heuristics; boss/quest lists require manual review before acceptance into the Unique
  pack (documented in L3D).

## Phase 0 build evidence (2026-10-26)

- Project: Runefall\\src\\plugin (template-derived CMake + vcpkg, MSVC + Ninja).
- Pinned library: alandtse/CommonLibSSE-NG **v14.0.1** (commit 723cfda480ae, 2026-10-25); this
  is the active fork carrying 1.7.99+ support. CharmedBaryon repo is not the build target.
- vcpkg: submodule pinned to baseline ee12231b20c95013c6638d845d04c91559a1d1ff; packages
  installed with triplet x64-windows-static-md.
- Plugin scaffold: spdlog file log (Runefall.log), Papyrus bridge stubs
  (RunefallNative.GetApiVersion / GetPluginVersion), co-save callbacks registered
  (unique id RNFL, record RNF1), kDataLoaded message.
- Build command: cmake --preset release-windows; cmake --build --preset release-windows.
- Build result: PASS. Configure PASS (vcpkg deps installed; CommonLibSSE-NG v14.0.1 compiled),
  link PASS. Artifact: build/msvc/Runefall.dll, 613,376 bytes,
  SHA-256 00F206D5211AA24851700D132A78095ED34E993FC536160C35E49EF415532B2A.
- Exports verified with dumpbin: SKSEPlugin_Load, SKSEPlugin_Query, SKSEPlugin_Version.
- Compile fixes during Phase 0: SerializationInterface has no Skip (read/discard payload) and
  its load callback is void(SerializationInterface*) with the record version obtained from
  GetNextRecordInfo.
- Runtime status: build/export validated, NOT yet loaded in game (UNTESTED). Staged test mod:
  Runefall\build\phase0-mod\SKSE\Plugins\Runefall.dll.
- Known tooling substitution: houseCARL/Skyrim Forge not installed; Mutagen validator stands in
  for forge plugin-info in the ship gate.

## 1.7.104 Steam build verification - DEFERRED (superseded by D-062)

Kept as environment evidence only. 1.7.104 is **not a v1 target**: SKSE 2.3.1 and Address Library
v13 are Nexus-login gated and the plugin ecosystem is not ready. The 1.7.104 clone was removed.

- Path: E:\SteamLibrary\steamapps\common\Skyrim Special Edition
- SkyrimSE.exe: 37,910,440 bytes, FileVersion/ProductVersion **1.7.104.0** (written 2026-08-27),
  SHA-256 846EFCCF0C1374D71F892907F46549560F2FCB0A75CB87A3EED438BAA0F1402F.
- Steam appmanifest_489830: buildid 24914197, StateFlags 4 (installed), LastUpdated 2026-08-27.
- Data: Skyrim.esm, Update.esm, Dawnguard.esm, HearthFires.esm, Dragonborn.esm present; 10 CC
  ESM masters and 65 ESL files (AE content installed).
- No SKSE loader, no Data\SKSE\Plugins, no Address Library binaries. SKSE site confirms the
  current AE build is **2.3.1 for game 1.7.104** (fetched 2026-10-26); Address Library Format 5
  still required.
- Not pristine: Data contains one non-vanilla plugin, "the great temple 2.esp" (617,413 bytes,
  2026-08-02), and root has CreationKit.exe, Papyrus Compiler and empty Mods/Creations folders.
  Local Plugins.txt lists that one plugin.
- Verdict: the build is indeed 1.7.104.0, but it is **deferred/unsupported for v1 (D-062)**, not a
  pending validation target. No SKSE/AL installation was attempted.

## Test environments built 2026-10-26

### J:\SkyrimTest\1.6.1170 - ready to run

- Built from H:\Skyrim\MantrasOfMara\Stock Game (6.02 GB, 29 files, SkyrimSE.exe 1.6.1170
  37,157,144 bytes, SHA-256 C434208894F07F604B852F29B8EDC3A58C4DE63DE783373733E72B2B73F33BE9).
- SKSE 1.6.1170 installed from the Mantras SKSE mod: skse64_loader.exe (412,160 bytes,
  4DF90470...8CA1) and skse64_1_6_1170.dll (1,173,504 bytes, C9A2C8A8...5D54); 124 SKSE Scripts
  copied to Data\Scripts.
- Address Library 1.6.1170 installed from the Mantras AL mod:
  versionlib-1-6-1170-0.bin (795,129 bytes, C4093C56...D452) and versionlib-1-6-1170-0-1.bin.
- Runefall.dll Phase 0 build installed in Data\SKSE\Plugins (613,376 bytes,
  00F206D5...32B2A).
- Launch script: Launch-Runefall-Test.bat (runs skse64_loader.exe). README-TEST-ENV.txt included.
- Does not modify Mantras MO2, the Steam install or saves. Steam was running.
- **Runtime status: LOADED AND VERIFIED (2026-10-26 20:06).** Launched via skse64_loader from the
  test env; SKSE log: "plugin Runefall.dll (00000001 Runefall 00020000) loaded correctly".
  Runefall.log (392 bytes, SHA-256 5FA129DB...D281A):
    [20:06:48] Runefall v0-2-0-0 loading (api 1)
    [20:06:48] runtime: AE version 1-6-1170-0
    [20:06:48] co-save callbacks registered
    [20:06:53] RunefallNative Papyrus functions registered (api 1)
    [20:06:59] Runefall data loaded: bootstrap complete, API 1
  Evidence copies in Runefall\evidence\phase0\. Game process stopped after verification.
  Not yet tested: Papyrus call from a script, co-save save/load round-trip, hooks, menu page.

### J:\SkyrimTest\1.7.104 - REMOVED (deferred)

The 19.5 GB clone was deleted on 2026-10-26 after the user deferred 1.7.104 support (D-062).
Its exe hash and buildid remain recorded above for reference only. No 1.7.104 test environment
is maintained; the supported test environment is J:\SkyrimTest\1.6.1170.

## I1 native core build (2026-10-26, implement-first)

- New modular DLL build: 780,288 bytes,
  SHA-256 7867ABEB86EAC4890E1A0C47022A8F3640365E067D14E418B6C757F424FE1ECB.
- Modules implemented and compiled: Config (INI), DataStore (CSV formats.json + 18 expected
  tables, strict parser), PackManager (loaded Runefall plugins + pack manifests via nlohmann),
  FormResolver (EditorID lookup, DraugrRace self-test), CoSave (versioned TLV, 5 sections,
  encode/decode/truncation self-tests), DebugHarness (writes RunefallProbe.log with PASS/WARN/FAIL),
  PapyrusBridge (4 native functions), plugin bootstrap.
- Static status: build PASS. Runtime status of this modular build: NOT yet launched (implement-first;
  probe suite is designed to run at T1). The earlier 613 KB build's runtime evidence remains
  recorded above and applies to the bootstrap only.
- Probe log path: Data\SKSE\Plugins\RunefallProbe.log; summary also printed to console and
  Runefall.log.
- Deployed to J:\SkyrimTest\1.6.1170 and runefall\build\phase0-mod for the T1 session.

## I4 Menu Framework pages build (2026-10-26)

- Switched to the framework's **fixed dynamic header** (SKSEMenuFramework.h, 563 KB): it resolves
  AddSectionItem/AddWindow and the ImGui C API from SKSEMenuFramework.dll at runtime; no import
  library and no imgui.lib/imgui.dll dependency in our DLL. Header needs GetModuleHandleW, handled
  with a local macro in MenuUI.cpp (PCH has no UNICODE).
- Pages: Character, Rune Forge, Hunt Codex, Item Detail; registered under "Runefall".
- Artifact: Runefall.dll 914,432 bytes, SHA-256 B88B509A566E156D9B762306A542EA00CDE944C2D294FC398BE3EE143F295399. Build PASS.
- Runtime menu open is a T1 check (F1 → Runefall section).

## I4 Menu Framework pages (first build, superseded)

- MenuUI.cpp compiles against the downloaded SDK (SKSEMenuFramework.h + ImGui 1.90.8 release
  libs); pages: Character, Rune Forge, Hunt Codex, Item Detail. Registered at kDataLoaded via
  SKSEMenuFramework::SetSection("Runefall") + AddSectionItem.
- CMake detects the SDK at RUNEFALL_MENU_SDK and defines RUNEFALL_MENU_FRAMEWORK; otherwise the
  UI compiles out with a warning.
- Artifact: Runefall.dll 913,408 bytes, SHA-256 927CF942...996D. ImGui runtime imgui.dll
  (65B02627...863D) installed in the test env root and SKSE\Plugins.
- Not runtime-tested; T1 must open the Runefall menu (F1 framework toggle) and verify pages.

## Required-mod install in test env (2026-10-26)

- SKSE Menu Framework 3.18 release (Nexus 120352) installed into J:\SkyrimTest\1.6.1170\Data\SKSE:
  SKSEMenuFramework.dll 4,583,936 bytes (SHA-256 0C107AD4...D55A2), fonts, ini, strings, themes.
- Runefall layers re-installed by Runefall\tools\setup-test-env.ps1: generated content, Papyrus
  PEX files and Runefall.dll (908,800 bytes). Framework PDB excluded.
- Repo README now documents the four hard requirements (Skyrim 1.6.1170, SKSE 1.6.1170, Address
  Library 1.6.1170, SKSE Menu Framework 3.18) and installation/load order; "not required" list
  (SkyUI/MCM Helper/JContainers/PapyrusUtil) is explicit.
- Framework DLL load is a T1 check (SKSE log must list SKSEMenuFramework).

## I4 Papyrus scripts build (2026-10-26, implement-first)

- Sources: Runefall/src/papyrus/RunefallNative.psc (native bridge declarations) and
  RunefallProbe.psc (quest probe calling api/menu/attribute/probe functions).
- Compiled with E:\SteamLibrary\steamapps\common\Skyrim Special Edition\Papyrus Compiler\
  PapyrusCompiler.exe, flags TSEV_Papyrus_Flags.flg and original sources from
  Data\Source\Scripts: 0 errors, 0 warnings; RunefallNative.pex 1,323 bytes,
  RunefallProbe.pex 1,432 bytes. Build script: Runefall/tools/build-papyrus.ps1.
- Generator attaches RunefallProbe to RF_QUST_Debug via Quest VMAD; validator PASS.
- PEX files deployed to build/dist/Data/Scripts and the 1.6.1170 test env.
- T1 test command: cqf RunefallProbe RunProbe (console).

## I4 input manager build (2026-10-26, implement-first)

- InputManager: registers BSTEventSink<InputEvent*> on BSInputDeviceManager; F10 prints the
  MenuApi snapshot, F11 reruns the probe harness, F12 injects the probe item. Self-test checks
  device manager availability + registration. Build PASS, DLL 908,800 bytes,
  SHA-256 E956464F3B5A5E54E3E1C9A7E8C6CB2170FECCFD8CF7C532D9EA5365AAB454AB.
- SKSE Menu Framework pages are NOT implemented yet; the hotkeys currently print to log/console.
  Papyrus probe script source + compiled PEX still pending.

## I2 record families build (2026-10-26, implement-first)

- Generator now emits skill records: 27 SPEL (9 active skills x 3 ranks), 27 PERK (9 passive
  skills x 3 ranks), 3 QUST hosts (Core/Debug/Attunement), 5 FLST pools.
- Base plugin grew from 184 to 246 records; validator now checks 27/27/3/5 and PASSes.
- Still pending: COBJ deterministic recipes and NPC/CLAS boss templates (need master FormKey
  resolution for vanilla races/workbenches; next content phase).
- Multi-ESL packs unchanged: Unique 40 records, Hunt 38 records.
- Deployed to J:\SkyrimTest\1.6.1170\Data (T1 load-order enabling still required).

## I4 Papyrus bridge + menu API build (2026-10-26, implement-first)

- Papyrus bridge expanded: GetAttribute, GetAttributePoints, GetSkillPoints, GetRuneTier,
  GetSkillRank, GetRuneCount, GetHuntFirstKill, SpendAttributePoint, SpendSkillPoint,
  GetMenuSnapshot (plus the earlier probe functions).
- MenuApi: compact JSON snapshot (api, tier, points, attributes, runes, item count, hunt families,
  affix count, rank count) with self-test validating keys/braces.
- CoSave now syncs Progression on save/load (Progression::SaveToCoSave on save,
  LoadFromCoSave after a successful co-save decode).
- Artifact: Runefall.dll 903,680 bytes, SHA-256 AFBD033BC5026DF9FC92A09ED752DDF6D54E65A0E7AF562511DE3B5E3C5A600D. Build PASS.

## I3 loot service + death hook build (2026-10-26, implement-first)

- LootService: death event sink (ScriptEventSourceHolder + BSTEventSink<TESDeathEvent>); on a
  non-player death it rolls rank/tier/ilvl/family, builds metadata, creates the runtime
  enchantment, sets an override display name and adds the item to the dying actor's inventory.
  One roll per actor FormID; player deaths ignored.
- Debug/probe path: on kNewGame/kPostLoadGame in debug mode, one probe item (Iron Sword, rolled
  metadata + runtime enchantment) is injected into the player inventory and logged/printed.
- Self-test: checks TESDataHandler + attribute data + vanilla base item resolution.
- Artifact: Runefall.dll 888,320 bytes, SHA-256 C66FB1B5057193A160283382BAFC1598C55F1D68231C09D0368A13E44F2EFF16. Build PASS.
- NOT yet runtime-tested; T1 must verify death drops, probe item, extra-data persistence and
  save/load. This is the first live-flow test target.

## I3 dynamic enchantment build (2026-10-26, implement-first)

- DynamicEnchantment: creates a runtime EnchantmentItem via IFormFactory, fills one Effect per
  enhancement with the affix MGEF template resolved by EditorID, registers it with the data
  handler, and attaches it to a world reference's extra data or an inventory entry's extra data
  list (with optional override display name).
- Self-test creates a runtime enchantment with one effect from RF_MGEF_Affix_000; warns before
  data load, fails if the MGEF is unresolved or the factory is unavailable.
- Artifact: Runefall.dll 869,376 bytes, SHA-256 865DC871A4382B265E9D9A720A8E1CB508E208610A20D29F7C4A8CD7B32FD364. Build PASS.
- NOT yet wired to live loot/container/save flow; that is the next implementation step and the
  first thing the T1 probe must exercise.

## I3 RuneForge build (2026-10-26, implement-first)

- RuneForge transactions: reroll (with optional locked enhancement), rarity upgrade (60 percent),
  socket/extract rune, rune conversion (5 -> 1 next rank), salvage yield; rune wallet spent from
  CoSave state. LootEngine gained RerollEnhancements (keeps rarity/sockets/ilvl, category-unique
  affix selection).
- Self-test covers reroll spend/fill, socket+extract, repeated upgrade until success, conversion
  and salvage yield; wallet restored afterwards.
- Artifact: Runefall.dll 865,280 bytes, SHA-256 51F28C256636C8D884411CF3B01D5B29EE037D7181F70EA3F68F78927BB0BAD0. Build PASS.

## I3 hunt/progression engines build (2026-10-26, implement-first)

- HuntEngine: parses race_groups.csv + drops_races.csv; race->family lookup, reward lookup,
  firstOnly state via co-save hunt flags; self-test covers family mapping, reward types and
  first-kill marking.
- Progression: 9-attribute array, attribute/skill points, skill ranks 0-3, spend/add/respec,
  soft-cap formula (30 full / 31-60 half / block above 60), co-save sync; self-test covers
  accounting and respec.
- Both wired into DebugHarness (Reload + self-tests at probe time).
- Artifact: Runefall.dll 857,600 bytes, SHA-256 83E50D9A9BB14EAA00DF7E4CA939FB9CA7A987799D81E82DC28A1FD0B4956A51. Build PASS.

## I3 loot/tier engines build (2026-10-26, implement-first)

- LootEngine: parses affixes/rarity/rarity_weights/drops_general from DataStore; deterministic
  xorshift RNG; rank-biased rarity selection; one-affix-per-category rule; slot/ilvl filters;
  socket rolls; produces ItemMetadata and registers it. Self-test: same seed = identical item,
  500-roll sweep checks rarity 0-5, enhancementCount == rarity, unique categories, value bounds.
- TierEngine: parses enemy_ranks/spawn_weights; deterministic weighted rank roll; self-test over
  500 seeds x 6 RF tiers.
- Both plugged into DebugHarness (Reload after DataStore load, self-tests run at probe time).
- Artifact: Runefall.dll 840,704 bytes,
  SHA-256 4DEDE109D7C86B710953230840BF080BDD6ED65F9F536E9EE225CE503DD11C16.
- Build PASS; static self-tests compile. Generator unchanged (PASS).

## I3 item metadata core build (2026-10-26, implement-first)

- New module: ItemRegistry (metadata: registryId, base form, rarity, ilvl, sockets, up to 5
  enhancements with affix/effect ids, tags). Self-test covers insert/find/id uniqueness/clear.
- CoSave extended with section 3 (item registry): u32 count + per-entry metadata (strings,
  floats, sockets, tags). Save pulls ItemRegistry; load rebuilds it; revert clears it; co-save
  self-test now round-trips one item entry.
- Artifact: Runefall.dll 797,696 bytes,
  SHA-256 BDC4822952E31E4F113256D6C2F82EBEB56F26F6AA6614F163B461DC2017E38A.
- Build PASS; static self-tests compile (probe runs at T1). Generator output unchanged (PASS).

## I2 content generator build (2026-10-26, implement-first)

- Tool: Runefall\tools\Runefall.Build (C#, net8.0, Mutagen.Bethesda.Skyrim 0.50.2).
- Command: dotnet Runefall.Build.dll --out Runefall\build\dist --deploy J:\SkyrimTest\1.6.1170\Data
- Generator result: PASS. Plugins written and re-opened with Mutagen:
  - Runefall.esp: 184 records (18 KYWD, 29 GLOB, 17 MISC, 120 MGEF), ESL=true, master Skyrim
  - Runefall - Unique.esp: 40 MGEF, ESL=true
  - Runefall - Hunt.esp: 19 MGEF + 19 MISC, ESL=true
  - all FormIDs inside 0x800-0xFFF; Skyrim.esm master retained.
- Data files: 18 CSVs + formats.json with 600 affix rows, 59 unique rows, 59 unique-source rows,
  19 hunt families, 38 race-reward rows, 36 general-drop rows, 31 material rows, 8 recipes,
  18 skills, 9 attributes, 82 string keys; two pack manifests; runefall.ini.
- Outputs: Runefall\build\dist (BUILD-REPORT.md), deployed to the 1.6.1170 test env.
- WARN (next phase): SPEL/PERK skill records, COBJ/QUST/NPC records not generated yet.

## Test policy - implement first, batch test (D-063)

- During implementation, every build runs the static gates (compile, data validator, plugin
  header audit, Papyrus compile) and the in-DLL probe harness self-checks.
- No per-feature manual game testing. Manual sessions are batched: T1 runs the full probe suite
  and a scripted walkthrough in one launch; T2 reruns after fixes only if needed.
- Phase 0 runtime evidence on 1.6.1170 (plugin load, Papyrus/co-save registration, kDataLoaded)
  remains valid and is not re-tested feature by feature.

## L4 runtime budgets (to measure in Phase 0)

- Data load under 100 ms; loot hook p99 0.5 ms; kill hook 0.5 ms; spawn hook 0.2 ms; cell attach
  2 ms; save write 5 ms; load/revert 20 ms; co-save under 1 KB per 100 tracked items.
- No per-frame gameplay work. Phase 0 checklist: SPECS\L4-RUNTIME.md section 14 (12 items).

## Compatibility suite

| Target | What to test | Status |
|---|---|---|
| Copied Mantras profile (1.6.1170), 3,753 plugins | load, loot, tiers, UI, save/load | pending |
| Minimal profile (SKSE + base only) | core works without optional mods | pending |
| OWL + Randomized Special Loot | additive mode, no double loot | pending |
| Enemy overhaul/boss mods in list | rank stacking caps, density | pending |
| DiabloRim coexistence (optional) | both engines, patch disables one loot path | pending |
| No SkyPatcher installed | core works, patches skipped cleanly | pending |
| 1.7.x | NOT a v1 target; deferred until the ecosystem supports it (D-062) | later |

## Performance gates

- No per-frame work; hooks measured at loot/spawn time only.
- Record hook timing and object counts under a stress scenario (dense dungeon + many NPCs).

## Regression and release gates

- Re-run the full suite on the release candidate; record hashes.
- Ship gate: load-order docs, dependency list, uninstall note, supported-runtime statement (1.6.1170; 1.7.x unsupported).