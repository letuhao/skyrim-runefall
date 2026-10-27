# Codex of Power NG — Deep Research

> Research artifact for Runefall. Read-only analysis of `CodexOfPowerNG`.  
> Base path (READ-ONLY): `D:\Works\source\skyrim-moddings\rpg-mechanism\Prisma UI 148718 1.5.1 2026-09-20T08-10Z HZCMOy8nO\CodexOfPowerNG`  
> Version analyzed: **1.2.1** (CMake project version).  
> Date: 2026-10-28.

---

## 1. Mod identity

- **Name:** Codex of Power NG (CodexOfPowerNG). New mod line, **not compatible** with Codex of Power / SVCollection; no save migration.
- **Type:** ESP-less SKSE plugin (`SKSE/Plugins/CodexOfPowerNG.dll`) + Prisma UI web overlay view.
- **Game:** Skyrim SE 1.5.97 + AE 1.6+ (CommonLibSSE-NG multi-target, single DLL).
- **Core loop:** Inventory item → `Register` (consumes 1 copy, persists record) → accumulates `Attack / Defense / Utility` **build score** (records + build points) → unlock options → slot into 6 build slots → applies ActorValue effects to player.
- **UI:** Prisma UI (Ultralight) web overlay, hotkey F4 (configurable DIK). No SkyUI/MCM/UIExtensions.
- **Persistence:** SKSE co-save (registered/blocked/notified/rewards/build scores/slots/migration/undo) + JSON settings (`settings.json` defaults + `settings.user.json` override).
- **Requirements:** SKSE, Prisma UI (`PrismaUI.dll`), Address Library, Media Keys Fix SKSE.
- **No ESP/ESM forms.** Pure DLL + view assets.

---

## 2. File layout (counts & groups)

Top-level:
- `src/` — 45 `.cpp` + 9 `.h` (internal)
- `include/CodexOfPowerNG/` — 37 headers (public API)
- `PrismaUI/views/codexofpowerng/` — 1 `index.html` + 19 JS modules + assets
- `SKSE/Plugins/CodexOfPowerNG/` — `settings.json`, `exclude_map.json`, `exclude_user.json`, `variant_map.json`, `lang/{en,ko}.json`
- `docs/` — 118 files: 4 contracts, 41 plans, 3 superpowers plans/specs groups, ~70 release notes
- `tests/` — 52 `.cjs` (Node) + 18 `.cpp` (C++)
- `external/PrismaUI/PrismaUI_API.h` — Prisma UI C API
- `cmake/` toolchains, vcpkg ports/triplets; `.github/workflows/validate.yml`

Source groups (by subsystem):

| Group | Files |
|---|---|
| Core / entry | `main.cpp`, `State.cpp`, `Config.cpp`, `L10n.cpp`, `TaskScheduler.cpp`, `Util.h` |
| Events | `Events.cpp`, `EventsNotifyGate.h` |
| Inventory / Registration | `Inventory.cpp`, `Registration.cpp`, `RegistrationInternal*.cpp`, `RegistrationMaps.cpp`, `RegistrationQuestGuard.cpp`, `RegistrationRules.cpp`, `RegistrationQuickListBuilder.cpp`, `RegistrationQuickRegister.cpp`, `RegistrationStateStore.cpp`, `RegistrationUndo.cpp` |
| Build progression | `BuildProgression.cpp`, `BuildStateStore.cpp`, `BuildOptionCatalog.cpp`, `BuildEffectRuntime.cpp`, `BuildTypes.h` |
| Rewards (legacy) | `Rewards.cpp`, `RewardsSyncEngine.cpp/.h`, `RewardStateStore.cpp`, `RewardCaps.h`, `RewardsResync.h`, `RewardsSyncPolicy.h`, `RewardsSyncRuntime.h` |
| Serialization | `Serialization.cpp`, `SerializationLoad.cpp`, `SerializationSave.cpp`, `SerializationStateStore.cpp`, `SerializationInternal.h`, `SerializationStateStoreOps.h`, `SerializationWriteFlow.h` |
| State stores | `NotifiedStateStore.cpp`, `BuildStateStore.cpp`, `RegistrationStateStore.cpp`, `RewardStateStore.cpp`, `SerializationStateStore.cpp` |
| Prisma UI | `PrismaUIManager.cpp`, `PrismaUIInternal.h`, `PrismaUIViewState.cpp/.h`, `PrismaUIViewWorkers.cpp`, `PrismaUIViewLifecycle.cpp`, `PrismaUITogglePolicy.cpp`, `PrismaUIToggleActions.cpp`, `PrismaUIRequests.cpp`, `PrismaUIRequestOps*.cpp/.h`, `PrismaUISettings*.cpp/.h`, `PrismaUIStatePayload.cpp`, `PrismaUIPayloads*.cpp/.h` |

---

## 3. Architecture overview

```
SKSEPluginLoad (main.cpp)
  ├─ SetupLogging, LoadSettingsFromDisk, L10n::Load
  ├─ Detect legacy SVCollection residue
  ├─ Serialization::Install (co-save callbacks)
  └─ Register SKSE messaging listener
        kPostLoad       → PrismaUIManager::OnPostLoad (acquire Prisma API)
        kInputLoaded    → RegisterInputSink (hotkey)
        kPreLoadGame    → Reset sync schedulers, ForceCleanupForLoadBoundary
        kDataLoaded     → Registration::Warmup (maps)
        kPostLoadGame/kNewGame → OnGameLoaded, Events::Install, ProcessBuildMigrationState,
                                 SyncCurrentBuildEffectsToPlayer, NotifyLegacyResidue
```

- **Single runtime state** `RuntimeState` (mutex-guarded) in `State.cpp`.
- **Thread model:** Main thread (`QueueMainTask` → SKSE TaskInterface `AddTask`) for game object access (inventory, RemoveItem, ActorValue). UI thread (`QueueUITask` → `AddUITask`) for PrismaUI API (Show/Focus/Hide/CallJS/InteropCall).
- **C++ ↔ JS:** `CallJS`/`InteropCall` (C++→JS), `RegisterJSListener` (JS→C++).
- **Persistence:** co-save records per save; global JSON settings.

---

## 4. Subsystem deep dives

### 4.1 Runtime state & threading
- `State.h::RuntimeState`: mutex + registeredItems (FormID→group), blockedItems, notifiedItems, rewardTotals, buildAppliedEffectTotals, attack/defense/utility score & build points (centi), activeBuildSlots[6], buildMigration fields, undoHistory deque, undoNextActionId.
- `TaskScheduler`: SKSE task interface wrapper; test override hook.
- `Util::NowMs` steady clock ms.

### 4.2 Config / settings
- `Config.h::Settings` with fields: toggleKeyCode, uiDisableFocusMenu, uiPauseGame, uiInputScale, uiDestroyOnClose, languageOverride, normalizeRegistration, requireTccDisplayed, protectFavorites, enableLootNotify, legacy reward fields.
- Layered load: `settings.json` (defaults) then `settings.user.json` (override), clamped.
- Atomic save: write `.tmp`, rotate `.bak`, commit rename; repair on load (recover temp/backup).
- `ClampSettings` validates language, input scale 0.5–3.0, rewardEvery, multiplier.
- Save runs on a background `std::thread` worker; in-memory apply immediate; UI feedback queued.

### 4.3 Localization
- `L10n`: loads `lang/{en,ko}.json` based on settings (auto detects game `sLanguage`); dotted path lookup `T(path, fallback)`. Active language reported to UI.

### 4.4 Events / loot notification
- `Events::Install` registers `TESContainerChangedEvent` sink.
- On item added to player: if discoverable + unregistered + not notified → `DebugNotification`. Debounce 5s on load; throttle 750ms; `notifiedItems` dedup. Respects `enableLootNotify`.

### 4.5 Inventory & safe removal
- `Inventory::SelectSafeRemoval`: scans `InventoryEntryData::extraLists`, skips worn/hotkey (favorite) stacks, computes safe count + base count. Never removes worn; optionally protects hotkey items.
- `Registration::BuildQuickRegisterList`: collects eligible unregistered owned items from player inventory changes (fallback to `GetInventory`), sorts by group then name, paginates (total always accurate). Short-lived cache 750ms keyed by generation + settings mask.
- Quest-protected & TCC-gated rows remain visible but blocked (`disabledReason`).

### 4.6 Registration
- `TryRegisterItem(formId)`: validate form, register key, exclusions, quest guard, TCC gate, registered state, discoverability; find inventory entry (with fallback); check quest object; safe removal; `RemoveItem(1, kRemove, extraList)`; verify count decreased; auto-block on failure; insert registered; apply build contribution; sync build effects; push undo record; invalidate cache.
- `IsQuestObject()` pre-check removed (v1.0.5) — large quest mod compat; `RemoveItem` + count compare is real safety.
- Normalization: optional template weapon + variant map collapse.

### 4.7 Registration rules / groups / exclusions
- `RegistrationRules`: hard-intrinsic excludes (keys, lockpicks, gold, dragon claws by name). Group from FormType: Weapon/Ammo→0 Attack; Armor→1 Defense; AlchemyItem→2; Ingredient→3; Book→4; Scroll/SoulGem/Misc→5 Utility-ish (group 5 misc).
- Discovery group 0..5; >5 = not discoverable.
- `RegistrationMaps`: loads `exclude_map.json`, `exclude_user.json`, `exclude_patch_01..32.json` (break on first missing), `variant_map.json` (variant→base). Forms resolved via `TESDataHandler::LookupForm(localID, fileName)`.
- `RegistrationQuestGuard`: TTL-cached (2s) snapshot of base FormIDs referenced by running quest RefAliases (live refs + created objects).
- `RegistrationTccGate`: optional LOTD/Curator's Companion gate: require item displayed in `dbmDisp` list before registration; fail-closed if lists unavailable.

### 4.8 Build progression
- `MakeRegistrationContribution(group, formType)`: discipline from legacy group (0→Attack,1→Defense,2-5→Utility), points from `ResolveBuildPointsForFormType` (Weapon 70, Ammo 20, Armor 30, Alchemy 25, Ingredient 10, Book 8, Scroll 18, SoulGem 30, Misc 5; centi points ×100? Actually points in whole, converted to centi via ×100 in MakeRegistrationContribution? `pointsDeltaCenti = points` where points is whole number like 70; BuildPointCenti scale 100, so 70 = 0.70? Wait `ToBuildPointCenti(wholePoints)=wholePoints*100`. But `ResolveBuildPointsForFormType` returns 70 (not multiplied). Hmm in MakeRegistrationContribution pointsDeltaCenti = static_cast<int>(pointsCenti) = 70. Then Apply adds 70 centi = 0.70 points. Tier threshold 800 centi = 8 points. So ~12 weapons to tier. OK.)
- Apply: increments score (record count) and build points centi per discipline.
- Rollback (undo): decrements; clears active slots that no longer meet unlock threshold.
- Migration: `NormalizeLoadedSnapshot` converts legacy v1 saves (registered items → derived scores/points by form type; legacy rewards flag pending cleanup). `TryFinalizePendingMigration` removes legacy reward totals via cleanup callback and marks complete. Migration notice surfaced to UI.

### 4.9 Build option catalog & effects
- `BuildOptionCatalog`: ~40 slotted options across Attack/Defense/Utility themes (devastation/precision/fury, guard/bastion/resistance, livelihood/exploration/trickery), each with unlock points centi, slot compatibility, effect type, effect key, base magnitude + per-tier, exclusivity.
- 6 slots: Attack1/2, Defense1, Utility1/2, Wildcard1.
- `BuildEffectRuntime::SyncCurrentBuildEffectsToPlayer`: computes desired ActorValue totals from active slots (respecting unlock thresholds, slot compatibility, once-only stacking, bundle expansion), diffs against applied totals, applies delta via `RestoreActorValue(kPermanent)`, persists applied totals. Handles weapon ability refresh.
- `ClampBuildSyncDeltaForActorValue` guards shout recovery mult floor 0.30.

### 4.10 Rewards (legacy compatibility)
- NG no longer grants random rewards for new registrations. Legacy RWDS records persist for migration/cleanup/refund.
- `RewardStateStore`: map ActorValue→total (clamped by caps). Caps defined in `RewardCaps.h` (attack damage mult 1.0, crit 50, resists 85, absorb/reflect 80, speed mult 0.50; skill AVs truncated).
- `Rewards::SyncRewardTotalsToPlayer`: multi-pass (180) sync with streak gating (min 3 missing) to recover missing ActorValue bonuses after load, with migration temp→permanent channel handling, non-converging guards.
- `CleanupLegacyRewardTotals`: subtracts totals, clears state, schedules carry weight resync.
- `RefundRewards`: subtracts all totals, clears state (does not restore items).
- `RollbackRewardDeltas`: undo path for old records.

### 4.11 Undo
- LIFO history (max 10). `TryUndoRegistration(actionId)`: only latest; removes registered, restores item via `AddObjectToContainer`, rolls back build contribution (and legacy reward deltas), syncs effects, invalidates cache. If progression rollback not applied, appends warning suffix.

### 4.12 Serialization (co-save)
- `Serialization::Install`: SetUniqueID('CPNG'), revert/save/load callbacks.
- Records: REGI (registered FormID+group), BLCK, NTFY, RWDS, UNDO (v4), BSCR (v2 with points), BSLT, BMIG, BEFX.
- FormID remapping via `ResolveFormID`. Length-bounded reads, skip unknown. Atomic per-record isolation.
- Revert clears state + cache. Load calls `NormalizeLoadedSnapshot` + `TryFinalizePendingMigration` (cleanup callback = CleanupLegacyRewardTotals).

### 4.13 Prisma UI integration
- View path `codexofpowerng/index.html` via `CreateView`.
- API acquired `kPostLoad` via `RequestPluginAPI(V1)`; re-acquire late if needed.
- Listeners registered on view creation: `copng_*` (log, requestState, requestToggle, requestInventory, requestRegistered, requestRewards, requestBuild, requestUndoList, getSettings, saveSettings, registerItem, requestRegisterBatch, activate/deactivate/swapBuildOption, undoRegisterItem, refundRewards).
- C++→JS: `InteropCall(view, fn, json)` for hot paths (`copng_setState` via SendState? Actually SendJS uses InteropCall), `Invoke`? Contract says Invoke/InteropCall. Implementation uses InteropCall.
- View lifecycle: lazy creation on toggle; DOM-ready callback queues open; focus with delay; close retry loop up to 20 attempts; destroy-on-close or hide; force-hide focus menu/cursor safety.
- Toggle policy: armed after game loaded + 2s grace; debounced 350ms; blocked during main menu/loading/game inactive.
- Payloads: inventory (page/pageSize/total/hasMore/items/sections), registered, rewards, build (disciplines/options/themes/activeSlots/migration), undo, settings, state.
- Pending refresh replay if main-thread queue unavailable.

### 4.14 Build / packaging
- CMake 3.25, C++23, CommonLibSSE-NG flatrim, nlohmann_json, spdlog.
- Static libs: build_catalog, build_state_support, build_progression_support, build_effect_runtime_support, build_request_support.
- Plugin target via `add_commonlibsse_plugin`.
- Install: DLL to `SKSE/Plugins`, config dir, view dir to `PrismaUI/views`.
- Presets: windows/wsl debug/release. Cross-build WSL clang-cl.
- Tests: ctest C++ (opt-in COPNG_BUILD_CPP_TEST_TARGETS) + Node `node --test tests/*.test.cjs`.
- Package: `scripts/package_release.sh` → zip + sha256; `check_release_zip.sh` smoke.

### 4.15 Web UI architecture
- `index.html`: DOM skeleton + CSS + module assembly.
- JS modules (loaded via script tags): keycodes, lang_ui, virtual_tables, ui_wiring, ui_i18n, ui_state, ui_interactions, ui_bootstrap, interop_bridge, ui_build_panel, ui_register_batch_panel, ui_rendering, native_state_bridge, native_bridge_bootstrap, input_shortcuts, input_correction, reward_orbit, lang_ui.
- Tabs: Quick Register, Registered, Undo, Build, Settings.
- Virtualized tables for large inventories; input correction for high DPI; shortcuts.
- i18n via data-i18n attributes + lang_ui dictionaries.

---

## 5. Compatibility / pitfalls

- Legacy residue detection: warns if `MCM/Settings/SVCollection.ini` or keybinds entry exists.
- No save migration from old Codex/SVCollection.
- Combat-locks build mutations.
- High DPI: `uiInputScale` (0.5–3.0) corrects click coordinates.
- Performance mode reduces shadows/hover for cursor smoothness.
- Destroy-on-close recommended to avoid FPS drops.
- Quest object items: no pre-check; removal + count compare; auto-block on failure.
- Large inventories: pagination + virtualization + caching.
- Co-save per-record isolation: one record failure doesn't affect others.
- Thread safety: main vs UI tasks; no synchronous UI ops from main except toggle fallback.
- TCC/LOTD gate fail-closed if lists unavailable.

---

## 6. Relevance to Runefall

Runefall is a Skyrim modding workspace. CodexOfPowerNG is a reference for:

1. **Build/progression system design**
   - Score + build points (centi) dual metric; tier thresholds; option unlock by points; magnitude scaling per tier.
   - Slotted build: 6 slots, compatibility matrix, once-only stacking, bundle effects.
   - Effect application via ActorValue permanent modifiers with diff/sync and persistence of applied totals.

2. **Registration pattern** (item sink)
   - Consume 1 item, persist FormID, undo with item restore + progression rollback.
   - Safe removal with worn/favorite protection; auto-block on failure.
   - Normalization (template/variant maps) for collapsing item variants.

3. **Inventory integration**
   - `GetInventoryChanges` + fallback `GetInventory`; pagination; caching; virtualized UI.
   - Exclusion maps (base + user + patches) resolved by ESM/ESP file + local FormID.

4. **Quest protection** (TTL-cached RefAlias snapshot) — reusable for protecting quest items.

5. **Prisma UI integration blueprint**
   - Full C++↔JS contract, view lifecycle, focus/close retry, high-DPI input correction, settings persistence with atomic files, background save worker.
   - Task scheduler split (main/UI threads).

6. **Persistence blueprint**
   - Co-save record schema with versioning, FormID remapping, per-record isolation, migration normalization.

7. **Compatibility/pitfalls** to avoid in Runefall: legacy residue, combat locks, fail-closed gates, performance mode, destroy-on-close.

---

## 7. Source paths (key files)

- Entry: `src/main.cpp`
- State: `include/CodexOfPowerNG/State.h`, `src/State.cpp`
- Config: `include/CodexOfPowerNG/Config.h`, `src/Config.cpp`
- L10n: `include/CodexOfPowerNG/L10n.h`, `src/L10n.cpp`
- Events: `src/Events.cpp`
- Inventory: `include/CodexOfPowerNG/Inventory.h`, `src/Inventory.cpp`
- Registration: `include/CodexOfPowerNG/Registration.h`, `src/Registration.cpp`, `src/RegistrationInternal*.cpp`, `src/RegistrationMaps.cpp`, `src/RegistrationQuestGuard.cpp`, `src/RegistrationRules.cpp`, `src/RegistrationQuickListBuilder.cpp`, `src/RegistrationQuickRegister.cpp`, `src/RegistrationStateStore.cpp`, `src/RegistrationUndo.cpp`
- Build: `include/CodexOfPowerNG/Build*.h`, `src/Build*.cpp`, `src/BuildOptionCatalog.cpp`, `src/BuildEffectRuntime.cpp`
- Rewards: `include/CodexOfPowerNG/Rewards.h`, `src/Rewards.cpp`, `src/RewardsSyncEngine.cpp`, `src/RewardStateStore.cpp`
- Serialization: `include/CodexOfPowerNG/Serialization.h`, `src/Serialization*.cpp`
- Prisma UI: `src/PrismaUIManager.cpp`, `src/PrismaUI*.cpp`, `src/PrismaUIInternal.h`, `src/PrismaUIPayloads*.cpp`
- Tasks: `include/CodexOfPowerNG/TaskScheduler.h`, `src/TaskScheduler.cpp`
- Web UI: `PrismaUI/views/codexofpowerng/index.html` + JS modules
- Config data: `SKSE/Plugins/CodexOfPowerNG/settings.json`, `exclude_map.json`, `variant_map.json`, `lang/{en,ko}.json`
- Contracts: `docs/contracts/prismaui-js-api.md`, `docs/contracts/legacy-rewards-compatibility.md`
- Plans: `docs/plans/2026-02-03-codexofpowerng-bootstrap.md`, `docs/plans/2026-02-03-codexofpowerng-core.md`
- Build: `CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json`

---

*End of research.*