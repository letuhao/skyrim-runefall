# Adaptive Attributes SKSE — Deep Research

**Mod:** Adaptive Attributes SKSE (AAS)  
**Plugin / DLL name:** `AdaptiveAttributesSKSE` (`SKSE/Plugins/AdaptiveAttributesSKSE.dll`)  
**Plugin version (CMake):** 2.1.0 (installed archive labelled 2.0, 2026-10-26)  
**Language / toolchain:** C++23, MSVC, CMake 3.21+, Visual Studio 17 2022, vcpkg `x64-windows-static-md`  
**Runtime support:** Skyrim SE 1.5, AE 1.6/1.7, Skyrim VR (all three required at configure time)  
**Dependencies:** CommonLibSSE-NG (alandtse fork, pinned revision `723cfda…`), fmt, spdlog, SimpleIni, RapidCSV, DirectX TK/Math (declared in `vcpkg.json`)  
**Role:** Native progression engine — turns gameplay activity (movement, combat, casting, OStim scenes, food/sleep) into XP, converts XP into permanent Health/Magicka/Stamina bonuses through milestones with caps, persists via SKSE co-save, exposes native periodic update + settings UI.

**Base (read-only):**  
`D:\Works\source\skyrim-moddings\rpg-mechanism\Adaptive Attributes SKSE 179815 2.0 2026-10-26T13-29Z rvwm2BrSG\AdaptiveAttributeSKSE Source 179815 2.0 2026-10-26T13-28Z s6Og0dsRt\AdaptiveAttributeSKSE_Source`  
**Installed mod (INI + DLL):**  
`D:\Works\source\skyrim-moddings\rpg-mechanism\Adaptive Attributes SKSE 179815 2.0 2026-10-26T13-29Z rvwm2BrSG\Adaptive Attribute SKSE`

No docs/README shipped; behaviour is documented only by source and INI comments.

---

## 1. File layout

```
AdaptiveAttributeSKSE_Source/
  CMakeLists.txt
  CMakePresets.json
  vcpkg.json / vcpkg-configuration.json
  src/
    PCH.h
    Logging.h
    main.cpp
    Plugin.h / Plugin.cpp
    Events/
      EventSinks.h / .cpp
      VRActionEventSink.h / .cpp
    Forms/
      FormRegistry.h / .cpp
    Hooks/
      StaminaHook.h / .cpp          (intentionally inactive V1)
    Papyrus/
      PapyrusBridge.h / .cpp
    Progression/
      TrainingManager.h / .cpp     (core XP/milestone/cap engine)
    Serialization/
      Serialization.h / .cpp       (SKSE co-save, schema v7)
    settings/
      Settings.h / .cpp
      SettingsCatalog.inc
      SettingsEditor.h / .cpp
    Systems/
      NativeUpdateManager.h / .cpp
      MovementTracker.h / .cpp
      HealthTracker.h / .cpp
      StaminaCombatTracker.h / .cpp
      SpellCastTracker.h / .cpp
      CarryWeightManager.h / .cpp
      VanillaLevelUpManager.h / .cpp
      LegacyMigration.h / .cpp
      OStimIntegration.h / .cpp
    ThirdParty/SKSEMenuFramework/
      SKSEMenuFramework.h
    UI/
      SettingsMenu.h / .cpp
      NativeStatus.h / .cpp
```

---

## 2. Build & packaging

- `CMakeLists.txt`: project `AdaptiveAttributesSKSE` v2.1.0; C++23; requires CommonLibSSE-NG supporting Skyrim 1.7 + Address Library V5 (asserted by text scan of `Version.h`/`Interfaces.h`); `find_path(SIMPLEINI_INCLUDE_DIR SimpleIni.h REQUIRED)`.
- All three runtimes (SE/AE/VR) must be ON or configure fails.
- `add_commonlibsse_plugin(... USE_ADDRESS_LIBRARY SOURCES ...)` lists every .cpp (note `Settings/` capitalisation in CMake vs `settings/` on disk — Windows case-insensitive).
- `AAS_PUBLISH` option invokes `scripts/Publish.ps1` (not shipped in source tree here) to deploy DLL + archive.
- Preset `default`: VS17 2022 x64, vcpkg toolchain via `VCPKG_ROOT`, triplet `x64-windows-static-md`.
- `vcpkg.json`: fmt, spdlog, simpleini, rapidcsv, directxtk, directxmath.

---

## 3. Plugin lifecycle (`main.cpp`, `Plugin.h/cpp`)

`SKSEPluginLoad`:
1. `InitializeLogging()` → `AdaptiveAttributesSKSE.log` in SKSE log dir, spdlog info/debug depending on NDEBUG.
2. `SKSE::Init`, requires TaskInterface.
3. Logs runtime version + family (VR / SE 1.5 / AE 1.6 / AE 1.7).
4. `AAS::Plugin::Initialize()`:
   - `Settings::GetSingleton()->Load()` (INI).
   - `TrainingManager::GetSingleton()->Initialize()` (zeroes state).
   - `Serialization::Register()` (SKSE unique ID `'AAST'`).
   - `RegisterMessaging()`.

Messaging (`RegisterMessaging`/`OnSKSEMessage`):
- `kDataLoaded`: `RegisterEventSinks()` — UI menu sink, spell-cast/hit sinks, book-read sink; resolve forms; init Movement/SpellCast/Health/StaminaCombat/VanillaLevelUp/CarryWeight trackers; register VR action sink; install OStim integration; install NativeUpdateManager; register SettingsMenu. Also `FormRegistry::ResolveForms()`.
- `kNewGame`: reset OStim + native updates; invalidate status; register player animation sink; task: `RecheckBaseline`, `LegacyMigration::Prepare`, `NativeUpdateManager::Reset(true)`.
- `kPreLoadGame`: reset native updates; invalidate status.
- `kPostLoadGame`: reset native updates; if load ok: register animation sink; task: `RecheckBaseline`, `ReconcileAppliedBonuses`, `LegacyMigration::Prepare`, `NativeUpdateManager::Reset(true)`.
- `Revert` (serialization) also resets trackers/state.

Threading model:
- Native worker thread (`NativeUpdateManager::_worker`, `std::jthread`) sleeps 250 ms, submits a task to SKSE TaskInterface, guarded by `_taskQueued` CAS. All game mutation happens on game thread via `UpdateOnGameThread`.
- `TrainingManager` protects `_state` with `std::mutex`; OStim defers events to game thread via task queue with generation counter.

---

## 4. Progression system (`Progression/TrainingManager`)

### 4.1 State (`TrainingState`)

```
healthXP/magickaXP/staminaXP (float)
bonusHealth/bonusMagicka/bonusStamina (int)
healthMilestones/magickaMilestones/staminaMilestones (int)
appliedCarryWeightBonus (float)
tomeReadDays (map<FormID,int>)
spilloverHealthAccumulator/staminaAccumulator (float)
appliedSpilloverHealth/stamina (int)
baselineHealth/Magicka/Stamina/CarryWeight (float)
```

### 4.2 XP → milestone conversion (`TryConvertXPToPermanentBonuses`)

- For each attribute in order Health, Magicka, Stamina:
  - while `bonus < maxBonus`:
    - `requiredXP = XpPerAttributePoint * pow(ProgressionGrowthMult, milestones)` (exponential growth, default 10 * 1.2^n).
    - if `xp < requiredXP` break.
    - `pointsGained = min(PointsPerMilestone, maxBonus - bonus)`.
    - `xp -= requiredXP`; `bonus += pointsGained`; `++milestones`.
    - `ApplyPermanentBonus(type, pointsGained)` → `SetBaseActorValue(av, oldBase + points)` (mutates base AV). Notification HUD.
  - Magicka gains trigger spillover: `ApplyMagickaSpillover(magickaPointsGained)` adds fractional Health/Stamina accumulators, applies integer deltas to base AVs when accumulator crosses integer, capped by maxSpillover, with notifications.
  - Stamina milestones trigger `HealthTracker::OnStaminaMilestone()` (health XP from stamina milestone, with food/sleep multipliers).
  - Stamina bonus changes refresh carry weight.
- OStim multiplier applied to Health/Stamina XP in `AddXP` before storage.

### 4.3 Caps

- `maxBonusHealth/Magicka/Stamina` (default 300 in installed INI; 100 in code default).
- `maxSpilloverHealth/Stamina` (default 25).
- Milestone XP cost grows exponentially; effectively soft cap via cost + hard cap via maxBonus.

### 4.4 Baseline tracking & reconciliation

AAS keeps a `baseline*` for Health/Magicka/Stamina/CarryWeight representing the character's non-mod attribute base, so bonuses can be re-derived and external changes absorbed:

- `InitializeBaseline`: reads current base AVs, subtracts expected bonuses, clamps <50 to 100, stores baseline. Skips if already set.
- `RecheckBaseline` (on load/new game): if baseline unset, initialise; else compute drift between current base and `baseline + bonuses`; if drift > 0.5, recalibrate baseline (absorbs external level-up/perk changes). This is the reconciliation hook.
- `ReconcileAppliedBonuses`:
  - If `DisableVanillaLevelUpAttributeBonus` is false (default): absorb any delta between current base and expected into baseline (keeps external/vanilla increases).
  - If true: force base AV back to expected (overwrites external).
  - Then refresh carry weight if something changed.

This is the "reconciliation" pattern: co-save stores bonus + baseline; on load, re-derive expected base, absorb or correct drift.

### 4.5 Carry weight (`CarryWeightManager`)

- Optional: `bEnableCarryWeightFromStaminaBonus` + `fCarryWeightPerStaminaBonus`.
- Baseline captured from current carry weight minus applied bonus.
- On refresh: `newBase = baselineCarryWeight + bonusStamina * perPoint`. Set base AV.
- Race-switch handling: registers `TESSwitchRaceCompleteEvent` sink; schedules 12 delayed checks (500 ms) to repair carry weight after beast-form races; beast races skip.
- Update loop calls `UpdateRaceCheck()`.

### 4.6 Vanilla level-up negation (`VanillaLevelUpManager`)

- If `bDisableVanillaLevelUpAttributeBonus`, arms a pending check when Stats/LevelUp menu closes; 500 ms later, if any base AV equals `expected + 10` (within 0.5), resets to expected (removes vanilla +10). Only affects future level-ups (not retroactive).

### 4.7 Health training (`HealthTracker`)

- `_foodBonusActive` set on food consumption (alchemy item IsFood); `_sleepBonusActive` set on Sleep/Wait menu close.
- On each Stamina milestone: `healthXP += HealthXpPerStaminaMilestone * (1 + foodBonus? + sleepBonus?)`; food/sleep bonuses are additive and consumed.
- Damage-based health XP from `SpellHitEventSink` when player takes unblocked hit: `DamageHealthXp`.
- Health XP flows through `TrainingManager::AddXP(kHealth)`.

### 4.8 Magicka spillover

Magicka milestones grant secondary Health/Stamina via accumulators with ratios and caps (see 4.2).

---

## 5. Native hooks & trackers

### 5.1 `NativeUpdateManager`

- Installed at DataLoaded; starts `std::jthread` worker.
- Worker wakes every 250 ms (`kNativeUpdateInterval`), CAS `_taskQueued`, submits `UpdateOnGameThread` task.
- `UpdateOnGameThread` (game thread): if not game-ready reset state; else VanillaLevelUp check, CarryWeight race check, OStim update, SpellCast magicka sample, movement sample.
- `SampleMovementOnGameThread`: requires player in cell, 3D loaded, not paused; enforces configured interval (0.05–2.0 s), validates delta (min 0.001, max `max(1.0, interval*4)`); resets on invalid gaps; computes sprint/swim flags; calls `MovementTracker::ProcessSample`.
- `Reset(bool gameReady)`: sets game-ready flag, resets OStim scene, magicka tracking, carry weight race check (if not ready), movement state, vanilla pending check.
- `RefreshSettings`: reinitialises movement state (or installs if inactive).

### 5.2 `MovementTracker`

- Planar distance between samples; speed = distance/delta.
- Mounted: `mountedXpPerSecond * delta * armorMult`.
- On ground: if speed < minimumMoveSpeed skip; swimming → swim XP; sprinting → sprint XP; else running XP if configured.
- Armor multiplier: body/hands/feet slots; heavy (1.30) > light (1.10) > clothing (1.00). Heavy takes priority if mixed.
- OStim multiplier not applied here (only health/stamina via AddXP? Actually AddXP applies OStim multiplier for health/stamina — yes MovementTracker calls `TrainingManager::AddXP(kStamina)`, so OStim bonus applies).

### 5.3 `StaminaCombatTracker`

- Animation tags: power attack (`PowerAttack_Start_end`, MCO, BFCO, etc.), bow draw (`bowDrawStart`, `BowDrawStart`, `arrowDraw`), wood chop (`SlaveWoodChopIdle`), mining (`SoundPlay.NPCHumanPickAxe` / `SoundPlay`+payload).
- Power attack XP: base * weapon mult (dagger 0.80, 1H 1.00, 2H 1.30) * armor mult; game-time cooldown.
- Bow draw: base * armor mult; requires bow/crossbow equipped.
- Wood chop / mining: base * armor mult; mining every N swings (default 3).
- VR: `OnVRActionEvent` handles bow draw / weapon swing power attacks via SKSE action events instead of animation tags.
- Cooldowns use `RE::Calendar` hours-passed * 3600.

### 5.4 `SpellCastTracker`

- Magicka XP from:
  - Direct spell cast: `magickaCost * XpPerMagickaSpent` if cost >= minimum.
  - Scripted drain (zero-cost spells like healing): observes magicka drop over ~750 ms window; awards `spent * XpPerMagickaSpent` if spent >= minimum; handles ambiguity (cell/race/max/health changes, other casts).
  - Spell hits: offensive vs hostile target → `XpPerSpellHit`/`XpPerConcentrationHit`; healing vs wounded target.
  - Spell tomes: `SpellTomeXp`, optionally once per tome per day (tracked in `tomeReadDays`).
- Dedup window 300 ms per spell.
- Concentration spells excluded from cast XP (handled via hits).
- Valid spell types: `kSpell`/`kLeveledSpell`.

### 5.5 `StaminaHook` (`Hooks/StaminaHook.cpp`)

- `Install()` is a no-op: "Intentionally inactive for V1. Hybrid Papyrus + native bridge is the stable path." (Legacy note; native path is now primary.)

---

## 6. Event sinks (`Events/EventSinks`, `VRActionEventSink`)

- `MenuOpenCloseEventSink`: Sleep/Wait menu close → `HealthTracker::OnSleepMenuClosed`; Stats/LevelUp menu close → `VanillaLevelUpManager::ArmPendingCheck`.
- `SpellCastEventSink` (TESSpellCastEvent): player casts → if not VR registered, `SpellCastTracker::OnPlayerSpellCast`; food alchemy → `HealthTracker::OnFoodConsumed`.
- `PlayerAnimEventSink` (BSAnimationGraphEvent): stamina combat tracker; BeginCastRight/Left → if equipped spell and not VR, `SpellCastTracker::OnPlayerSpellCast(spell, false)`.
- `BookReadEventSink`: spell-teaching books → `OnSpellTomeRead`.
- `SpellHitEventSink` (TESHitEvent): if player is target: cancel pending drain; blocked hit → stamina block XP; unblocked hit from external cause → health damage XP. If player is cause and spell hit target → `OnSpellHit`.
- `VRActionEventSink`: registers only on VR runtime via `SKSE::GetActionEventSource`; handles spell cast + stamina combat VR actions; sets `_registered` flag used to suppress duplicate desktop paths.

---

## 7. Form registry (`Forms/FormRegistry`)

Resolves vanilla weapon-type keywords by EditorID: `WeapTypeDagger/Sword/WarAxe/Mace/Greatsword/Battleaxe/Bow/Crossbow/Warhammer`. Used for weapon multipliers in stamina combat tracking.

---

## 8. Settings (`settings/Settings`, `SettingsEditor`, `SettingsCatalog.inc`, `UI/SettingsMenu`)

### 8.1 `Settings`

- Singleton; `Load()` reads `Data/SKSE/Plugins/AdaptiveAttributesSKSE.ini` via CSimpleIni.
- Sections: General, OStim, Debug, Compatibility, Stamina, Magicka, Health, Progression.
- Typed getters for every tunable (XP rates, caps, multipliers, toggles, OStim, debug, native interval).
- Defaults shown in code; installed INI overrides many (e.g. max bonus 300, health XP per stamina milestone 10, damage XP 0.5, block XP 0.35, wood/mining XP 0.25).

### 8.2 `SettingsEditor` + catalog

- `Definitions()` returns array from `SettingsCatalog.inc` (bilingual EN/FR metadata, page, group, label, help, kind, initial/min/max/step).
- `Read`/`Save`: SimpleIni round-trip preserving comments/unrelated keys; atomic replace via temp file + backup + `MoveFileExW`; verification re-read.
- `IsValid`: finite + range; integer kinds require whole number.

### 8.3 `SettingsMenu` (SKSE Menu Framework)

- Registers section "Adaptive Attributes" with pages: Status, General, Stamina, Magicka, Health, Progression, OStim, Advanced.
- In-game editing: draft/baseline vectors, save/discard/reload, presets (Fast/Balanced/Slow/Immersive) staging 40+ settings, preview diff table.
- Status page: native progression table (earned/cap, secondary, XP/next milestone, carry weight, native active flag, legacy migration status).
- OStim page: shows active bonus seconds/multiplier, scene-in-progress.
- ApplyRuntime reloads settings, updates OStim, log level, native movement interval, vanilla level-up pending, carry weight.
- Language auto-detect from `sLanguage:General` INI (French).

### 8.4 Presets

Four progression presets (Fast/Balanced/Slow/Immersive) with absolute values for ~45 settings; staged into draft, then saved.

---

## 9. Serialization / co-save (`Serialization`)

- SKSE serialization interface; unique ID `'AAST'`; record type `'STAT'` (state) + `'OSTM'` (OStim bonus).
- Schema version 7 (`kSerializationVersion = 7`).
- Save writes full `TrainingState` + OStim bonus seconds.
- Load supports migration from v1..v7:
  - v1/v2: legacy structs + `UpgradeLegacyMilestones` (milestones = max(0, bonus)).
  - v3/v4: core + tome history; v4 adds spillover.
  - v5: adds baseline (no carry weight).
  - v6: adds carry-weight baseline.
  - v7: full + explicit milestones.
- FormID resolution for tome history via `ResolveFormID`.
- `Revert`: resets native updates, status, training state, OStim.
- Save/load are SKSE co-save callbacks (run around save/load). Data size modest.

---

## 10. Papyrus bridge (`Papyrus/PapyrusBridge`)

Registered functions on script `AdaptiveAttributesSKSE`:
- `SubmitMovementSample(deltaTime, x, y, z)` — legacy Papyrus-driven movement sampling (native now primary).
- `GetBonusHealth/Magicka/Stamina()` — read bonus ints.
- `ProcessPendingLevelUpCheck()` — trigger vanilla level-up negation.

**Note:** `PapyrusBridge::Register` is defined but never called anywhere in the provided source (no `SKSE::GetPapyrusInterface()->Register(...)`). Either dead/legacy code or registration omitted; native path is authoritative. This matters for Runefall which follows DLL-first policy.

---

## 11. VR support

- Runtime detection via `REL::Module::IsVR()`.
- `VRActionEventSink` registers with `SKSE::GetActionEventSource()` only on VR; tracks `_registered` flag.
- VR path suppresses desktop animation-based stamina/spell handling to avoid duplicates; uses action events (power attack, bow draw, spell cast).
- Logging labels runtime family.
- All core trackers work on both; movement sampling uses actor state sprint/swim.

---

## 12. OStim integration (`Systems/OStimIntegration`)

- Detects `OStim.dll` at install; registers `SKSE::ModCallbackEvent` sink if present.
- Listens for `ostim_thread_start`/`ostim_thread_end` (numArg 0); defers to game thread via task queue with `_generation` counter (invalidates on reset).
- Tracks scene duration; on end with duration >= minimum (default 30 s): grants fixed Health/Stamina XP (OStimHealthXP/StaminaXP), then sets bonus multiplier (default 1.25x) for bonus duration (default 600 s, excluding pause menus). Scene rewards excluded from multiplier.
- Bonus seconds persist via `'OSTM'` record; ticked down in `Update()` every native cycle (elapsed 0–1 s guard).
- Multiplier applies to Health/Stamina XP in `TrainingManager::AddXP`.
- Reset scene on new game/load/revert.

---

## 13. Legacy migration (`Systems/LegacyMigration`)

- Detects old plugin `AdaptiveAttributesSKSE.esp` (light or normal).
- If present: finds legacy quest `0xD61`, stops it; removes legacy power `0x002` from player.
- Sets `LegacyMigration::Result` (pluginLoaded, questFound, stopRequested, powerRemoved) surfaced in UI with migration warning.
- Does not purge VM stacks (logged warning).

---

## 14. Compatibility & pitfalls

1. **Base actor-value mutation:** AAS directly edits base Health/Magicka/Stamina/CarryWeight via `SetBaseActorValue`. This conflicts with other attribute mods and is not reversible on uninstall (Runefall explicitly rejects this; uses perks/GLOB). Reconciliation logic tries to absorb external deltas but only when vanilla level-up bonus enabled; otherwise overwrites.
2. **Baseline clamping:** baselines <50 forced to 100; assumes vanilla-ish starting values.
3. **Drift threshold 0.5:** small edits may be ignored or absorbed depending on setting.
4. **OStim thread safety:** generation counter + task deferral; events from worker thread not allowed to touch actors.
5. **Native update thread:** 250 ms worker; `_taskQueued` prevents stacking; exceptions disable native updates (logged).
6. **VR duplicates:** `_registered` flag coordinates desktop vs VR paths.
7. **Papyrus registration absent:** native functions may be unavailable unless registered elsewhere.
8. **Settings path relative:** `Data/SKSE/Plugins/...` relative to CWD (game root) — assumes running from Skyrim root.
9. **No uninstall cleanup:** bonuses persist in save + base AVs; removing mod leaves inflated attributes.
10. **Co-save versioning:** robust migration; OStim record separate.

---

## 15. Relevance to Runefall

Runefall's progression is a 9-attribute system with perk/MGEF/SPEL-driven derived effects, soft/hard caps, co-save TLV, and explicitly **no permanent base actor-value edits**. Adaptive Attributes SKSE is a useful **template for plumbing**, not for effect model.

**Take (adapt):**
- **XP → milestone → cap pipeline** with exponential cost growth (`base * growth^n`) and per-attribute caps/maxBonus. Runefall can reuse the math and the "points per milestone" concept, but apply effects via perks instead of base AVs.
- **Baseline + reconciliation pattern**: store baseline + earned bonus in co-save; on load, recompute expected, absorb or correct drift. Runefall needs this for derived-effect reapplication and for detecting external mod changes.
- **Native update manager**: 250 ms worker + SKSE task queue + game-ready gate + invalid-delta handling. Directly applicable to Runefall's movement/loot/tier sampling and avoids Papyrus polling (aligns with D-006/D-064).
- **Event sink architecture**: menu open/close, spell cast/hit, book read, animation, VR action events — Runefall's HookManager can mirror these for loot/tier triggers.
- **Settings system**: SimpleIni + typed Settings + bilingual catalog + editor with atomic save/backup + presets. Runefall could adopt the editor/preset UX and INI schema validation.
- **Co-save versioning + migration**: AAS's v1→v7 migration is a strong model for Runefall's TLV schema evolution (D-052, D-028).
- **OStim integration**: generation-counter + task deferral + scene-duration gating + persisted bonus. Runefall could reuse for adult-mod hooks if needed (fictional adult content).
- **Legacy migration**: detection + quest stop + power removal pattern.
- **Carry-weight race-switch repair**: delayed checks after beast form; applicable to any base-AV-derived bonus.
- **Vanilla level-up negation**: optional suppression of vanilla +10 attribute bonus; relevant if Runefall wants exclusive progression.

**Do not take (Runefall anti-patterns):**
- Direct base AV edits as the effect mechanism (Runefall uses perks/GLOB; derived effects via PERK/MGEF/SPEL).
- Absorbing external deltas into baseline when vanilla bonuses enabled (Runefall wants deterministic, reversible application).
- Papyrus movement sampling as primary (Runefall is DLL-first).
- `StaminaHook` inactive placeholder (use actual hooks or skip).

**Key contrast for design doc:** AAS mutates base actor values; Runefall's stated invariant is "No permanent base actor-value edits, so uninstall and cross-mod coexistence stay clean" (GAME-DESIGN §4). AAS therefore informs the *plumbing* (XP math, native loop, co-save, settings, reconciliation) but its *effect application* must be replaced by Runefall's perk-based derived bonuses.

---

## 16. Source paths (key files)

- `src/main.cpp` — SKSE entry, logging, runtime detection.
- `src/Plugin.cpp` — init, messaging, event sink registration.
- `src/Progression/TrainingManager.h/.cpp` — XP, milestones, caps, baseline, reconciliation, spillover, carry weight hooks.
- `src/Serialization/Serialization.h/.cpp` — co-save schema v7 + migration.
- `src/Systems/NativeUpdateManager.h/.cpp` — native 250 ms update loop.
- `src/Systems/MovementTracker.h/.cpp`, `StaminaCombatTracker.h/.cpp`, `SpellCastTracker.h/.cpp`, `HealthTracker.h/.cpp` — XP sources.
- `src/Systems/CarryWeightManager.h/.cpp`, `VanillaLevelUpManager.h/.cpp`, `LegacyMigration.h/.cpp`, `OStimIntegration.h/.cpp`.
- `src/Events/EventSinks.h/.cpp`, `Events/VRActionEventSink.h/.cpp`.
- `src/Forms/FormRegistry.h/.cpp`.
- `src/settings/Settings.h/.cpp`, `SettingsCatalog.inc`, `SettingsEditor.h/.cpp`.
- `src/UI/SettingsMenu.h/.cpp`, `UI/NativeStatus.h/.cpp`.
- `src/Papyrus/PapyrusBridge.h/.cpp`.
- `src/Hooks/StaminaHook.h/.cpp` (inactive).
- `CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json`.
- Installed: `SKSE/Plugins/AdaptiveAttributesSKSE.ini`.
- No README/docs shipped.
