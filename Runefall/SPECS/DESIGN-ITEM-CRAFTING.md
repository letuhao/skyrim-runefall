# DESIGN-ITEM-CRAFTING — Item and crafting feature cluster

Status: design draft, 2026-10-27. Companion to SPECS/AUDIT-NEW-FEATURES-DESIGN-DEPTH.md and
SPECS/REDTEAM-NEW-FEATURES.md. Resolves five interconnected features: Imbue (RG-055b),
Durability and break (RG-018), Monster Hunter material system (RG-203/207/215/225/232),
Tier upgrade (RG-056/C-05), and Material salvage (C-02 UI + transaction).

Layered per D-013: each feature is given L1 rule + numbers, L2 engine mapping, L3 data
schema, L4 code module, compatibility notes, test entry, and open questions. Mechanic IDs
stay traceable to L1-GAMEPLAY-MODEL.md; new IDs are introduced here and must be folded
into L1 in a follow-up doc pass.

Scope note (D-047): all five features are currently RESERVE/optional. This document is
design only; promotion to v1 requires a register update first. Durability ships as an
optional FOMOD module (default off in core). Imbue, MH materials extension, and any new
tier axis are RESERVE until v1 ships and the register is updated.

---

## 0. Cross-cutting resolutions

### 0.1 I-08 formal amendment (resolves C-DUR-1, audit §2)

L1 I-08 currently reads "No bind-on-pickup, no durability, no item degradation." This is
amended to:

> **I-08 (amended 2026-10-27):** No bind-on-pickup. No item degradation in the **core**.
> Durability and break is an **optional module** (I-10) shipped as a separate FOMOD module
> (default off). When the module is off, items never degrade and SetItemHealthPercent is
> free for the tempering channel (D-023). When the module is on, durability owns item
> health and tempering moves to a co-save field (0.3). Bind-on-pickup is never used in
> any configuration.

### 0.2 D-023 collision resolution: who owns SetItemHealthPercent (resolves C-DUR-2)

D-023 uses `SetItemHealthPercent` as the tempering channel for dynamic item mutation.
Durability also wants item health. Two consumers, one channel. Resolution (channel
ownership rule, single writer):

- **Durability module OFF (default):** `SetItemHealthPercent` represents tempering, as
  today. ItemMetadata has no durability field (or it is ignored). Tempering bonus is
  applied through item health percent.
- **Durability module ON:** durability owns `SetItemHealthPercent`. Tempering bonus moves
  to a new co-save field `ItemMetadata.temperPercent` (float, default 100.0) applied as a
  stat multiplier through the wearer's ability package (Progression), not through item
  health. The DLL switches channels at DataLoaded based on the INI flag and never writes
  both in the same session.
- The switch is logged at startup ("Durability module: ON — tempering channel moved to
  co-save"). A mid-session toggle is not supported; change requires restart.

### 0.3 D-027 compliance: no base-record mutation (resolves A-IMB-1)

All five features obey D-027: no vanilla or shared records are mutated. Imbue, durability,
and tier upgrade state live in `ItemMetadata` (co-save, D-028). Application channels:

- Imbue boost: applied as a runtime fortify effect on the **wearer** (Progression ability
  package), conditioned on the equipped item's imbue state read from the registry. Does
  not touch the item's enchantment, name, or health. No collision with I-05 sockets
  (socket power is encoded in the dynamic enchantment per D-023) or with reroll/upgrade
  (those mutate the item instance, not the wearer).
- Durability: owns item health percent (0.2).
- Tier upgrade: mutates ItemMetadata.rarity/ilvl in place (no new registry ID, S-TU-1).

### 0.4 Co-save schema bump: batch into one migration (resolves N-01 churn)

All new ItemMetadata fields and the MaterialRegistry are added in a single schema bump
**v1 -> v2**, not six separate bumps. Migration function: old saves load with new fields
defaulted (see each feature). New section 10 (materials) added. Downgrades refused with a
clear message (D-052).

### 0.5 Hook map: no new hooks for v1 (resolves P-DUR-1, C-AIL-1 pattern)

The 11-hook map (D-050) is unchanged. Durability degradation is kill/encounter-driven via
the existing `ActorDeath` hook (per-kill cost to equipped items). Per-hit degradation is
deferred to an optional 12th hook (damage) only if Phase 0 proves a clean damage entry
point; until then, event-driven degradation is the design. Imbue use-time break chance is
also evaluated on `ActorDeath` (per kill), not per hit.

---

## 1. Imbue (RG-055b) — new mechanic ID I-09

**Status:** RESERVE (Crafting pack). Design only; no v1 code until register update (D-047).

### L1 — Rule and numbers

- **I-09 Imbue.** A material is fused into an equipment item at a forge to grant a stat
  boost. Replaces the impossible socket-gem/runeword system (GAP-REGISTER §8.1: socket
  gems with colors/links cannot be expressed in Skyrim). Imbue is a **second power
  channel** distinct from sockets (I-05): sockets carry rune power (R1-R5 percent item
  stat), imbue carries a material-derived stat boost. They stack.
- **I-09.1 Cap: one active imbue per item.** No multi-imbue stacking. Applying a new
  imbue to an already-imbued item overwrites the old one (the old imbue is lost, its
  material is not recovered). Overwrite costs a 50% rune surcharge on top of the normal
  cost (replacement fee).
- **I-09.2 Cost.** Materials: 1-2 units of the item's material tier (ore/essence/hide
  matching the item's family; if no match, any material of that tier). Runes: `5 * 5^itemTier`
  rune units (tier 0 = 5, tier 1 = 25, ..., tier 5 = 15625, capped by INI). Plus the
  imbue material itself (1 unit of a material of the chosen tier).
- **I-09.3 Success chance.** Base 70%, +0.5% per point of the Precision attribute
  (P-01, Warrior), capped at 95%, floored at 40%. On failure: material and runes are
  consumed, item is unchanged, no imbue applied (mirrors C-05 failure semantics).
- **I-09.4 Stat boost magnitude by material tier.** Common +3%, Refined +6%, Exotic +10%,
  Mythic +15%, Hunt (tier 4 hunt materials) +20%. Material family maps to stat category:
  ore → offense (melee/ranged damage), essence → elemental/spell magnitude, hide → defense
  (armor/health). The specific stat is chosen by the player from a list of 3 compatible
  stats for that family (UI shows the list; DLL validates the choice).
- **I-09.5 Break (craft-time).** A failed imbue craft consumes materials and leaves the
  item unchanged. No item destruction. Player-visible: "Imbue failed — materials lost."
- **I-09.6 Break (use-time, optional, INI-gated default off).** An applied imbue has a
  per-kill break chance (default 0.5% per enemy kill at RF Tier 2, scaling +0.25% per RF
  tier). On break, the imbue is lost; item survives; player-visible notification. Can be
  re-imbued normally. Default off in v1 to avoid economy churn; enable via INI.
- **I-09.7 Reroll interaction (S-IMB-1).** Reroll (C-04) keeps rarity and sockets; it
  also **preserves** the imbue (imbue is independent metadata, not an enhancement). Reroll
  does not remove or change imbue magnitude.
- **I-09.8 Salvage interaction (M-IMB-1).** Salvage (C-02) returns base materials and
  runes only; the imbued material is **not** recovered. Imbue is a sunk cost; break risk
  is the sink. This prevents imbue from being a risk-free material reversal.
- **I-09.9 Upgrade interaction.** Rarity upgrade (C-05) preserves imbue (in-place metadata
  update). An upgrade that changes item tier does not invalidate the imbue, but the boost
  magnitude is capped at the item's new tier's maximum (no downgrade, no removal).
- **I-09.10 Failure modes.** Imbue fails → materials/runes consumed, item unchanged, log.
  Invalid stat choice → transaction rejected, no consumption. Item already at max imbue
  tier → overwrite allowed with surcharge (0.1). Module disabled (INI) → command returns
  error code, no consumption.

### L2 — Engine mapping

- **Application channel (0.3):** imbue boost is applied as a runtime fortify effect on the
  **wearer** through the Progression ability package, conditioned on the equipped item's
  imbue state read from ItemRegistry. The DLL recomputes the package on equip/unequip/gear
  change. No item mutation. No collision with D-023 (enchantment/name/temper) or I-05
  (socket power in enchantment).
- **Read path:** ItemRegistry lookup by equipped item's registryId → ItemMetadata.imbue* →
  stat key + magnitude → Progression applies fortify effect via GLOBAL record + MGEF.
- **Hook:** MenuOpen (forge) for the craft UI; ItemAdded (metadata finalize) if an imbued
  item is acquired outside our hooks. No new hook.
- **Uninstall:** imbue state is co-save only; removing the DLL leaves items as base forms
  with no imbue effect (engine data gone). Documented limitation: applied fortify effect
  disappears on load.

### L3 — Data schema

- **No new CSV required for v1.** Imbue uses existing `materials.csv` (materialId, tier,
  family) and `attributes.csv` (stat keys). A new optional data file `imbue_stats.csv`
  (material family → compatible stat list) may be added later:
  `family,statKey1,statKey2,statKey3` (3 rows: ore/essence/hide). For v1, the compatible
  stat list is hardcoded in the DLL from the P-01 table (ore → melee/ranged/crit; essence →
  spell magnitude/cooldown/mana; hide → armor/health/block).
- **FormID range:** none new. Imbue uses existing MGEF templates (fortify effects) already
  allocated in the 0x900-0x9FF range. No new records.
- **ItemMetadata new fields (0.4):** `imbueMaterialId` (string), `imbueStatKey` (string),
  `imbueMagnitude` (float), `imbueTier` (uint8), `imbueBreakChancePct` (float, default 0).

### L4 — Code module

- **Owner:** RuneForge (transaction validation + cost deduction) + Progression (application
  of the fortify effect on the wearer). ItemRegistry provides the read path.
- **Papyrus bridge:** new mutation function `int ImbueItem(Form itemRefOrForm, int materialId,
  int statChoiceIndex)` returning 0 = ok, nonzero = error code. New query
  `string GetItemImbue(Form)` returning compact "materialId|statKey|magnitude|tier" or empty.
- **UI surface:** Rune Forge page (L4 §7 #2) gets an "Imbue" action: select item → select
  material (from inventory) → select stat → confirm → result message. Confirmation dialog
  shows cost and success chance.
- **Co-save:** ItemMetadata fields above; schema v2 migration defaults to empty (no imbue).

### Compatibility

- Additive: no vanilla record edits. Imbue boost is a runtime fortify effect on the
  wearer, applied through our own GLOBAL/MGEF records. Other mods' enchantments, tempering
  (when durability off), and socket power are untouched.
- Interaction with reroll: preserved (0.9.7). Interaction with upgrade: preserved (0.9.9).
- Interaction with salvage: material not recovered (0.9.8).

### Test entry

- Unit: ImbueItem with insufficient materials → error code, no consumption. With sufficient
  materials → success/failure roll; on success, ItemMetadata.imbue* populated; on failure,
  fields empty and materials consumed.
- Integration: equip an imbued item → wearer's stat increases by magnitude; unequip → reverts.
- Save/load round-trip: imbue state persists.
- Migration: load a v1 save with v2 DLL → imbue fields default empty, no crash.

### Open questions

- Q1 (channel): Fortify-effect-on-wearer vs. a second dynamic enchantment slot. The
  wearer-effect channel is chosen (0.3); a second enchantment slot is not available in
  Skyrim. If Phase 0 shows the wearer-effect package cannot be recomputed fast enough on
  gear change, fall back to storing the boost in ItemMetadata and applying it through the
  item's own enchantment magnitude (would collide with D-023 affixes — avoid).
- Q2 (break): Whether use-time break chance (0.9.6) ships or stays INI-gated default off.
  Recommend default off for v1 to avoid economy churn; revisit after playtesting.
- Q3 (conflict): "One imbue per item" vs. "one per category." Chosen: one per item
  (simpler, less multiplicative soup). Revisit if playtests show demand for stacking.
- Q4 (field): New fields vs. reusing an existing field. Chosen: new fields (0.4).
- Q5 (channel): 0.3 (no base-record mutation).
- Q6 (failure): material consumed, item unchanged (0.9.3).
- Q7 (save compat): schema v2 migration, defaults empty (0.4).

---

## 2. Durability and break (RG-018) — new mechanic ID I-10 (optional module)

**Status:** OPTIONAL MODULE (FOMOD "Durability", default off). Design complete; ships only
if the Phase 0 proof spike (A-DUR-1) succeeds. Fail-closed if unproven.

### L1 — Rule and numbers

- **I-10 Durability (optional).** Items have a durability pool that degrades during
  combat. At 0 durability the item is **broken** and cannot be equipped; it must be
  repaired at a forge. Repair restores full durability for a cost. Quest items are immune.
- **I-10.1 Pool by rarity.** Common 50, Refined 75, Exquisite 100, Radiant 125, Mythic 150,
  Runeforged 200. Hunt materials (tier 4) use the item's rarity pool, not a separate one.
  Endurance attribute (P-01) adds +1 durability per 2 points (capped +15). Material family
  modifier: hide-family items +25% pool (armor durability).
- **I-10.2 Degradation (kill-driven).** Each enemy kill degrades all equipped Runefall
  items by `degradationRate * rankMultiplier`: base 1 point per kill; rank multiplier
  rank 1 = 1, rank 2 = 2, rank 3 = 3, rank 4 = 5, rank 5 = 8. Only equipped items
  degrade; inventory items do not. Degradation is applied in the ActorDeath hook.
- **I-10.3 Broken state.** At 0 durability: item cannot be equipped (attempt to equip
  shows "Broken — repair at forge"). Item remains in inventory. Broken items can be
  salvaged (yield unaffected by durability). Broken items cannot be imbued or upgraded
  until repaired.
- **I-10.4 Repair.** Cost: 1-2 material units of the item's tier + rune units equal to
  10% of the item's rune value (R-02 ladder). Default: deterministic full repair (cost
  paid, durability restored to max). Optional INI toggle `bRiskyRepair` (default 0): on
  failure (20% chance), materials and runes consumed, durability unchanged. Player-visible
  message on failure.
- **I-10.5 Quest items.** Items tagged `isQuestItem = 1` in metadata never degrade and
  cannot break. Tag is set by the DLL when the item is acquired via a quest reward
  (drops_quests.csv) or when a Papyrus script calls `SetQuestItem(form, 1)`.
- **I-10.6 Module gating.** INI flag `bEnableDurability = 0` default. FOMOD module
  "Durability (optional)" installs the INI preset enabling it. When off, the DLL skips
  all durability logic and the tempering channel stays in SetItemHealthPercent (0.2).
- **I-10.7 Compatibility auto-disable (M-DUR-1).** At DataLoaded, the Compatibility
  module (P4.1) soft-detects known durability mods (EquipmentDurabilitySystem NG,
  "EquipmentDurabilitySystem*.dll", and any mod exposing a durability keyword). If
  detected, our module auto-disables and logs "Durability: disabled — conflicting mod
  detected: <name>". No crash, no silent corruption.
- **I-10.8 Failure modes.** Module disabled by conflict → log + skip. Co-save durability
  field malformed → drop to full pool, log. Item with durability > max → clamp to max.
  Broken item equip attempt → reject + notification.

### L2 — Engine mapping

- **Channel (0.2):** durability owns `SetItemHealthPercent` when module on. Durability
  state is mirrored in ItemMetadata.durability/durabilityMax (co-save authoritative,
  D-028) and re-applied to item health on load/equip. Engine-native health is a display
  cache, not the source of truth.
- **Degradation:** ActorDeath hook applies -N durability to all equipped Runefall items
  (O(equipped) ≤ 6, fits 0.5 ms budget). No per-frame work.
- **Repair:** MenuOpen (forge) → Rune Forge page repair action → RuneForge.RepairItem
  transaction validates cost, deducts materials/runes, restores durability, re-applies
  health percent.
- **Quest item tag:** set at acquisition (LootEngine quest path) or via Papyrus bridge
  `SetQuestItem(Form, int)`.
- **Uninstall:** durability state is co-save only; removing the DLL leaves items at
  whatever health percent was last applied (engine data). Documented limitation.

### L3 — Data schema

- **No new CSV required.** Durability uses `rarity.csv` (rarityId → pool base) and
  `materials.csv` (repair cost materials). Pools are defined in INI `[Durability]`
  section: `DurabilityPool_Common=50`, etc. (tunable).
- **FormID range:** none new.
- **ItemMetadata new fields (0.4):** `durability` (uint16), `durabilityMax` (uint16),
  `isBroken` (uint8), `isQuestItem` (uint8), `temperPercent` (float, default 100.0).

### L4 — Code module

- **Owner:** A new `DurabilityModule` (or folded into TierEngine as a subsystem) that
  registers with HookManager only when `bEnableDurability = 1`. RuneForge owns repair
  transactions. Progression reads `temperPercent` for the stat multiplier when durability
  is on.
- **Papyrus bridge:** `int RepairItem(Form itemRefOrForm)`, `int GetDurability(Form)`,
  `int GetDurabilityMax(Form)`, `int SetQuestItem(Form, int)`.
- **UI surface:** Rune Forge page shows durability bar per equipped item; repair button
  with cost preview. Item Detail page shows durability + broken state.
- **Co-save:** ItemMetadata fields above; schema v2 migration defaults durability to
  max pool (if module enabled) or 0 (if disabled, ignored).

### Compatibility

- Auto-disable on conflicting durability mod (0.7). Additive: no vanilla record edits.
- Interaction with OWL/loot mods: none (durability is post-acquisition).
- Interaction with enchanting overhauls: temperPercent applied as stat multiplier; may
  stack with overhaul bonuses (capped by INI).

### Test entry

- Unit: degrade an item to 0 → isBroken = 1, cannot equip. Repair → durability = max,
  isBroken = 0.
- Integration: kill 10 enemies with a rank-3 enemy → equipped weapon durability
  decreases by 30 (10 * 3).
- Migration: load v1 save with durability module on → all items default to full pool.
- Conflict: install EquipmentDurabilitySystem NG alongside → our module auto-disables, log.
- Performance: ActorDeath hook with 6 equipped items degrades in <0.1 ms (budget 0.5 ms).

### Open questions

- Q8 (degrade): kill-driven (chosen) vs. per-hit (needs 12th hook, deferred). Per-hit
  is more faithful but unproven entry point.
- Q9 (at 0): broken = unequippable (chosen) vs. destroyed (rejected: conflicts with
  salvage loop and ItemRegistry pruning).
- Q10 (repair cost): materials + runes, deterministic (chosen) vs. risky (INI toggle).
- Q11 (D-023 collision): 0.2 (durability owns health when on; tempering moves to
  co-save).
- Q12 (EquipmentDurabilitySystem NG patterns): UNVERIFIED. Phase 0 proof spike required
  before design finalization. If unproven, disable path (fail-closed, AGENTS §3.1 rule 9).
- Q13 (gating): INI flag + FOMOD module (chosen).
- Q14 (performance): kill-driven, O(equipped) (chosen).

---

## 3. Monster Hunter material system (per-race/NPC loot) — C-10

**Status:** PARTIAL. Race-hunt materials (19 families) are already designed (L-11, L3E).
This section specifies the MaterialRegistry (the missing piece, FIX-PLAN P4.8) and the
scope decision for per-NPC tables. Mechanic ID: **C-10 Material Registry and hunting
loop** (extends L-11 and C-01).

### L1 — Rule and numbers

- **C-10.1 MaterialRegistry.** A central store of material counts keyed by materialId,
  persisted in co-save (section 10). Operations: `Add(materialId, amount)`,
  `Consume(materialId, amount) → bool`, `GetCount(materialId)`, `GetAll()`. All
  RuneForge transactions (salvage, reroll, upgrade, repair, imbue) go through it.
- **C-10.2 Scope decision (A-MH-1).** v1 uses the already-designed race-hunt materials
  (19 families) plus base materials (12). Broader per-NPC material tables (one material
  per individual NPC type) are **RESERVE** (Unique/Crafting packs). If promoted later,
  scope cap: only named/boss NPCs (not generic enemies), to keep data effort bounded.
  Per-NPC tables are NOT in v1.
- **C-10.3 Material tiers (P-16).** Keep the existing 4 tiers (Common/Refined/Exotic/
  Mythic, C-01). MH-style additional granularity (more tiers) is NOT adopted for v1 to
  avoid scope creep. A tier 5 (Legendary) is reserved for the Crafting pack.
- **C-10.4 Hunting loop (C-17).** Materials come from three sources:
  1. Salvage (C-02): 1-3 units of the item's tier.
  2. Ranked enemy drops (T-07): material tier bonus by rank.
  3. Nodes and caches (C-07): tiered by area tier.
  4. Race hunt (L-11): 19 family materials, repeatable, chance scales with RF tier.
  All flow into MaterialRegistry.
- **C-10.5 Dead-material gate (E-MH-2).** Every material must have at least one use
  (recipe, salvage output, imbue cost, repair cost, or upgrade cost). The build validator
  enforces material→use linkage (P3.6). A material with no use is a build error.
- **C-10.6 Uniqueness gate (E-MH-1).** No two race families share a primary material.
  Each hunt material is unique to its family. Enforced by validator (L3E gate #7).
- **C-10.7 Material counts in co-save.** MaterialRegistry state is persisted as a map
  (materialId → count) in co-save section 10. On pack removal, orphaned material counts
  are kept (N-04); on reinstall, they resume.
- **C-10.8 Failure modes.** Consume with insufficient count → transaction rejected, no
  partial deduction. Unknown materialId → log + skip. Co-save section malformed → drop
  section, default empty, log.

### L2 — Engine mapping

- **Owner:** HuntEngine (race→family lookup, firstOnly rewards, material rolls) + a new
  `MaterialRegistry` (counts, persistence). HuntEngine already exists (L4 §1); MaterialRegistry
  is new (FIX-PLAN P4.8).
- **Hook:** ActorDeath (hunt material roll, firstOnly flag) — already assigned (L4 §2).
  ContainerGeneration for node yields (if nodes are containers). No new hook.
- **Persistence:** co-save section 10 (new). Material counts are write-on-change, batched
  to save time (not per-kill).
- **UI:** Hunt Codex page (L4 §7 #3) shows material counts (reads MenuApi snapshot).

### L3 — Data schema

- **Existing files:** `materials.csv` (12 base + 19 hunt = 31 rows), `race_groups.csv`
  (19 families), `drops_races.csv` (38 rows). No new CSV for v1.
- **New co-save section:** section 10 "materials" — map<string, uint32> (materialId → count).
- **FormID range:** none new (materials use existing RF_MISC_Mat_* in 0xB80-0xBFF).
- **Pack manifest:** Hunt pack declares its materials; base materials ship in base.

### L4 — Code module

- **Owner:** MaterialRegistry (new module, or folded into DataStore/HuntEngine). Recommended:
  separate `MaterialRegistry.h/.cpp` for clarity, following the ItemRegistry pattern.
- **Papyrus bridge:** `int GetMaterialCount(string materialId)`, `int AddMaterials(string
  materialId, int amount)`, `int ConsumeMaterials(string materialId, int amount)`. All
  validated, return codes.
- **UI surface:** Rune Forge page shows material counts (for costs). Hunt Codex shows
  hunt material counts and first-kill state.
- **Co-save:** section 10; schema v2 migration defaults empty (all counts 0).

### Compatibility

- Additive: no vanilla record edits. Materials are MISC records in our ESL.
- Interaction with other loot mods: our loot is additive (D-008); material drops from
  our race hunt are independent of other mods' loot.
- Interaction with OWL: material nodes may overlap; our node yields are independent.

### Test entry

- Unit: Add 5 ore → count = 5. Consume 3 → count = 2. Consume 5 → false, count = 2.
- Integration: salvage an item → materials + runes granted → MaterialRegistry count
  increases → co-save persists → reload → count restored.
- Migration: load v1 save → all material counts 0, no crash.
- Validator: run L3E gates (race group member counts, 36-cell coverage, firstOnly rules,
  unique pool separation, material→use linkage).

### Open questions

- Q15 (per-race): already 19 families (chosen). Per-NPC tables RESERVE.
- Q16 (tiers): 4 tiers (chosen). MH granularity deferred.
- Q17 (loop): node + drop + salvage (chosen). MaterialRegistry is the unblocker.
- Q18 (MaterialRegistry): specified here; implementation is FIX-PLAN P4.8.
- Additional: whether MaterialRegistry counts should be per-character or shared across
  saves. Chosen: per-save (co-save), consistent with D-028.

---

## 4. Tier upgrade with tier materials (MH-style) — clarification of C-05

**Status:** RESERVE (v1 core is C-05 rarity upgrade, already designed). This section
resolves the ambiguity (C-TU-1): there is **one** upgrade axis in v1, not two.

### L1 — Rule and numbers

- **Clarification (C-TU-1 resolved):** "Tier upgrade" (RG-056) is **not a new system**.
  It is the existing rarity upgrade (C-05) implemented via the DLL (not COBJ, because
  COBJ cannot express its rules). The phrase "MH-style tier upgrade via DLL" meant
  implementation, not a new mechanic. **No second upgrade axis ships in v1.**
- **C-05 (restated).** Upgrade rarity by one band. Cost: one rarity step in materials
  (count = currentRarity + 1 units of the item's material tier) + rune units
  (`50 * 5^currentRarity`) + a chance component (60% success, tunable in INI). On
  failure: materials and runes consumed, item unchanged (mirrors I-09.3).
- **C-05.1 Radiant → Mythic.** Requires 1 R3+ rune (consumed) + 1 boss material of
  the area tier (from drops_bosses.csv / hunt). Success chance 60%.
- **C-05.2 Mythic → Runeforged.** Requires 1 R4+ rune (consumed) + 2 boss materials of
  the area tier. Success chance 40% (harder). On success, the item gains the optional
  unique runeforged effect (I-02).
- **C-05.3 Tier materials as cost (Q20).** Upgrade consumes materials of the item's
  tier from MaterialRegistry. The materials are the item's own tier (not the target
  tier's materials) — you upgrade the item, not the materials.
- **C-05.4 Success rate modification.** Base 60% (or 40% for Mythic→Runeforged).
  Precision attribute adds +0.5% per point (capped +15%). INI multiplier
  `UpgradeSuccessMult` (default 1.0) scales the base.
- **C-05.5 Failure mode.** Materials + runes consumed, item unchanged, no rarity
  change. Player-visible: "Upgrade failed — materials lost." Item can be retried.
- **C-05.6 Interaction with imbue.** Upgrade preserves imbue (in-place metadata update,
  S-TU-1). Boost magnitude is capped at the new tier's maximum (no removal).
- **C-05.7 Interaction with reroll.** Upgrade preserves enhancements (reroll is separate).
  Upgrade changes rarity and socket count (I-02: socket count is fixed by rarity);
  if socket count increases, new sockets are empty (no auto-socketing).
- **C-05.8 Reserved future axis (C-11).** A separate item-quality/ilvl upgrade axis
  (MH-style quality 1-10) is **RESERVE** for the Crafting pack. It would require a new
  ItemMetadata field (qualityLevel), new UI, and new data. Not in v1. If promoted, it
  must be designed as a distinct mechanic with its own L1→L4 pass.

### L2 — Engine mapping

- **Owner:** RuneForge.UpgradeItem transaction validates cost (materials, runes, boss
  materials), rolls success, mutates ItemMetadata.rarity/ilvl/socketCount on success,
  re-applies dynamic enchantment/name/temper (D-023) for the new rarity.
- **Hook:** MenuOpen (forge). No new hook.
- **In-place update:** no new registry ID (S-TU-1). ItemRegistry updates the existing
  entry.

### L3 — Data schema

- **Existing files:** `rarity.csv` (rarityId, enhancementCount, socketMin, socketMax),
  `materials.csv`. Upgrade costs are computed by the DLL from rarity + materials, not
  from a separate table. An optional `upgrade_costs.csv` (rarityFrom, rarityTo,
  materialCount, runeCost, bossMaterialCount, successPct) may be added for tunability;
  for v1, costs are formula-driven (C-05) to keep data simple.
- **FormID range:** none new.

### L4 — Code module

- **Owner:** RuneForge.UpgradeItem (already in Papyrus bridge L4 §6). Implementation
  is FIX-PLAN P4.4/P4.8.
- **Papyrus bridge:** `int UpgradeItem(Form itemRefOrForm)` (exists). Returns 0 = ok,
  nonzero = error (insufficient materials, insufficient runes, max rarity).
- **UI surface:** Rune Forge page upgrade action: select item → preview cost + success
  chance → confirm → result message. Confirmation dialog shows "On failure, materials
  are lost."

### Compatibility

- Additive: no vanilla record edits. Upgrade is a metadata mutation on our own registry.
- Interaction with imbue: preserved (0.6). Interaction with sockets: new sockets empty
  on rarity increase.

### Test entry

- Unit: UpgradeItem with insufficient materials → error, no consumption. With sufficient
  materials → 60% success roll; on success, rarity increments, socket count updates; on
  failure, rarity unchanged, materials consumed.
- Integration: upgrade Refined → Exquisite → item has 2 enhancements and 1 socket.
- Save/load: rarity persists.
- Edge: upgrade at max rarity (Runeforged) → error "already max rarity."

### Open questions

- Q19: resolved — same as C-05, not a new axis. Optional quality axis RESERVE.
- Q20: tier materials from salvage/drops/nodes (chosen).
- Q21: cost formula + 60%/40% success (chosen).

---

## 5. Material salvage via new menu + DLL (C-02 refinement)

**Status:** COMPLETE (design). This section documents the UI + transaction refinements
(confirmation dialog, atomicity, MaterialRegistry integration) that were underspecified.
Mechanic ID: C-02 (unchanged).

### L1 — Rule and numbers (unchanged from L1 §8)

- **C-02 (unchanged).** Any Runefall item can be dismantled at a forge. Yield: 1-3
  material units of the item's tier, plus rune value equal to 10-25% of the item's rune
  value. Salvage destroys the item; no undo. Affix Shard extraction is an expansion
  feature (C-09); v1 yields materials and runes only.
- **C-02.1 (new) Confirmation.** Salvage requires a confirmation dialog showing the
  item name, yield range, and "This cannot be undone." (E-SAL-1). No accidental
  salvage.
- **C-02.2 (new) Imbue not recovered.** If the item has an imbue (I-09), the imbued
  material is not returned (M-IMB-1). Salvage returns base materials + runes only.
- **C-02.3 (new) Durability.** Broken items can be salvaged; yield is unaffected by
  durability (item's tier determines yield). Salvaging a broken item is a valid
  recovery path.
- **C-02.4 (new) Atomicity.** The transaction is atomic on the main thread via the
  SKSE task interface: prune registry entry → grant materials → grant runes → destroy
  item. If any step fails, the transaction rolls back (item is not destroyed). (E-LE-1
  pattern.)
- **C-02.5 (new) Registry pruning (S-SAL-1).** Salvage must call ItemRegistry.Prune(registryId)
  so the co-save does not bloat. Verified in the SalvageItem transaction.

### L2 — Engine mapping

- **Owner:** RuneForge.SalvageItem transaction. Validates item metadata, deducts item,
  grants materials (MaterialRegistry.Add) + runes (rune wallet), prunes registry.
- **Hook:** MenuOpen (forge). No new hook.
- **Atomicity:** SKSE::GetTaskInterface()->AddTask on the main thread; validate before
  mutating; rollback on failure.

### L3 — Data schema

- **Existing files:** `materials.csv` (salvageYieldMin/Max columns), `rarity.csv`.
  No new CSV.
- **FormID range:** none new.

### L4 — Code module

- **Owner:** RuneForge. SalvageItem Papyrus bridge function already declared (L4 §6).
  Implementation is FIX-PLAN P4.4/P4.8.
- **Papyrus bridge:** `int SalvageItem(Form itemRefOrForm)` (exists). Returns 0 = ok.
- **UI surface:** Rune Forge page (L4 §7 #2) salvage list + confirmation dialog.
  Sub-view of Rune Forge, not a new system (audit §12 recommendation).

### Compatibility

- Additive: no vanilla record edits. Salvage is local to our registry.
- Interaction with item-tracking mods: base form remains; other mods tracking FormIDs
  are unaffected (M-SAL-1).

### Test entry

- Unit: SalvageItem with valid item → registry pruned, materials + runes granted,
  item destroyed. SalvageItem with invalid item → error, no change.
- Integration: salvage round-trip: destroy item → materials + runes granted → co-save
  persists → reload → counts restored.
- Edge: salvage an imbued item → materials granted, runes granted, imbued material NOT
  granted.
- Edge: salvage a broken item → yield unaffected.

### Open questions

- Q22 (UI): Rune Forge sub-view (chosen), not a new system.
- Q23 (transaction): atomic via task interface (chosen).
- Q24 (affix shards): deferred to C-09 (expansion). v1 yields materials + runes only.

---

## 6. Summary of red-team question coverage

| Q | Feature | Resolution section |
|---|---|---|
| Q1 | Imbue stat boost channel | §1 L2 (wearer fortify effect) |
| Q2 | Imbue break + re-imbue + material consumed | §1 L1 (0.9.3, 0.9.5, 0.9.6) |
| Q3 | Imbue material conflict | §1 L1 (0.9.1 one per item, overwrite with surcharge) |
| Q4 | ItemMetadata imbue field | §1 L3 (new fields) |
| Q5 | Application channel (no base-record mutation) | §0.3, §1 L2 |
| Q6 | Imbue failure mode | §1 L1 (0.9.3) |
| Q7 | Save compatibility (co-save migration) | §0.4, §1 L4 |
| Q8 | Durability degradation | §2 L1 (0.10.2 kill-driven) |
| Q9 | At 0 durability | §2 L1 (0.10.3 broken = unequippable) |
| Q10 | Repair cost formula | §2 L1 (0.10.4) |
| Q11 | D-023 SetItemHealthPercent collision | §0.2 |
| Q12 | EquipmentDurabilitySystem NG patterns | §2 Open questions (unverified, Phase 0 spike) |
| Q13 | Optional module gating | §2 L1 (0.10.6 INI + FOMOD) |
| Q14 | Performance | §2 L1 (0.10.2 ActorDeath hook) |
| Q15 | Per-race materials | §3 L1 (C-10.2, 19 families) |
| Q16 | Material tiers | §3 L1 (C-10.3, 4 tiers) |
| Q17 | Hunting loop | §3 L1 (C-10.4) |
| Q18 | MaterialRegistry | §3 L4 (new module) |
| Q19 | Distinct from rarity upgrade | §4 L1 (clarified: same as C-05) |
| Q20 | Tier material source | §4 L1 (C-05.3) |
| Q21 | Tier upgrade cost + success | §4 L1 (C-05, C-05.1, C-05.2) |
| Q22 | Salvage UI | §5 L4 (Rune Forge sub-view) |
| Q23 | Salvage transaction atomicity | §5 L1 (C-02.4) |
| Q24 | Affix shard extraction | §5 L1 (deferred to C-09) |

---

## 7. Cross-feature interaction matrix

| | Imbue (I-09) | Durability (I-10) | Materials (C-10) | Tier upgrade (C-05) | Salvage (C-02) |
|---|---|---|---|---|---|
| Imbue | — | broken item cannot be imbued | imbue costs material | upgrade preserves imbue | material not recovered |
| Durability | — | — | repair costs material | upgrade preserves durability | broken items salvageable |
| Materials | cost source | cost source | — | cost source | yield source |
| Tier upgrade | preserves imbue | preserves durability | consumes materials | — | item destroyed = no upgrade |
| Salvage | consumes imbue (no recovery) | can salvage broken | grants materials | destroys item | — |

---

## 8. Co-save schema v2 (batched migration)

Single bump v1 → v2. New ItemMetadata fields:

    imbueMaterialId (string, default "")
    imbueStatKey (string, default "")
    imbueMagnitude (float, default 0.0)
    imbueTier (uint8, default 0)
    imbueBreakChancePct (float, default 0.0)
    temperPercent (float, default 100.0)
    durability (uint16, default 0)
    durabilityMax (uint16, default 0)
    isBroken (uint8, default 0)
    isQuestItem (uint8, default 0)

New co-save section:

    10  materials  map<string,uint32> materialId→count

Migration (v1 → v2): all new ItemMetadata fields default; section 10 defaults empty.
If durability module is enabled at load, durability defaults to max pool for the item's
rarity (computed from rarity.csv + INI); if disabled, durability stays 0 and is ignored.
Downgrades (v2 save loaded by v1 DLL) refused with a clear message (D-052).

---

## 9. Implementation ordering (when promoted)

1. MaterialRegistry (C-10) + co-save section 10 — unblocks salvage yields, upgrade costs,
   repair costs, imbue costs. (FIX-PLAN P4.8)
2. Durability module (I-10) — only after Phase 0 proof spike (A-DUR-1). Default off.
3. Salvage transaction refinement (C-02.4, C-02.5) — confirmation dialog + atomicity +
   registry pruning. (FIX-PLAN P4.4)
4. Tier upgrade (C-05) — already partially wired; complete transaction + UI. (FIX-PLAN P4.4)
5. Imbue (I-09) — last, because it depends on MaterialRegistry and the wearer-effect
   channel proof. RESERVE until v1 ships.

---

## 10. Remaining open questions (consolidated)

1. Imbue wearer-effect channel performance on gear change (Q1).
2. Use-time imbue break chance default on/off (Q2).
3. One imbue per item vs. per category (Q3).
4. EquipmentDurabilitySystem NG pattern transfer (Q12) — Phase 0 spike.
5. Per-hit durability degradation (12th hook) — deferred.
6. Per-NPC material tables scope cap if promoted (C-10.2).
7. MH-style quality/ilvl axis (C-11) — RESERVE, separate design pass.
8. Whether material counts are per-save or shared (chosen: per-save).

---

RUNTIME STATUS: assistant-claimed (design document, no runtime verification)
SSEEDIT/CK: not launched
UNRESOLVED: Phase 0 proof spikes for durability channel and imbue wearer-effect performance;
scope promotion decisions (D-047 register updates) for all five features.