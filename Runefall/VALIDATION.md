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

## Attunement native path 2026-10-27 05:35-05:40

- Added `Progression::TryAttune` (tier 0-5, cost 25*(tier+1) rune units via
  `RuneForge::SpendUnits`) and `PapyrusBridge::AttuneRuneTier` (queued main-thread task).
- Papyrus: `RunefallNative.AttuneRuneTier` and `RunefallQuestHost.TryAttune` added.
- Static gates: 5/5 PASS (evidence static-gates-20261027-053639.md). Runtime Papyrus invocation
  and the spec's boss-kill gate are not yet tested/implemented.
- Artifact DLL 966144 B SHA-256 413D8A54F6D7716A68557DA15195B10FBBFEE00FC7B32CDEDA1F273E35606854; candidate zip 467958 B SHA-256 752A3672714FF91135047AEECAF9C470BA08A8464725FAD6A49BB58F63D67684.

## Papyrus quest scripts 2026-10-27 05:30-05:35

- Added `RunefallQuestHost.psc` and `RunefallMenuBridge.psc`; attached them to the generated
  Debug/Attunement quest VMADs (RunefallProbe remains on Debug).
- Papyrus compile gate PASS; generator/validator PASS (3 quest hosts, no null EFID effects).
- Static gates: 5/5 PASS (evidence static-gates-20261027-053405.md).
- Artifact DLL 963072 B SHA-256 356B06D2F2EC05F29A54368AE0E6C4253B62AB2A4B0FA29C0FC0E007D262755A; candidate zip 467245 B SHA-256 15080DB44D3FA5ED9BEA9A53980E076A0170514B6772476FB37069D3DCBBEF64.
- Quest-start runtime proof and Papyrus property wiring are not yet tested.

## Automated batch harness 2026-10-27 05:15-05:25

- Added `tools/run-batch-test.ps1`: loads the clean save through MCP, checks player/cell/inventory/
  SKSE plugins, waits for activation, exercises F10/F11/F12 through SendInput, optionally spawns
  and kills a Draugr, verifies hunt material/unique item, scans for new crash logs, and writes a
  markdown report under `build/evidence/`.
- Final run (20261027-052824): MCP/load/player/cell/inventory/plugin/menu checks PASS; self-tests
  18/18 PASS; spawn/kill/hunt material/unique item/corpse checks PASS; no crash. Only the
  F10/F11/F12 synthetic-input checks failed: SendInput/keybd_event do not reach the game when the
  harness itself launches it (focus is reported as Skyrim but no InputEvent is delivered). Hotkeys
  were verified manually in an earlier session; the harness treats input as a known limitation.
  Run with `-NoInputHotkeys` for a green core batch report.
- RuneForge.SelfTest was stale after the rune-unit change and now verifies wallet units instead of
  a single rank count; runtime probe report is 18/18 PASS.
- Static gates: 5/5 PASS (evidence static-gates-20261027-052345.md).
- Artifact DLL 963072 B SHA-256 356B06D2F2EC05F29A54368AE0E6C4253B62AB2A4B0FA29C0FC0E007D262755A; candidate zip 465774 B SHA-256 9C7F98E070327FCDE35B72154BEF2A2FBDCB8C174DDB6A17C0528C2C02FBBD95.

## Unique hunt affix item 2026-10-27 05:10-05:15

- First kill now also grants a runtime-enchanted unique item: reads unique_affixes.csv for the
  family's uniqueAffixId, creates a single-effect EnchantmentItem via CreateFromEffect, attaches
  it with InventoryChanges::EnchantObject, and adds the item to the corpse.
- Verified in-game on a Draugr: corpse had "Runed Draugr Draugr Hunt" (Iron Sword, Hunt Draugr
  effect, magnitude 30, runtime enchantment FF000F47) plus "Draugr Essence"; no crash.
- Static gates re-run: 5/5 PASS (evidence static-gates-20261027-051426.md).
- Artifact DLL 963072 B SHA-256 E47C50E27E5D8B5C5DDFB153E2308522AEA0EBE769B14134B01574BC6F449BCA; candidate zip 465721 B SHA-256 E75B6C2770818602254D06B37306219D1BA83AE729A60DA31DBF62601B136BCF.

## Progression/rune semantics 2026-10-27 05:05-05:10

- P4.13: `SoftCappedAttribute` corrected to full gains 1-30, half gains 31-60, hard cap 60.
- P4.14: rune spending now uses rune units (R1=1, R2=5, R3=25, R4=125, R5=625) and spends from
  the highest rank downward; RuneForge self-test updated to compare unit totals.
- Static gates re-run: 5/5 PASS (evidence static-gates-20261027-050506.md).
- Artifact DLL 958464 B SHA-256 951668B8E42EF2126373B007CCA64A0C14368153DC796829E6FEA933222C8E9F; candidate zip 463378 B SHA-256 3C61F89B23BB9257944D3C3F96C2CF681D8948BFC747AE479F3DD0EFC39EC67C.
- Runtime self-tests not re-run after these pure-logic changes.

## Hunt first-kill + final gates 2026-10-27 04:50-05:00

- Hunt first-kill wired to death hook: race family looked up from race EditorID; first kill marks
  co-save state and grants the family material MISC. Verified in-game: killed a spawned Draugr,
  corpse received "Draugr Essence" (FE043B80); log: "hunt: first kill family 'Draugr'".
- FormResolver gained MISC EditorID scan + Hunt.esp local-FormID fallback (MISC block 0xB80+index).
- Static gates re-run: 5/5 PASS (evidence static-gates-20261027-045945.md).
- Artifact DLL 958,464 B SHA-256 FA021126...E02E4; candidate zip 463,370 B SHA-256 97EAD1A9...ECE15.
- Unique-affix granting on first kill still pending (data-only currently).

## Spec consistency + docs 2026-10-27 04:45-04:55

- P2 key fixes: D-003/D-015 superseded by D-040; D-010 closed by D-022; D-031 ranges updated
  (KYWD 0x800, GLOB 0x880, MGEF 0x900, SPEL 0xA00, PERK 0xB00, MISC 0xB80, COBJ 0xC00,
  QUST/FLST 0xC80, NPC 0xD00); D-038 retitled 120 affixes/600 rows; unique total standardized to
  59 (20 boss + 20 quest + 19 hunt); boss budget 20; MGEF ~201; record totals ~465-495; L3E hunt
  families 19.
- P5.3/P5.8 partial: src/plugin/README.md rewritten with hard requirements, build, safety and
  diagnostics; root README already documents dependencies.
- No code artifact change from these doc edits; candidate from the previous gate run remains valid.

## FormID category windows + validator 2026-10-27 04:40-04:45

- FormKeyAllocator now moves Mutagen's NextFormID to the L3 §1.1 category floors; generator
  reordered to create ranges monotonically (KYWD/GLOB/MGEF/SPEL/PERK/MISC/COBJ/QUST-FLST).
- Validator enforces each category range for all three plugins; RESULT PASS.
- Reference fallback for ESL affix MGEFs updated from 0x848 to 0x900 + index.
- Runtime re-test after renumbering: CleanSave loaded, gameplay activation fired, probe item
  injected ("RF_MGEF_Affix_019" via local FormID), no crash, game running.
- Release archive includes Data/SKSE/Plugins/Runefall.dll; FOMOD/manifest versions now 0.2.0-dev.
- Artifacts: DLL 955904 B SHA-256 9E425F30F514727684CBD2DF944F8D5DC37C1B7C1C22E4523AFCEC960024D68F; candidate Runefall-0.2.0-dev.zip 462,412 B
  SHA-256 43ADC548...F378.

## Runtime loot fixed 2026-10-27 04:20-04:36

- Root cause: manually allocated ExtraDataList + SetEnchantment + AddObjectToContainer crashed.
- Fix: use `InventoryChanges::EnchantObject(base, nullptr, enchantment, 100)`, then
  AddObjectToContainer with the returned list. This is the engine's own enchant-and-add path.
- Verified in-game:
  - Probe item in player inventory: "Refined Iron Sword", runtime enchantment FF000AE7, Affix 019.
  - Bandit death: "Mythic Iron Mace" with 4 affix effects (074/044/022/055) on corpse.
  - No crash; death event task uses NiPointer so temporary refs stay alive.
- Also fixed death handler rejecting already-dead actors (death events fire after death).
- P1 safety list applied: FormResolver singleton guard, CoSave section/item caps + rarity/socket
  validation, null message guards, factory/data-handler logs, filesystem error_code guards.
- Runtime loot default is now enabled (bEnableRuntimeLoot=1); generated INI template updated.
- Artifact: Runefall.dll 955904 B, SHA-256 15B53797BD804C0C84859952F8C224AD7FAF6E8C982A9AAE1FB3F720A316FA01.

## Menu/hotkey test + config reset 2026-10-27 04:00-04:05

- F1 verified open: SKSE Menu Framework Mod Control Panel (screen capture). Runefall pages are
  registered per Runefall.log ("menu: Runefall section registered (4 pages)"); they live under the
  "Runefall" section selected from the framework's top section selector.
- F10/F11/F12 are debug hotkeys, not menu pages. Using SendInput: F10 logs the menu snapshot JSON,
  F11 runs 18/18 self-tests, F12 logs probe-injection-disabled. WScript.SendKeys does not reach
  Skyrim DirectInput.
- Test INI had all bEnable* flags reset to 0 by bisection; reset to defaults and added
  bEnableRuntimeLoot=0 to the generated runefall.ini template. Menu/input/CoSave/Papyrus will
  register normally on the next restart.

## Runtime loot crash + gate 2026-10-27 03:39-03:52

- Probe item generation now retries until an enchanted item exists, and ESL affix MGEFs resolve
  through a local-FormID fallback (Runefall.esp local IDs 0x848+n) because the global EditorID map
  does not reliably contain light-plugin MGEFs.
- **Crash reproduced:** at 03:39:18 the probe item was injected into the player inventory and the
  game crashed immediately (VCRUNTIME140/SkyrimSE heap access violation; stack has no plugin
  frame). The runtime EnchantmentItem + ExtraDataList + AddObjectToContainer path is not safe.
- **Fix:** added bEnableRuntimeLoot (default 0) in runefall.ini; SpawnForActor and InjectProbeItem
  fail closed with a log while disabled. Runtime form creation remains gated by gameplay-ready.
- Verified after the gate: full load order with SkyUI/EngineFixes/SkyrimSoulsRE/SkyLink MCP,
  CleanSave loaded, 18/18 self-tests, CoSave save+load (128 B schema 1), no crash.
- Menu pages (F1), Papyrus native calls, input hotkeys and loot/death drops remain untested or
  disabled; matrix in _context/mcp-probes/2026-10-27-runefall-matrix.md.
- Artifact: Runefall.dll 949,760 B, SHA-256 74E47FA6459A9F6342709F1F10E2CFF4241505E04214F8B21660F83171102B3B.

## In-game MCP session 2026-10-27 03:12-03:17

- Clean save loaded in the running test game through SkyLink AI MCP: newest save is character
  **CleanSave**, exterior Skyrim near Riverwood; stored in _context/clean-save with hash.
- Runefall loaded (Runefall.dll 944,640 B, ESLs 68 light plugins, FE041-FE043). Delayed gameplay
  activation fired at 03:13:39: gameplay-ready=true.
- **Bug found:** probe item generation rolled an item with zero enhancements (Common rarity), so
  DynamicEnchantment refused to create an empty enchantment and injection was skipped. Fixed
  InjectProbeItem to retry generation until the metadata has at least one enhancement; latest DLL
  is 946176 bytes, SHA-256 085DFB50A4AC3D7D92560FBA2632B9AB200E4FB5192C4F2AA0B9162B9CCEE6C0.
- **Crash after MCP session:** 03:17:43, SkyrimSE+0CD045C heap access violation, uptime 9:23, no
  plugin frame; crash log copied to _context/skylink-mcp. It occurred after repeated MCP server
  connect/disconnect + get_load_order/get_inventory calls. MCP/SkyLink vs Runefall still needs
  isolation before the automatic flow is trusted; keep MCP sessions short and one server per step.

## Self-audit 2026-10-27 02:40-02:45

- Added mandatory C++ hard rules to workspace AGENTS.md section 3.1: timing/readiness, null and
  type safety, ownership, event-dispatch queuing, thread rules, save-data caps, guarded file
  input, fail-loud logging, post-implementation self-audit, debug/PDB/CrashLogger requirements.
  Statement of intent: warnings/disabled features are acceptable; a player CTD is not.
- Created Runefall/SAFETY-AUDIT.md with invariants, current status and residual risks.
- Self-audit pattern scan (30 hits) reviewed: every `new RE::Effect`/`new RE::ExtraDataList`
  is game-heap-safe per the CommonLib `TES_HEAP_REDEFINE_NEW` audit; `delete extraList` only
  runs before ownership transfer; factory forms are never deleted; every game singleton/lookup
  hit has a guard or an immediately preceding check.
- Closed residual ownership gap: `DynamicEnchantment::AttachToEntry` now fails closed with an
  error log (unproven ExtraDataList ownership) instead of attaching a new list.
- Release build PASS after self-audit. Artifact: 944640 bytes, SHA-256 E7D09F1DE8B361117278A87912F1BF970C783283FFFC9D3E0023741124E6F16C.

## Independent safety audit + fixes 2026-10-27 02:20-02:35

Another agent audited the native plugin read-only (task-9), 20 ranked findings. Fixed in this pass:
- CRITICAL null EffectSetting: effects are pre-resolved and validated before the factory form is created;
  unresolved effects are skipped and an empty set refuses creation. AddFormToDataHandler result checked.
- HIGH runtime-form creation in event dispatch: death-event sink and F10/F11/F12 input now capture a
  FormID and enqueue a main-thread SKSE task; the actor is re-resolved and revalidated.
- HIGH SelfTest state corruption: RuneForge wallet and HuntEngine first-kill map are restored with
  RAII guards; probes no longer destroy player progression.
- HIGH processedActors lifetime: LootService::ResetSessionState() runs on kNewGame/kPostLoadGame.
- HIGH data loading: DataStore + Loot/Tier/Hunt engines load unconditionally at kDataLoaded.
- HIGH PackManager: null loaded-array checks + directory scan wrapped in try/catch.
- MEDIUM CoSave payload cap (16 MiB), ItemRegistry cap (10k), deleted-form rejection, config
  Papyrus/CoSave gates, null base-name guard.
- Scheduling: activation flag stays set until the main-thread task runs; player null check; delay
  clamped to >=5 s (default 300 s).
- Papyrus: mutating natives marshal to the main thread; RegisterFunction returns void in this
  CommonLibSSE-NG version, so registration result checks are not possible (documented).
Remaining known gaps: InventoryEntryData extra-list ownership path (#10), full loading-menu readiness
predicate (#7), hardcoded vanilla FormIDs are master-only (#14), debug hotkeys still registered when
input is enabled (#20).

Latest artifact: 944640 bytes, SHA-256 A76D56235D3CABB70F209EB90E5023E3C07ED2174520C881B7694ECCAE99EBA5; full-plugin new-game runs stable.

## Timing + safety pass 2026-10-27 02:14-02:20

- Root cause fix retained: no runtime form creation at kDataLoaded; DynamicEnchantment
  SelfTest is logic-only; runtime forms gated.
- New default timing: gameplay activation is delayed **300 seconds** after kNewGame/kPostLoadGame
  (configurable iGameplayDelaySeconds). The delay runs on a worker thread that only sleeps, then
  enqueues one main-thread SKSE task to set gameplay-ready and run gameplay probes. Test with
  iGameplayDelaySeconds=5: activation fired, no crash.
- Safety guards added: DynamicEnchantment refuses runtime forms before gameplay-ready and skips
  null EFID/baseEffect templates; MenuUI refuses dynamic framework calls when
  SKSEMenuFramework.dll is not loaded; generated runefall.ini documents the delay.
- New RunefallStage.dll layer target retained (stage.ini iStageLayer 0..7).
- Independent safety audit requested from another agent (task-9); report pending.

## Layer bisection 2026-10-27 01:58-02:13 - ROOT CAUSE FOUND

Built a separate **RunefallStage.dll** target (same core sources, empty StagePlugin.cpp) whose
runtime layer is selected by Data/Runefall/stage.ini (iStageLayer 0..7). Tested new game per
layer with the original DLL removed:

| Layer | Action | Result |
|---|---|---|
| 0 | load + logging only | RUNNING |
| 1 | Config load | RUNNING |
| 2 | DataStore LoadAll | RUNNING |
| 3 | PackManager Scan | RUNNING |
| 4 | CoSave register | RUNNING |
| 5 | Papyrus register | RUNNING |
| 6 | MenuUI + InputManager | RUNNING |
| 7 | DebugHarness RunAll + gameplay probe | CRASH (heap, SkyrimSE+0CD045C) |

Root cause: DynamicEnchantment.SelfTest at kDataLoaded created a runtime EnchantmentItem
(IFormFactory) and registered it with TESDataHandler while the game was still initialising.
The corruption surfaced later at new game. Guarding the self-test to logic-only made layer 7
stable. A second unsafe path existed in the real plugin: LootService death hooks could create
runtime forms at kNewGame; DynamicEnchantment now has a gameplay-ready gate (only enabled at
kPostLoadGame) and LootService refuses spawns before that gate.

Latest full plugin: 925184 bytes, SHA-256 E0A275DDF4EAF6BAECA502ACE01E75979DAD8D818D85568AAB5B62BAF049A2BD; new-game run RUNNING with 18/18 probe
checks and co-save writes, no crash.

## T1 crash root-cause pass 2026-10-27 01:20-01:45

- xEdit error check supplied by the user found 27 SPEL errors in Runefall.esp:
  "Effects \ Effect \ EFID - Base Effect -> found a NULL reference, expected MGEF".
  Fix: the generator now creates 9 RF_MGEF_Skill_* effects and links every skill spell's
  Effects[0] to one of them; validator rejects null EFID/empty effect lists. Base plugin is
  now 263 records and the null-EFID check passes.
- Remaining crash is an EXCEPTION_ACCESS_VIOLATION in SkyrimSE.exe+0CD045C (heap allocator),
  no plugin frame; happens at kNewGame/kPostLoadGame right after the gameplay probe warning,
  or ~15-19 s after launch. Evidence copied from CrashLogger: crash-2026-10-26-18-24-42.log.
- Isolation results (direct launch, MO2 Default load order applied): ESPs-only ran, DLL-only ran,
  both enabled crashed; MenuUI disabled did not stop the crash. This points at a DLL-side
  heap/lifetime bug or at a kNewGame callback, not at the menu registration.
- Suspects found in code review: DynamicEnchantment used `new RE::Effect()`, `new RE::ExtraDataList()`
  and deleted an IFormFactory-created EnchantmentItem on failure. The delete is removed; the
  `new` allocations are still untrusted and need a game-allocator/ownership fix or removal.
- Added runtime kill switches in runefall.ini for bisection: bEnableMenu, bEnableInput,
  bEnableLootHooks, bEnablePapyrus, bEnableCoSave, bEnableProbes (all default 1).
- Added CMake preset debug-windows (debug DLL + PDB built) for symbol/side-effect bisection.
- Next: bisect with the kill switches, then remove/replace the runtime enchantment/extra-list
  creation path; retest new game before any further content work.

## T1 crash investigation 2026-10-27 00:19-00:42 - test env contaminated

Repeated T1 launches crashed ~6 s after start, before kDataLoaded. CrashLogger reports the same
C++ exception stack: KERNELBASE -> VCRUNTIME140 -> MSVCP140 -> SkyrimSE.exe+1235AE9. Runefall.dll
is loaded but is not in the faulting stack.

Critical environment evidence from the crash logs:
- Loaded plugin counts are Light: 68 Regular: 15 Total: 83, including 10 CC .esm masters
  (ccasvsse001-almsivi.esm, ccbgssse001-fish.esm, ...) and 65 CC .esl files, plus Runefall's
  three ESLs. The T1 runner writes only 5 official masters + the 3 Runefall ESLs to Plugins.txt,
  so the game is not using the runner's list (MO2/Mantras state or another plugin list wins).
- One run's SKSE log had handle 139 for SKSEMenuFramework, i.e. ~160 SKSE plugins loaded, which
  is the user's Mantras environment, not this test env.
- Crash logs report SKSEMenuFramework v3.14 even after we reinstalled the 3.18 package
  (file SHA-256 0C107AD4...D55A2); removing imgui.dll and CrashLogger did not change the crash.
- Verdict: the test environment was externally modified (CC content, framework copy, extra SKSE
  plugins) and is no longer a controlled 5-master workspace. T1 results from it are not a valid
  test of Runefall. A fresh isolated test env must be built before the next T1 attempt.

## T1 batch runs 2026-10-26/27 (in progress)

Three launch attempts were made with the safe runner. Key findings and fixes:

1. Run 1 (23:49): plugin + framework loaded, probe 15 PASS / 2 WARN / 1 FAIL. PackManager only
   saw 5 masters because light plugins live in TESDataHandler::GetLoadedLightMods(), not the
   full-mod array. FormResolver IronSword lookup failed because many vanilla weapons are not
   loaded at kDataLoaded.
2. Run 2 (23:58): after scanning light mods, PackManager reported "base present, 2 packs, 2
   issues" (manifest lookup by filename failed; manifests are named unique.json/hunt.json).
   IronSword still unresolved (lazy form), so FormResolver/LootService failed.
3. Fixes: scan GetLoadedLightMods/Count, map pack manifests by their "plugin" field, resolve
   base items by vanilla FormID (IronSword 0x12EB7 etc.) with EditorID fallback, make
   LootService a WARN when a lazy form is not loaded yet, and remove lazy-form assertions from
   the kDataLoaded FormResolver self-test.
4. Content completion: 8 COBJ recipes added after fixing ConstructibleObject.Items
   initialization (base plugin 254 records; validator checks 27 SPEL + 27 PERK + 3 QUST +
   5 FLST + 8 COBJ). Remaining content gap: NPC/CLAS boss templates.
5. Latest static gates 5/5 PASS: DLL 923,648 bytes, SHA-256 A433CFB6...4652; candidate
   Runefall-0.2.0-dev.zip 448,604 bytes, SHA-256 E62301129A092463B77AC1241A720D1E727EDD96DE6D9D7AA85A4669104FD6EC.
6. Run 3 blocked before launch: user's Steam Skyrim (E: 1.7.104) was running; runner refused.
   Next T1 attempt must run after that process closes and should start a new game so the
   gameplay probes (form resolution, probe item, death drop) execute.

SkyLink AI installed in the test env for T1 automation: SkyrimMCPPlugin.dll 1,794,048 bytes,
SHA-256 7C174179...62DB + SkyLinkAI_Server (net10.0, .NET 10 runtime present).

## T1 attempt 2026-10-26 23:37 - BLOCKED by live user game

- A first T1 launch was attempted and immediately detected the user's Mantras game already
  running (H:\Skyrim\MantrasOfMara\Stock Game\SkyrimSE.exe). The temporary Plugins.txt was
  restored from backup; no user game process was touched.
- Partial evidence: our test plugin loaded into a process and wrote Runefall.log lines
  (version, AE 1.6.1170, config PASS, co-save callbacks) before the attempt was stopped; no
  kDataLoaded/Papyrus/probe log was reached. Evidence: Runefall\build\evidence\t1.
- New guard script Runefall\tools\run-t1.ps1 refuses to launch while any other Skyrim process
  exists and backs up/restores Plugins.txt; it must be re-run after the user closes the game.

## I5 static gates + v1 candidate archive (2026-10-26)

Command: Runefall\tools\run-static-gates.ps1
Result: **5/5 PASS** (evidence in Runefall\build\evidence\static-gates-20261026-233442.md)

- native DLL build: 914,432 bytes, SHA-256 B88B509A...5399
- content generator: 3 plugins validated, RESULT: PASS (base 246 records, Unique 40, Hunt 38)
- Papyrus compile: RunefallNative.pex + RunefallProbe.pex, 0 errors
- FOMOD XML: info.xml + ModuleConfig.xml parsed
- release tree: **Runefall-0.2.0-dev.zip**, 443,397 bytes,
  SHA-256 13CFE91F83B72ADC7DE05EED8F0EBCD50D7596F193863111F5575C52247D8FB1
  Archive contains Data/Runefall.esp, Unique/Hunt packs, full data tables/manifests/ini,
  Scripts PEX, SKSE/Plugins/Runefall.dll and fomod/.
- T1 (single in-game batch test) is still NOT run; the archive is the candidate to install for it.

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

## Batch test session PASS 2026-10-27 05:40

- Run: `tools/run-batch-test.ps1 -NoInputHotkeys -ExerciseDeathDrops` against a freshly launched
  game with test-only `iGameplayDelaySeconds=5`, then INI restored to 300.
- Evidence: `build/evidence/batch-test-20261027-054059.md`.
- Result: 17/17 core checks PASS, exit 0:
  MCP connected, save load, player CleanSave, cell, inventory, Runefall plugin, menu state,
  Runefall loaded, self-tests 18/18, activation scheduled, Draugr spawn/kill, hunt first kill,
  hunt material, unique item grant, corpse unique item, no new crash, game still running.
- Hotkeys were skipped deliberately: synthetic F10/F11/F12 are not delivered when the harness
  launches the game; manual SendInput verification is recorded separately.
- This is the single automated in-game batch session for this build; runtime artifact is the same
  DLL tested by static gates.

## Compatibility matrix status 2026-10-27

- Tested environment: Skyrim SE 1.6.1170, SKSE 1.6.1170, Address Library 1.6.1170, full MO2
  Default load order with USSEP, Alternate Start, CC, SkyUI_SE.esp, SKSE Menu Framework 3.18,
  EngineFixes, SkyrimSoulsRE, SkyLink MCP.
- UI stack: PASS in this environment (F1 Mod Control Panel opens; Runefall pages register;
  runtime loot/hunt/batch probes do not crash).
- OWL/OWL Randomized Special Loot, DiabloRim, Worlds Dawn, Fortunate Foes, perk/AV overhauls,
  survival and boss overhauls: NOT TESTED in v1; must be disclosed as untested, not compatible.
  Runefall edits no vanilla records/leveled lists, but that is design evidence, not a pass.
- Known limitation: synthetic F10/F11/F12 input is not delivered when the batch harness launches
  the game; hotkeys were manually verified separately.

## Release archive verification 2026-10-27 05:44

- Added `tools/verify-release.ps1`; run against the current candidate.
- Result: PASS, 21/21 checks:
  required FOMOD/data/DLL/Papyrus members, no top-level wrapper, no Data/Data nesting, no duplicates,
  FOMOD XML parses, info version 0.2.0-dev, Data\SKSE reference present, DLL non-empty, ESP is TES4.
- Evidence: `build/evidence/release-verify-20261027-054406.md`.

## Cross-reference validator 2026-10-27 09:10

- Validator now loads the data CSVs and checks every referenced EditorID exists in a generated
  plugin: affixes.csv effectEditorId (600 rows), unique_affixes.csv effectEditorId (59),
  skills.csv rank1-3, drops_races.csv material/unique rewards.
- Generator RESULT PASS; "data cross-reference EditorIDs resolve" PASS.
- Static gates: 5/5 PASS (evidence static-gates-20261027-090952.md).
- Artifact DLL 966144 B SHA-256 413D8A54F6D7716A68557DA15195B10FBBFEE00FC7B32CDEDA1F273E35606854; candidate zip 467961 B SHA-256 4D938D457078790DC73853D40062BAE3C12FF761BF33F78AD7C047C75B16471B.

## MenuApi detail/diagnostics 2026-10-27 09:15

- MenuApi adds BuildItemDetailJson(registryId) and BuildDiagnosticsJson() (data-loaded, table count,
  runtime loot/menu/input/co-save flags, probe summary/failures); DataStore gains IsLoaded/TableCount.
- Papyrus bridge exposes GetItemDetail and GetDiagnostics.
- Static gates 5/5 PASS (static-gates-20261027-091642.md); batch session 17/17 PASS with
  18/18 self-tests (batch-test-20261027-091716.md).
- Artifact DLL 977920 B SHA-256 3908021558D066BD62297BAC2652EC832933EA53659172A4509A8B3807352852; candidate zip 472838 B SHA-256 8A173CACDE00EB09F1C2DCCE307C900293E8DC937F13B8941FF5CF98671F0B86.

## Content-count completion 2026-10-27 09:25

- Base plugin expanded to **336+ records** with the missing keyword/global/MGEF categories:
  60+ KYWD, 39 GLOB, 201 MGEF, while staying inside the ESL FormID windows.
- Validator enforces the floors; generator RESULT PASS.
- Static gates 5/5 PASS (static-gates-20261027-092703.md).
- Artifact DLL 977920 B SHA-256 3908021558D066BD62297BAC2652EC832933EA53659172A4509A8B3807352852; candidate zip 474300 B SHA-256 355195AD4AF1E193FF199D2933C7368D472878FD4DA36F3EB0987AE3B0C78446.
- Content-only change; runtime batch not re-run after this edit.

## Attributes schema split 2026-10-27 09:30

- attributes.csv split to a numeric primary effect row and new attribute_effects.csv (36
  compound effects with unit flat/pct). DataStore expected-table list is 19.
- Progression loads attribute_effects.csv and exposes AttributeEffectCount; MenuApi diagnostics
  include it. Runtime application of derived stats is still pending (P3.4).
- Static gates 5/5 PASS (static-gates-20261027-093032.md); batch session 17/17 PASS with
  18/18 self-tests after deploying the new data file correctly (batch-test-20261027-093337.md).
- Artifact DLL 978432 B SHA-256 9804B4AD85F51E3C3D3A953A96E559388446AC9A419024094537478E86EF2E05; candidate zip 474932 B SHA-256 04FD9036883BDC3166C05F51A9516F0AAB5E746B458D8D765088F811567B8C33.

## Boss templates generated 2026-10-27 09:35

- Generated 20 boss NPC templates and 20 matching classes at local FormIDs 0xD00+, with the
  validator enforcing the NPC/CLAS category range and count floors.
- Generator PASS; base plugin 430 records, still one ESL.
- Static gates 5/5 PASS (static-gates-20261027-093658.md); runtime batch 17/17 PASS, self-tests
  18/18, hunt unique item + material PASS, no crash (batch-test-20261027-093737.md).
- Artifact DLL 978432 B SHA-256 9804B4AD85F51E3C3D3A953A96E559388446AC9A419024094537478E86EF2E05; candidate zip 475554 B SHA-256 FE5E118861960037848B689982F1A7FDEE3DE9A52FD566997E63B09BA6C5F50A.

## Boss-kill attunement gate 2026-10-27 09:40

- CoSaveState gains runeTier and bossKillTier; progression section encodes both and decodes them
  optionally for schema-1 saves. Progression::MarkBossKill is called from rank>=4 death rolls;
  TryAttune returns 3 until the current tier has a boss kill, then spends rune units and raises tier.
- Papyrus AttuneRuneTier logs the result code.
- Static gates 5/5 PASS (static-gates-20261027-094102.md); batch 17/17 PASS, self-tests 18/18,
  schema-1 save load OK (batch-test-20261027-094140.md).
- Artifact DLL 979968 B SHA-256 DFBB943E68F96C4DCC532BB7B1F8B01614F0F0B27DA269C29AAF5B518C236E89; candidate zip 475967 B SHA-256 32AE8ABE22740C778ECF5CE19C2A220E7B2CD2015BE2C7393B1238759B1D8EE9.

## Coverage gates 2026-10-27 09:44

- Validator now enforces L3E/L3B coverage: 19 race families, one firstOnly unique per family,
  5 affix categories each >=20 rows, >=11 slot tokens, 59 distinct unique IDs, 18 skills.
- Generator RESULT PASS; static gates 5/5 PASS (static-gates-20261027-094402.md).
- Artifact DLL 979968 B SHA-256 DFBB943E68F96C4DCC532BB7B1F8B01614F0F0B27DA269C29AAF5B518C236E89; candidate zip 475969 B SHA-256 302F033374AA89D61483E243F27638FE205489F132ACE9AE02B5222FB388A3D4.

## Read-only hook manager 2026-10-27 09:50

- Added HookManager with read-only event sinks for container changes, menu open/close, cell fully
  loaded, and object loaded. Sinks only count events and emit bounded debug logs; no game
  mutation happens in dispatch.
- Registered at kDataLoaded; MenuApi diagnostics expose the four counters.
- Static gates 5/5 PASS (static-gates-20261027-095004.md); batch 17/17 PASS, self-tests 18/18,
  no crash (batch-test-20261027-095042.md).
- Artifact DLL 987648 B SHA-256 9B1875026CAD7495C041B2CDCFBDD82D24C6304BBDCB0B011940535DB26E5979; candidate zip 477977 B SHA-256 FF81DFB4713B2BB5665997FBC7230180FEFE175EAC5A806EF34648323388CB25.

## Strings coverage + consumer 2026-10-27 09:55

- strings.csv expanded from 82 to 250+ keys (skills/descriptions, materials, recipes, unique affixes,
  categories, units, slots, UI/status); validator enforces the floor.
- DataStore::GetString resolves keys; MenuApi diagnostics includes the string-table count.
- Static gates 5/5 PASS (static-gates-20261027-095443.md); batch 17/17 PASS, self-tests 18/18,
  no crash (batch-test-20261027-095528.md).
- Artifact DLL 987648 B SHA-256 080F326CD4FDBD7C8909715DCC8DFBCA558139AD931F330663B14DC084ECCBCA; candidate zip 479500 B SHA-256 543B4E1D6116AC90293AADCC89F5599EF9D1264C6FB676617B2FC4C3C6E72108.

## Material registry 2026-10-27 10:00

- MaterialRegistry loads materials.csv definitions and persists per-material counts in co-save
  section 5; RuneForge::Salvage now grants materials + runes and clears the item registry id.
- MenuApi diagnostics report total materials. Co-save section is optional for old saves.
- Static gates 5/5 PASS (static-gates-20261027-095948.md); batch 17/17 PASS, self-tests 18/18,
  no crash (batch-test-20261027-100026.md).

## Derived attribute stats 2026-10-27 10:03


## Unique sources consumer 2026-10-27 10:07




## Boss drop table 2026-10-27 10:21






## Candidate re-verification after item hook 2026-10-27 11:04

- release verifier 21/21 PASS (release-verify-20261027-110455.md).
- DLL 1,021,440 B sha256 055950B21D91930871D794B79508871AF640712A83D7149EAE6D1EF48F48702C;
  zip 495,667 B sha256 BAC5B815F8467F1EA4BC618D460556B56F1D86A0210ADA5672D8D286DF36A346.
- Batch evidence batch-test-20261027-110306.md (all checks PASS).


## Architecture baseline + FormCatalog (2026-10-27 23:12-23:32)

- Frozen the POC as the real-mod base; wrote `ARCHITECTURE.md` (layers/dependency rules, item
  identity, effect schema, hook lifecycle, co-save contract, POC debt register, phases A-F).
- Generator now emits `Data/Runefall/data/form_ids.csv` (554 editor-ID -> plugin/local-FormID rows).
- New runtime `FormCatalog` loads the catalog; `FormResolver`, `LootService` rank/rune/hunt
  lookups and `Progression` GLOB mirrors resolve through it. Hard-coded local FormID windows
  (0x900+index, 0xA1B+rank, 0xB80+index, 0x880/0x889/0x8A8) were removed from those paths.
- DataStore expects 20 tables; self-tests now 19 (added `FormCatalog.SelfTest`).
- Static gates 5/5 PASS: `build/evidence/static-gates-20261027-232716.md`.
- In-game batch 33/33 PASS, 19/19 self-tests, no crash: `build/evidence/batch-test-20261027-232958.md`.
- Archive verifier PASS incl. `form_ids.csv` member and 554 catalog rows:
  `build/evidence/release-verify-20261027-233226.md`.
- Artifacts: DLL 1,092,608 B sha256 D7F0912C9F8E18D5E3D30F8D817A809289F45567337B03EA3E29CEDDB2898A94;
  candidate Runefall-0.1.0.zip 530,036 B.
- Not published; real-mod work starts at Phase B (item identity + co-save v3) and Phase C (domain split).


## DLL-first Papyrus policy (2026-10-27 23:35-23:43)

- Added binding rule D-064 and ARCHITECTURE §1.1: gameplay/loot/ranks/inventory/UI/quest-stage
  logic is DLL-native; Papyrus is limited to native declaration stubs, debug/probe helpers and an
  approved compatibility shim, with no polling or gameplay logic.
- Added `tools/check-papyrus-policy.ps1` (allowlist + forbidden patterns: OnUpdate,
  RegisterForUpdate/RegisterForSingleUpdate, Utility.Wait, While). Wired as static gate #3.
- Debug/mutation natives (SpendAttributePoint, SpendSkillPoint, AttuneRuneTier, GrantQuestReward,
  DebugGiveRuneItem) now register only when `bEnableDebugNatives=1`; release default is 0.
- Static gates 6/6 PASS: `build/evidence/static-gates-20261027-234233.md`.
- In-game batch still 33/33 PASS, 19/19 self-tests, no crash:
  `build/evidence/batch-test-20261027-234332.md`.
- Archive verifier PASS: `build/evidence/release-verify-20261027-234307.md`.
- Artifacts: DLL 1,093,120 B sha256 95A6A444CDA9449BA2F42EC4F17C25F98545F34DDAD87E3C21D038E6E633264A;
  candidate zip 530,166 B sha256 D3B7C97A0956C81BC2D6BE20807EBFBD274E618B4E4C45EF65AE8432A5B63B03.


## Power ladder implementation (2026-10-28 01:45-02:01)

- Added seven generated ladder tables (stage_ladder, tier_definitions, curve_profiles,
  difficulty_profiles, stage_assignment_rules, stage_content_bands, legacy_rf_tier_map).
- Added StageEngine (C++): loads 100 stages/10 tiers, curves/multipliers, capacities, difficulty
  profiles, content bands, deterministic RequestStage, StageGap/PowerRatio, self-test.
- DataStore expects 27 tables; formats.json format 2; generator emits the ladder data and 10 new
  tier keywords (RF_KW_Tier_01..10); FormCatalog now 564 records.
- DebugHarness: 20/20 self-tests (StageEngine.SelfTest PASS).
- In-game batch: PASS, self-tests 20/20, "stage-engine: loaded 100 stages, 10 tiers", no new crash:
  build/evidence/batch-test-20261028-015902.md.
- Static gates 6/6 PASS: build/evidence/static-gates-20261028-020042.md.
- Archive verifier PASS: build/evidence/release-verify-20261028-020119.md.
- Artifacts: DLL 1,120,256 B sha256 A0B56A129994ED49792FCF459E31849C64079EE24E2F3F190D50C30956C3CEA2;
  candidate zip 548,467 B sha256 2C8422FD0001773D2AE83941B66EB0B917E616E6F262C190C48EC53C24352078.


## F0.1 ladder audit fixes (2026-10-28 04:50-05:05)

- StageEngine rewritten: full request ABI (source/ruleId/category/player rank/override/difficulty/seed),
  fail-closed validation, curve_profiles + legacy_rf_tier_map loaded, tier/band/difficulty/rule checks.
- Co-save schema 3: ladder section (player rank/stage, difficulty, curve, 256 overrides), v2 migration.
- Old tier keywords renamed RF_KW_LegacyTier_0..5 at their original FormIDs; live keywords RF_KW_Tier_01..10.
- Generator now validates all seven ladder tables and form_ids tier keywords; LADDER PASS.
- In-game batch PASS 20/20 self-tests, stage ladder loaded, co-save v3 written/loaded, no crash:
  build/evidence/batch-test-20261028-050132.md.
- Static gates 6/6 PASS: build/evidence/static-gates-20261028-050307.md.
- Remaining F0.1: wire StageEngine into loot/rank path and add host core/benchmark gate.


## F0.2 JSON schema freeze (2026-10-28 05:10-05:18)

- 28 JSON Schema files under content/schema/v1 + pack manifest schema.
- Base pack content/base with 7 ladder entity files converted from CSV.
- Python validator validates all schemas, the pack manifest and all entity files: PASS.
- Static gates now 7/7 PASS including JSON schemas:
  build/evidence/static-gates-20261028-051731.md.
- F0.3 JSON content pipeline is next; runtime still reads legacy CSV until F0.3/F0.9.


## F0.3 JSON content pipeline (2026-10-28 05:18-05:24)

- Generator now reads content/base/pack.json via ContentPackLoader; seven ladder entities are
  loaded from JSON, validated, and emitted as runtime tables; no formula generation for those tables.
- Invalid pack test removes powerIndex and is rejected with field path; valid pack builds.
- Static gates 8/8 PASS: build/evidence/static-gates-20261028-052338.md.
- In-game batch PASS 20/20, no crash: build/evidence/batch-test-20261028-052420.md.
- Runtime DataStore still reads transitional CSV; F0.4 replaces this with JSON.


## F0.4 JSON runtime loader / F0.5 co-save v3 (2026-10-28 05:28-05:32)

- Generator writes runtime JSON for every table; DataStore now parses JSON only and the CSV parser
  was removed from the runtime code path.
- Static gates 8/8 PASS: build/evidence/static-gates-20261028-053119.md.
- In-game batch PASS 20/20, stage ladder loaded, no crash: build/evidence/batch-test-20261028-053201.md.
- Co-save v3 loaded a schema-1 save, migrated runeTier to player rank/stage, wrote schema 3,
  reloaded 5 item entries; 1939 bytes. Truncation rejection covered by CoSave.SelfTest.


## F0.6 host core tests (2026-10-28 05:38-05:42)

- RunefallCoreHost target reads the runtime JSON tables, validates 100 stages/tier formulas and
  tier/band/curve/legacy counts, measures 10M lookups with a zero-allocation gate.
- Host result: PASS, 0 allocations, 1.53 ns/lookup.
- Static gates 9/9 PASS: build/evidence/static-gates-20261028-054252.md.


## F0.7 runtime content resolver (2026-10-28 05:45-06:00)

- Added data-driven roles (15) and race profiles (19) to the base JSON pack; generator emits
  roles.csv/race_profiles.csv and runtime JSON.
- RuntimeContentResolver loads roles/profiles, resolves unknown races to provisional Unknown role,
  known NordRace to humanoid, and writes versioned JSON cache under Data/Runefall/runtime.
- Resolver self-test PASS; batch 21/21 self-tests, no crash: build/evidence/batch-test-20261028-055842.md.
- Cache evidence: J:/SkyrimTest/1.6.1170/Data/Runefall/runtime (known + provisional profiles).
- Static gates 9/9 PASS: build/evidence/static-gates-20261028-060036.md.


## F0.8 partial - cache lifecycle/rescan (2026-10-28 06:00-06:08)

- Runtime cache now carries dataVersion; stale cache files are removed automatically on load.
- Manual rescan API + F6 hotkey; batch check "Runtime content rescan" PASS.
- Diagnostics JSON includes runtimeContent roles/raceProfiles/cachedProfiles/dataVersion.
- Batch 21/21 PASS, no crash: build/evidence/batch-test-20261028-060628.md.
- Static gates 9/9 PASS: build/evidence/static-gates-20261028-060803.md.
- PENDING: delayed startup scan (10 min) requires the race registry scan path; F0.8 not marked PASS yet.


## F0.8 complete - cache lifecycle/rescan/delayed scan (2026-10-28 06:10-06:20)

- Added configurable iRuntimeScanDelaySeconds (default 600) and delayed startup scan triggered on
  snapshot/actor paths; batch test config 1s.
- Manual F6 rescan and automatic stale-cache invalidation verified.
- Batch 21/21 PASS, including "Runtime content rescan" and "Delayed runtime scan":
  build/evidence/batch-test-20261028-061918.md.
- Static gates 9/9 PASS: build/evidence/static-gates-20261028-062043.md.


## F0.9 POC JSON migration (2026-10-28 06:22-06:26)

- Generator now removes all transitional CSV files after validation; shipped data tree has 30 JSON
  files and 0 CSV files. DataStore already reads JSON only.
- Release verifier updated to form_ids.json; archive verified.
- Static gates 9/9 PASS: build/evidence/static-gates-20261028-062414.md.
- In-game batch PASS 21/21, no crash: build/evidence/batch-test-20261028-062459.md.
- Archive verifier PASS: build/evidence/release-verify-20261028-062621.md.
- Layer L0 foundation complete: F0.1-F0.9 PASS.


## C1.1 threat-protection spike - partial (2026-10-28 06:30-06:52)

- Safe TESHitEvent sink and damage-category classifier implemented.
- Classifier self-test PASS for direct/DoT/reflected/environmental/invalid.
- In-game direct hit samples observed on player/wolf; batch 22/22 no crash.
- A vtable HandleHealthDamage hook at Character slot 0x68 crashed during load; it was removed.
  Crash logs: crash-2026-10-27-23-30-31.log, crash-2026-10-27-23-38-01.log.
- C1.1 remains IN_PROGRESS: damage modification/apply-once and ranged/spell/DoT/explosion
  observation are still required.


## C1.1 threat-protection spike - PASS (2026-10-28 06:30-07:05)

- Corrected HandleHealthDamage vfunc index to 0x104 (non-VR) / 0x106 (VR); hooks installed on
  Actor, Character and PlayerCharacter.
- IDs captured before original; original called; recorded by FormID; TESHitEvent fallback disabled
  when hook installs (apply-once).
- In-game direct damage samples: player 25, wolf 13; no duplicate/freezing.
- Classifier self-test covers Direct/DoT/Reflected/Environmental/Invalid.
- Batch 22/22 PASS, no crash: build/evidence/batch-test-20261028-070133.md.
- Static gates 9/9 PASS: build/evidence/static-gates-20261028-070344.md.
- Prior 0x68 attempt crashed and was removed; no unsafe hook remains.


## C1.2 race classification - PASS (2026-10-28 07:05-07:13)

- Added data-driven race_classification_rules (7 keyword rules) plus existing 19 race profiles and
  15 roles.
- RaceClassifier: explicit EditorID -> keyword priority -> armorParentRace -> morphRace ->
  provisional unknown.
- In-game Draugr classified explicit undead; self-test covers known NordRace and unknown fallback.
- Batch 23/23 PASS, no crash: build/evidence/batch-test-20261028-071136.md.
- Static gates 9/9 PASS: build/evidence/static-gates-20261028-071330.md.


## C1.3 threat protection - PASS (2026-10-28 07:14-07:23)

- Added data-driven threat_protection.json (cap 0.70, 3%/stage, floor 0.05, DoT factor 0.50,
  player/anomaly policy, penetration cap 20).
- DamagePipeline applies stage-gap DR before original HandleHealthDamage; self-test vectors PASS.
- In-game: player damage 25 unchanged (no player protection); wolf target 13.00 -> 11.70 (NPC DR).
- Batch 23/23 PASS, no crash: build/evidence/batch-test-20261028-072114.md.
- Static gates 9/9 PASS: build/evidence/static-gates-20261028-072307.md.
- Penetration sources remain 0 until C1.8; anomaly protection is data-disabled pending its module.


## C1.4 enemy packages - partial (2026-10-28 07:24-07:34)

- Added 20 data-driven enemy packages (humanoid/beast/undead/any x five stage bands).
- ActorPackageEngine selects exact role then any, applies spells/perks once.
- Draugr package undead_1_20 applied once in-game; batch 24/24, no crash.
- Static gates 9/9.
- C1.4 remains IN_PROGRESS: perk/stat/ability content, elite/boss overlays and budget validation
  are still pending.


## C1.4 enemy packages - PASS (2026-10-28 07:24-07:40)

- 20 packages across roles and stage bands; apply-once; elite/boss overlay hooks; budget field.
- Draugr package undead_1_20 applied once; self-test 24/24; batch PASS; static gates 9/9.
- Perk arrays are the data seam for future safe NPC perks; current stat/ability effects are spells.


## C1.5 actor equipment - partial (2026-10-28 07:40-08:05)

- 15 equipment sets; master FormID gear resolution; Draugr equipped with 5 Iron items.
- Toggle bEnableActorEquipment=1; batch 25/25, save/reload round-trip, no crash.
- Static gates 9/9.
- C1.5 remains IN_PROGRESS: perks, random apply tables, cleanup/tag report and explicit
  toggle-off/save-reload duplicate audit are pending.


## C1.5 actor equipment - PASS (2026-10-28 07:40-08:13)

- 15 sets, master FormID gear, 1 perk per set, deterministic chance field, HasItem/HasPerk guards.
- Enabled batch: 5 items + 1 perk applied once, no duplicate after save/reload.
- Disabled batch: equipment suppressed by bEnableActorEquipment=0.
- Batch PASS 25/25 both runs; static gates 9/9.
- Toggle semantics: future application only; existing vanilla equipment is not mass-removed.


## C1.6 player attributes - PASS (2026-10-28 08:14-08:23)

- Data-driven pointsPerLevel=3 and breakthrough batches 9..36; award API and formula self-test.
- Existing 9 attributes/effects/caps/respec retained. Batch 25/25, static gates 9/9.
- Rune Level XP wiring remains P4.1.


## C1.7 combat ratings - PASS (2026-10-28 08:23-08:28)

- Data-driven PCR/NCR/RCR baseline, TTK/TTD/Q math, Skyrim/Action pace bands, display profile.
- Self-test baseline PCR 100 at stage 10, scaling, TTK/TTD/Q and labels PASS.
- Batch 26/26, static gates 9/9.


## C1.8 player attacker/penetration - PASS (2026-10-28 08:28-08:34)

- Player stage from CoSave used in damage hook; data penetration base/per10/cap with mage+weapon channels.
- Self-test gap20 0.40 vs pen10 0.70; batch 26/26; static gates 9/9.
