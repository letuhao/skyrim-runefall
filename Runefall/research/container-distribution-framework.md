# Container Distribution Framework (CDF) — Research Notes (Runefall)

Scope: CDF installed mod (read-only) + upstream source (SeaSparrowOG/DynamicContainerInventoryFramework).
This file mirrors the `skyrim-container-distribution` topic for Runefall planning.

Runefall context: native DLL + ESL, DLL-first gameplay, runtime distribution vs SPID/KID/FLM,
co-save safety, packaging. See `Runefall/ARCHITECTURE.md` and `DECISIONS.md`.

---

## 1. Mod identity

- **Name:** Container Distribution Framework (CDF) (a.k.a. Dynamic Container Inventory Framework)
- **Nexus mod id:** 120152
- **Author:** SeaSparrow (Shekhinaga)
- **Installed version:** 3.1.4.0 (DLL `ContainerDistributionFramework.dll`)
- **Upstream source (pinned):** `SeaSparrowOG/DynamicContainerInventoryFramework`, commit
  `38ca39282e24ed8f7f8423f106d0a660689673c6` (v3.1.0)
- **Runtime requirement:** Skyrim SE/AE 1.6.1130+ (AE structs, Address Library)
- **Dependencies:** SKSE (runtime), CommonLibSSE (AE), clib-util (RNG), JsonCpp, Address Library
- **License:** AGPL-3.0
- **Type:** SKSE plugin, data-driven (JSON rules), no ESP/ESL, no Papyrus

---

## 2. Source paths (read-only)

Installed mod (base path):
- `rpg-mechanism\Container Distribution Framework 120152 3.1.4 2026-10-24T04-57Z 9LMpgbfIm\`
  - `SKSE\Plugins\ContainerDistributionFramework.dll` (1,116,160 bytes, v3.1.4.0)
  - `SKSE\Plugins\ContainerDistributionFramework.ini` (fMaxRefLookupDistance = 25000.0)
  - `SKSE\Plugins\ContainerDistributionFramework.pdb` (debug symbols)

Upstream source (fetched, not vendored):
- `https://github.com/SeaSparrowOG/DynamicContainerInventoryFramework` (commit `38ca392...`)
- Key source files:
  - `src/export/SKSEPlugin.cpp` — plugin lifecycle, messaging, hooks install, INI/JSON read
  - `src/hooks/hooks.cpp` / `hooks.h` — ContainerManager (rule registration, apply, PreCheck, hooks)
  - `src/settings/JSONSettings.cpp` / `.h` — JSON parser, condition parsing, rule classification
  - `src/settings/INISettings.cpp` / `.h` — INI (fMaxRefLookupDistance)
  - `src/conditions/*.cpp` / `.h` — condition implementations
  - `src/merchantCache/merchantCache.cpp` / `.h` — vendor container cache
  - `src/utilities/utilities.h` — Forms::GetFormFromString, String utils, EDID
  - `CMakeLists.txt` — project metadata (v3.1.0)

Local skill references:
- `.agents\skills\skyrim-container-distribution\references\CDF-SOURCE-LOCK.md`
- `.agents\skills\skyrim-container-distribution\scripts\lint_cdf_json.py`

---

## 3. File layout

Installed runtime layout (what ships in a mod / game Data):
```
Data\
  SKSE\
    Plugins\
      ContainerDistributionFramework.dll
      ContainerDistributionFramework.ini
      ContainerDistributionFramework\*.json   <- rule files read here
```

JSON rules are enumerated from `Data/SKSE/Plugins/ContainerDistributionFramework/` (only top-level
`.json` files, sorted). Each file must be a JSON object with a `rules` array.

Log output: `SKSE/Logs/ContainerDistributionFramework.log` (spdlog, info level release; debug if
BUILD_TEST).

---

## 4. Architecture

### 4.1 Plugin lifecycle (SKSEPlugin.cpp)

- `SKSEPlugin_Query`: rejects editor; requires runtime >= 1.6.1130.
- `SKSEPlugin_Load`:
  - init log; `SKSE::Init`; alloc trampoline (28 bytes).
  - register messaging listener.
  - `Hooks::Install()` → `ContainerManager::Install()` (hooks Initialize/Reset).
  - `Settings::INI::Read()` → reads fMaxRefLookupDistance.
- Messaging `kDataLoaded`:
  - `ContainerManager::WarmCache()` (worldspace map markers → nearest location fallback).
  - `MerchantCache::BuildCache()` (vendor factions → merchant containers).
  - `Settings::JSON::Read()` (parse all JSON rules).
  - `ContainerManager::PrettyPrint()` (log rules).

### 4.2 Runtime hook (hooks.cpp)

CDF hooks two `RE::TESObjectREFR` vtable-ish call sites via Address Library relocation:
- `RE::Offset::TESObjectREFR::Initialize` at `+0x78C` (trampoline write_call<5>)
- `RE::Offset::TESObjectREFR::Reset` at `+0x12B`

After calling the original, if `a_container` has a base object that is `TESObjectCONT`,
`ProcessContainer(a_container)` runs all rules.

This means distribution happens at container initialization/reset time (when a container
reference is loaded/initialized in-game), i.e. native runtime distribution, not load-order
patching.

### 4.3 ContainerManager (hooks.h)

Singleton with:
- `storedConditions`: `vector<unique_ptr<Condition>>` (indexed condition store).
- Rule vectors: `adds`, `removes`, `removeKeywords`, `replaces`, `replaceKeywords`.
- `worldspaceMarkers`: `unordered_map<worldspace*, vector<refr*>>` (map markers with location).
- `maxLookupDistance` (default 25000, clamped 10000–150000).

Methods:
- `RegisterRule(raw, conditions, safe, vendors, onlyVendors, random)` — classifies change into
  one of 5 rule types.
- `ProcessContainer(refr)` — runs adds → removes → removeKeywords → replaces →
  replaceKeywords, each `Apply`.
- `PreCheck(refr)` — vendor/safe/condition gating.
- `WarmCache()` — collects persistent-cell map markers per worldspace.
- `GetNearestMarkerLocation(refr)` — nearest marker within maxLookupDistance (for containers
  lacking a direct location).
- `RegisterDistance(float)` — clamp and store.

### 4.4 Rule types

| Rule struct | Trigger condition (change keys) | Effect |
|---|---|---|
| `AddRule` | `add` only | Add `newForms` (count per form; random pick if randomAdd) |
| `RemoveRule` | `remove` only | Remove `form` (ruleCount, 0 = all) |
| `RemoveKeywordRule` | `removeByKeywords` only | Remove inventory items having ALL listed keywords |
| `ReplaceRule` | `add` + `remove` | Remove all of oldForm, add newForms (same total count) |
| `ReplaceKeywordRule` | `add` + `removeByKeywords` | Remove items with ALL keywords, add newForms (total count) |

`ruleCount` default: AddRule 1; RemoveRule 0 (all). Replace rules derive count from removed items.

Leveled lists (`TESLeveledList`) are resolved recursively at player level via
`CalculateCurrentFormList` before adding (see `ResolveLeveledList`/`AddLeveledListToContainer`).

### 4.5 Conditions (filters)

Condition keys (top-level under `conditions`). Each non-`!` key is positive; `!key` inverted.
All conditions in a rule are ANDed (every stored condition must pass).

| Key | Type | Meaning |
|---|---|---|
| `plugins` | string[] | All named plugins must be loaded (LookupModByName). Missing → skip whole rule. |
| `bypassUnsafeContainers` | bool | Allow distribution to safe (non-respawning) containers. |
| `allowVendors` | bool | Allow vendor/merchant containers (default false). |
| `onlyVendors` | bool | Restrict to vendor containers (sets allowVendors true). *(source v3.1.0 has a no-op `onlyVendors;` statement; may be fixed in 3.1.4)* |
| `randomAdd` | bool | Random selection among `add` forms. |
| `containers` / `!containers` | string[] | Base container form must be in list. |
| `locations` / `!locations` | string[] | Container's location (or nearest marker location) must match; parent location traversal. |
| `worldspaces` / `!worldspaces` | string[] | Container's worldspace (or parent worldspace) must match. |
| `locationKeywords` / `!locationKeywords` | string[] | Location (or parent) must have keyword. |
| `playerSkills` / `!playerSkills` | string[] | `"Skill|Level"` — player actor value >= level. |
| `globals` / `!globals` | string[] | `"Global|Value"` — global value >= value. |
| `questConditions` | object | `{ questID, stageDone:[uint], completed:bool }`. |
| `references` / `!references` | string[] | `"FormIDHex|Plugin.esp"` or EditorID of the container reference. |

Inversion is encoded by the `!` prefix on the key name, not by a value flag in JSON. Internally
`Condition::inverted` is set per parsed condition group.

Condition resolution details:
- `containers`: TESObjectCONT forms via `GetFormFromString`.
- `locations`: BGSLocation forms; parent location chain walked.
- `locationKeywords`: BGSKeyword via `LookupByEditorID` (not GetFormFromString).
- `worldspaces`: TESWorldSpace forms; parent worldspace chain walked.
- `playerSkills`: actor value by name (`RE::LookupActorValueByName`).
- `globals`: TESGlobal by EditorID.
- `questConditions`: TESQuest by EditorID; stage/completion checks via `RE::IsQuestStageDone`.
- `references`: FormID+plugin or EditorID of TESObjectREFR; compared by formID.

### 4.6 Safety / container gating (PreCheck)

For every rule application:
1. Merchant detection: faction owner `IsVendor()` OR cached merchant container. If merchant and
   `!allowVendors` → skip. If `!merchant` and `onlyVendors` → skip.
2. Safe container detection:
   - base container is safe if `!(CONT_DATA.flags & kRespawn)` (non-respawning).
   - OR parent cell encounter zone has `kNeverResets` → safe.
   - If safe and `!allowSafeBypass` → skip.
   So by default rules only touch respawning (unsafe) containers unless
   `bypassUnsafeContainers: true`.
3. All stored conditions must pass (`IsValid`).

Additionally, unresolved forms skip the change entry (not the whole rule file):
- Each `add` element must resolve; one failure → skip that change.
- `remove` must resolve; failure → skip.
- `removeByKeywords` elements must resolve; failure → skip.
- `count` must be unsigned int; invalid → skip.
- Missing `plugins` → skip whole rule (return from ReadConfig).

### 4.7 Merchant cache (merchantCache.cpp)

At `kDataLoaded`: iterate all TESFaction, if `IsVendor()`, cache
`faction->vendorData.merchantContainer` reference in `storedReferences` (unordered_map).
`IsMerchantContainer(refr)` checks membership. This is how CDF avoids touching merchant
containers by default.

### 4.8 WarmCache / location fallback

For each worldspace, iterate persistent cell references; collect those with a MapMarker
extra (kMapMarker) that have a current location. Stored in `worldspaceMarkers`. Used by
`GetNearestMarkerLocation` when a container has no direct location (distance clamped to
maxLookupDistance, default 25000). This is what the INI `fMaxRefLookupDistance` controls.

---

## 5. Distribution grammar (JSON syntax reference)

File location: `Data/SKSE/Plugins/ContainerDistributionFramework/*.json`

Root:
```json
{
  "rules": [
    {
      "friendlyName": "Human readable name",
      "conditions": { ... },
      "changes": [
        {
          "add": ["FormString", "..."],
          "remove": "FormString",
          "removeByKeywords": ["KeywordFormString", "..."],
          "count": 1
        }
      ]
    }
  ]
}
```

- `friendlyName`: required string.
- `conditions`: optional object; absent → no filters (but still vendor/safe gating).
- `changes`: required array; each entry is one operation classified by keys present:
  - `add` + `remove` → ReplaceRule
  - `add` + `removeByKeywords` → ReplaceKeywordRule
  - `add` only → AddRule
  - `remove` only → RemoveRule
  - `removeByKeywords` only → RemoveKeywordRule
- `count`: unsigned int. For AddRule default 1; for RemoveRule 0 means "all". For Replace rules
  count is derived from removed items (count field ignored in classification path? parsed but
  ruleCount only set for Add/Remove).

Form string resolution (`Utilities::Forms::GetFormFromString`):
- `"0xABCD1234|Plugin.esp"` → LookupForm by FormID in named mod (mod must be loaded).
- `"EditorID"` → `LookupByEditorID<T>` (any form type).
- No wildcards: `*Claw` is NOT expanded. (lint_cdf_json.py rejects `*`-prefixed add entries.)

Keyword form strings for `removeByKeywords` resolve as BGSKeyword (via GetFormFromString, so
also FormID|Plugin or EditorID).

Condition string formats:
- `containers`/`locations`/etc.: form strings (FormID|Plugin or EditorID).
- `locationKeywords`: EditorID only (`RE::TESForm::LookupByEditorID<BGSKeyword>`).
- `playerSkills`: `"SkillName|Level"` (e.g. `"OneHanded|30"`).
- `globals`: `"GlobalEditorID|Value"`.
- `questConditions`: object `{ "questID": "EditorID", "stageDone": [10, 20], "completed": true }`.
- `references`: `"FormIDHex|Plugin.esp"` or EditorID of a TESObjectREFR.
- `plugins`: exact plugin filenames (e.g. `"Skyrim.esm"`).

---

## 6. Compatibility

- **SPID:** CDF targets containers (TESObjectCONT refs), SPID targets actors. Complementary,
  not overlapping. CDF does not distribute spells/perks to actors.
- **KID:** KID distributes keywords to items/actors; CDF distributes items to containers.
  No syntax sharing; do not reuse KID `*`/`#` prefixes in CDF JSON.
- **FLM:** FLM has `*FormList`, `#Group`, `#Collection` special syntax; CDF has none of these.
- **Leveled lists:** CDF resolves TESLeveledList at player level and adds concrete items.
- **Merchants:** left untouched by default (allowVendors false); merchant containers detected
  via faction vendor data cache.
- **Safe containers:** non-respawning / never-reset containers skipped by default
  (bypassUnsafeContainers false). This avoids corrupting quest/unique containers.
- **USSEP / Engine Fixes:** CDF uses Address Library + AE structs; requires Engine Fixes? No
  hard dependency, but Engine Fixes ref-handle limits matter if many items added. Save size
  (iSaveGameMaxSize) relevant with heavy distribution.

---

## 7. Config (INI)

`Data/SKSE/Plugins/ContainerDistributionFramework.ini`:
```ini
[General]
fMaxRefLookupDistance = 25000.0
```
- Clamped to [10000, 150000]. Used for nearest map-marker location lookup for containers
  without a direct location.

---

## 8. Caps / cooldowns / safety checks

- No caps or cooldowns in CDF: every matching rule applies on every Initialize/Reset of a
  container reference. Re-entering a cell re-runs rules (items may be re-added). Mod authors
  must design idempotency via Remove/Replace or accept duplication.
- Safety: unresolved forms skip the change entry (not crash); missing plugin skips the rule.
- Safe-container and vendor gating prevents touching unique/merchant containers by default.
- No wildcard expansion → no accidental mass distribution; exact forms only.
- Logging: all skipped/invalid entries logged with friendlyName and path.

Known source quirk (v3.1.0): `onlyVendors` flag is never assigned (statement `onlyVendors;`
is a no-op), so `onlyVendors: true` may not actually restrict. Verify against 3.1.4 before
relying on it.

---

## 9. Relevance to Runefall

Runefall is native DLL + ESL with DLL-first gameplay. CDF is a direct precedent for a
**native runtime distribution layer** that Runefall can mirror or depend on:

- **Distribution template:** Instead of generating patch ESPs (like Dynamic Container Loot) or
  relying on SPID/KID for actors, CDF hooks TESObjectREFR Initialize/Reset and mutates
  container inventories at runtime. Runefall can adopt the same hook points for rune/loot
  injection, or simply ship CDF JSON rules alongside Runefall data.
- **Native runtime distribution vs SPID/KID:** SPID/KID distribute to actors/forms via
  `_DISTR.ini`; CDF distributes items to containers via JSON. For container loot, CDF is the
  correct tool; for actor spells/perks, SPID/KID. Runefall should not conflate their syntax.
- **JSON grammar:** Runefall's data tables can follow CDF's `{ rules: [ { friendlyName,
  conditions, changes } ] }` shape, with exact form strings (`FormID|Plugin.esp` or EditorID),
  no wildcards, and conditions as ANDed filters. This matches Runefall's data-driven table
  policy.
- **Filters:** location/worldspace/locationKeyword/container/reference/quest/global/playerSkill
  filters plus vendor/safe gating are a ready-made filter vocabulary for Runefall's distribution
  conditions. The `!`-prefix inversion and parent-location/worldspace traversal are reusable
  patterns.
- **Caps:** CDF has no caps/cooldowns and re-runs on every init/reset → duplication risk.
  Runefall should add idempotency (e.g. a marker/flag on the container or co-save entry) if
  using CDF-style runtime injection, or prefer one-shot distribution with a saved "distributed"
  set.
- **Safety checks:** adopt CDF's unresolved-form-skip, missing-plugin-skip, safe-container
  default-skip, and merchant-default-skip policies to avoid breaking quest/unique/merchant
  containers.
- **Packaging:** CDF ships only DLL + INI + JSON (no ESP). Runefall can ship optional CDF
  JSON rule packs as a compatibility layer without touching its core ESL.

Source paths for follow-up:
- Installed: `rpg-mechanism\Container Distribution Framework 120152 3.1.4 2026-10-24T04-57Z 9LMpgbfIm\`
- Upstream: `https://github.com/SeaSparrowOG/DynamicContainerInventoryFramework` @ `38ca392...`
- Skill: `.agents\skills\skyrim-container-distribution\`

---

*All source paths are read-only references. No modifications made to rpg-mechanism.*
