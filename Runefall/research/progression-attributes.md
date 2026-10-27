# Progression & Attributes — reference research

Read-only survey of RPG-mechanism reference mods for Runefall’s nine-attribute / 18-skill progression design.  
Runefall constraints applied throughout: native co-save is authoritative (D-028), no vanilla actor-value repurposing (D-006), derived bonuses re-applied via our own PERK/MGEF/SPEL records on load, DLL-first (D-064).

Base reference root: `D:\Works\source\skyrim-moddings\rpg-mechanism\`

## Reference mod index

| Mod | Extracted folder (relative to rpg-mechanism) |
|---|---|
| Actor Value Generator 2.3.6.0 (main) | `Actor Value Generator - Main File 84743 2.3.6.0 2026-09-06T04-44Z gbrZ4tQ11\` |
| AVG header 2.3.1.3 | `...\AVG Header File 84743 2.3.1.3 2026-08-29T22-22Z Z3mWyIRd6\` (inside main folder) |
| AVG 1.9.5.6 (older) | `Actor Value Generator 1.9.5.6\` |
| Adaptive Attributes SKSE 2.0 (binary) | `Adaptive Attributes SKSE 179815 2.0 2026-10-26T13-29Z rvwm2BrSG\Adaptive Attribute SKSE\` |
| Adaptive Attributes SKSE 2.0 (source) | `...\AdaptiveAttributeSKSE Source 179815 2.0 2026-10-26T13-28Z s6Og0dsRt\AdaptiveAttributeSKSE_Source\` |
| Concentration Strain 1.0 | `Concentration Strain 189317 1 2026-08-23T03-29Z kATsMWjXx\` |
| Dynamic Perk Framework 1.0.0 | `Dynamic Perk Framework 189660 1.0.0 2026-09-18T22-54Z IoFlhfaOb\` |
| Custom Skills Framework 3.1.0 | `Custom Skills Framework\` (+ `Example custom skill-41780-3-1713723980\`) |
| Worlds Dawn SSE 2.14b | `Worlds Dawn SSE ESPFE-111869-2-14b-1708173423\` |
| World Dawn AVG patch | `World Dawn - Actor Value Generator-150740-1-0-1747954878\` |
| Fourth Era Spell-Crafting 1.0.1 | `Fourth Era Spell-Crafting-157287-1-0-1-1763966322\` |

---

## 1. Actor Value Generator (AVG) 2.3.6.0

**Type:** SKSE plugin + data-driven TOML actor-value definitions. No ESP required.  
**Key files:**
- `SKSE\Plugins\ActorValueGenerator.dll`, `.pdb`
- `SKSE\Plugins\ActorValueData\!ActorValue_AVG.toml`, `!Commons_AVG.toml`, `ActorValue_AVG.toml`, `Commons_AVG.toml`
- `SKSE\Lexicon\Resources\ActorValueGenerator\Utility.toml`
- `SKSE\Lexicon\scripts\Shared\AVG.lsi`
- `AVG Header File ... \API_ActorValueGenerator.h`

**Architecture:**
- Three AV classes: **Adaptive** (per-actor, damageable/regenerating, storable base), **Functional** (formula-only, no per-actor storage), **Exclusive** (adaptive on player, functional on NPCs).
- New AVs defined in TOML (`[Name] type = "Adaptive|Functional"`, `alias = "..."`, `get/set` formulas). Adaptive values can alias vanilla AV slots (`alias = "FavorActive"` etc.) — this is Worlds Dawn’s “28-AV technique”.
- Formulas use routines (`GetActorValue`, `GetLevel`, `GetItemCount`, `HasPerks`, `GetHasMagicEffect`, …) and properties; native-flagged routines call into the DLL.
- Distribution: KID `_DISTR.ini` support; `Include` maps plugin → AV list. No ESP/ESL needed.
- SKSE API (`API_ActorValueGenerator.h`): `ResolveExtraValue`, `RegisterForActorValueChange`, `RegisterAVDelegate` (functional formula delegates). `ExtraValue` wrapper resolves name → `RE::ActorValue` at runtime.
- Engine hooks (addresses documented in meta.ini): `ActorValueUpdate`, `RecalculateLeveledActor`, `GetActorValueIDFromName`, `ModActorValue`, `GetActorValue` vfunc, nullified comparisons, enlarged actor-value list.
- Persistence: values live in the **game’s own actor-value system** (saved by Skyrim), not a separate co-save. AVG only remaps IDs/hooks. Functional values are recomputed.

**Relevance to Runefall:**
- **Reject AV aliasing** (D-006): Worlds Dawn repurposes vanilla AV slots (`FavorActive`, `VoicePoints`, …); conflict bomb. Runefall instead stores attribute points in native co-save + our own GLOBAL records and applies derived effects through our own PERK/MGEF/SPEL (re-applied on load).
- **Useful pattern:** data-driven AV definition (TOML/CSV) and functional “proxy” values for conditions; KID-style distribution for attaching AVs to forms by keyword without ESP edits. Formula routines show how derived stats can be expressed as data.
- **Not a persistence model** for Runefall; AVG relies on Skyrim’s AV save, which Runefall cannot use for custom attributes without repurposing.

---

## 2. Adaptive Attributes SKSE 2.0 (binary + source)

**Type:** Native SKSE plugin (CommonLibSSE). Progression on three vanilla attributes (Health/Magicka/Stamina) only, but excellent native patterns.  
**Key files:**
- Binary: `Adaptive Attribute SKSE\SKSE\Plugins\AdaptiveAttributesSKSE.dll`, `AdaptiveAttributesSKSE.ini`
- Source: `src\Plugin.cpp`, `src\Progression\TrainingManager.{h,cpp}`, `src\Serialization\Serialization.{h,cpp}`, `src\Systems\*`, `src\Settings\*`, `src\UI\*`

**Architecture:**
- **Event-driven, not polling:** registers UI/script/book/spell/animation sinks at `kDataLoaded`; movement/spell/combat trackers feed XP via events.
- **Progression model:** XP → milestones → permanent bonus points; exponential required-XP curve (`fXpPerAttributePoint * growth^milestones`); hard caps (`iMaxBonusHealth/Magicka/Stamina`); spillover (magicka points → health/stamina); carry-weight from stamina.
- **Derived application:** applies permanent bonuses by `SetBaseActorValue` on the player’s AV owner; reconciles baseline vs. applied bonuses on load to absorb external deltas or enforce expected values.
- **Baseline capture:** stores `baselineHealth/Magicka/Stamina/CarryWeight`, subtracts expected bonuses to derive baseline; clamps low baselines to 100; drift detection (`kDriftThreshold 0.5`) recalibrates.
- **Persistence (co-save):** SKSE serialization with `SetUniqueID`, save/load/revert callbacks; versioned record (v1→v7 migration) storing XP, bonuses, milestones, spillover, baselines, carry weight, tome-read history, OStim bonus seconds. `ResolveFormID` for form IDs. Revert resets all state.
- **Settings/MCM:** INI + SKSE Menu Framework ImGui panel (`SettingsMenu`), debug logging toggle.
- **Native UI:** HUD notifications via `SendHUDMessage::ShowHUDMessage`.
- **Integration:** OStim multiplier hook, vanilla level-up attribute bonus disable toggle.

**Relevance to Runefall:**
- **Template for Runefall progression service:** XP curves, milestone conversion, caps, spillover, and reconciliation logic map directly onto attribute points. Reuse the *pattern* (baseline + applied bonuses + reconcile-on-load), but store in Runefall co-save section 1, not SKSE serialization of vanilla AVs.
- **Derived bonuses via base AV modification is NOT allowed** (D-006): Adaptive modifies Health/Magicka/Stamina directly. Runefall must apply derived effects through PERK/MGEF/SPEL on our own records, re-applied on load, while keeping raw attribute points in co-save.
- **Baseline/drift reconciliation** is valuable for absorbing external mod changes and preventing double-application; adapt to Runefall’s global records + co-save state.
- **Caps and migration discipline** (v1→v7) exemplify D-008 (versioned co-save).

---

## 3. Concentration Strain 1.0

**Type:** Small native SKSE plugin; input-event handling.  
**Key files:** `src\Plugin.cpp`, `src\Settings.{h,cpp}`, `src\UI.{h,cpp}`

**Architecture:** input button sink on left/right attack/block; concentration-cast session tracking; interrupt after configured seconds; INI settings + SKSE Menu Framework ImGui slider; no persistence.

**Relevance to Runefall:** Low for progression. Useful as a **native input-hook + config + MCM pattern** reference (no Papyrus). Could inform active-skill cast hotkey layer (D-021) but not attributes.

---

## 4. Dynamic Perk Framework 1.0.0

**Type:** Editor + Writer native plugins with addon ABI; perk-tree placement and effect application.  
**Key files:**
- `FOMOD_FrameworkWriter\core\SKSE\Plugins\DynamicPerks\Config\DynamicPerks.ini`
- `...\Config\*.json` (entry points, conditions, UI strings, argument metadata)
- `optional - example addon\Docs\...\AddonAPI.h`
- `optional - example addon\Docs\...\plugin.cpp`, `README.md`

**Architecture:**
- Split **Editor** (SKSE Menu Framework UI, edits JSON on disk) and **Writer** (applies JSON to live perks in game).
- Addon ABI (plain C, append-only structs, versioned `kDynamicPerksAddonAPIVersion`) with tree providers, tree identity (Uid), tree placement, perk-entry info/appliers, cycle begin/end hooks, description overrides.
- Trees are claimed by addons (e.g., Custom Skills Framework) via `ClaimsTree`; writer delegates placement; backup/restore of original node content for undo.
- Description overrides via `SetFormDescriptionOverride` (intercepted `TESDescription::GetDescription`).
- Move entire perk-tree graphs between skills (`MoveTreeGraph`) for “emptying” trees without misaligning skydome.

**Relevance to Runefall:**
- **Reference for native perk-tree manipulation** if Runefall later exposes skill trees as interactive perk trees (18 skills × 3 ranks). Runefall already ships its own PERK records (0xB00 range) and 54 nodes; Dynamic Perk Framework shows how to safely apply/move/backup perk nodes at runtime.
- **Not a dependency:** Runefall should not depend on it (D-005-style optional layer only); but its backup/undo and cycle-hook patterns are reusable for our own writer if needed.
- **ABI pattern** (plain C across DLL boundary, append-only versioned structs) is a good model for any Runefall plugin/addon surface.

---

## 5. Custom Skills Framework 3.1.0

**Type:** SKSE plugin + JSON skill config; skills backed by **global variables**, not actor values.  
**Key files:**
- `SKSE\Plugins\CustomSkills.dll`
- `Source\Scripts\CustomSkills.psc` (native declarations)
- `Example custom skill...\SKSE\Plugins\CustomSkills\myskill.json`

**Architecture:**
- Skill identity/config in JSON: skydome model, showMenu/debug globals, per-skill level/ratio/legendary globals, experience formula, node graph (id/perk/x/y/links).
- Optional perk points (own or vanilla), legendary toggle, skill books/XP perks/workbenches via keywords.
- Leveling either by modifying globals directly or vanilla-style formulas via Papyrus API (`AdvanceSkill`, `IncrementSkill`, …).
- No ESP data beyond perk records; no co-save (globals are saved by Skyrim). Uninstall-safe.

**Relevance to Runefall:**
- **Alternative skill substrate** that avoids AV repurposing, but it is **Papyrus/global-variable driven** and conflicts with D-064 (DLL-first, no polling/gameplay in Papyrus). Runefall’s 18 skills are currently PERK-node based; if a native skill menu is needed, prefer a native implementation (possibly borrowing CSF’s JSON node schema).
- **Useful data model:** node graph (id, perk FormID, x, y, links) and experience-formula block; keyword-based XP/book/workbench hooks.
- **No co-save pattern** (relies on Skyrim globals) — insufficient for Runefall’s versioned state.

---

## 6. Worlds Dawn SSE 2.14b (+ World Dawn AVG patch)

**Type:** Large RPG overhaul using AVG’s 28-AV technique + Papyrus.  
**Key files:**
- `Worlds Dawn SSE ESPFE-111869-2-14b-1708173423\scripts\source\xtd_attributes.psc` (and `.pex`)
- `World Dawn - Actor Value Generator-150740-1-0-1747954878\SKSE\Plugins\ActorValueData\WorldDawnNewAVs_AVG.toml`
- `World Dawn - Actor Value Generator...\scripts\source\xtd_*.psc`

**Architecture:**
- Defines ~20 adaptive AVs via AVG, aliased onto vanilla AV slots (`FavorActive`, `FavorsPerDay`, `VoicePoints`, `OneHandedSkillAdvance`, …).
- Papyrus `XTD_Attributes` quest holds inherent/bonus globals per attribute (Agi/Dex/End/Int/Per/Str/Wis) and derived AV arrays; `SetAttribute` computes `sum` via `GetModifier` (level-scaled) and applies derived bonuses with `ModAV`, refreshing on `PlayerStatusUpdate` mod event.
- Attributes feed derived AVs (crit, speed, carry weight, spell damage, gold find, etc.) with per-attribute weights.
- Uses potions/meshes for attribute items; AVG toml maps each AV to an alias slot.

**Relevance to Runefall:**
- **Explicitly rejected as a model** (D-006): aliasing vanilla AVs is a conflict bomb; Runefall must not do this.
- **Useful concept:** attribute → derived-stat weights and level-scaling modifier (`GetModifier`); non-combat domain coverage (gold find, temper find, loot). Runefall’s `attribute_effects.csv` already encodes per-point derived effects; keep the *coverage idea*, apply via our own MGEF/PERK, not AV aliases.
- **Pattern to avoid:** Papyrus-driven `ModAV` refresh on mod events (polling/update loops prohibited by D-064).

---

## 7. Fourth Era Spell-Crafting 1.0.1

**Type:** SKSE + Papyrus + MCM spell-creation mod.  
**Key files:** `Scripts\*.pex`, `SKSE\Plugins\EM03SpellCrafting.dll`, `Seq\...seq`

**Architecture:** spell crafting via MCM; SKSE helper functions; not attribute progression. Persistence through quests/globals.

**Relevance to Runefall:** Low. Useful only as a **native SKSE + MCM (SKSE Menu Framework/SkyUI) pattern** and as a reminder that derived gameplay (spell effects) should be data-driven; not directly applicable to attributes.

---

## Cross-cutting patterns for Runefall progression

1. **Native co-save is authoritative** (Adaptive Attributes): versioned TLV sections, baseline vs. applied-bonus reconciliation, drift detection, form-ID resolution, revert handling. Runefall already uses co-save sections 1–12; progression is section 1.
2. **No vanilla AV repurposing** (D-006): store attribute points in co-save + our GLOBAL records; derived effects via our own PERK/MGEF/SPEL, re-applied on load.
3. **Caps are data + safety invariants**: soft/hard caps (attributes.csv: softCap 30, hardCap 60; perPoint values) must be enforced in native code; derived bonuses clamped.
4. **Perk/MGEF application pattern** (Dynamic Perk Framework + Worlds Dawn): derived effects should be applied as managed perk nodes/MGEFs and backed up/restored on management changes; re-apply on load.
5. **DLL-first** (D-064): all XP, caps, application, reconciliation in native code; Papyrus only stubs/debug.
6. **Data-driven derived effects** (attribute_effects.csv): 9 attributes × derived stats; validate coverage (every attribute touches a non-combat domain) per D-010/L2A.

## Relevance summary

| Reference | Use for Runefall | Avoid |
|---|---|---|
| AVG | Data-driven AV definitions, functional proxy values, KID distribution | AV aliasing onto vanilla slots |
| Adaptive Attributes | XP curves, milestones, caps, spillover, reconciliation, co-save versioning | Direct SetBaseActorValue on vanilla AVs |
| Concentration Strain | Native input hook + INI + MCM pattern | — |
| Dynamic Perk Framework | Perk tree placement/backup/undo, cycle hooks, ABI discipline | Hard dependency |
| Custom Skills Framework | JSON node graph, XP/book/workbench keywords | Papyrus/global runtime, no co-save |
| Worlds Dawn | Attribute→derived-stat weights, non-combat coverage | 28-AV aliasing, Papyrus ModAV refresh |
| Fourth Era Spell-Crafting | Native+MCM pattern | — |
---

*Generated by research subagent; sources read from extracted mod folders only (read-only).*