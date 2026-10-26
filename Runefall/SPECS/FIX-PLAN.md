# Runefall — Fix Plan (from 4-agent audit, 2026-10-27)

Status: **P1 safety fixes applied 2026-10-27** (runtime loot verified in-game). Version stays `0.2.0-dev`.

## Status update 2026-10-27

- **P1.1 FIXED:** runtime loot now uses the engine's `InventoryChanges::EnchantObject` instead of a
  manually allocated ExtraDataList + AddObjectToContainer. Verified in-game: probe item "Refined
  Iron Sword" and death drop "Mythic Iron Mace" with 4 affix effects on a Bandit corpse, no crash.
- **P1.2-P1.8 FIXED:** FormResolver singleton captured/guarded; CoSave section/item caps and
  rarity/socket validation; null message guards in both plugins; DynamicEnchantment factory and
  data-handler logs; Core/DataStore filesystem error_code guards; explicit loot failure logs.
- **P1.9 N/A:** `AddObjectToContainer` returns void in this CommonLibSSE-NG version; documented.
- Runtime loot default is now **enabled** (`bEnableRuntimeLoot=1`) and both probe/death paths pass.
- **P3.1 FIXED:** FormKeyAllocator now moves ModHeader.Stats.NextFormID to the L3 §1.1 category floors (KYWD 0x800, GLOB 0x880, MGEF 0x900, SPEL 0xA00, PERK 0xB00, MISC 0xB80, COBJ 0xC00, QUST/FLST 0xC80; packs start MGEF at 0x900 and hunt MISC at 0xB80).
- **P3.2 FIXED:** validator now enforces each category range and fails on violations; generator RUNEFALL PASS.
- **P3.8 FIXED:** release archive now contains Data/SKSE/Plugins/Runefall.dll (verified in zip).
- **P5.1 PARTIAL:** FOMOD info.xml and pack manifests now use 0.2.0-dev; CMake/DLL metadata still 0.2.0 until promotion.
- **P2 FIXED (key items):** D-003/D-015 marked superseded by D-040; D-010 closed by D-022;
  D-031 ranges updated to L3 §1.1; D-038 retitled 120 affixes/600 rows; unique totals standardized
  to 59; boss budget 20; MGEF ~201; records reconciled to ~465-495; hunt families 19 in L3E.
  Deeper cross-reference/open-question tables remain nice-to-have.
- **P5.3/P5.8 PARTIAL:** src/plugin/README.md now documents runtime requirements, build, safety
  and diagnostics; root README documents dependencies. Nexus media plan/credits still pending.
- **P4.9 PARTIAL (2026-10-27):** death hook now grants the race-family hunt material on first kill
  (Draugr -> "Draugr Essence" verified in-game via local FormID fallback); unique-affix granting
  and hunt-category wiring remain.
- Remaining work: P3.3-P3.7/P3.9-P3.17 data pipeline, P4 feature gaps, P5 release gates.

Audit sources:
- Spec consistency audit (15 findings)
- Implementation vs spec audit (17 modules, 5/11 hooks, 6/9 co-save sections, ~10/30 Papyrus functions)
- Safety/runtime audit (score 7.5/10, 3 top CTD vectors)
- Data pipeline audit (37 findings)
- Compatibility/ship gate audit (10 categories, release NOT READY)

Total findings: ~100 across all audits. Organized by priority below.

---

## Priority 1 — Safety / CTD (must fix before any feature work)

### P1.1 ExtraDataList ownership (CRITICAL)
- **Files:** `src/plugin/src/LootService.cpp:160-172, 217-227`
- **Issue:** `new RE::ExtraDataList()` passed to `AddObjectToContainer` with return value ignored. Engine ownership of the extra list is unproven (same reason `AttachToEntry` is disabled).
- **Fix:** Either (a) verify ownership by testing in a minimal repro and document proof, or (b) fail-closed: disable the loot injection path and log, until ownership is proven. Do not ship with unproven ownership.
- **Effort:** Small (disable) or Medium (prove).

### P1.2 FormResolver null deref (HIGH)
- **Files:** `src/plugin/src/FormResolver.cpp:19-22`
- **Issue:** `RE::TESDataHandler::GetSingleton()` called twice; first result not captured/checked before `->LookupModByName("Skyrim.esm")`.
- **Fix:** Capture the singleton into a local, null-check, log and return on null. Pattern:
  ```cpp
  auto* dh = RE::TESDataHandler::GetSingleton();
  if (!dh) { logger::error("FormResolver: TESDataHandler null"); return; }
  auto* mod = dh->LookupModByName("Skyrim.esm");
  ```
- **Effort:** Small.

### P1.3 Uncapped sectionCount in CoSave (MEDIUM)
- **Files:** `src/plugin/src/CoSave.cpp:250`
- **Issue:** `sectionCount` (U32) not capped; malformed save can force unbounded iteration/allocation.
- **Fix:** Cap `sectionCount` at a reasonable maximum (e.g., 64) before the loop. Return early with a log if exceeded.
- **Effort:** Small.

### P1.4 socketCount / rarity validation in CoSave (MEDIUM/LOW)
- **Files:** `src/plugin/src/CoSave.cpp:284, 286`
- **Issue:** `item.rarity` not range-checked (0-5); `socketCount` (U8) not validated (<=3).
- **Fix:** Reject items with `rarity > 5` or `socketCount > 3` with a log; skip the item rather than storing invalid state.
- **Effort:** Small.

### P1.5 Null-guard a_msg in OnMessage handlers (MEDIUM)
- **Files:** `src/plugin/src/Plugin.cpp:111`, `src/plugin/src/stage/StagePlugin.cpp:80`
- **Issue:** `switch (a_msg->type)` with no null guard on `a_msg`.
- **Fix:** Add `if (!a_msg) return;` at the top of both handlers.
- **Effort:** Small.

### P1.6 Missing logs in DynamicEnchantment::Create (MEDIUM)
- **Files:** `src/plugin/src/DynamicEnchantment.cpp:59-64`
- **Issue:** `IFormFactory::GetFormFactoryByType` and `factory->Create<EnchantmentItem>()` return null without logging.
- **Fix:** Add `logger::error` calls on both null returns, naming the failing step.
- **Effort:** Small.

### P1.7 FindGameRoot / DataStore unguarded filesystem calls (LOW)
- **Files:** `src/plugin/src/Core.h:76-93`, `src/plugin/src/DataStore.cpp:137-148`
- **Issue:** `fs::exists()` without `error_code`; permission/IO errors throw `filesystem_error` (uncaught).
- **Fix:** Use `fs::exists(path, ec)` overload and log on error; return empty/defaults.
- **Effort:** Small.

### P1.8 LootService missing log on HasData() false (LOW)
- **Files:** `src/plugin/src/LootService.cpp:128-129`
- **Issue:** `SpawnForActor` returns false when `HasData() == false` without logging which condition failed.
- **Fix:** Log the specific failed condition (data loaded? gameplay ready?).
- **Effort:** Small.

### P1.9 AddObjectToContainer return value (MEDIUM)
- **Files:** `src/plugin/src/LootService.cpp:172, 227`
- **Issue:** Return value of `AddObjectToContainer` not checked.
- **Fix:** Check return; log on failure; do not assume success.
- **Effort:** Small.

---

## Priority 2 — Spec consistency (fix before implementation continues)

### P2.1 Stale one-ESP decisions
- **Files:** `DECISIONS.md` (D-003, D-015)
- **Fix:** Mark D-003 and D-015 as superseded by D-040. Add "Superseded by D-040" note.

### P2.2 UI framework decision pending
- **Files:** `DECISIONS.md` (D-010)
- **Fix:** Close D-010 as superseded by D-022 (SKSE Menu Framework for v1).

### P2.3 Stale FormID map in D-031
- **Files:** `DECISIONS.md` (D-031)
- **Fix:** Update D-031 to the revised map from D-035, or mark superseded by D-035.

### P2.4 D-038 title mismatch
- **Files:** `DECISIONS.md` (D-038)
- **Fix:** Rename title to "120 affixes / 600 rows". Retain 108 as coverage-derived intermediate.

### P2.5 Unique-affix total 40 vs 59
- **Files:** `SPECS/L3-CONTENT-DATA.md` (sections 1, 3.5.1), `SPECS/L3B-CONTENT-MATH.md` (sections 11, 12), `SPECS/L3D-UNIQUE-AFFIXES-MINING.md` (sections 8, 9), `SPECS/L5-COMPAT-RELEASE.md` (Unique module), `SPECS/GAP-REGISTER-FUTURE.md` (roadmap), `PLAN.md`
- **Fix:** Standardize on 59 total (20 boss + 20 quest + 19 hunt). Update stale 40-only statements to "59 (40 base + 19 hunt)".

### P2.6 Boss-template count 12-16 vs 20
- **Files:** `SPECS/L3-CONTENT-DATA.md` (1.1, 2), `SPECS/L3A-CONTENT-SIZE-COMPARISON.md` (G4), `SPECS/L3B-CONTENT-MATH.md` (section 7), `SPECS/L3C-REFERENCE-PARITY.md` (section 3), `SPECS/L3D-UNIQUE-AFFIXES-MINING.md` (section 8)
- **Fix:** Adopt 20 as v1 budget. Update L3, L3A, L3B. State how many templates live in base ESL vs Bosses pack.

### P2.7 MGEF total 182 vs ~201
- **Files:** `SPECS/L3-CONTENT-DATA.md` (section 1 table: 182; sections 1.1/2: ~201), `SPECS/L3B-CONTENT-MATH.md` (sections 2.4/12)
- **Fix:** Update L3 table to ~201 (120 affix + 62 skill/monster/utility + 19 hunt).

### P2.8 Records-total drift
- **Files:** `SPECS/L3A-CONTENT-SIZE-COMPARISON.md` (~440-470), `SPECS/L3C-REFERENCE-PARITY.md` (~435-465), `SPECS/L3E-HUNT-AND-DROP-TABLES.md` (~480-510), `SPECS/L3-CONTENT-DATA.md` / `SPECS/L3B-CONTENT-MATH.md` (~465-495)
- **Fix:** Reconcile all layers to final v1 figure ~465-495. Label earlier figures as pre-revision.

### P2.9 Hunt families below threshold 8 vs 10
- **Files:** `SPECS/L3E-HUNT-AND-DROP-TABLES.md` (section 2: 8 listed), `SPECS/GAP-REGISTER-FUTURE.md` (F-08: 10), `SPECS/L3D-UNIQUE-AFFIXES-MINING.md` (29/19 math)
- **Fix:** Update L3E list to 10 (add bear, bristleback) or correct total to 8 everywhere.

### P2.10 Economy L2 mapping missing
- **Files:** `SPECS/L1-GAMEPLAY-MODEL.md` (sections 9-10, E-00..E-11), `SPECS/L2-SKYRIM-MAPPING.md`
- **Fix:** Add an L2 economy section mapping faucets/sinks/pity/anti-abuse/cost multiples/RF Tier bands to engine/data surfaces, or explicitly state economy is data-only (DLL/DataStore evaluated).

### P2.11 L1 open questions 12-13 not in L2 Phase 0
- **Files:** `SPECS/L1-GAMEPLAY-MODEL.md` (section 12, items 12-13), `SPECS/L2-SKYRIM-MAPPING.md` (section 11)
- **Fix:** Add Phase 0 items for (a) AVDamageResist armor-channel verification in 1.6.1170, (b) non-combat anchor hook safety (gather, craft, potion, spell learning, fall reduction, sneak reduction).

### P2.12 L2 doesn't resolve 13 L1 open questions
- **Files:** `SPECS/L1-GAMEPLAY-MODEL.md` (section 12), `SPECS/L2-SKYRIM-MAPPING.md`
- **Fix:** Add an L1-open-question → L2-resolution table in L2 (resolved / Phase 0 / deferred).

### P2.13 FLST/QUST no L4 owner
- **Files:** `SPECS/L3-CONTENT-DATA.md` (section 2), `SPECS/L4-RUNTIME.md` (section 1)
- **Fix:** Add ownership notes (MenuApi owns FLST pools; PapyrusBridge owns QUST hosts) or remove unused record types from L3 budget.

### P2.14 L4 modules without L5 test/compat entries
- **Files:** `SPECS/L4-RUNTIME.md` (section 1), `SPECS/L5-COMPAT-RELEASE.md` (sections 2, 6)
- **Fix:** Extend L5 test matrix with per-module smoke checks (hotkeys, logging export, UI snapshot/action, pack disable/reenable, respec/runeforge transactions, bridge API).

### P2.15 Crafting module missing from L5 FOMOD matrix
- **Files:** `SPECS/L3-CONTENT-DATA.md` (section 5), `SPECS/L5-COMPAT-RELEASE.md` (section 4)
- **Fix:** Add "Crafting (reserved, not in v1)" row to L5 FOMOD matrix, or remove from L3's FOMOD list.

---

## Priority 3 — Data pipeline

### P3.1 FormKeyAllocator not category-aware (CRITICAL)
- **Files:** `tools/Runefall.Build/PluginFactory.cs:11-19`
- **Issue:** Single sequential counter from 0x800; records violate L3 §1.1 category windows (GLOBs at 0x812 instead of 0x880, MGEFs at 0x848 instead of 0x900, etc.).
- **Fix:** Make allocator category-aware: each record type allocates from its designated range base. E.g., `Allocate(FormType kind)` returns next ID in that kind's range. Add range-exhaustion check.
- **Effort:** Medium.

### P3.2 Validator missing category-range enforcement (CRITICAL)
- **Files:** `tools/Runefall.Build/Validator.cs:37-42`
- **Issue:** Only rejects IDs >= 0x1000; never checks 0x800-0x87F KYWD, 0x880-0x8FF GLOB, etc.
- **Fix:** Add per-category range checks matching L3 §1.1. Report all violations (don't `break` on first).
- **Effort:** Medium.

### P3.3 attributes.csv column semantics (HIGH)
- **Files:** `tools/Runefall.Build/ContentFactory.cs:173-192`, `attributes.csv`
- **Issue:** `effectKey` holds `attr.<name>` while `perPoint` holds compound `key:value;...` string. Spec says effectKey is stable modifier key, perPoint is numeric.
- **Fix:** Split into proper columns: `attributeId,name,effectKey,perPoint,softCap,hardCap`. Move compound effects to a separate `attribute_effects.csv` or encode as structured data.
- **Effort:** Medium.

### P3.4 attributes.csv not consumed by DLL (HIGH)
- **Files:** `src/plugin/src/DataStore.cpp` (GetTable calls), `src/plugin/src/Progression.cpp`
- **Issue:** `attributes.csv` loaded but never consumed. Derived attribute effects not applied.
- **Fix:** Implement attribute effect application in Progression module: read attributes.csv, apply per-point effects to GLOBAL records, enforce soft/hard caps.
- **Effort:** Large.

### P3.5 Boss NPC templates not generated (HIGH)
- **Files:** `tools/Runefall.Build/ContentFactory.cs` (no boss generation), `Validator.cs:113` (WARN only)
- **Issue:** Spec'd 12-20 boss NPC templates not generated. Validator only warns.
- **Fix:** Add boss NPC template generation (FormID range 0xD00+, NPC_/CLAS records). Make it an error if count < minimum.
- **Effort:** Large.

### P3.6 Missing cross-reference validation (HIGH)
- **Files:** `tools/Runefall.Build/Validator.cs`
- **Issue:** No validation of skills.csv rank1-3 EditorIDs against SPEL/PERK records; recipes.csv material references; affixes.csv effectEditorId for all 600 rows; L3B coverage gates; L3E drop table gates; localization coverage.
- **Fix:** Add validation methods for each cross-reference. Implement L3B coverage gates (attribute×category ≥2, skill coverage ≥2, slot coverage ≥18, tier monotonic, build parity 40/40/40, category pools ≥21). Implement L3E gates (race group member counts, 36-cell coverage, firstOnly rules, unique pool separation).
- **Effort:** Large.

### P3.7 10 of 18 CSVs loaded but not consumed (HIGH)
- **Files:** `src/plugin/src/DataStore.cpp`, consumers
- **Issue:** `materials.csv`, `recipes.csv`, `skills.csv`, `attributes.csv`, `unique_affixes.csv`, `unique_sources.csv`, `strings.csv`, `drops_containers.csv`, `drops_quests.csv`, `drops_bosses.csv` loaded but never consumed.
- **Fix:** Implement consumers for each: materials (MaterialRegistry), recipes (RuneForge/Crafting), skills (Progression), attributes (Progression), unique_affixes (LootEngine), unique_sources (HuntEngine), strings (MenuApi/localization), drops_* (LootEngine).
- **Effort:** Large (phased).

### P3.8 FOMOD missing Data\SKSE (HIGH)
- **Files:** `fomod/ModuleConfig.xml:8`, `build/dist/Data` (no SKSE dir)
- **Issue:** FOMOD references `Data\SKSE` which doesn't exist in build/dist. DLL not in FOMOD tree.
- **Fix:** Add `Data\SKSE\Plugins\Runefall.dll` to build/dist in build-release.ps1 (already copies DLL separately). Or add SKSE directory to FOMOD source and copy DLL there.
- **Effort:** Small.

### P3.9 FOMOD module matrix mismatch (MEDIUM)
- **Files:** `fomod/ModuleConfig.xml`, `SPECS/L5-COMPAT-RELEASE.md` (section 4)
- **Issue:** FOMOD ships only Core + 2 optional packs (Unique, Hunt). L5 expects 10 modules. No dependency/conflict declarations.
- **Fix:** Either (a) reconcile L5 to actual shipped modules (remove Bosses/Affixes/Quests/Patches/UI/Balance from v1 matrix, add as "reserved" rows), or (b) generate the missing packs. Add dependency/conflict declarations per L5 rules.
- **Effort:** Medium.

### P3.10 Monster affixes not generated or applied (MEDIUM)
- **Files:** `tools/Runefall.Build/ContentFactory.cs`, `src/plugin/src/TierEngine.cpp`
- **Issue:** Pool of 10 monster affixes spec'd but not generated as records or applied at runtime.
- **Fix:** Generate MGEF/SPEL/PERK records for 10 monster affixes. Apply 1-4 affixes per ranked enemy at spawn via TierEngine.
- **Effort:** Large.

### P3.11 KYWD/GLOB/MGEF count shortfalls (MEDIUM)
- **Files:** `tools/Runefall.Build/PluginFactory.cs`, `ContentFactory.cs`
- **Issue:** KYWD 18 (spec ~60), GLOB 29 (spec ~40), MGEF 188 (spec ~201).
- **Fix:** Add missing keywords (skill, slot, socket, scratch, material-tier markers), globals (RF tier state, feature gates), MGEFs (monster-affix 20, utility/visual/condition 15, passive skill effects).
- **Effort:** Medium.

### P3.12 strings.csv coverage (MEDIUM)
- **Files:** `tools/Runefall.Build/ContentFactory.cs:430-447`
- **Issue:** 82 strings vs spec ~254 keys.
- **Fix:** Add missing localization keys for all player-facing FULL strings (affix names, rarity, skills, attributes, materials, recipes, UI).
- **Effort:** Medium.

### P3.13 Papyrus script count (MEDIUM)
- **Files:** `src/papyrus/` (2 scripts)
- **Issue:** Only 2 scripts vs spec 5-10.
- **Fix:** Add missing scripts (UI glue, optional integrations, quest hosts). Compile all with build-papyrus.ps1.
- **Effort:** Medium.

### P3.14 Naming convention inconsistencies (LOW)
- **Files:** `tools/Runefall.Build/ContentFactory.cs:200-205`, `PluginFactory.cs:107`, `PluginFactory.cs:66-68`
- **Issue:** Spell/perk IDs lowercase vs MGEF mixed-case; rank keywords use RuneTouched vs spec rank markers.
- **Fix:** Standardize naming: use consistent case for skill families; use spec rank markers (Elite/Champion/Mini-boss/Boss/Rune-Mythic).
- **Effort:** Small.

### P3.15 drop table consumption (HIGH)
- **Files:** `src/plugin/src/LootEngine.cpp`, `DataStore.cpp`
- **Issue:** Only `drops_general.csv` consumed; container/quest/boss/race tables not wired to loot hook.
- **Fix:** Implement container/quest/boss/race drop resolution in LootEngine. Wire race drops to HuntEngine first-kill logic.
- **Effort:** Large.

### P3.16 Pity, magic find, smart drops not implemented (HIGH)
- **Files:** `src/plugin/src/LootEngine.cpp`
- **Issue:** L-06 pity, L-05 magic find, L-07 smart drops spec'd but not implemented.
- **Fix:** Add pity counter (hidden, per-kill without Refined+), magic find weight shift, smart drop bias (weapons only default).
- **Effort:** Medium.

### P3.17 Loot authority modes (MEDIUM)
- **Files:** `src/plugin/src/Config.cpp`, `src/plugin/src/LootService.cpp`
- **Issue:** Config has loot authority flag but no mode implementation (additive/hybrid/takeover).
- **Fix:** Implement additive (default) mode; hybrid and takeover as optional modules.
- **Effort:** Medium.

---

## Priority 4 — Implementation gaps

### P4.1 Missing modules: HookManager, Compatibility, Logger
- **Files:** absent from `src/plugin/src/`
- **Fix:** Create `HookManager.h/.cpp` (centralize 11 hooks), `Compatibility.h/.cpp` (loot authority, mod detection), `Logger.h/.cpp` (wrap spdlog with rate-limiting and export). Update Plugin.cpp bootstrap.
- **Effort:** Large.

### P4.2 Missing hooks (6 of 11)
- **Files:** `src/plugin/src/LootService.cpp` (ActorDeath only), no ContainerGeneration/ActorSpawn/LoadedCellAttach/MenuOpen/ItemAdded
- **Fix:** Implement each hook:
  - ContainerGeneration: roll loot when container generates (primary loot path).
  - ActorSpawn: assign rune rank + buff package (TierEngine).
  - LoadedCellAttach: cell-level processing (density caps).
  - MenuOpen: salvage/crafting UI entry point.
  - ItemAdded: rune currency aggregation, salvage validation.
  - MenuRender: already via SKSE Menu Framework.
- **Effort:** Large.

### P4.3 Missing co-save sections (3 of 9)
- **Files:** `src/plugin/src/CoSave.cpp`
- **Issue:** Sections 5 (container state), 6 (unique sources), 9 (migration) missing. Flags field missing from header.
- **Fix:** Add sections 5, 6, 9 to Encode/Decode. Add `u32 flags` to header. Implement migration stub (N-01).
- **Effort:** Medium.

### P4.4 Papyrus bridge gaps
- **Files:** `src/plugin/src/PapyrusBridge.cpp`
- **Issue:** Missing 10 query/mutation functions + 6 events.
- **Fix:** Implement `GetRuneTotal`, `GetItemRarity`, `GetItemEnhancements`, `GetLootAuthorityMode`, `GetDiagnostics`, `AddRunes`, `SpendRunes`, `RespecAttributes`, `RespecSkills`, `SalvageItem`, `RerollItem`, `UpgradeItem`, `SocketItem`, `ExtractSocket`, `SetDebugLootRoll`. Add all 6 `SendModEvent` calls.
- **Effort:** Large.

### P4.5 Derived attribute effects not applied
- **Files:** `src/plugin/src/Progression.cpp`
- **Issue:** Attribute points stored but derived effects (per P-01 table) not applied to player. Soft cap defined but not enforced in spend.
- **Fix:** Apply derived effects via GLOBAL records + PERK/MGEF/SPEL records granted at runtime. Enforce soft cap (half gains 31-60). Implement rune-bought points (P-07) and respec (P-08).
- **Effort:** Large.

### P4.6 Skill gates and casting not implemented
- **Files:** `src/plugin/src/Progression.cpp`, `src/plugin/src/RuneForge.cpp`
- **Issue:** Generic SkillProgress vector; no skills.csv loading; no Skyrim-skill/attribute gate validation; no active/passive spell/perk grants; no casting model (D-021 native hotkey layer).
- **Fix:** Load skills.csv; validate gates (Skyrim skill 20/50/75 + attribute 10/20/30); grant SPEL/PERK records at runtime; implement native hotkey cast model with cooldowns.
- **Effort:** Large.

### P4.7 Monster affixes not applied
- **Files:** `src/plugin/src/TierEngine.cpp`
- **Issue:** 10 monster affixes spec'd but not generated or applied.
- **Fix:** Generate records (P3.10); apply 1-4 affixes per ranked enemy at spawn; add visual/name indicators.
- **Effort:** Large (depends on P3.10).

### P4.8 Materials not tracked/consumed
- **Files:** `src/plugin/src/DataStore.cpp`, `src/plugin/src/RuneForge.cpp`
- **Issue:** materials.csv loaded but no consumer. No material tracking in co-save.
- **Fix:** Create MaterialRegistry; track material counts in co-save; consume in RuneForge (reroll/upgrade costs). Implement salvage transaction (destroy item, grant materials).
- **Effort:** Large.

### P4.9 Unique affixes not implemented
- **Files:** `src/plugin/src/LootEngine.cpp`, `src/plugin/src/HuntEngine.cpp`
- **Issue:** 59 unique affixes spec'd (20 boss + 20 quest + 19 hunt) but not in loot pools. Hunt first-kill logic exists in data but not wired to death hook.
- **Fix:** Add unique affixes to LootEngine pools; wire HuntEngine first-kill to ActorDeath hook; grant unique affix on first kill of each race family.
- **Effort:** Large.

### P4.10 Attunement not implemented
- **Files:** `src/plugin/src/Progression.cpp`
- **Issue:** RF Tier advancement (kill tier boss + forge ritual + rune cost) not implemented.
- **Fix:** Implement attunement flow: boss kill flag → forge activation → rune cost deduction → RF Tier increment. Add Papyrus bridge function.
- **Effort:** Medium.

### P4.11 Socket-add recipe
- **Files:** `src/plugin/src/RuneForge.cpp`
- **Issue:** Socket count fixed by rarity (I-02); no adding sockets in v1 (reserved for Crafting Table expansion). Spec says no adding sockets in v1.
- **Fix:** Not needed for v1. Document as deferred to Crafting Table expansion.

### P4.12 MenuApi incomplete
- **Files:** `src/plugin/src/MenuApi.cpp`
- **Issue:** Minimal JSON snapshot; missing item detail, diagnostics, debug RNG seed, hot reload.
- **Fix:** Extend MenuApi with item detail snapshot, diagnostic export (N-08), debug RNG seed (dev).
- **Effort:** Medium.

### P4.13 SoftCappedAttribute formula deviation
- **Files:** `src/plugin/src/Progression.cpp` (SoftCappedAttribute)
- **Issue:** Formula `45 + clamp(raw-60,0,15)` deviates from spec (half gains 31-60, hard cap 60).
- **Fix:** Correct to: full gains 1-30, half gains 31-60, blocked above 60.
- **Effort:** Small.

### P4.14 RuneForge rune-unit semantics
- **Files:** `src/plugin/src/RuneForge.cpp`
- **Issue:** Spends runes from lowest rank upward; should use rune-unit ladder (R1=1, R2=5, R3=25, R4=125, R5=625).
- **Fix:** Convert costs to rune units; spend from highest rank first (most efficient), or allow player choice.
- **Effort:** Medium.

---

## Priority 5 — Release blockers

### P5.1 Version-string mismatch
- **Files:** `CURRENT.txt`, `CHANGELOG.txt`, `fomod/info.xml`, `CMakeLists.txt`, `version.rc.in`, pack manifests
- **Issue:** CURRENT/CHANGELOG say 0.2.0-dev; FOMOD/CMake/DLL/manifests say 0.2.0.
- **Fix (user decision: keep 0.2.0-dev):** Align FOMOD `info.xml` `<Version>`, CMake `project(VERSION)`, DLL metadata, and pack manifests `minBaseVersion` to `0.2.0-dev`. Update all to `0.2.0` only when promoting to release.
- **Effort:** Small.

### P5.2 FOMOD module matrix
- **Files:** `fomod/ModuleConfig.xml`, `SPECS/L5-COMPAT-RELEASE.md` (section 4)
- **Issue:** Only 2 optional packs (Unique, Hunt) vs L5's 10 modules. No dependency/conflict declarations.
- **Fix:** Reconcile L5 to actual shipped modules (remove Bosses/Affixes/Quests/Patches/UI/Balance from v1 matrix, add as "reserved"). Or generate missing packs. Add dependency/conflict declarations per L5 rules.
- **Effort:** Medium.

### P5.3 Nexus publication deliverables
- **Files:** `src/plugin/docs/nexus-page.md` (placeholder), `scripts/generate-nexus-page.py` (broken, depends on missing README.md), `src/plugin/README.md` (absent)
- **Issue:** Page generator broken (missing README.md); no media plan; no AI disclosure; no credits.
- **Fix:** Create `src/plugin/README.md` with project info. Fix generator. Create `NEXUS-MEDIA-PLAN.md`. Add AI disclosure. Finalize credits (CommonLibSSE-NG MIT, no Bethesda assets, etc.).
- **Effort:** Medium.

### P5.4 Runtime reconciliation missing
- **Files:** `VALIDATION.md`, `src/plugin/src/PackManager.cpp`, `src/plugin/src/DataStore.cpp`
- **Issue:** No persisted record of packs_discovered/validated/enabled/disabled, data_rows_accepted/rejected, unresolved_editorids, features_disabled.
- **Fix:** Add reconciliation logging in PackManager (after discovery) and DataStore (after load). Write to RunefallProbe.log or a separate reconciliation file. L5: "A pack with zero accepted rows fails the release."
- **Effort:** Medium.

### P5.5 T1 in-game test blocked
- **Files:** `VALIDATION.md` (test-env section)
- **Issue:** Test env contaminated (CC content, 68 light plugins, SKSEMenuFramework 3.14 vs 3.18). Post-MCP heap crash unresolved.
- **Fix:** Rebuild clean test env: remove CC content, install SKSE Menu Framework 3.18, disable conflicting plugins. Resolve heap crash (isolate via stage bisection). Then run T1.
- **Effort:** Large.

### P5.6 N-10 uninstall/pack-disable matrix untested
- **Files:** `SPECS/L5-COMPAT-RELEASE.md` (section 6), `VALIDATION.md`
- **Issue:** Matrix defined but no runtime evidence.
- **Fix:** Execute matrix on clean test env after T1. Record results in VALIDATION.md.
- **Effort:** Medium (depends on P5.5).

### P5.7 Compatibility pass matrix all pending
- **Files:** `SPECS/L5-COMPAT-RELEASE.md` (section 2), `VALIDATION.md`
- **Issue:** 11 targets all "to test"/"pending". Fortunate Foes keyword gap (no KYWD records) needs patch.
- **Fix:** Test each target on copied Mantras profile. Record PASS/PASS WITH PATCH/KNOWN LIMITATION/FAIL. Ship patch pack for Fortunate Foes keyword provider.
- **Effort:** Large (depends on P5.5).

### P5.8 Dependency documentation
- **Files:** no `README.md`, no machine-readable deps in FOMOD
- **Issue:** CHANGELOG claims README documents hard requirements but README doesn't exist. FOMOD has no machine-readable dependency declarations.
- **Fix:** Create README.md with hard/optional deps and versions. Add dependency declarations to ModuleConfig.xml (if FOMOD schema supports) or document in info.xml prose.
- **Effort:** Small.

### P5.9 Native gate (multi-runtime build)
- **Files:** `CMakeLists.txt`, build scripts
- **Issue:** Only 1.6.1170 built/tested. 1.7.x deferred by policy (D-062).
- **Fix:** Not needed for v1 (deferred). Document clearly. No action required.

---

## Summary

| Priority | Findings | Total effort |
|----------|----------|-------------|
| P1 Safety/CTD | 9 | Small-Medium |
| P2 Spec consistency | 15 | Small-Medium |
| P3 Data pipeline | 17 | Small-Large |
| P4 Implementation gaps | 14 | Medium-Large |
| P5 Release blockers | 9 | Small-Large |

**Critical path:** P1 (safety) → P3.1/P3.2 (FormID allocation + validator) → P4.1 (missing modules) → P5.5 (clean test env + T1) → P5.6/P5.7 (matrices) → P5.3 (Nexus deliverables) → release.

**Estimated total effort:** P1 is quick (1-2 sessions). P2 is doc edits (1 session). P3/P4 are substantial (multiple sessions each). P5 depends on P3/P4 + test env.

**Recommendation:** Start with P1 (safety) + P2 (spec consistency, doc-only) in parallel, then P3.1/P3.2 (FormID + validator, critical for data integrity), then P4.1/P4.2 (modules + hooks, critical for functionality), then P5.5 (test env) for T1 validation.