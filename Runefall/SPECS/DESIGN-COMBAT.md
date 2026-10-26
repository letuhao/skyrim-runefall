# DESIGN-COMBAT — Combat feature cluster (ailments + Last Epoch-style random roll)

Status: design draft, 2026-10-27. Companion to SPECS/AUDIT-NEW-FEATURES-DESIGN-DEPTH.md and
SPECS/REDTEAM-NEW-FEATURES.md. Resolves two interconnected features: the
ailment/damage-type/penetration framework (RG-004, RG-005, RG-006, RG-023) and the Last
Epoch-style random roll (RG-031 extended, new flow). Layered per D-013: each feature is
given L1 rule + numbers, L2 engine mapping, L3 data schema, L4 code module,
compatibility notes, test entry, and open questions. Mechanic IDs stay traceable to
L1-GAMEPLAY-MODEL.md; new IDs are introduced here and must be folded into L1 in a
follow-up doc pass (same convention as SPECS/DESIGN-ITEM-CRAFTING.md).

Scope note (D-047): both features are currently RESERVE (Combat pack / Crafting pack). This
document is design only; promotion to v1 requires a register update first. The ailment
framework ships as an optional `Runefall - Combat.esp` pack (default off in v1 core). The
LE random roll ships as a simplified subset in v1 base (Rune Forge action, INI-gated) with
the full LE experience reserved for the Crafting pack (1.1).

---

## 0. Cross-cutting resolutions

### 0.1 D-006 compliance: no vanilla AV repurposing (resolves A-AIL-2, M-AIL-1)

The ailment framework needs per-damage-type resistance and penetration stats. D-006 forbids
repurposing vanilla actor values. Resolution (two-tier, fail-closed):

- **Primary (always): our own GLOBAL records (GLOB)** are the source of truth for every
  resistance/penetration stat: `RF_GLOB_Resist_<Type>`, `RF_GLOB_Pen_<Type>`,
  `RF_GLOB_Conv_<Type>` (conversion percent). The DLL reads/writes these GLOBALs from
  character attributes, item affixes, and monster affixes. MGEF conditions reference the
  GLOBALs directly. No vanilla AV is written.
- **Optional enhancement (AVG):** if Actor Value Generator (AVG) is installed and its TOML
  config declares our AVs (`Runefall_Resist_*`, `Runefall_Pen_*`), the DLL mirrors the
  GLOBAL values to AVG AVs on change for UI/console display. If AVG is absent, the GLOBALs
  remain authoritative and the UI reads them through the Papyrus bridge. No hard dependency.
- **Penetration** is a per-attack stat (from attributes/items), not a persistent AV. The
  DLL computes effective resistance at hit resolution:
  `effectiveResist = clamp(0, resistCap, baseResist - penetration)`.
- **Resistance cap:** 85% (configurable in INI `[Combat] ResistCapPct`). Penetration cannot
  push resistance below 0 (floor). Penetration cannot push resistance above the cap either
  (no negative penetration stacking).

### 0.2 D-027 compliance: no base-record mutation (resolves A-IMB-1 pattern, M-AIL-1)

All combat state obeys D-027: no vanilla or shared records are mutated.

- **Ailments:** applied as our own MGEF records (in `Runefall - Combat.esp`) via a perk
  entry point / Papyrus `CastSpell`. Ailment state on actors is runtime-only (not co-save)
  because MGEFs are self-expiring (S-AIL-1). No base-record mutation.
- **Resistance/penetration:** GLOBAL records + DLL computation. No record mutation.
- **Damage conversion:** applied as a runtime damage multiplier computed by the DLL; the
  converted damage is delivered as an additional MGEF (our own record) on hit. No weapon
  record mutation.
- **LE random roll:** mutates `ItemMetadata` in the co-save (in-place, no new registry ID,
  S-TU-1 pattern from DESIGN-ITEM-CRAFTING.md §4). Re-applies the dynamic enchantment/name/
  temper (D-023) for the new rarity/affixes. No base-record mutation.

### 0.3 Co-save schema: reuse v2, add section 11 (resolves N-01 churn)

The v2 schema bump defined in DESIGN-ITEM-CRAFTING.md §8 is extended with one new section.
No further bump beyond v2 for this cluster:

- **Section 11 `ailment_runtime`** — reserved but **not persisted** (runtime-only map). The
  section ID is allocated in the header enum for forward compatibility; the payload is empty
  in v1. This keeps the section table stable if a future pack persists ailment state.
- **Section 12 `le_roll_state`** — per-item LE crafting state that must persist: forging
  potential remaining, fracture count, sealed-affix flags, rune-ascendance tier. Stored as
  an extension of `ItemMetadata` (new fields, see §1.3/§2.3), not a new section. Schema
  migration v2 → v2 (additive fields only, defaults applied).

New `ItemMetadata` fields added in this cluster (batched into the single v2 migration):

    // Ailment-related (reserved, used by Combat pack when installed)
    ailmentImmuneFlags (uint16, default 0)   // bitfield: 1=bleed,2=burn,4=chill,8=shock,16=poison,32=stun,64=slow

    // LE random roll (base, v1)
    forgingPotential (uint16, default 0)      // 0 = not craftable / exhausted
    forgingPotentialMax (uint16, default 0)   // original max for display
    fractureCount (uint8, default 0)          // number of fractures on this item
    sealedAffixIndex (int8, default -1)       // -1 = none; index of affix immune to reroll
    leCraftTier (uint8, default 0)            // LE crafting tier for ascendance

Migration (v2 → v2): all new fields default. If the Combat pack is not installed,
`ailmentImmuneFlags` stays 0 (no immunity). If LE roll is disabled by INI, `forgingPotential`
stays 0 (item shows "not craftable").

### 0.4 Hook map: no 12th hook for v1 (resolves C-AIL-1, P-DUR-1 pattern)

The 11-hook map (D-050) is unchanged. Ailment application uses MGEF-self-expiring effects
plus a Papyrus `OnHit` event handler (event-driven, not per-frame). LE roll uses the
existing `MenuOpen` hook. Neither feature adds a hook. A 12th hook (OnHit/damage) is
proposed as an **optional future amendment** (see §1 open questions Q2) if per-hit damage
conversion cannot be expressed through MGEF + Papyrus. Until then, the hook map stays at 11.

### 0.5 Performance: no per-frame work (resolves P-AIL-1)

- Ailment ticking is handled by the MGEF engine (duration + tick interval), not by the DLL.
  The DLL only sets the MGEF on application and decrements the stack counter on expiry via
  the Papyrus `OnEffectFinish` event. No polling.
- LE roll transactions run on `MenuOpen` (0.2 ms budget) through the SKSE task interface.
  No per-frame work.

---

## 1. Ailment / damage-type / penetration framework

Mechanic IDs: DMG (damage types), AIL (ailments), PEN (penetration/resistance).
Status: RESERVE (Combat pack). Design only; no v1 code until register update (D-047).

### 1.1 L1 — Rule and numbers

- **DMG-01 Damage-type taxonomy (5 types).** Physical, Fire, Cold, Lightning, Poison.
  Expandable: a 6th (Shadow) is reserved for the Unique/Combat packs. Every damage
  instance in Runefall is tagged with exactly one type. Vanilla damage (untyped weapon
  damage) is treated as Physical.
- **DMG-02 Conversion.** A character or item may convert a percentage of one damage type
  into another: `conversionPct` per source→target pair (e.g., 30% Physical → Fire). Total
  conversion out of a single source type is capped at 100% (no damage multiplication).
  Converted damage is applied as an additional MGEF of the target type on hit; the
  original source damage is reduced by the converted amount (no double-dip).
- **AIL-01 Ailment registry (5 ailments).** Each ailment is tied to one damage type:

  | Ailment | Type | Effect | Max stacks | Duration (s) | Tick interval (s) |
  |---|---|---|---|---|---|
  | Bleed | Physical | DoT, scales with stack count | 8 | 5 | 1 |
  | Burn | Fire | DoT, refreshes on re-apply | 1 | 4 | 1 |
  | Chill | Cold | slows movement/attack speed | 10 | 6 | — (no tick) |
  | Freeze | Cold | stun (at 10 Chill stacks) | 1 | 2 | — |
  | Shock | Lightning | increases crit chance taken | 1 | 8 | — |
  | Poison | Poison | DoT, stacks additively | 1 (magnitude stacks) | 8 | 2 |

  (Chill and Freeze are two states of the Cold ailment: 10 Chill stacks convert to one
  Freeze. Shock is a flat debuff, not a stack. Poison magnitude stacks instead of count.)

- **AIL-02 Stacking rules.**
  - **Bleed:** stacks add (up to 8). Each stack adds its own DoT tick; duration refreshes
    to full on re-apply (refresh, not extend). At max stacks, new applications refresh
    duration only (no overflow).
  - **Burn:** single application; re-apply refreshes duration and takes the higher
    magnitude (max, not sum). No stacking.
  - **Chill:** stacks add (up to 10). Each stack adds slow. At 10 stacks, converts to
    Freeze (all Chill stacks consumed, Freeze applied for 2 s). Freeze is immune to
    Chill application while active.
  - **Shock:** single application; re-apply refreshes duration and takes the higher
    magnitude.
  - **Poison:** magnitude stacks (up to a cap of 5x base magnitude, tracked as one
    application with a multiplier). Duration refreshes on re-apply.
  - **Decay:** all ailments decay naturally when their MGEF duration expires. No manual
    decay tick. Re-application during the decay window (last 1 s) refreshes to full
    duration (no partial-stack loss).
- **AIL-03 Application.** Ailments are applied by a perk entry point on hit ("Apply Spell
  on Hit" / "Add Spell on Hit" — Phase 0 verifies the exact entry point name) that casts
  our ailment SPEL (MGEF, touch, fire-and-forget) on the target. Application chance is
  per-hit: `applyChancePct = baseChance + (penetration * 0.5)` capped at 95%, floored at
  5%. Base chances by ailment: Bleed 15%, Burn 10%, Chill 20%, Shock 12%, Poison 18%.
  Ailment application is skipped if the target is immune (AIL-04) or if the damage type
  of the hit does not match the ailment's type (unless conversion applies).
- **AIL-04 Immunity and cleanse.**
  - **Immunity flags:** per-actor bitfield (stored in `ItemMetadata`-equivalent actor state
    in a runtime map, not co-save). Bosses (rank 4-5) are immune to Freeze and Stun by
    default; other ailments apply at 50% duration. Configurable per rank in
    `ailments.csv` (`immuneRanks` column).
  - **Cleanse sources:** (a) time (MGEF expiry), (b) a "Cleanse" spell (restoration
    MGEF that removes all ailment MGEFs), (c) entering a new cell (resets all ailments on
    all actors in the old cell — prevents cross-cell persistence), (d) death (clears all).
  - **Interaction matrix:** Freeze cancels Burn (Freeze's stun overrides Burn's DoT tick
    pause; Burn resumes after Freeze expires if duration remains). Chill + Burn: Burn
    reduces Chill duration by 50% (heat melts ice). Poison + Bleed: Bleed tick damage
    increased by 25% if Poison is active (wound infection). All other pairs: independent.
- **PEN-01 Resistance model.** Each damage type has a resistance value (0-85%). Resistance
  reduces incoming damage of that type: `damageMultiplier = 1 - (effectiveResist / 100)`.
  Resistance is sourced from: (a) character attributes (e.g., Endurance → Physical resist
  +0.5%/pt, Wisdom → Cold resist +1%/pt), (b) item affixes, (c) monster affixes
  (Warded = +30% to one type), (d) vanilla magic resistance (mapped to a generic
  `magicResist` that feeds all elemental types at 25% weight, additive, capped).
- **PEN-02 Penetration.** Each damage type has a penetration stat (from attributes/items).
  Penetration reduces the target's effective resistance: `effectiveResist = max(0, resist - pen)`.
  Penetration is additive across sources. Penetration cannot make resistance negative.
- **PEN-03 Caps.** Resistance floor 0%, ceiling 85%. Penetration has no cap (excess is
  wasted). Ailment application chance cap 95%, floor 5%. DoT tick damage minimum 1 (no
  zero-damage ticks).

### 1.2 L2 — Engine mapping

- **Application channel (0.3):** ailments are MGEF records (fire-and-forget, touch) in
  `Runefall - Combat.esp`. A perk (BGSPerk) with the "Apply Spell on Hit" entry point
  (Phase 0 verifies exact name; fallback: Papyrus `OnHit` event handler calling
  `target.CastSpell(ailmentSpell, target)`) casts the ailment SPEL on hit. The DLL
  computes application chance and magnitude from the GLOBALs and passes them through the
  MGEF's magnitude (MGEFs support variable magnitude via `SetMagnitude`? No — MGEF
  magnitude is a record field. Alternative: one MGEF per tier, or use the spell's
  magnitude override at cast time via `MagicCaster::CastSpell` with a modified effect.
  Phase 0 verifies whether magnitude can be overridden at cast time; if not, ship 3 tiers
  of MGEF per ailment (low/mid/high) and select by tier.)
- **Damage conversion:** the DLL computes a per-hit damage multiplier per type from the
  attacker's conversion stats (GLOBALs). Converted damage is delivered as an additional
  MGEF cast on the target (our own record, magnitude = converted amount). The original
  physical damage is reduced by the conversion amount via a perk entry point
  ("Mod Incoming Damage" or "Mod Outgoing Damage" — Phase 0 verifies). If no clean
  reduction channel exists, conversion is limited to additive (converted damage added
  without reducing base) and documented as a known limitation.
- **Resistance application:** the DLL writes `RF_GLOB_Resist_<Type>` on the target when
  resistance changes (gear change, buff). MGEF conditions on our damage MGEFs reference
  the GLOBAL to scale magnitude: `magnitude = baseMagnitude * (1 - effectiveResist/100)`.
  This keeps resistance evaluation in the engine's condition system, not the DLL.
- **Penetration:** applied at hit resolution in the DLL (computes effective resistance
  before the MGEF is cast). The MGEF magnitude is pre-scaled by the DLL using the
  effective resistance value.
- **Stacking counter:** runtime map `std::map<Actor*, std::map<AilmentId, AilmentState>>`
  where `AilmentState { stacks, magnitude, expiryTime, appliedBy }`. Incremented on
  application (if < cap), decremented on `OnEffectFinish` (Papyrus event). Not persisted
  (resets on load). On cell change, all entries for actors in the old cell are cleared.
- **Hook:** `ActorDeath` (clear ailment state for the dead actor). `MenuOpen` (forge
  shows resistance stats). No new hook. `OnHit` is a Papyrus event, not an L4 hook.
- **Uninstall:** ailment MGEFs are our own records; removing the Combat pack leaves
  base game unaffected. Ailment state is runtime-only (no co-save). Documented
  limitation: active ailments disappear on uninstall (expected).

### 1.3 L3 — Data schema

- **Plugin:** `Runefall - Combat.esp` (new pack, ESL-flagged). Owns its own 0x800-0xFFF
  window. Base plugin is not required to be present for the pack's records to exist, but
  the DLL requires base for GLOBAL resolution.
- **New CSV files (in `Data\Runefall\data\`):**
  - `damage_types.csv`: `damageTypeId,name,ailmentId,color,icon,resistGlobKey,penGlobKey`
    (6 rows: 5 types + reserved Shadow).
  - `ailments.csv`: `ailmentId,damageTypeId,name,maxStacks,durationSec,tickIntervalSec,
    tickMagnitudePerStack,decayRule,applyChanceBasePct,immuneRanks,interactionMatrix`
    (6 rows: 5 ailments + reserved Stun).
  - `combat_stats.csv`: `statId,name,type,damageTypeId,default,cap,source`
    (resistance/penetration/conversion rows; ~30 rows: 5 types × 3 stats + character
    attribute contributions).
  - `ailment_interactions.csv`: `ailmentA,ailmentB,interaction,effect` (matrix of
    pairwise interactions; ~15 rows).
- **MGEF records:** 5 ailment SPELs × 3 tiers = 15 MGEFs in `Runefall - Combat.esp`
  at 0x900-0x9FF. Plus 5 damage-type MGEFs for converted damage = 5. Plus 1 Cleanse
  SPEL. Total ~21 MGEFs (well within 256).
- **FormID range:** Combat pack uses its own 0x800-0xFFF window. Ailment MGEFs at
  0x900-0x9FF, ailment SPELs at 0xA00-0xAFF, ailment PERKs at 0xB00-0xB7F, ailment
  GLOBs at 0x880-0x8FF. No collision with base (different plugin).
- **Localization:** `strings.csv` gets ailment names, descriptions, and UI labels
  (N-05). ~40 new keys.

### 1.4 L4 — Code module

- **Owner:** A new `CombatModule` (or folded into `TierEngine` as a subsystem) that
  registers with HookManager only when the Combat pack is installed and
  `bEnableAilments = 1` (INI). Owns: ailment application, stack tracking, resistance/
  penetration computation, damage conversion, immunity checks.
- **Dependencies:** DataStore (loads combat CSVs), FormResolver (resolves ailment MGEFs/
  SPELs from Combat pack), ItemRegistry (reads item affix resistance/penetration),
  Progression (reads character attribute resistances), HookManager (ActorDeath).
- **Papyrus bridge:** new functions:
  - `int ApplyAilment(Actor target, string ailmentId, float magnitude)` → 0 ok.
  - `int CleanseAilments(Actor target)` → 0 ok.
  - `int GetAilmentStacks(Actor target, string ailmentId)` → count.
  - `float GetResistance(Actor target, string damageTypeId)` → 0-85.
  - `float GetPenetration(Actor attacker, string damageTypeId)` → value.
  - `int SetAilmentImmunity(Actor target, string ailmentId, int immune)` → 0 ok.
- **Events:** `Runefall_OnAilmentApplied (actorRef, ailmentId, stacks)`,
  `Runefall_OnAilmentExpired (actorRef, ailmentId)`,
  `Runefall_OnAilmentImmunity (actorRef, ailmentId, immune)`.
- **UI surface:** Character sheet shows resistance/penetration per type (read-only,
  from GLOBALs). Item Detail shows ailment-related affixes. No new page needed.
- **Co-save:** ailment state is runtime-only (not persisted). `ailmentImmuneFlags`
  in `ItemMetadata` is reserved for item-granted immunity (e.g., an affix that grants
  Burn immunity). Actor immunity is runtime-only (resets on load; bosses re-apply at spawn).

### 1.5 Compatibility

- **Vanilla resistances:** our `magicResist` mapping is additive (25% weight to all
  elemental types), capped at 85% total. Vanilla `magicResist` is never reduced below
  its base value; our bonus is added on top and capped.
- **Other ailment mods:** our MGEFs are our own records; if another mod applies its own
  ailment MGEF, both coexist (separate MGEF records, separate durations). Stacking
  interactions are undefined (documented as known limitation). We do not detect or
  suppress other mods' ailments.
- **Elemental Reactions Framework (163378):** pattern reference only (D-012). Our
  implementation is independent. If both are installed, both apply their own effects
  (potential double-application). Documented as a known limitation; recommend disabling
  one via INI.
- **Monster affixes:** "Warded" (elemental resistance) and "Venomous" (Poison DoT)
  from L1 T-03 are implemented as monster affixes using the ailment MGEFs. If the
  Combat pack is not installed, these affixes degrade to no-op (logged) or are replaced
  by vanilla-equivalent effects (e.g., "Warded" becomes +armor instead).

### 1.6 Test entry

- Unit: `ApplyAilment` with valid target → stacks increment, MGEF cast. With immune
  target → rejected, no MGEF. With max stacks → duration refreshes, no overflow.
- Unit: `effectiveResist = clamp(0, 85, resist - pen)`. Test floor (pen > resist → 0),
  ceiling (resist > 85 → 85), and normal case.
- Unit: conversion: 30% Physical → Fire on a 100-damage hit → 70 Physical + 30 Fire
  MGEF. Total damage = 100 (no multiplication).
- Integration: hit a target with a Bleed-applying weapon 8 times → 8 stacks, DoT ticks
  at 8× magnitude. 9th hit → refreshes duration, no 9th stack.
- Integration: 10 Chill stacks → Freeze applied, Chill consumed. Freeze expires → Chill
  can re-apply.
- Integration: boss (rank 4) hit with Freeze-applying attack → immune, no Freeze, 50%
  duration Chill (if Chill applies).
- Save/load: ailment state resets on load (documented). Resistance GLOBALs persist
  (they are GLOB records, engine-persisted).
- Performance: ailment application (perk entry point) adds <0.1 ms to the hit path
  (budget 0.5 ms for ActorDeath, but ailment is on-hit, not on-death; measured separately).

### 1.7 Open questions (ailment)

- Q1 (application): Whether the "Apply Spell on Hit" perk entry point exists in 1.6.1170
  and supports magnitude override at cast time. Phase 0 must verify. Fallback: Papyrus
  `OnHit` event handler calling `CastSpell`. If neither supports magnitude override,
  ship 3 MGEF tiers per ailment (low/mid/high) and select by tier.
- Q2 (12th hook): Whether to add an OnHit/damage hook to the DLL for per-hit damage
  conversion and ailment chance. Current design uses MGEF + Papyrus OnHit (no new hook).
  If Phase 0 shows Papyrus OnHit is too slow or unreliable, propose a 12th hook
  amendment to D-050 with a 0.1 ms budget. Decision deferred to Combat pack design.
- Q3 (stacking): "Refresh on re-apply" vs. "extend duration." Chosen: refresh (simpler,
  no duration math). Revisit if playtests show frustration with lost stacks.
- Q4 (resistance/penetration): GLOBAL records vs. AVG AVs. Chosen: GLOBALs primary, AVG
  optional mirror. Revisit if AVG proves reliable and users want console access.
- Q5 (conversion): Additive (converted damage added without reducing base) vs.
  subtractive (base reduced by converted amount). Chosen: subtractive (no double-dip).
  Requires a clean damage-reduction channel (Phase 0 verifies).
- Q6 (performance): Per-hit ailment check must not add per-frame work. Chosen: MGEF
  engine handles timing; DLL only sets the MGEF. Verified by design (no polling).
- Q7 (save compat): Ailments do not persist across saves (MGEF self-expiring).
  Resistance GLOBALs persist (engine-persisted GLOBs). Documented limitation.
- Q8 (compatibility): Interaction with other ailment mods is undefined (both apply
  their own MGEFs). Documented as known limitation; recommend INI toggle to disable
  our ailment application if another mod is preferred.

---

## 2. Last Epoch-style random roll

Mechanic IDs: LE (Last Epoch roll), FP (forging potential), GLY (glyphs), RUNE (LE runes).
Status: SIMPLIFIED SUBSET in v1 base (Rune Forge action); FULL EXPERIENCE reserved for
Crafting pack (1.1). Design only for the full feature; v1 ships the subset noted per
section.

### 2.1 L1 — Rule and numbers

- **LE-01 Forging potential (FP).** A per-item counter representing how much crafting
  the item can endure. FP is consumed by every LE craft action. When FP reaches 0, the
  item can no longer be crafted (shows "Forging potential exhausted"). FP does not
  regenerate.
  - **Default FP by rarity:** Common 15, Refined 25, Exquisite 35, Radiant 45, Mythic
    55, Runeforged 75. Tunable in INI `[LERoll] FP_<Rarity>`.
  - **FP cost per action:** Reroll all affixes: 5 FP. Reroll one affix (locked): 3 FP.
    Upgrade rarity: 8 FP. Add affix (empty slot): 10 FP. Remove affix: 2 FP. Refine
    values (reroll magnitudes only): 4 FP. Ascendance (upgrade rarity tier): 12 FP.
  - **FP is stored in `ItemMetadata.forgingPotential` (co-save, v2).** On item creation
    (loot drop), FP is set to the default for the item's rarity. On salvage, FP is lost
    (item destroyed).
- **LE-02 Fracture.** A craft action has a chance to fracture the item based on the
  item's current instability (a hidden counter that increases with each craft). On
  fracture:
  - **v1 (simplified):** one random enhancement is removed (rarity decreases by 1 if
    enhancement count exceeds the new rarity's count; socket count is preserved). FP is
    set to 0 (item can no longer be crafted). Player-visible: "Fractured — an
    enhancement was lost. Forging potential exhausted."
  - **Full (Crafting pack):** item is destroyed (lost entirely). Player-visible:
    "Fractured — item destroyed."
  - **Fracture chance:** `fracturePct = baseFracturePct + (instability * 0.5)`, capped
    at 60%. Base: 5% (tunable). Instability increases by 1 per craft action.
  - **Glyph of Protection** prevents fracture on a single craft (consumed).
- **LE-03 Glyphs (v1 ships 3, full ships 6).**
  - **Glyph of Hope (v1):** 25% chance that the craft action does not consume FP. If
    the roll succeeds, FP is preserved; if it fails, FP is consumed normally. Does not
    affect fracture chance.
  - **Glyph of Chaos (v1):** rerolls the affix type (category + specific affix) of one
    chosen enhancement slot, keeping the tier. Costs 2 FP (in addition to the action's
    FP cost). Fracture chance +5%.
  - **Glyph of Order (v1):** rerolls the magnitude of one chosen enhancement within its
    current tier band (min-max). Costs 1 FP. No fracture chance change.
  - **Glyph of Protection (v1):** prevents fracture on this craft. Consumed. Costs 0 FP
    (the craft's normal FP cost still applies). v1 ships this glyph.
  - **Glyph of Despair (Crafting pack 1.1):** removes one affix and refunds 5 FP.
  - **Glyph of the Guardian (Crafting pack 1.1):** reduces fracture chance by 50% for
    this craft.
  - **Glyph costs:** Glyphs are MISC items (consumed on use). They drop from ranked
    enemies (rank 2+), containers, and vendors. They are not craftable in v1.
- **LE-04 LE Runes (v1 ships 4, full ships 6).**
  - **Rune of Shattering (v1):** breaks the item into salvage materials + runes (same
    yield as salvage C-02). Destroys the item. Costs 0 FP (item is destroyed).
  - **Rune of Removal (v1):** removes one chosen enhancement and grants a material of
    that affix's tier (1 unit). Costs 2 FP.
  - **Rune of Refinement (v1):** rerolls all enhancement magnitudes within their tiers.
    Costs 4 FP. Fracture chance +3%.
  - **Rune of Ascendance (v1):** upgrades rarity by one band (if FP >= 8 and materials
    available). Costs 12 FP. On success, new sockets are empty. Fracture chance +8%.
  - **Rune of Discovery (Crafting pack 1.1):** adds an affix to an empty slot. Costs
    10 FP.
  - **Rune of Creation (Crafting pack 1.1):** adds a new enhancement slot (increases
    rarity by 1 without upgrading). Costs 15 FP.
- **LE-05 Scope decision (Q14): v1 ships a simplified subset.**
  - **v1 (base):** FP tracking, fracture (v1 = lose enhancement, not destroy), Glyphs
    of Hope/Chaos/Order/Protection, Runes of Shattering/Removal/Refinement/Ascendance.
    Integrated into the Rune Forge page as new actions. No new UI page.
  - **Full (Crafting pack 1.1):** all 6 glyphs/runes, item destruction on fracture,
    sealed affixes (an affix that cannot be rerolled), instability display, dedicated
    crafting page with FP bar and fracture chance preview.
  - **v1 must not depend on the Crafting pack.** If the pack is absent, the v1 subset
    works standalone. If the pack is present, it extends the v1 subset with the full
    experience (more glyphs/runes, sealed affixes, full fracture).

### 2.2 L2 — Engine mapping

- **Transaction model (Q13): LE roll is a new transaction layer, not just a UI flow over
  `RerollItem`.** It reuses `RerollItem`/`UpgradeItem` internally but adds FP accounting,
  fracture rolling, glyph consumption, and rune effects. Each LE action is a single atomic
  transaction on the main thread via the SKSE task interface:
  1. Validate preconditions (FP sufficient, item not fractured, materials available).
  2. Consume glyph/rune (if used) from inventory.
  3. Roll fracture chance (if not protected). On fracture: apply fracture effect, set FP
     to 0, deduct consumed items, log, return fracture code.
  4. If no fracture: apply the craft effect (reroll/upgrade/refine/etc.), deduct FP,
     increment instability, log, return success code.
  5. On any validation failure: no consumption, no state change, return error code.
- **FP storage:** `ItemMetadata.forgingPotential` / `forgingPotentialMax` (co-save, v2).
  Read/written by RuneForge. No base-record mutation.
- **Glyph/rune items:** MISC records in base plugin (`RF_MISC_Glyph_*`, `RF_MISC_Rune_*`)
  at 0xB80-0xBFF (spares). Consumed via the standard inventory path (remove item).
- **UI:** Rune Forge page (L4 §7 #2) gets new actions: "LE Reroll" (with glyph/rune
  selector), "Refine", "Ascend", "Shatter", "Remove Affix". Each shows FP cost, fracture
  chance, and result preview. Confirmation dialog for fracture-risk actions.
- **Hook:** `MenuOpen` (forge). No new hook. `ActorDeath` (glyph/rune drops from ranked
  enemies — handled by LootEngine, not CombatModule).
- **Uninstall:** LE state is co-save only. Removing the DLL leaves items with their last
  applied rarity/affixes (engine data). FP state is lost (documented limitation).

### 2.3 L3 — Data schema

- **Plugin:** Base plugin (`Runefall.esp`) for v1 subset (glyph/rune MISC records). Full
  LE experience uses `Runefall - Crafting.esp` (1.1) for additional records.
- **New CSV files:**
  - `le_glyphs.csv`: `glyphId,name,effect,fpCostModifier,fractureChanceModifier,description`
    (4 rows for v1: hope, chaos, order, protection).
  - `le_runes.csv`: `runeId,name,effect,fpCost,materialsRequired,description`
    (4 rows for v1: shatter, removal, refinement, ascendance).
  - `le_fp_defaults.csv`: `rarity,fpDefault,fpCostRerollAll,fpCostRerollOne,fpCostUpgrade,
    fpCostAddAffix,fpCostRemoveAffix,fpCostRefine,fpCostAscendance` (6 rows, one per
    rarity). Or fold into `rarity.csv` as new columns.
- **FormID range (base plugin):** Glyphs/runes at 0xB80-0xBFF (MISC, spares: 0xBC0-0xBFF
  has 64 slots, enough for 8 glyphs/runes). No collision with existing MISC (0xB80-0xBBF
  used for runes/materials).
- **ItemMetadata new fields (0.3):** `forgingPotential`, `forgingPotentialMax`,
  `fractureCount`, `sealedAffixIndex`, `leCraftTier`, `instability` (uint8, default 0).
- **Localization:** `strings.csv` gets glyph/rune names and descriptions (~30 new keys).

### 2.4 L4 — Code module

- **Owner:** `RuneForge` (extended). LE roll transactions are new methods on RuneForge:
  `LERollItem`, `LERefineItem`, `LEAscendItem`, `LEShatterItem`, `LERemoveAffix`,
  `LEApplyGlyph`, `LEApplyRune`. All validated, return codes (0 = ok, nonzero = error).
- **Dependencies:** ItemRegistry (reads/writes ItemMetadata), DataStore (loads LE CSVs),
  MaterialRegistry (costs), RuneForge (reuses RerollItem/UpgradeItem internals).
- **Papyrus bridge:** new functions:
  - `int LERollItem(Form itemRefOrForm, int glyphId, int targetAffixIndex)` → 0 ok,
    1 = fractured, 2 = insufficient FP, 3 = invalid item.
  - `int LERefineItem(Form itemRefOrForm, int glyphId)` → 0 ok.
  - `int LEAscendItem(Form itemRefOrForm, int glyphId)` → 0 ok.
  - `int LEShatterItem(Form itemRefOrForm)` → 0 ok.
  - `int LERemoveAffix(Form itemRefOrForm, int affixIndex, int runeId)` → 0 ok.
  - `int GetForgingPotential(Form itemRefOrForm)` → remaining FP.
  - `int GetFractureChance(Form itemRefOrForm)` → percent.
  - `int GetInstability(Form itemRefOrForm)` → count.
- **Events:** `Runefall_OnLERoll (itemRef, result, fpRemaining)`,
  `Runefall_OnLEFracture (itemRef, lostAffixIndex)`,
  `Runefall_OnLEAscendance (itemRef, newRarity)`.
- **UI surface:** Rune Forge page (L4 §7 #2) extended with LE actions. Sub-view, not
  a new system (audit §12 recommendation). Each action shows: FP cost, fracture chance,
  result preview, confirmation dialog for fracture-risk actions.
- **Co-save:** ItemMetadata fields above (0.3). Schema v2 migration defaults: FP = 0
  (not craftable) unless the item was dropped after the LE roll feature was enabled
  (new drops get FP from `le_fp_defaults.csv`). Existing items in old saves default to
  FP = 0 (cannot be LE-crafted until re-dropped or a "grant FP" debug command is used).

### 2.5 Compatibility

- **Additive:** no vanilla record edits. LE roll is a metadata mutation on our own
  registry. Glyphs/runes are MISC records in our ESL.
- **Interaction with reroll (C-04):** LE roll's "reroll all affixes" action uses the same
  internal path as C-04 but adds FP accounting and fracture. C-04 (basic reroll) remains
  available without FP cost (no fracture risk) for players who don't want LE mechanics.
  INI toggle `bEnableLERoll = 1` (default on in v1) controls whether LE actions appear
  in the Rune Forge. If off, only basic reroll/upgrade (C-04/C-05) are available.
- **Interaction with upgrade (C-05):** LE Ascendance uses C-05 internally but adds FP
  cost and fracture risk. C-05 (basic upgrade) remains available without FP cost.
- **Interaction with salvage (C-02):** Shattering uses the same yield as salvage but
  is triggered by a rune item (consumed). Salvage remains available without a rune.
- **Interaction with imbue (I-09):** LE roll preserves imbue (in-place metadata update,
  S-TU-1). Fracture does not remove imbue (imbue is separate metadata).
- **Interaction with durability (I-10):** LE roll on a broken item is blocked (must
  repair first). Fracture sets FP to 0 but does not break the item (unless full mode).
- **Interaction with other crafting mods:** our crafting is additive (we add actions to
  our own Rune Forge, not to vanilla workbenches). Other crafting mods are unaffected.

### 2.6 Test entry

- Unit: `LERollItem` with sufficient FP → affixes rerolled, FP deducted, instability
  incremented. With insufficient FP → error code 2, no change.
- Unit: fracture roll: with instability = 0, base 5%, 100 rolls → ~5 fractures. With
  Glyph of Protection → 0 fractures.
- Unit: `LEAscendItem` at max rarity (Runeforged) → error "already max rarity."
- Unit: `LEShatterItem` → item destroyed, materials + runes granted, registry pruned.
- Integration: craft an item 10 times → instability = 10, fracture chance = 5 + 5 = 10%.
- Integration: apply Glyph of Chaos → affix type changes, FP cost +2, fracture chance +5%.
- Save/load: FP, instability, fracture count persist.
- Migration: load v1 save (no LE fields) with v2 DLL → FP = 0 for all items, no crash.
  New drops get FP from defaults.
- Edge: LE roll on a broken item → error "item is broken, repair first."
- Edge: LE roll with Glyph of Hope → 25% chance FP not consumed (verify FP unchanged
  on success, consumed on failure).

### 2.7 Open questions (LE roll)

- Q9 (FP): Per-item counter (chosen) vs. global resource. Per-item is more faithful
  to LE and creates interesting item decisions (high-FP items are more valuable).
- Q10 (fracture): v1 = lose one enhancement + FP exhausted (chosen). Full = item
  destroyed (Crafting pack). Revisit v1 fracture severity after playtesting.
- Q11 (glyphs): v1 ships Hope, Chaos, Order, Protection (4). Full ships 6 (add
  Despair, Guardian). Revisit if playtests show demand for more.
- Q12 (runes): v1 ships Shattering, Removal, Refinement, Ascendance (4). Full ships
  6 (add Discovery, Creation). Revisit if playtests show demand for more.
- Q13 (scope): New transaction layer (chosen), not just UI over RerollItem. Reuses
  RerollItem/UpgradeItem internally but adds FP/fracture/glyph/rune. This keeps basic
  reroll available without LE mechanics.
- Q14 (scope): Simplified subset for v1 (chosen). Full LE in Crafting pack (1.1).
  v1 must not depend on the Crafting pack.

---

## 3. Red-team coverage table

| Challenge ID | Feature | Severity | Resolution section |
|---|---|---|---|
| C-AIL-1 | Ailment needs hit/damage hook | CRITICAL | §0.4 (no 12th hook; MGEF + Papyrus OnHit) |
| A-AIL-1 | Elemental Reactions Framework API unverified | HIGH | §1.5 (pattern reference only, D-012) |
| A-AIL-2 | AVG config path unverified | HIGH | §0.1 (GLOBALs primary, AVG optional mirror) |
| E-AIL-1 | Ailment interactions undefined | MEDIUM | §1.1 AIL-04 (interaction matrix) |
| E-AIL-2 | Ailment-immune bosses | MEDIUM | §1.1 AIL-04 (immunity flags per rank) |
| S-AIL-1 | Ailment state on actors | MEDIUM | §1.2 (runtime-only, MGEF self-expiring) |
| M-AIL-1 | Compatibility with vanilla resistances | HIGH | §0.1, §1.5 (additive, capped, floor) |
| P-AIL-1 | Per-hit ailment processing | MEDIUM | §0.5 (MGEF engine handles timing) |
| Q-AIL-1 | RESERVE status | LOW | §1 header (Combat pack, register update first) |
| C-DUR-1.. (pattern) | I-08 amendment | — | Inherited from DESIGN-ITEM-CRAFTING §0.1 |
| D-006 collision | No vanilla AV repurposing | — | §0.1 (GLOBALs + optional AVG) |
| D-027 collision | No base-record mutation | — | §0.2 (MGEF + co-save) |
| C-IMB-1 (pattern) | Second power channel | — | Inherited (sockets + imbue + ailments are separate channels) |

LE random roll red-team (from REDTEAM-NEW-FEATURES, adapted):

| Challenge | Resolution section |
|---|---|
| Q9 (FP definition) | §2.1 LE-01 (per-item counter, default by rarity) |
| Q10 (fracture) | §2.1 LE-02 (v1 = lose enhancement + FP exhausted) |
| Q11 (glyphs) | §2.1 LE-03 (4 glyphs in v1) |
| Q12 (runes) | §2.1 LE-04 (4 runes in v1) |
| Q13 (transaction vs UI) | §2.2 (new transaction layer) |
| Q14 (scope) | §2.1 LE-05 (simplified subset for v1) |

---

## 4. Co-save schema additions (v2, batched)

All new fields are additive (single v2 migration, no new version bump beyond v2):

**ItemMetadata new fields (this cluster):**

    // Ailment (Combat pack, reserved)
    ailmentImmuneFlags (uint16, default 0)

    // LE random roll (base, v1)
    forgingPotential (uint16, default 0)
    forgingPotentialMax (uint16, default 0)
    fractureCount (uint8, default 0)
    sealedAffixIndex (int8, default -1)
    leCraftTier (uint8, default 0)
    instability (uint8, default 0)

**New co-save section:**

    11  ailment_runtime  (reserved, empty payload in v1; allocated for forward compat)

Migration (v2 → v2): all new fields default. Existing items in old saves get FP = 0
(cannot be LE-crafted until re-dropped or granted via debug command). Ailment immunity
defaults to 0 (no immunity). Downgrades refused (D-052).

---

## 5. Implementation ordering (when promoted)

1. **Co-save v2 fields** (FP + ailment immunity) — batched into the single v2 migration
   alongside DESIGN-ITEM-CRAFTING.md §8. (FIX-PLAN P4.8, P4.3)
2. **LE roll v1 subset** (RuneForge transactions + Rune Forge UI) — depends on
   MaterialRegistry (C-10) and co-save v2. Ships in base. (FIX-PLAN P4.4)
3. **Combat pack** (ailment MGEFs + CombatModule) — only after v1 ships and the
   register is updated (D-047). Depends on Phase 0 verification of MGEF magnitude
   override and perk entry point. Default off.
4. **Full LE experience** (Crafting pack 1.1) — depends on LE roll v1 subset and
   Crafting Table expansion (D-045).

---

## 6. Remaining open questions (consolidated)

1. MGEF magnitude override at cast time (Q1) — Phase 0 spike.
2. 12th hook (OnHit/damage) amendment (Q2) — deferred to Combat pack design.
3. Ailment stacking refresh vs. extend (Q3) — chosen refresh; revisit after playtesting.
4. AVG mirror reliability (Q4) — optional; no hard dependency.
5. Damage conversion subtractive channel (Q5) — Phase 0 verifies damage reduction
   entry point.
6. LE fracture severity in v1 (Q10) — chosen "lose enhancement"; revisit after
   playtesting.
7. Glyph/rune count expansion (Q11, Q12) — v1 ships 4 each; full ships 6 each.
8. FP grant for existing items in old saves (Q14 related) — chosen 0 (cannot craft);
   provide debug command to grant FP. Revisit if users complain.
9. Whether LE roll ships in v1 base or is reserved (D-047) — this design assumes
   simplified subset in v1 base; register update required before code.

---

RUNTIME STATUS: assistant-claimed (design document, no runtime verification)
SSEEDIT/CK: not launched
UNRESOLVED: Phase 0 proof spikes for MGEF magnitude override, perk entry point name,
damage reduction channel; register updates (D-047) for both features before code.