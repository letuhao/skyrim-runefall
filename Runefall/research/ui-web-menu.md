# Research: In-Game Web UI / Menu Systems (PrismaUI & Alternatives)

Source: read-only scan of `D:\Works\source\skyrim-moddings\rpg-mechanism\`.  
Goal: inform Runefall's choice of UI surface for an allocation panel, crafting/salvage panel, and MCM alternative.

## 1. Summary / Recommendation

- **PrismaUI** (Nexus 148718, v1.5.1) is the right foundation for Runefall's web UI menus. It embeds an **Ultralight** (WebKit-based) view into the Skyrim render pipeline, driven by a C++ SKSE plugin through a small C API. UI is authored in **HTML/CSS/JS** and shipped under `Data/PrismaUI/views/<ViewName>/`. Communication is bidirectional: C++ → JS via `Invoke`/`InteropCall`, JS → C++ via `RegisterJSListener`. This is the only option in the scanned set that gives a real web stack (DOM, CSS, JS) inside the game.
- **SKSE Menu Framework** (Nexus 120352) is the ImGui/C++ alternative: immediate-mode GUI drawn by the framework DLL, good for debug/numeric panels, not web content, and requires the framework DLL + ImGui.
- **TrueHUD** is Scaleform (SWF/Flash) — legacy, not web, requires Flash assets.
- **MeridianUI** appears as a second web-view bridge name in Agility/Intelligence Matters widgets (same HTML ships in both `PrismaUI/views/` and `MeridianUI/` roots), but no SDK/binary was found in the scanned set; treat as an unverified/possibly alias bridge. PrismaUI is the verified one.

## 2. PrismaUI (priority)

### 2.1 What it is

- Nexus 148718, v1.5.1. Ships:
  - `SKSE/Plugins/PrismaUI.dll` (+ `.pdb`) — the renderer/bridge plugin. Closed source.
  - `PrismaUI/` runtime payload: `libs/` (Ultralight.dll, UltralightCore.dll, WebCore.dll, AppCore.dll, icudt67l.dat, cacert.pem, cursor.png), `inspector/` (Chromium DevTools frontend), `misc/PrismaUI_API.d.ts`.
- Renderer is **Ultralight** (CPU renderer). Implication: avoid shadows/gradients/blur; keep CSS flat for FPS (Spell Research DEVNOTES §5).
- Requires PrismaUI installed as a dependency mod. API is requested at runtime via `GetModuleHandle(L"PrismaUI.dll")` + `GetProcAddress("RequestPluginAPI")`.

### 2.2 Source vs packaged

- No C++ source for PrismaUI itself (closed DLL). What we have:
  - API header: `Prisma UI API Header File .../PrismaUI_API.h` (copy into your project).
  - TS definitions: `PrismaUI/misc/PrismaUI_API.d.ts` (JS-side `window.prismaUi`).
  - Example plugin: `Prisma UI Example Plugin .../PrismaUI/views/PrismaUI-Example-UI/index.html` + `SKSE/plugins/PrismaUI-Example-Plugin.dll` (no source).
  - PUISM (settings menu) ships a DLL + HTML + JSON, no source.
- Real consumer source we do have: **Spell Research - Prisma UI Source Files** (`plugin/src/main.cpp`, `view/index.html`, `papyrus/*.psc`, `SKSE/Plugins/SpellResearchPrismaUI.ini`).

### 2.3 Architecture

```
Your SKSE plugin (C++)
  -> requests PrismaUI API (IVPrismaUI1/2) after kMessage_PostLoad
  -> CreateView("YourMod/index.html", onDomReady)
  -> registers JS listeners (JS -> C++)
  -> on demand: Show/Hide/Focus/Unfocus, InteropCall/Invoke to push data
  -> Papyrus natives registered on a script (e.g. SRPrismaUI) for quests
PrismaUI.dll (Ultralight)
  -> loads Data/PrismaUI/views/YourMod/index.html
  -> injects window.prismaUi before page scripts run
Data/PrismaUI/views/YourMod/   (HTML/CSS/JS + assets)
```

### 2.4 C++ API surface (`PrismaUI_API.h`)

```cpp
typedef uint64_t PrismaView;
namespace PRISMA_UI_API {
  constexpr const auto PrismaUIPluginName = "PrismaUI";
  enum class InterfaceVersion : uint8_t { V1, V2 };

  typedef void (*OnDomReadyCallback)(PrismaView view);
  typedef void (*JSCallback)(const char* result);
  typedef void (*JSListenerCallback)(const char* argument);
  enum class ConsoleMessageLevel : uint8_t { Log, Warning, Error, Debug, Info };
  typedef void (*ConsoleMessageCallback)(PrismaView view, ConsoleMessageLevel, const char* message);

  // V1
  class IVPrismaUI1 {
    PrismaView CreateView(const char* htmlPath, OnDomReadyCallback onDomReadyCallback = nullptr);
    void Invoke(PrismaView view, const char* script, JSCallback callback = nullptr);          // run arbitrary JS
    void InteropCall(PrismaView view, const char* functionName, const char* argument);        // call window.<fn>(arg)
    void RegisterJSListener(PrismaView view, const char* functionName, JSListenerCallback cb); // JS -> C++
    bool HasFocus(PrismaView view);
    bool Focus(PrismaView view, bool pauseGame = false, bool disableFocusMenu = false);
    void Unfocus(PrismaView view);
    void Show(PrismaView view);
    void Hide(PrismaView view);
    bool IsHidden(PrismaView view);
    int  GetScrollingPixelSize(PrismaView view);
    void SetScrollingPixelSize(PrismaView view, int pixelSize);
    bool IsValid(PrismaView view);
    void Destroy(PrismaView view);
    void SetOrder(PrismaView view, int order);
    int  GetOrder(PrismaView view);
    void CreateInspectorView(PrismaView view);
    void SetInspectorVisibility(PrismaView view, bool visible);
    bool IsInspectorVisible(PrismaView view);
    void SetInspectorBounds(PrismaView view, float topLeftX, float topLeftY, unsigned int width, unsigned int height);
    bool HasAnyActiveFocus();
  };

  // V2 extends V1
  class IVPrismaUI2 : public IVPrismaUI1 {
    void RegisterConsoleCallback(PrismaView view, ConsoleMessageCallback callback); // nullptr unregisters
  };

  // Request
  void* RequestPluginAPI(InterfaceVersion a_interfaceVersion = InterfaceVersion::V1);
  template <typename T> T* RequestPluginAPI();   // e.g. RequestPluginAPI<IVPrismaUI1>()
}
```

Acquisition pattern (from Spell Research `main.cpp`):

```cpp
g_prisma = static_cast<PRISMA_UI_API::IVPrismaUI1*>(
    PRISMA_UI_API::RequestPluginAPI(PRISMA_UI_API::InterfaceVersion::V1));
if (!g_prisma) { /* PrismaUI missing */ }
```

Request during/after `SKSEMessagingInterface::kMessage_PostLoad` (or `kDataLoaded`).

### 2.5 JS side (`PrismaUI_API.d.ts`)

PrismaUI injects `window.prismaUi` before page scripts run:

```ts
interface PrismaUiControls extends EventTarget {
    map?: PrismaUiControlMap;            // latest key-binding snapshot
    refresh(): void;
    getEventIdByButtonIndex(w3cButtonIndex, contextName?): string | null;
    addEventListener('gamepadbuttondown' | 'gamepadbuttonup' | 'refreshcomplete', ...);
}
interface PrismaUi { controls: PrismaUiControls; }
interface Window { prismaUi: PrismaUi; }
```

Use `window.prismaUi.controls` to read Skyrim key bindings (gamepad type, contexts, per-device mappings) and observe gamepad button down/up. Good for mapping menu accept/cancel to gamepad.

### 2.6 View lifecycle (Spell Research pattern)

```cpp
// InitUI() called on kDataLoaded
g_view = g_prisma->CreateView("SpellResearch/index.html", [](PrismaView a_view) {
    g_domReady.store(true);
    if (g_prisma) g_prisma->Hide(a_view);   // start hidden
});
g_prisma->Hide(g_view);   // hide IMMEDIATELY (DOM-ready can be seconds later)

// Open
g_prisma->InteropCall(g_view, "srShowCauldron", payload.c_str());
g_prisma->Show(g_view);
g_prisma->Focus(g_view, /*pauseGame*/false);

// Close
g_prisma->Hide(g_view);   // Hide() calls Unfocus() internally
g_isOpen.store(false);
```

Notes:
- Create view path is relative to `Data/PrismaUI/views/` (e.g. `"SpellResearch/index.html"`).
- `Hide()` internally unfocuses; `Show()`/`Hide()` are the documented efficient toggle (hidden view stops rendering). Do not CSS-hide (keeps compositing).
- `Focus(view, pauseGame, disableFocusMenu)` — Spell Research passes `pauseGame=false` deliberately (stuck pause is unrecoverable).
- `SetOrder` controls z-order among views.

### 2.7 Data bridge

C++ → JS:
- `Invoke(view, "window.foo = 1; bar();", callback)` — run arbitrary JS; optional `JSCallback` receives a result string.
- `InteropCall(view, "functionName", argument)` — calls `window.functionName(argument)` (best performance). Used heavily (Spell Research pushes JSON payloads to `srShowCauldron`, `srUpdateCauldron`, etc.).

JS → C++:
- `RegisterJSListener(view, "fnName", cb)` makes `window.fnName(...)` available in the page; calling it delivers the string argument to `cb` (runs on Ultralight thread).
- Spell Research registers: `srRequestClose`, `srPoolQuery`, `srMenuData`, `srMenuChoice`. JS calls `window.srMenuChoice("3")`.

JS page pattern (Spell Research `view/index.html`):

```js
window.srSetData = function (json) { /* render */ };
// send to C++:
if (typeof window.srMenuChoice === 'function') window.srMenuChoice(String(i));
if (typeof window.srMenuData === 'function') window.srMenuData(parts.join(','));
```

Example plugin HTML (`PrismaUI-Example-UI/index.html`) shows the minimal shape:

```js
window.updateFocusLabel = (data) => { document.querySelector("#focus-label").textContent = data; };
function sendInputData() {
    window.sendDataToSKSE(document.querySelector("#data-input").value); // registered listener
}
```

### 2.8 Threading (critical)

Spell Research's `main.cpp` documents this clearly:
- `KeyHandler` (BSTEventSink) runs on the **input thread** (pool, different thread per press).
- Calling ControlMap/PrismaUI from those threads races the game update → ~50% failures, stuck controls.
- **All** game/UI state changes must go through `SKSE::GetTaskInterface()->AddTask([](){ ... })`, which defers to the main thread during the next game update.
- Ultralight listener callbacks also are not on the main thread; any FormList/inventory read must hop via `AddTask` (Spell Research's `srPoolQuery` listener does this).

### 2.9 Focus / input ownership

- `HasFocus`, `HasAnyActiveFocus` for state queries.
- Escape is often handled **natively** (not in JS) because Escape never reaches the Ultralight view. Spell Research registers a native key sink for Escape and, when a menu is open, stores the cancel choice instead of hiding (so Papyrus polling resolves).
- `disableFocusMenu` option on `Focus`.

### 2.10 Debugging

- `CreateInspectorView`, `SetInspectorVisibility`, `IsInspectorVisible`, `SetInspectorBounds` expose a Chromium DevTools inspector over the view. `PrismaUI/inspector/` ships the frontend. Use during development.

### 2.11 Packaging / layout

- View files go in `Data/PrismaUI/views/<ViewName>/` (index.html + css/js/assets). Mod's SKSE plugin in `SKSE/Plugins/`.
- Requires PrismaUI mod installed (declare dependency). PrismaUI.dll must be present in `SKSE/Plugins`.
- Views are referenced by name: `CreateView("SpellResearch/index.html", ...)`.

### 2.12 Console callback (v2)

`IVPrismaUI2::RegisterConsoleCallback(view, cb)` receives JS console messages (level + message) — useful for in-game log of UI errors.

### 2.13 Real-world consumers in the scanned set

| Mod | Role | Pattern |
|---|---|---|
| Spell Research - Prisma UI | Full replacement of MessageBox menus | One view, C++ plugin, Papyrus natives, polling choice, mass-tracker overlay |
| Prisma UI Settings Menu (PUISM) | MCM alternative | Multi-mod settings aggregator; HTML/JS settings UI; C++ DLL + JSON mod manifests |
| HERO AVATAR HUD / More Diverse HUD | HUD overlay | PrismaUI view (HTML/CSS/JS with jQuery/Tailwind), C++ DLL pushes HP/MP/SP via `InteropCall` to global JS functions |
| IsekaiHero | Full character/skill/shop UI | PrismaUI view, C++ DLL pushes scale/locale/pad input |
| AgilityMatters / Intelligence Matters (widgets) | Status panels | Papyrus pushes a text payload (not JSON) to a PrismaUI view; also ships a `MeridianUI/` copy of the same HTML |

## 3. Spell Research deep dive (the canonical example)

Files: `Spell Research - Prisma UI Source Files .../plugin/src/main.cpp`, `view/index.html`, `papyrus/SRPrismaUI.psc`, `SKSE/Plugins/SpellResearchPrismaUI.ini`.

### 3.1 Init

- `SKSEPluginLoad`: `LoadConfig()` (reads `ToggleKey` from ini), registers SKSE messaging listener, registers Papyrus natives (`SRPrismaUI`), serialization (co-save for dissolved reagents).
- On `kDataLoaded`: `InitUI()` acquires API, `CreateView("SpellResearch/index.html", onDomReady)`, hides immediately, registers JS listeners, registers toggle key + Escape key sinks.

### 3.2 Papyrus bridge (`SRPrismaUI` script)

Natives: `IsAvailable`, `OpenCauldron`, `OpenAlembic`, `OpenJournal`, `SetMenuTitle`, `ShowNotice`, `ShowConfirm`, `ShowChoice`, batch notice functions, `UpdateCauldron`, `PushLog`, `ShowMassTracker/HideMassTracker/IsMassTrackerOn`, `GetChoice`, `GetChoiceData`, `OpenThesisCraft`, `OpenComposeSpell`, `NoteDissolved`, `RevealAnalysed`, `CloseMenu`.

Papyrus polling pattern (not latent — latent can hang the thread):

```papyrus
SRPrismaUI.OpenCauldron(fuel, weight)
loop
    Utility.Wait(0.1)
    opt = SRPrismaUI.GetCauldronChoice()
endloop
SRPrismaUI.CloseCauldron()
```

`GetChoice()` returns -1 while pending, then the index. `GetChoiceData()` (string) is set **before** the index, so a poll that sees the index move is guaranteed data is ready. `AwaitAndClose()` polls, closes, then drains `PrismaUI_FocusMenu` from the stack (control-lockup prevention).

### 3.3 Menus vs mass tracker

- **Menu screen**: `g_menuOpen=true`, view Shown + Focused. Choice via `srMenuChoice` listener. Escape stores cancel index.
- **Mass tracker (overlay)**: view Shown but **never Focused**; `g_isOpen`/`g_menuOpen` untouched; polled from a C++ `jthread` (120 ms) via `SKSE::GetTaskInterface()->AddTask` (inventory reads on main thread). Renders beside a Skyrim menu (e.g. gift menu). `srMassSuppress`/`srMassResume` manage visibility when a panel opens over it.
- Choice indices are documented to match the original `Message.Show()` button order, so fallback (no PrismaUI) returns identical indices.

### 3.4 Notice batching

`BeginNoticeBatch`/`EndNoticeBatch` collect `QueueNotice`/`QueueLearn` lines and flush as a single panel (`srShowLearned` with structured gains or `srShowNotice`). Avoids N single-OK panels.

### 3.5 Config

`Data/SKSE/Plugins/SpellResearchPrismaUI.ini` with `ToggleKey=` (key name or scan code). Hand-rolled parser.

---

## 4. skyrim-menu-framework (ImGui)

Location: `rpg-mechanism/skyrim-menu-framework/`. Contains `SKSE-Menu-Framework-SDK-main/` (SDK) and `SKSE Menu Framework ...` (runtime).

- Framework DLL `SKSEMenuFramework.dll` + `imgui.dll` (or `imguid.dll` debug). Renders an ImGui menu inside Skyrim; style selectable (Skyrim/Modern/Default). Toggle key `F1` (ini).
- SDK header `SKSEMenuFramework.h` defines `AddSectionItem(path, renderer)`, `AddWindow(renderer)`, `SetSection(key)`, FontAwesome helpers. Mod code writes a `void __stdcall Render()` using `ImGui::*` calls.
- Build integration: CMake `find_package(ImGui)`, link `SKSEMenuFramework.lib`, include framework DEV headers. Debug vs release DLL must match build type (CTD otherwise).
- Not web; C++ immediate-mode only. Good for debug/numeric/config panels, not for rich layouts. No source example included (only compiled `SKSEMenuFrameworkExample.dll`).

---

## 5. TrueHUD (Scaleform/SWF)

Location: `rpg-mechanism/TrueHUD 62775 1.1.10 ...`.

- HUD overlay via Scaleform Flash (`Interface/TrueHUD*.swf`, `TrueHUD_Widgets.swf`, `TrueHUD_Assets*.swf`). C++ plugin `TrueHUD.dll` drives SWF. Config via MCM (`MCM/Config/TrueHUD`) and ini (`SKSE/Plugins/TrueHUD/TrueHUD_base.ini`).
- Legacy stack: requires SWF authoring tools, not web. Not recommended for new web menus.

---

## 6. HERO AVATAR HUD / More Diverse HUD (PrismaUI HUD)

- PrismaUI-based HUD overlay: `PrismaUI/views/PCHUD/CEC_HUD.html` + `Hud.css` + `PCHUD.js`/`CEC_HUD.js`, jQuery/jQuery-UI/Tailwind. C++ plugin `Prisma_Combat_HUD.dll` pushes data via InteropCall to global JS functions (`UpdateCorePoint`, `UpdateRagePoint`, `updateCombo`, etc.). Also exposes editor mode for repositioning HUD elements (`ToggleditorMode`, `updateLayout`, `readLayoutSetting`).
- "More Diverse HUD" is essentially the same PCHUD view with more assets. Both ship under `PrismaUI/PMCM/PCHUD.txt` (PMCM integration).
- Pattern: HUD = always-visible PrismaUI view, no Focus, data pushed periodically; editor mode toggles drag/rotate.

---

## 7. AgilityMatters / Intelligence Matters (widgets)

- Widgets are **Papyrus-driven** status panels: `AgilityMatters - Widget ...` ships `PrismaUI/views/AgilityMatters/index.html` + `study.html` and `MeridianUI/AgilityMatters/...` (identical bytes). The view receives a plain-text payload (not JSON) from Papyrus: sections marked `##title`, rows `label: value`, plus directives (`#CAP-BEGIN/END`, `#BAR pct|seconds|label|value`, `#WARN`, `#TENSION`, `#COLS-BEGIN/END`). Papyrus has no JSON encoder, so they designed a line protocol.
- `Intelligence Matters - Widget` ships more views (fusion/meditation/refine/research) under both `PrismaUI/views/` and `MeridianUI/`.
- Intelligence Matters V3 full mod is Papyrus/MCM heavy; the widget is the web part.
- MeridianUI appears to be a parallel bridge (same HTML), but no SDK/DLL found in scanned set — unverified. Treat PrismaUI as the supported bridge.

---

## 8. IsekaiHero (PrismaUI full UI)

- `PrismaUI/views/IsekaiHero/index.html` + `SKSE/Plugins/IsekaiHeroSKSE.dll`. Web UI for character tree, shop, blessings. JS globals driven from C++: `window.isekaiScale`, `window.isekaiLoc`, `window.isekaiPad`. Supports gamepad navigation (`padTargets`, `padMove`) because PrismaUI gives keyboard/mouse but not controller. Good reference for a complex menu with localization and scaling.

---

## 9. Prisma UI Settings Menu (PUISM) — MCM alternative

- Nexus 191932, v1.1.0. A settings aggregator built on PrismaUI: mods ship a JSON manifest (`SKSE/Plugins/<Mod>/<Mod>.json`) declaring settings (key, label, type, tab, default, options, description) and a DLL. PUISM renders a sidebar of mods, tabs, toggles/sliders/dropdowns/color pickers/order lists, auto-saves via `saveSettings` JS listener, and supports a "Compatibility/Rules" container editor.
- This is the model for a Runefall MCM-alternative: rather than SkyUI MCM, ship a PrismaUI settings view + JSON manifest + C++ save/load. Or simpler: embed settings directly in Runefall's own PrismaUI view.

---

## 10. Relevance to Runefall

Runefall needs: an allocation panel, a crafting/salvage panel, and an MCM alternative. Recommended approach, mirroring Spell Research + PUISM + HERO AVATAR HUD:

1. **One PrismaUI view** `Runefall/index.html` under `Data/PrismaUI/views/Runefall/`, with sub-screens toggled by JS (`showScreen`) and data pushed from C++ via `InteropCall(view, "rfShowAllocation", json)`. Keep CSS flat (no shadows/gradients) for CPU renderer.
2. **SKSE plugin** (C++, CommonLibSSE-NG) that:
   - Acquires `IVPrismaUI1` in `kDataLoaded`/`kMessage_PostLoad`.
   - `CreateView("Runefall/index.html", onDomReady)`; hide immediately.
   - Registers JS listeners: `rfRequestClose`, `rfChoice`, `rfData`, plus domain queries (e.g. `rfAllocateQuery`, `rfCraftQuery`) that hop to main thread via `SKSE::GetTaskInterface()->AddTask` before touching forms/inventory.
   - Exposes Papyrus natives on a `RunefallUI` script (`IsAvailable`, `OpenAllocation`, `OpenCrafting`, `OpenSalvage`, `GetChoice`, `GetChoiceData`, `CloseMenu`) so quests/aliases can open panels.
   - All game-state reads/writes on main thread; polling (0.1s) rather than latent for choices.
3. **Data protocol**: prefer JSON via `InteropCall` (we control both sides). Use the Spell Research text-line protocol only if Papyrus must push directly without C++.
4. **Focus/input**: open with `Show` + `Focus(view, false)` (no pause); handle Escape natively to store cancel; support gamepad via `window.prismaUi.controls`.
5. **Overlays** (HUD-style): for a live allocation/salvage progress readout, show the view without Focus (mass-tracker pattern) and poll from C++.
6. **Settings/MCM alternative**: either ship a PUISM-style JSON manifest + view, or add a "Settings" tab inside the same Runefall view. Given PUISM exists, integrating with it (registering Runefall settings) may be less work than building a parallel settings system.
7. **Packaging**: declare PrismaUI as required dependency; ship `SKSE/Plugins/RunefallUI.dll` + `Data/PrismaUI/views/Runefall/...`. Use FOMOD to install view files under the right root.
8. **Debug**: enable inspector during dev (`CreateInspectorView`/`SetInspectorVisibility`).

### 10.1 What to avoid

- Do not use Scaleform/SWF (TrueHUD-style) for new menus — Flash toolchain, fragile under UI overhauls.
- Do not use SKSE Menu Framework (ImGui) for rich panels — it's C++ immediate-mode, no styling/content pipeline, and needs the framework DLL.
- Do not pause the game on Focus (`pauseGame=false`) unless explicitly wanted; Spell Research found stuck pauses unrecoverable.
- Do not call PrismaUI/game state from input-thread or Ultralight-thread callbacks without `AddTask`.

---

## 11. File paths (key references)

PrismaUI:
- `rpg-mechanism/Prisma UI 148718 1.5.1 .../Prisma UI API Header File 148718 1.5.0 .../PrismaUI_API.h`
- `rpg-mechanism/Prisma UI 148718 1.5.1 .../PrismaUI/misc/PrismaUI_API.d.ts`
- `rpg-mechanism/Prisma UI 148718 1.5.1 .../Prisma UI Example Plugin 148718 1.5.0 .../PrismaUI/views/PrismaUI-Example-UI/index.html`
- `rpg-mechanism/Prisma UI 148718 1.5.1 .../SKSE/plugins/PrismaUI.dll`

Spell Research source:
- `rpg-mechanism/Prisma UI 148718 1.5.1 .../Spell Research - Prisma UI Source Files 187900 0.2.0 .../plugin/src/main.cpp`
- `.../plugin/src/pch.h`, `plugin/xmake.lua`
- `.../view/index.html`
- `.../papyrus/SRPrismaUI.psc` (and packaged `Scripts/Source/SRPrismaUI.psc`)
- `.../SKSE/Plugins/SpellResearchPrismaUI.ini`

skyrim-menu-framework:
- `rpg-mechanism/skyrim-menu-framework/SKSE-Menu-Framework-SDK-main/Mods/SKSE Menu Framework DEV/SKSE/Plugins/SKSEMenuFramework.h`
- `rpg-mechanism/skyrim-menu-framework/SKSE-Menu-Framework-SDK-main/README.md`
- `rpg-mechanism/skyrim-menu-framework/SKSE-Menu-Framework-SDK-main/Mods/SKSE Menu Framework DEV/SKSE/Plugins/SKSEMenuFramework.ini`

TrueHUD:
- `rpg-mechanism/TrueHUD 62775 1.1.10 .../Interface/TrueHUD*.swf`
- `rpg-mechanism/TrueHUD 62775 1.1.10 .../SKSE/Plugins/TrueHUD/TrueHUD_base.ini`
- `rpg-mechanism/TrueHUD 62775 1.1.10 .../MCM/Config/TrueHUD/config.json`

PrismaUI HUDs:
- `rpg-mechanism/Prisma UI 148718 1.5.1 .../HERO AVATAR HUD 172644 1.3 .../PrismaUI/views/PCHUD/CEC_HUD.html`, `PCHUD.js`, `Hud.css`
- `rpg-mechanism/Prisma UI 148718 1.5.1 .../More Diverse HUD 1.1 .../PrismaUI/views/PCHUD/...`

Widgets:
- `rpg-mechanism/Prisma UI 148718 1.5.1 .../AgilityMatters - Widget V1-0-0 194928 1.0 .../PrismaUI/views/AgilityMatters/index.html` (+ `MeridianUI/...`)
- `rpg-mechanism/Prisma UI 148718 1.5.1 .../Intelligence Matters - Widget 181842 1.0 .../PrismaUI/views/IntelligenceMatters/...`
- `rpg-mechanism/Prisma UI 148718 1.5.1 .../IsekaiHero V0.9.0 185548 0.9.0 .../PrismaUI/views/IsekaiHero/index.html`

PUISM:
- `rpg-mechanism/Prisma UI 148718 1.5.1 .../Prisma UI Settings Menu (PUISM) 191932 1.1.0 .../payload/s0c0/PrismaUI/views/PrismaUISettingsMenu/menu.html`
- `rpg-mechanism/Prisma UI 148718 1.5.1 .../Prisma UI Settings Menu (PUISM) 191932 1.1.0 .../payload/s0c0/SKSE/Plugins/PrismaUISettingsMenu/PrismaUISettingsMenu.json`

Calamity ADR (PrismaUI overlay):
- `rpg-mechanism/Prisma UI 148718 1.5.1 .../calamity-reactive-loot-affixes/docs/adr/0002-prisma-ui-tooltip-overlay.md````
