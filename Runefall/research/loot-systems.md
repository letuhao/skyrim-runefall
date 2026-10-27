# Loot Systems Research

Evidence extracted from reference mods under `D:\Works\source\skyrim-moddings\rpg-mechanism\` (read-only). Paths below are relative to that root.

Runefall loot pillar (GAME-DESIGN §5): six rarity bands 0-5 (rarity index = enhancement count), affix CSV (`affixId,name,category,tier,points,minValue,maxValue,unit,duration,area,slots,weight,minItemLevel,mgef,npcExclude,prefix,suffix`), additive runtime injection (no leveled-list edits), salvage, magic find. This note maps reference mods to those needs.

## 1. DiabloRim (Nexus 190634, v1.0.7)
Path: `DiabloRim 190634 1.0.7 2026-09-15T16-44Z GkqKVMBdo/DiabloRim_1-0-7/`

Files:
- `SKSE/Plugins/DiabloInSkyrim.dll`
- `SKSE/Plugins/DiabloLoot.ini`
- `SKSE/Plugins/DiabloLoot_affixes.csv`

Architecture:
- SKSE DLL hooks loot generation at container open, corpse/vendor stock, quest rewards. Rolls affixes onto eligible items by creating enchantments from CSV rows (`mgef` magic effect token).
- Rarity bands via point total (1-15) → colour: blue/yellow/purple/orange/red; tier markers `★` repeated per band; `TierMarkerInName` toggles name mutation.
- Affix CSV schema matches Runefall's `affixes.csv` almost 1:1 (affixId,name,category,tier,points,minValue,maxValue,unit,duration,area,slots,weight,minItemLevel,mgef,npcExclude,prefix,suffix).
- Container loot multiplier re-resolves leveled lists N times at same level (not new tables); only random leveled-list content, not hand-placed items. Hard ceiling 64 bonus items.
- Vendor stock rolled on barter menu open and restock; price multipliers by colour band (`PriceMultBlue` etc.); `SellPricesScaled`.
- Quest rewards rolled on inventory entry unless artifact/unique (not in any leveled list), alias still held, multi-stack, or crafted output.
- Weapon charge system optional (charge cost per hit scaled by affix points).
- Discovery tool writes `DiabloLoot_discovered.csv` from third-party enchantment effects for curation.
- Add-on CSVs: `DiabloLoot_affixes_<name>.csv` merge after base (e.g. Summermyst).

Config highlights (`DiabloLoot.ini`): `Distribution`, `QuestRewards`, `ContainerLootMultiplier`, `VendorStock`, `PriceMult*`, `WeaponCharge*`, `WielderBuffs`, `TierMarker`, `TierMarkerInName`, `Discovery`.

Pitfalls: name mutation persists in saves; Grid Inventory handles glyphs better than vanilla menus; price hook scans game code at launch.

Relevance to Runefall: direct template for Runefall's affix CSV and roll timing (container/corpse/vendor/quest). Additive loot multiplier pattern validates "resolve leveled lists again" instead of editing lists. Discovery pass is a model for generating affix CSV from installed mods. Price-by-rarity and charge systems are optional modules. Runefall's rarity 0-5 (0-5 enhancements) maps to DiabloRim's point bands; can reuse CSV schema exactly.

## 2. Grid Inventory - DiabloRim (Nexus 190634, v1.0.9)
Path: `Grid Inventory - DiabloRim 190634 1.0.9 2026-09-20T22-36Z CmfxYjJzk/...`

Files: `GridInventory.dll`, `GridInventory_ui.ini`, `meta.ini`.

Architecture: UI extension for DiabloRim; shop window priced via extension API (not game-code hook). Colours items by band.

Relevance: if Runefall ships a grid inventory, expose pricing/rarity via extension API rather than patching value routine. Otherwise keep `TierMarkerInName=0` and rely on colour.

## 3. Fortunate Foes SE (Nexus 120871, v7.62)
Path: `Fortunate Foes Standard Edition 120871 7.62 2026-07-20T21-59Z 587RqTh02/Data/`

Files: SPID `_DISTR.ini` (`Aptitude_DISTR.ini`, `FF7_BCommander_DISTR.ini`, `FFAlternate_Perks_DISTR.ini`), SkyPatcher INIs under `SKSE/Plugins/Skypatcher/{leveledlist,npc}/`, ESPs.

Architecture:
- Uses SPID to distribute keywords/perks to NPCs by filters and exclusive groups (weighted buckets).
- Uses SkyPatcher `filterByLLNPCs=...:addToLLs=...` to append NPCs to leveled lists at runtime (no ESP leveled-list edits).
- CombatLevel.esp drives level variance.

Relevance: demonstrates runtime leveled-list mutation via SkyPatcher and SPID keyword distribution. Runefall's "no leveled-list edits" can still use SkyPatcher `addToLLs` for enemy tiers/loot lists, keeping changes runtime-injected and reversible. SPID pattern useful for tagging NPCs with rune ranks.

## 4. Summermyst Option (Nexus 190634, v1.0.0)
Path: `Summermyst Option 190634 1.0.0 2026-09-03T22-42Z FpnkHZYYg/`

Files: `SKSE/Plugins/DiabloLoot_affixes_summermyst.csv`.

Architecture: add-on affix table merging into DiabloRim base via `DiabloLoot_affixes_*.csv` glob. Weight 40 vs base 100; curated cuts documented.

Relevance: model for Runefall mod-compat packs: ship `affixes_<mod>.csv` add-ons; document curation rules (script-archetype exclusion, sub-components, toggles, magnitude caps). Weighting controls dominance.

## 5. Worlds Dawn SSE ESPFE (Nexus 111869, v2.14b) + Actor Value Generator (Nexus 150740, v1.0)
Path: `Worlds Dawn SSE ESPFE-111869-2-14b-1708173423/`, `World Dawn - Actor Value Generator-150740-1-0-1747954878/`

Files: `scripts/source/WDContainerScript.psc`, `SKSE/Plugins/Worlds Dawn/strings.json`, AVG `SKSE/Plugins/ActorValueData/WorldDawnNewAVs_AVG.toml`.

Architecture:
- `WDContainerScript`: scans valid containers, skips blacklisted/disabled/token-marked ones, adds a token, forces a reference alias, registers `ObjRefFill` mod event, sends modevent. Pattern for container scanning/registration.
- AVG TOML defines new actor values (attributes) consumed by the mod.

Relevance: container-scan + alias/token marking pattern can drive Runefall container loot hooks without touching leveled lists. New actor values (magic find, rune essence) can be registered via AVG-compatible TOML. Not a loot generator itself.

## 6. Death Drop Overhaul (Nexus 151590, v1.3.6) + Diablo IV Style Loot Beams (Nexus 191798, v1.0.0)
Paths: `Death Drop Overhaul - Latest Version 151590 1.3.6 .../`, `Death Drop Overhaul-Diablo IV Style Loot Beams 191798 1.0.0 .../`

Files:
- DDO: `Data/SKSE/Plugins/DeathDropOverhaul.dll`, `.ini`.
- D4SLB: `DeathDropOverhaulD4SLB.esp`, `SKSE/Plugins/DeathDropOverhaulD4SLB.dll/.ini`, `LightPlacer/.../loot-beams.json`, NIFs/sounds/textures, README.

Architecture:
- DDO: physical gear drop on death (weapons/shields detach from skeleton), collision/motion, despawn timeout 14 days, recoverable from inventory, special-NPC exclusion, IED integration.
- D4SLB: hooks DDO 1.3.6; five tiers (common/magic/rare/legendary/mythic) with value thresholds (`iMagicValue=25`, rare 35, legendary 45, mythic 55), drop worn armor/jewelry, max additional drops 8, spawn delay/follow interval, per-tier volume, beam NIFs + Light Placer JSON (color/radius/fade per tier), sounds.

Loot-beams.json maps each beam NIF to a light (color, radius, fade, flags). Colours: common white-ish (0.92,0.95,1.0), magic blue (0.28,0.53,1.0), rare yellow (1.0,0.83,0.26), legendary orange (1.0,0.41,0.11), mythic purple (0.72,0.32,1.0).

Relevance: corpse-loot presentation layer. Runefall can reuse the 5-beam + light pattern for rarity tiers (0-5) and DDO's physical drop for enemy gear. Threshold-based tiering by item value is a quick classifier; Runefall should instead use rarity band. Note D4SLB requires exact DDO 1.3.6 and SKSE Menu Framework 3; version pinning matters.

## 7. Dynamic Container Loot (Nexus 172018, v0.6.0) + source
Path: `Dynamic Container Loot 172018 0.6.0 2026-09-19T21-30Z Ks18n0GQY/` (source under `DynamicContainerLoot src 172018 sec .../`).

Files: `src/main.cpp`, `src/Compatibility/*`, `src/Merge/ContainerMerger.*`, `src/ESP/*`, `src/HTML/*`, fomod.

Architecture:
- Build-time: at kDataLoaded, merges container records across plugins into a generated `DynamicContainerLoot.esp`, with conflict resolution and an optional HTML report. Hooks: none at runtime for injection? It rewrites ESP records (LeveledItem/CONT) offline.
- Policy engine chooses base/winner sources, additions/removals/counts; protects manually edited patches (fingerprints).
- Supports runtime-only mode (no ESP) if patch disabled.

Relevance: alternative strategy to runtime injection. Runefall's default is additive runtime injection, but DCL shows how to generate a merged container ESP for "takeover" authority mode. Its SaveGame/persistence concerns differ (ESP records persist). Fingerprint protection is a good pattern for generated patches.

## 8. Container Distribution Framework (Nexus 120152, v3.1.4)
Path: `Container Distribution Framework 120152 3.1.4 2026-10-24T04-57Z 9LMpgbfIm/`

Files: `SKSE/Plugins/ContainerDistributionFramework.dll`, `.ini` (`fMaxRefLookupDistance=25000`), `.pdb`.

Architecture: binary SKSE plugin/framework (no source or data files in package). INI only max ref lookup distance. Likely consumed by other mods' JSON configs; provides runtime container distribution API.

Relevance: if Runefall needs a distribution framework dependency, CDF is a candidate for location-based container targeting. But Runefall's own runtime injector (CID-style) avoids extra dependency. Keep CDF as optional integration, not core.

## 9. Container Item Distributor NG (Nexus 193332, v2.1.5.1) + source; legacy CID (Nexus 99486, v2.1.3)
Paths:
- `Container Item Distributor NG for Skyrim 1.7.X 193332 2.1.5.1 .../` (binary + `CID NG Skyrim ... Source Code .../`)
- `Container Item Distributor-99486-2-1-3-1733635860/`

Files (source): `src/Hooks.cpp`, `src/Events.cpp`, `src/Distributor.cpp`, `src/Parser.cpp`, `include/*`, contrib configs.

Architecture:
- Hooks: `TESObjectREFR::Load3D` (containers), `Character::Load3D` (NPC containers), `TESObjectREFR::ResetInventory` (respawn), `TESObjectREFR::SaveGame` (strip added objects before save).
- Events: `TESLoadGameEvent` clears processed/added maps.
- Distributor: per-ref or base-form lookup in `distr_map`; `to_add` (item or leveled item resolved via `AddObjectsFromResolvedList`), `to_remove`, `to_remove_all`; chance roll; location/location_keyword skip; tracks `added_objects` for save stripping.
- Parser: scans `Data/*_CID.ini`; sections `[General]` keys are container refs (form IDs or EditorIDs), values are pipes:
  - Add: `ItemFormID|count|location?chance` (with optional `@locationKeyword`)
  - Remove: `-ItemFormID|count|...`
  - RemoveAll: `-ItemFormID`
  - Chance via `?N`.
- SaveGame hook removes added objects so they are not persisted; re-added on next load via Load3D. This keeps saves clean and avoids duplicate accumulation.
- Legacy CID is same family without NG rebuild.

Relevance: **primary template for Runefall additive container loot.** Runefall should replicate this hook set and save-stripping for runtime-injected items, while driving rolls from its own loot engine instead of static `_CID.ini`. The persistence pattern (strip on save, re-roll on load) is essential for "additive loot, no leveled-list edits" and save safety. Location/keyword filters map to Runefall's loot context (dungeon tags).

## 10. CraftingPullsResourceFromContainers (Nexus 135613, v1.1.1)
Path: `CraftingPullsResourceFromContainers-135613-1-1-1-1754824867/`

Files: `SKSE/Plugins/CraftingPullFromContainer.dll`, `.ini` (`Range=500`, `IgnoreOwnership`, `IncludeMerchantChest`, `PermaLinks`).

Architecture: pulls crafting resources from nearby containers within range into crafting menu; ownership/merchant filters.

Relevance: crafting economy integration. Runefall salvage/crafting can pull materials from nearby containers; supports "salvage destroys item returns fraction" flow. Config shows range/ownership toggles Runefall should expose.

---

## Cross-cutting recommendations for Runefall
- Affix schema: reuse DiabloRim CSV exactly; add Runefall effect spec columns later (ARCHITECTURE §5).
- Runtime injection: adopt CID NG hook set (Load3D/Load3DCharacter/ResetInventory/SaveGame + load-game clear) and save-stripping; roll via LootEngine at container open.
- Additive loot multiplier: resolve leveled lists again (DiabloRim) for chest volume; do not duplicate hand-placed items.
- Rarity VFX: reuse D4SLB 5-beam + light pattern, mapping to rarity 0-5.
- Distribution config: optional SkyPatcher `addToLLs` for enemy tiers; SPID for NPC tagging; CDF as optional location-targeting dependency.
- Compat packs: `affixes_<mod>.csv` merge with weights (Summermyst model); discovery pass to generate from installed mods.
- Salvage/crafting: CraftingPull range/ownership pattern; atomic transactions in DLL.