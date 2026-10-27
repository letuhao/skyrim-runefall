# Frameworks & Infra — Research Notes (Runefall)

Scope: infrastructure/framework reference mods under  
`D:\Works\source\skyrim-moddings\rpg-mechanism\` (read-only).  
This file mirrors the "frameworks-infra" topic in `research/README.md`.  
Sources are read-only; paths below point back into `rpg-mechanism\`.

Runefall context: native DLL + ESL architecture, DLL-first gameplay, versioned co-save,
runtime form creation, hooks/task queue, compatibility, packaging/FOMOD. See
`Runefall/ARCHITECTURE.md` and `DECISIONS.md`.

---

## 1. Behavior Data Injector (BDI) — Nexus 78146, v0.13

**Source path**
- `rpg-mechanism\Behavior Data Injector-78146-v0-13-1667564855\Behavior Data Injector\`
- `rpg-mechanism\Behavior Data Injector-78146-v0-13-1667564855.7z`

**Files**
- `SKSE\Plugins\BehaviorDataInjector.dll`
- `SKSE\Plugins\BehaviorDataInjector.ini`
- `SKSE\Plugins\BehaviorDataInjector\Test_BDI.json`

**Architecture**
- SKSE plugin that injects data into Skyrim's behavior graphs (animation/behavior XML)
  without requiring Nemesis/Pandora recompilation at runtime.
- Data is declared in JSON with three keys per entry:
  - `projectPath` — behavior project path (e.g. `Actors`, `actors\\Character`, `Actors\\Horse`).
  - `type` — value type: `kInt`, `kBool`, `kFloat`, `kEvent`.
  - `name` — variable/event name injected into the behavior graph.
  - `value` — initial value (omitted for `kEvent`).
- `Test_BDI.json` example injects `MaxsuTestInt` (kInt), `MaxsuTestBool` (kBool),
  `MaxsuTestFloat` (kFloat) and `MaxsuTestEvent` (kEvent) into Actor/Character/Horse graphs.
- Debug toggle in `BehaviorDataInjector.ini`: `EnableDebugLog`.

**Config**
```ini
[Debug]
EnableDebugLog = false
```

**Relevance to Runefall**
- If Runefall adds custom animation states (e.g. rune forging, attunement), BDI-style JSON
  injection is the "no Nemesis rebuild" route for behavior variables/events.
- Pattern: ship a small JSON manifest alongside the DLL and let an SKSE plugin apply it at
  load; keep data-driven and versioned. Complements OAR/DAR condition files rather than
  replacing them.
- Not a gameplay runtime dependency; treat as optional animation infra.

---

## 2. Payload Interpreter (PIE) — Nexus 65089, v1.0.x (Nemesis-less patch)

**Source path**
- `rpg-mechanism\Payload Interpreter - Nemesis Less Patch-78146-v1-0-1667564948\`
- `rpg-mechanism\Payload Interpreter - Nemesis Less Patch-78146-v1-0-1667564948.7z`

**Files**
- `Payload Interpreter-BDI Patch\meta.ini` (Nexus/MO2 metadata, modid 65089)
- `Payload Interpreter-BDI Patch\SKSE\Plugins\BehaviorDataInjector\PayloadFramework_BDI.json`

**Architecture**
- A BDI *patch* that registers a single behavior event named `PIE` on the `Actors` graph:
```json
[ { "projectPath": "Actors", "type": "kEvent", "name": "PIE" } ]
```
- "Nemesis-less" approach: instead of generating a full Nemesis output, injects a minimal
  event hook (`PIE`) into the base behavior graph and interprets payloads at runtime.
- GitHub: `D7ry/PayloadInterpreter`, docs on `D7ry/PayloadFramework`. Depends on DKUtil and
  CommonLibSSE (clib).

**Relevance to Runefall**
- Model for a *thin* behavior hook: inject one event, interpret payloads in the DLL. Useful if
  Runefall needs to trigger animation/behavior side effects from C++ without shipping a
  generated behavior output.
- Keep as optional compat layer; not required for core loop (DLL-first).

---

## 3. Dynamic Persistent Forms — Nexus 116001, v2.21

**Source path**
- `rpg-mechanism\Dynamic Persistent Forms-116001-2-21-1714046026\`
- `rpg-mechanism\Dynamic Persistent Forms-116001-2-21-1714046026.7z`

**Files**
- `Dynamic Persistent Forms.esp`
- `Scripts\DynamicPersistentForms.pex`
- `SKSE\Plugins\DynamicPersistentForms.dll`
- `Source\Scripts\DynamicPersistentForms.psc` (native declarations only)

**Architecture**
- SKSE plugin exposing a Papyrus native API for *runtime form creation and co-save persistence*.
- Core API (from `DynamicPersistentForms.psc`):
  - `Form Create(Form item)` — clone a base form; mutations persist in save game.
  - `Dispose(Form item)` — release a created form.
  - `Track(Form item)` / `UnTrack(Form item)` — persist mutations on a non-created form.
  - Mutation helpers: `AddMagicEffect`, `ClearMagicEffects`, `CopyMagicEffects`,
    `CopyAppearance`, `SetSpellTomeSpell`, spell/enchantment overrides
    (`SetSpellAutoCalculate`, `SetSpellCostOverride`, `SetSpellChargeTime`,
    `SetSpellCastDuration`, `SetSpellRange`, `SetSpellCastingPerk`,
    `SetEnchantmentAutoCalculate`, `SetEnchantmentChargeOverride`, etc.),
    ammo (`SetAmmoDamage`, `SetAmmoProjectile`), soul gems
    (`SetSoulGemCapacity`, `SetSoulGemCurrentSoul`, `LinkSoulGems`).
- Pattern: Papyrus is only a thin native-declaration surface; all heavy logic is in the DLL.

**Relevance to Runefall**
- Direct precedent for Runefall's `DynamicEnchantment` and any runtime item/enchantment creation.
- Co-save implication: dynamically created forms need a versioned persistence contract; Runefall's
  `CoSave` TLV schema must cover instance-keyed metadata (decision D-064/D-065 direction).
- Reference for "create → mutate → track/dispose" lifecycle and save-safety. Runefall should
  implement equivalent in C++ behind `IItemMutator`, not via Papyrus.
- Caveat: runtime form creation increases reference-handle pressure; pair with Engine Fixes
  ref-handle warnings and caps.

---

## 4. Lexicon — Nexus 153176, v0.6.3.0

**Source path**
- `rpg-mechanism\Lexicon - Main File 153176 0.6.3.0 2026-09-06T05-03Z VMSnJrsdO\`

**Files**
- `LexiconSKSE.esl` (ESL plugin providing script records)
- `SKSE\Plugins\LexiconSKSE.dll`, `LexiconSKSE.pdb`
- `SKSE\Plugins\lexicon.ini`
- `SKSE\Lexicon\scripts\*.lsi` and `Shared\*.lsi` (Lexicon Script Interface)
- `SKSE\Lexicon\Resources\LexiconSKSE\Main.json`, `LexTest.json`
- `SKSE\Lexicon\report\english.ini`

**Architecture**
- A custom scripting runtime layered on SKSE: an ESL plugin + DLL + a bespoke language
  (`.lsi` interface files) + JSON resource definitions.
- `lexicon.ini` configures:
  - `sDataDir = Data/SKSE/Lexicon`
  - `sCoreDir = Data/SKSE/Lexicon/scripts`
  - `sRequiredVersion = 0.6.1.4`
  - debug: `sDefaultLogLevel`, `bRuntimeBreakpoint`, `bDumpManualScripts`, `bDumpAST`.
- `Main.json` defines functions (formula + typed parameters + defaults) and properties
  (`True`, `False`, `Player`).
- `.lsi` files declare interfaces/intrinsics (e.g. `interface Object intrinsic CORE::0;`).
- Report localization via `report/english.ini`.

**Relevance to Runefall**
- Example of "DSL + JSON + SKSE DLL + ESL records" packaging. Useful pattern if Runefall ever
  needs a data-defined scripting surface, but Runefall's stated policy is DLL-first with
  data-driven tables (CSV/JSON), not a new language. Prefer extending data tables over adopting
  Lexicon.
- Demonstrates version pinning (`sRequiredVersion`) and log-level config; mirror in Runefall.ini.

---

## 5. PROTEUS — Nexus 62934, v3.4.0

**Source path**
- `rpg-mechanism\PROTEUS - 3.4.0-62934-3-4-0-1714763289\`

**Files (key)**
- `Data\PROTEUS.esp`
- `Data\SKSE\plugins\Proteus.dll`
- `Data\Source\Scripts\*.psc` (incl. `ProteusDLLUtils.psc`, `ProteusMCMScript.psc`)
- `Data\Scripts\Proteus JSON\*.json` templates
- `Data\Interface\` (uilb SWFs, translations)
- `Data\Patches\Proteus - <Mod> Patch.esp` (many)
- `fomod\info.xml`, `fomod\ModuleConfig.xml`
- `Images\*.jpg/png`

**Architecture**
- In-game editor/character manager: edits actors, weapons, armor, spells, weather, etc. at
  runtime via an SKSE DLL.
- `ProteusDLLUtils.psc` declares native functions (all `global native`): actor/spell/perk
  enumeration, sex/level setters, form search by text/editor id, favorites, etc.
  Example: `ProteusGetItemBySearch`, `ProteusGetAllByFormId`, `SetSex`, `SetLevel`,
  `ShowRaceMenu`, `ProteusGetFormEditorID`.
- MCM integration via `ProteusMCMScript extends SKI_ConfigBase` with many `globalvariable`
  properties and hotkeys.
- JSON templates for editing (`Proteus_Armor_Template.json`, etc.).
- Optional compatibility patches shipped as separate ESPs, selected in FOMOD by file
  dependency.

**FOMOD pattern (`ModuleConfig.xml`)**
- `requiredInstallFiles`: Interface, Scripts, Source, ESP, DLL.
- `installSteps`: "Compatibility Patches" with `SelectAny` groups; each plugin declares
  `fileDependency` on the target mod's ESP (Active → Recommended, Missing → NotUsable).
- Images per option.

**Relevance to Runefall**
- Reference for native-function surface design and MCM/JSON-template editing UX.
- FOMOD dependency-gated optional patches is the packaging model Runefall should follow for
  optional compatibility (e.g. OStim/SPID patches).
- Runtime form editing precedent; aligns with Dynamic Persistent Forms and Runefall's
  DynamicEnchantment.

---

## 6. Media Keys Fix — Nexus 92948, v1.0.2

**Source path**
- `rpg-mechanism\Media Keys Fix 92948 1.0.2 2026-08-21T18-58Z Yj6wQRo26\`

**Files**
- `SKSE\Plugins\MediaKeysFix.dll`, `.ini`, `.pdb`

**Config**
```ini
[General]
DisableWindowsKey = true
DisableDeadKeys = false
BackgroundAccess = false
```

**Architecture**
- Small SKSE plugin hooking DirectInput keyboard acquisition; config-driven toggles.

**Relevance to Runefall**
- Pattern for a tiny, config-driven input hook with an INI in `SKSE\Plugins\<Name>.ini`.
- Useful if Runefall needs global hotkeys; prefer SKSE Menu Framework/ImGui for UI instead.

---

## 7. Engine Fixes (All-In-One) — Nexus 17230, v7.0.19 (SSE 1.6.1170+)

**Source**
- `rpg-mechanism\Engine Fixes (All-In-One) for 1.6.1170 and newer-17230-7-0-19-AIO-1758815694.7z`
  (archive only; extracted listing via 7z)

**Files (archive)**
- `data\skse\plugins\EngineFixes.dll`, `EngineFixes.pdb`
- `data\skse\plugins\EngineFixes.toml`
- `data\skse\plugins\EngineFixes_preload.txt`, `EngineFixes_SNCT.ini`
- `d3dx9_42.dll`
- `SSE Engine Fixes - Install Instructions.txt`, `vortex_override_instructions.json`

**Architecture**
- SKSE plugin + TOML config with sections: `[General]`, `[Fixes]`, `[Patches]`,
  `[MemoryManager]`, `[Warnings]`, `[Debug]`.
- Notable for Runefall:
  - `bESLCELLLoadBug` — fixes interior cells created in ESL files (relevant to Runefall ESL).
  - `bFormCaching`, `bTreeLodReferenceCaching` — form lookup performance.
  - `iSaveGameMaxSize` — expands save size beyond 64 MB (relevant with dynamic forms/co-save).
  - `bCleanSKSECoSaves` — deletes orphan SKSE co-saves.
  - Memory manager overrides (tbbmalloc) and ref-handle warnings (`bRefHandleLimit`,
    `uRefrMainMenuLimit`, `uRefrLoadedGameLimit`) — relevant to runtime form creation and spawns.
  - `bSafeExit`, `bRegularQuicksaves`.

**Relevance to Runefall**
- Treat Engine Fixes as a hard dependency for any Runefall build that creates forms/spawns actors.
- Config knobs that Runefall docs should mention: save size, co-save cleanup, ref-handle limits.
- Do not duplicate its fixes; depend on it and document minimum version.

---

## 8. Unofficial Skyrim Special Edition Patch (USSEP) — Nexus 266, v4.3.9c

**Source**
- `rpg-mechanism\Unofficial Skyrim Special Edition Patch 266 4.3.9c 2026-09-05T22-22Z lNkz9LChN.7z`

**Files (archive)**
- `unofficial skyrim special edition patch.esp` (21.6 MB)
- `unofficial skyrim special edition patch.bsa` (241 MB)
- `unofficial skyrim special edition patch - textures.bsa` (39.8 MB)
- `unofficial skyrim special edition patch.ini`
- `unofficial skyrim special edition patch.modgroups`
- `BashTags\unofficial skyrim special edition patch.txt`
- `Docs\...`

**Architecture**
- Standard master-level patch mod: ESP + BSA assets + INI + modgroups.
- `modgroups` lists load-order group including all official masters + `_ResourcePack.esl`
  and the patch ESP.
- INI: `[Audio] uMaxSizeForCachedSound=4096`, `[Actor] fVisibleNavmeshMoveDist=12288.0000`.

**Relevance to Runefall**
- Baseline compatibility target: Runefall ESL/ESP must load after USSEP in load order.
- Packaging reference: ESP + BSA + INI + modgroups + BashTags for a large patch mod.
- Runefall's own ESPs should declare masters and be tested against USSEP.

---

## 9. TrueHUD — Nexus 62775, v1.1.10 (HUD infra)

**Source path**
- `rpg-mechanism\TrueHUD 62775 1.1.10 2026-08-31T16-21Z 31i7IoHze\`

**Files**
- `TrueHUD.esl`
- `SKSE\Plugins\TrueHUD.dll`, `TrueHUD.pdb`
- `SKSE\Plugins\TrueHUD\TrueHUD_base.ini` (boss recognition lists)
- `Interface\TrueHUD*.swf`, `Interface\Translations\TrueHUD_*.txt`
- `MCM\Config\TrueHUD\config.json`, `settings.ini`
- `Scripts\TrueHUD_MCM.pex`

**Architecture**
- HUD overlay framework: ESL plugin + SKSE DLL + Scaleform SWF + MCM config + translations.
- `TrueHUD_base.ini` configures boss recognition by Race/NPC/LocRefType/Blacklist using
  `Plugin.esp:0xFormID` references.
- MCM settings via `config.json` + `settings.ini`.

**Relevance to Runefall**
- Reference for HUD infra if Runefall adds custom HUD widgets (rank, rune meters).
- Pattern: ESL + DLL + SWF + MCM config + translation files.
- Boss-recognition config style (FormID lists) can inform Runefall's enemy-rank/boss
  classification data.

---

## 10. SKSE Menu Framework — Nexus 120352 (cross-ref)

**Source path**
- `rpg-mechanism\skyrim-menu-framework\`

**Files (key)**
- `SKSE Menu Framework 120352 3.18 .../SKSE/Plugins/SKSEMenuFramework.dll`, `.ini`, `.pdb`
- `SKSEMenuFrameworkMenuConfig.json`, `SKSEMenuFrameworkStrings*.json`
- `SKSEMenuFrameworkThemes/*.json`, fonts
- `SKSE-Menu-Framework-SDK-main/` (headers, ImGui libs, example)
- `SKSE Menu Framework 3 Header (Fixed).../SKSEMenuFramework.h`

**Architecture**
- ImGui-based in-game menu framework; mods register sections/items via
  `SKSEMenuFramework::SetSection` / `AddSectionItem` with render callbacks.
- SDK requires linking `SKSEMenuFramework.dll`/`.lib` and ImGui; env vars
  `SKYRIM_MODS_FOLDER`, `IMGUI_LIB_FOLDER`.

**Relevance to Runefall**
- See `research/ui-web-menu.md` for full UI stack comparison. If Runefall uses SKSE Menu
  Framework for debug/MCM-style panels, follow the SDK CMake pattern above.
- Runefall already has `MenuApi`/`MenuUI` snapshot JSON; integration point is a render
  callback reading immutable snapshots (ARCHITECTURE §6).

---

## 11. Source samples / SDK patterns

### 11.1 Dynamic Enemy Spawns SKSE — Nexus 178556, v3.4 (source + PDB)

**Source**
- `rpg-mechanism\Source And PDB Files 178556 3.4 2026-09-29T14-44Z HZCMOyR1r.zip`

**Files**
- `src\main.cpp`, `SaveHook.h`, `Settings.*`, `SpawnManager.*`, `Strings.*`, `UI.*`,
  `Version.h`, `PCH.*`
- `PDB\AutoEnemySpawn.pdb`

**Architecture patterns**
- `SKSEPlugin_Load`: init log, load settings, `SKSE::Init`, install hooks, register messaging.
- Message handler: `kPreLoadGame` → `HandlePreLoadGame`; `kDataLoaded` → build caches,
  register, load strings/UI.
- Vtable hooks via `stl::write_vfunc<RE::Character, ...>` on slots `0x0D` (`ShouldSaveGame`)
  and `0x0E` (`SaveGame`) to prevent mod-spawned actors from being saved (load-transition aware).
- Spawn tracking (`IsSpawnedByMod`) and load-transition gating.

**Relevance to Runefall**
- Direct template for Runefall's `HookManager`: vtable-hook install pattern, messaging lifecycle,
  load-transition handling, and "do not save spawned actors" co-save safety.
- Save-hook pattern is reusable for any runtime-created actor/form.

### 11.2 Concentration Strain — Nexus 189317, v1

**Source**
- `rpg-mechanism\Src 189317 1 2026-08-23T03-33Z Dc5uGkCfW.zip`

**Files**
- `src\Plugin.cpp`, `Settings.*`, `UI.*`, `Utils.h`, `PCH.h`

**Architecture patterns**
- Registers `BSTEventSink<RE::InputEvent*>` via `BSInputDeviceManager::AddEventSink`.
- Button-event handling: IsDown/IsUp/IsHeld, held-duration validation, interrupt casting.
- Messaging: `kPostLoad` → UI register; `kDataLoaded` → `OnDataLoaded`.
- Uses `SKSE::PluginDeclaration`, `SKSE::Init`.

**Relevance to Runefall**
- Input-event sink pattern for hotkeys/gestures; session-state machine example (active/lockout).

### 11.3 Dynamic Container Loot — Nexus 172018, v0.6.0 (src)

**Source**
- `rpg-mechanism\DynamicContainerLoot src 172018 sec 2026-09-19T21-31Z hfH9s6cjx.zip`

**Files (key)**
- `src\main.cpp`, `plugin.h`, `Settings.*`, `Compatibility\*`, `ESP\*` (Parser/Writer/Fingerprint),
  `Merge\ContainerMerger.*`, `Ecosystem\*`, `HTML\*`.

**Architecture patterns**
- `kDataLoaded` handler orchestrates: reset editor, load settings, begin report, check loaded
  patch ESP + protection, merge all containers, optionally `ESP::WriteESP`, save fingerprints,
  complete rebuild.
- Generates a patch ESP at runtime from computed merges; protects manually edited patches.
- Error handling: try/catch with ecosystem report; clear merged containers.

**Relevance to Runefall**
- Precedent for Runefall generating ESPs from data (container loot) and protecting user edits.
- Report/fingerprint pattern for reproducible build outputs; aligns with Runefall.Build.

---

## 12. Cross-cutting packaging patterns

| Pattern | Example | Notes |
|---|---|---|
| SKSE plugin + INI | Media Keys Fix, Engine Fixes, BDI | INI under `SKSE\Plugins\<Name>.ini` or `.toml`. |
| ESL + DLL + SWF + MCM | TrueHUD | HUD/overlay stack. |
| ESP + BSA + INI + modgroups + BashTags | USSEP | Large patch mod baseline. |
| FOMOD with fileDependency-gated optional patches | PROTEUS | Compatibility patch selection. |
| Runtime ESP generation + fingerprints | Dynamic Container Loot | Protect manual edits. |
| Runtime form creation + co-save | Dynamic Persistent Forms | Create/Track/Dispose lifecycle. |
| Behavior graph data injection (JSON) | BDI / Payload Interpreter | No Nemesis rebuild. |
| Vtable hooks + messaging lifecycle | Dynamic Enemy Spawns | Save-hook safety. |

---

## 13. Relevance summary for Runefall

- **Native DLL + ESL**: TrueHUD, Lexicon, Engine Fixes (ESL cell fix), USSEP show ESP/ESL + DLL
  + data asset layouts. Runefall's ESL records should be tested against USSEP and Engine Fixes.
- **Co-save**: Dynamic Enemy Spawns save hooks + Dynamic Persistent Forms Track/Dispose + Engine
  Fixes co-save cleanup. Runefall's `CoSave` must be versioned and instance-keyed (ARCHITECTURE §8).
- **Form creation**: Dynamic Persistent Forms is the direct precedent; implement in C++ behind
  `IItemMutator`; monitor ref-handle limits.
- **Hooks**: Dynamic Enemy Spawns and Concentration Strain show vtable/event-sink install and
  messaging lifecycle; Runefall's `HookManager` should mirror and add unregister/preflight.
- **Compatibility**: PROTEUS FOMOD pattern for optional patches; USSEP as baseline master.
- **Packaging/FOMOD**: PROTEUS optional-patch selection; USSEP ESP+BSA+modgroups; Engine Fixes
  TOML config; TrueHUD MCM config.

---

*All source paths are read-only references under `rpg-mechanism\`.*