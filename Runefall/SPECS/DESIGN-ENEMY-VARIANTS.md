# DESIGN-ENEMY-VARIANTS — Enemy variant system (champion packs)

Status: design draft, 2026-10-27. Companion to SPECS/AUDIT-NEW-FEATURES-DESIGN-DEPTH.md
and SPECS/REDTEAM-NEW-FEATURES.md. Resolves the enemy variant system
(RG-125b: modify existing NPCs into variants; RG-125c: better rewards when hunted;
RG-125d: naming/tagging). Layered per D-013: L1 rule + numbers, L2 engine mapping,
L3 data schema, L4 code module, compatibility notes, test entry, open questions.
Mechanic IDs stay traceable to L1-GAMEPLAY-MODEL.md (T-01..T-08); new IDs are
introduced here as V-* and must be folded into L1 in a follow-up doc pass
(same convention as SPECS/DESIGN-ITEM-CRAFTING.md and SPECS/DESIGN-COMBAT.md).

Scope note (D-047): enemy variants are currently RESERVE (Enemy pack). This document is
design only; promotion to v1 requires a register update first. The design assumes
the feature ships as an optional `Runefall - Enemy.esp` pack (default off in v1 core)
that depends on the base plugin. No new NPCs, no world/cell edits (constraint §5 of
GAP-REGISTER-FUTURE.md).

---

## 0. Cross-cutting resolutions

### 0.1 Variant is a SECOND axis, orthogonal to rank (resolves E-VAR-1, audit §7)

L1 T-01 defines rank (rune rank 1-5: Elite/Champion/Mini-boss/Boss/Mythic) as the
power scale: multipliers, affix count, rune wealth. The variant system (RG-125) is a
*separate* axis: a "champion pack" overlay that grants a named perk/skill package and
better rewards. They do NOT replace each other and they do NOT stack freely:

- **Rank** is the power scale (health/damage/armor/rune multipliers, affix count).
- **Variant** is a perk/ability package + reward multiplier applied to an otherwise
  normal enemy.
- **Mutual exclusion rule:** variants apply ONLY to rank 0 (normal) and rank 1 (elite)
  actors. Rank 2+ (champion/mini-boss) and bosses (rank 4-5) NEVER receive a variant;
  they use their own hand-authored templates (D-026). This prevents multiplicative
  soup (E-VAR-1) and keeps boss mods' templates untouched.
- A rank-1 variant = rank-1 multipliers (1.5x HP / 1.2x dmg) PLUS the variant package
  (+30-80% HP, +1-2 monster affixes). Effective difficulty ~2.0-2.7x a normal enemy,
  which fits the L1 E-09 target (elite 10-20 s, champion 20-40 s).
- A rank-0 variant = 1.0x base + variant package. Effective ~1.3-1.8x. Reads as a
  "special" normal enemy, not a full elite.

This is the resolution to audit Q7 ("Can a variant be a boss?"): **No in v1.**
Variants and bosses are mutually exclusive. If a "boss variant" is desired, author it
as a boss template (D-026) in `Runefall - Bosses.esp` carrying the same perk package,
not as a runtime combination. Revisit stacking in v1.1 only if playtests demand it.

### 0.2 D-025 compliance: no permanent perk grants (resolves C-VAR-1, audit §7)

D-025: "No permanent perk grants to generic NPCs." Variants obey this strictly:

- Variant perks/abilities are granted at spawn via `AddPerk`/`AddSpell` on the actor
  reference (runtime). They are NOT written into the base NPC record and NOT persisted
  as permanent grants.
- Variant identity is NOT stored in co-save. It is **re-derived at every spawn/load**
  from a deterministic seed (0.6). No co-save section, no save bloat, no orphaned
  state on uninstall (N-04).
- Idempotency (E-VAR-2): an exclusive keyword `RF_KW_Variant` marks an actor that
  has already received a variant package. The spawn hook checks the keyword first and
  skips if present. `AddPerk`/`AddSpell` are themselves idempotent (no-op if already
  granted), so a double application is harmless even if the keyword check races.
- On cell unload + reload, the actor is re-spawned/re-loaded and the variant is
  re-derived and re-applied. The keyword is re-applied too (runtime, on the reference).
- Uninstall: removing the DLL leaves actors as base forms; variant perks disappear on
  load; no permanent state remains. Documented limitation (N-10).

### 0.3 D-005 compliance: no SkyPatcher dependency (resolves C-VAR-2, audit §7)

Core must never depend on SkyPatcher (D-005). Variants are applied by our own DLL at
the `ActorSpawn` hook using our own PERK/SPEL records from the Enemy pack. No INI
patching, no runtime record mutation of vanilla NPCs.

- **Optional enhancement (Dynamic Perk Framework 189660):** if installed and
  `bUseDynamicPerkFramework = 1` (INI), the DLL delegates perk granting to DPF's
  runtime API for packages that exceed our own record set. If DPF is absent, our own
  PERK records suffice and DPF features degrade to no-op (logged). DPF is never a hard
  dependency (D-056).
- Fortunate Foes (reference) uses SPID/SkyPatcher for perk packages; we do NOT adopt
  that path. Our packages are our own records + `AddPerk` at spawn. The Fortunate
  Foes keyword-provider gap (no KYWD records in its ESPs) does not affect us because
  we define our own keywords.

### 0.4 No new NPCs, no world changes (resolves Q-VAR-1)

Constraint (GAP-REGISTER-FUTURE §5): variants modify existing NPCs only. No new NPC
records for variants (boss templates are separate, D-026, and live in the Bosses pack).
No cell/worldspace edits. All variant state is runtime-derived from existing actors.

### 0.5 Hook map unchanged (resolves P-VAR-1)

The 11-hook map (D-050) is unchanged. Variant application uses the existing
`ActorSpawn` hook (0.2 ms budget) plus `LoadedCellAttach` (2 ms budget) for density
tracking and refresh. No 12th hook. All package data is pre-built into an O(1)
lookup map at `DataLoaded` (100 ms budget).

### 0.6 Deterministic selection (resolves C-VAR-1, E-VAR-1)

Variant identity is derived from a deterministic seed, not from mutable RNG state, so
that:

- The same spawn point yields the same variant across save/load (no "variant flicker"
  on cell reload).
- Different spawn points yield different variants (no "every draugr in the dungeon is
  the same variant").
- Save-scumming (reload to get a better variant) does not work, because the seed
  depends on cell FormID + placement index + base NPC EditorID, not on RNG state.
- Seed formula: `seed = hash(cellFormId) ^ hash(baseNpcEditorId) ^ (placementIndex * 0x9E3779B9) ^ (rfTier * 0x517CC1B7)`.
  The variant chance roll and the package selection both consume this seed via our
  deterministic RNG (same RNG family as L-03 loot rolls).

---

## 1. Variant identity and tagging

Mechanic ID: **V-01 Variant marker keyword.**

### L1 — Rule and numbers

- **V-01.1 Marker.** A single keyword `RF_KW_Variant` (KYWD) marks any actor that
  has received a variant package. Applied at spawn via `actor->AddKeyword(rfKwVariant)`.
  Exclusive: an actor either has it or not. Never applied to rank 2+ or bosses.
- **V-01.2 Package id.** The specific package applied is NOT encoded in a keyword
  (avoids keyword explosion). It is held in a runtime map
  `std::unordered_map<RE::Actor*, std::int32_t> variantPackageByActor` (actor pointer
  → packageId). Re-derived at spawn; on map miss at death, re-derive from the seed
  (0.6) so reward scaling is always consistent (S-VAR-1).
- **V-01.3 Interop.** Other mods can check `RF_KW_Variant` to detect variant enemies
  (e.g., for UI badges or loot filters). Package-level detail is available through
  the Papyrus bridge `GetVariantPackage(actor)` for mods that want it.
- **V-01.4 Visual badge.** Name prefix via `ObjectReference::SetDisplayName` where
  proven (L2 cut list: implement attempt, fallback to UI badge). Prefix is one word
  from the package's `namePrefix` (e.g., "Frenzied", "Leeching", "Volatile").
  Optional TrueHUD integration (optional dep) shows a floating badge icon; default off.
- **V-01.5 Player-visible.** Variant enemies are visually distinguishable: name
  prefix in the health bar/name, plus the package's monster affixes (T-03) which
  are already visible. No separate "variant" UI element is required for gameplay
  (the affixes + name prefix are the tell, per Torchlight readability).

### L2 — Engine mapping

- **Keyword:** `RF_KW_Variant` is a new KYWD record in the Enemy pack
  (`Runefall - Enemy.esp`), FormID in the KYWD range (0x800). Base plugin is not
  required to be present for the keyword to exist, but the DLL requires base for
  GLOBAL resolution.
- **Application:** `ActorSpawn` hook calls `actor->AddKeyword(rfKwVariant)` (runtime,
  on the reference). Idempotent. If the actor is already marked (keyword present),
  skip the entire package application (E-VAR-2).
- **Name prefix:** `actor->SetDisplayName(prefix + " " + baseName)` if the API is
  available and the actor's name is mutable; otherwise log and skip (non-fatal, the
  affixes still convey the variant). Phase 0 verifies `SetDisplayName` on actors
  (L2 cut list already flags this as "implement attempt").
- **Runtime map:** `variantPackageByActor` is a game-thread-only structure (no lock
  needed; all access on game thread). Entries are added at spawn and erased on
  `ActorDeath` and on cell unload (to avoid dangling pointers). Erasure uses the
  actor pointer; if the actor is destroyed, the entry is orphaned until the next
  cell unload sweep (acceptable; pointer is never dereferenced after erase).
- **Uninstall:** keyword application is runtime-only; removing the DLL leaves actors
  unmarked. No permanent state.

### L3 — Data schema

- **No new CSV for the marker.** `RF_KW_Variant` is a single KYWD record in the
  Enemy pack. Its EditorID is referenced by the DLL via FormResolver (EditorID-keyed,
  D-040).
- **FormID range:** Enemy pack owns its own 0x800-0xFFF window. `RF_KW_Variant` at
  0x800 (KYWD). Variant package PERK records (if new passives are needed) at
  0xB00-0xB7F. No collision with base (different plugin).

### L4 — Code module

- **Owner:** TierEngine (extended). Variant marker application is part of
  `TierEngine::ApplyVariantPackage(actor, packageId)`. The runtime map lives in
  TierEngine. HookManager registers `ActorSpawn`.
- **Dependencies:** DataStore (variant_packages.csv), FormResolver (RF_KW_Variant),
  HookManager (ActorSpawn, LoadedCellAttach), LootEngine (reads variant marker at
  death for reward scaling).
- **Papyrus bridge:** new functions:
  - `int IsVariant(Actor a_actor)` → 1 if `RF_KW_Variant` present, else 0.
  - `int GetVariantPackage(Actor a_actor)` → packageId or 0 (none). Re-derives if
    the runtime map misses.
  - `string GetVariantNamePrefix(Actor a_actor)` → prefix or "".
- **Events:** `Runefall_OnVariantAssigned (actorRef, packageId)` sent on spawn.
- **UI surface:** No new page. The variant name prefix appears in the actor's display
  name (game UI). Debug export (N-08) includes variant counts per cell.
- **Co-save:** None. Variant identity is re-derived (0.2, 0.6).

### Compatibility

- Other mods reading actor keywords see `RF_KW_Variant` (additive, no collision).
- TrueHUD (optional) integration is an optional dep; if absent, no floating badge
  (name prefix + affixes remain the tell).
- Uninstall leaves no permanent keyword state.

### Test entry

- Unit: `ApplyVariantPackage` on an unmarked actor → keyword present, map entry
  created. Second call on same actor → skipped (keyword present), no double grant.
- Unit: map miss at death → re-derive returns same packageId as spawn (deterministic).
- Integration: spawn a variant enemy, kill it, reload cell → variant re-derived with
  same packageId and keyword.
- Performance: `ActorSpawn` with variant application (map lookup + keyword check +
  perk grant + name set) < 0.2 ms p99 (budget).

### Open questions

- Q1: Whether `ObjectReference::SetDisplayName` works on actor references in 1.6.1170
  (L2 cut list flags this as "implement attempt"). Phase 0 verifies; fallback is the
  affix-based tell + optional TrueHUD badge.
- Q2: Whether per-package keywords (`RF_KW_Var_<n>`) are worth shipping for
  interop. Chosen: single marker for v1; per-package markers are RESERVE (would add
  ~10 KYWD records, low cost but not needed).
- Q3: Whether the runtime map should use `RE::Actor*` (raw pointer) or FormID +
  load order. Chosen: raw pointer with cell-unload erasure; simpler and faster.
  Revisit if dangling-pointer crashes appear in Phase 0.

---

## 2. Variant selection (which enemy, which package)

Mechanic ID: **V-02 Variant eligibility and selection.**

### L1 — Rule and numbers

- **V-02.0 Variants per base NPC (Q2).** A base NPC has N *candidate* variant packages (typically 3-6, filtered by race eligibility in `variant_packages.csv`); at most ONE is applied per instance (or none, if the chance roll fails or the density cap is reached). "Normal" is the no-variant outcome. There is no separate "Elite Variant" vs "Boss Variant" axis — see §0.1 and §5 (variants and bosses are mutually exclusive). So the prompt's "3 variants per NPC" maps to "3 candidate packages, one applied or none."
- **V-02.1 Eligibility (V-13 pool).** An actor is eligible for a variant if ALL of:
  1. It is hostile (base race is in the hostile race set: the 19 race families from
     L-11 plus vanilla hostile races not in a family).
  2. It is NOT essential, NOT protected, NOT a quest alias, NOT a follower.
  3. It is NOT a boss (rank 4-5) and NOT already rank 2+ (champion/mini-boss).
  4. Its level >= `VariantMinLevel` (default 5, INI `[Variants] MinLevel`).
  5. Its base NPC EditorID is NOT in the denylist (INI `[Variants] DenylistEditorIds`,
     comma-separated; default empty).
  6. The Combat/Enemy pack is installed and `bEnableVariants = 1`.
- **V-02.2 Variant chance by RF tier (V-14 selection).** At spawn, roll
  `variantChancePct` from the seed (0.6). If the roll fails, no variant (normal
  enemy). If it succeeds, select one eligible package by weighted roll (same seed).

  | RF Tier | Variant chance | Max variants per cell |
  |---|---|---|
  | 0 | 5%  | 1 |
  | 1 | 10% | 2 |
  | 2 | 15% | 3 |
  | 3 | 20% | 4 |
  | 4 | 25% | 5 |
  | 5 | 30% | 6 |

  Values are defaults in `variant_rules.csv` (6 rows, one per RF tier) and tunable
  via INI multipliers (`VariantChanceMult`, default 1.0).
- **V-02.3 Density cap (V-12).** A per-cell counter tracks how many variants have
  been assigned in the current cell. If the counter >= `maxVariantsPerCell` for the
  RF tier, the variant chance roll is forced to fail (no variant), regardless of
  the seed roll. Counter resets on cell unload. This prevents "40 variants in one
  dungeon" (L1 T-08, E-VAR-3).
- **V-02.4 Package eligibility.** Each package declares `eligibleRaces` (semicolon-
  separated race EditorIDs or the token `all_hostile`) and `minRfTier`/`maxRfTier`.
  A package is only selectable if the actor's race is in its eligibility set and
  the current RF tier is within its range. If no package is eligible, no variant.
- **V-02.5 Weighted selection.** Among eligible packages, select by weight
  (`weight` column, default 100). Ties broken by packageId alphabetical order
  (deterministic). If only one eligible package, it is selected (no roll needed).
- **V-02.6 Failure modes.** No eligible package → normal enemy, no keyword, no log
  spam (debug log only). Eligibility check throws → catch, log, treat as ineligible.
  Seed computation overflow → use 64-bit hash, no overflow.

### L2 — Engine mapping

- **Data:** `variant_rules.csv` (rfTier, variantChancePct, maxPerCell) and
  `variant_packages.csv` (packageId, name, namePrefix, badgeIcon, minLevel, maxRankAllowed,
  hpMult, dmgMult, armorBonus, affixCount, perkEditorIds, abilityEditorIds, rewardMult,
  runeMult, dropChanceMult, materialTierBonus, weight, eligibleRaces, minRfTier,
  maxRfTier). Both shipped in the Enemy pack's data directory.
- **Pre-build at DataLoaded:** TierEngine builds two maps:
  1. `variantRulesByTier[rfTier]` → {chance, maxPerCell}.
  2. `variantPackagesByRace[raceEditorId]` → list of eligible packageIds (pre-filtered
     by race eligibility). Plus a global list for `all_hostile` packages.
  Both are built once (100 ms budget) and are read-only at spawn.
- **Spawn flow (ActorSpawn hook):**
  1. Eligibility check (V-02.1). If ineligible → return (no variant).
  2. Density check (V-02.3). If cap reached → return.
  3. Compute seed (0.6). Roll variant chance. If fail → return.
  4. Look up eligible packages for the actor's race (V-02.4). If none → return.
  5. Weighted selection (V-02.5) → packageId.
  6. `ApplyVariantPackage(actor, packageId)` (section 1).
  7. Increment cell variant counter.
- **Cell counter:** `std::unordered_map<FormID, std::int32_t> variantCountByCell`,
  reset on `LoadedCellAttach` (old cell erased) and on cell unload. Game-thread only.

### L3 — Data schema

- **New CSV files (in `Data\Runefall\data\`, shipped by Enemy pack):**
  - `variant_rules.csv`: `rfTier,variantChancePct,maxPerCell` (6 rows).
  - `variant_packages.csv`: `packageId,name,namePrefix,badgeIcon,minLevel,maxRankAllowed,
    hpMult,dmgMult,armorBonus,affixCount,perkEditorIds,abilityEditorIds,rewardMult,
    runeMult,dropChanceMult,materialTierBonus,weight,eligibleRaces,minRfTier,maxRfTier`
    (~8-12 rows for v1).
- **FormID range:** Enemy pack uses its own 0x800-0xFFF window. `RF_KW_Variant` at
  0x800. Variant PERK records (if new passives) at 0xB00-0xB7F. No collision.
- **Localization:** `strings.csv` gets package display names and badge labels (~20
  new keys, N-05).

### L4 — Code module

- **Owner:** TierEngine (extended). Variant selection + application + density tracking.
  No new module (D-049 module table already lists TierEngine with "monster-affix
  packages, scaling caps, boss handling" — variants are monster-affix packages).
- **Dependencies:** DataStore (loads variant CSVs), FormResolver (RF_KW_Variant,
  perk/ability EditorIDs), HookManager (ActorSpawn, LoadedCellAttach), Config (INI
  toggles), Compatibility (overhaul detection).
- **Papyrus bridge:** `IsVariant`, `GetVariantPackage`, `GetVariantNamePrefix` (section 1).
  Plus debug: `int GetVariantCountInCell()` → count for the current cell.
- **Events:** `Runefall_OnVariantAssigned (actorRef, packageId)`.
- **UI surface:** No new page. Debug export includes variant counts and package
  distribution (N-08).
- **Co-save:** None.

### Compatibility

- Enemy overhauls: eligibility excludes rank 2+ (which overhauls often produce).
  Soft-detect overhauls (Compatibility module); if a known overhaul is detected,
  auto-disable variants and log "Variants: disabled — enemy overhaul detected: <name>"
  (M-VAR-1). Caps prevent runaway even if not detected.
- Boss mods: variants skip rank 4-5 and essential/protected actors, so boss mod
  templates are untouched.
- Fortunate Foes: its NPCs are boss templates (rank 4) → skipped by rank rule.
  No conflict.
- Density caps prevent variant spam in any cell, including overhaul-populated dungeons.

### Test entry

- Unit: eligibility check on a rank-0 hostile actor at level 10 in tier 2 → eligible.
  On an essential actor → ineligible. On a rank-3 actor → ineligible. On a level-3
  actor → ineligible (below min level).
- Unit: density cap: cell with 3 variants, 4th spawn → no variant (forced fail).
- Unit: deterministic seed: same cell + placement + NPC + tier → same packageId
  across 100 calls.
- Unit: weighted selection with 3 eligible packages of weights 100/50/50 →
  distribution matches (within 5% over 10k rolls).
- Integration: spawn 20 enemies in one cell at tier 2 → at most 3 variants (density
  cap). Variant count logged.
- Performance: full spawn flow (eligibility + density + seed + chance + selection +
  apply) < 0.2 ms p99 (budget).

### Open questions

- Q4: Whether variant chance should scale with player level (higher level = more
  variants) in addition to RF tier. Chosen: RF tier only for v1 (simpler). Revisit
  if playtests show too few variants at high player level / low RF tier.
- Q5: Whether per-NPC variant overrides (specific packages for specific NPC types)
  should ship. Chosen: global packages with race eligibility for v1 (bounded data
  effort). Per-NPC overrides are RESERVE (Unique/Enemy packs, 1.2+).
- Q6: Whether the density cap should be per-cell or per-area (all cells in the
  loaded area). Chosen: per-cell (simpler, matches L4 LoadedCellAttach). Revisit
  if players report variant clustering at cell boundaries.
- Q7: Whether a "variant chance" INI slider should be exposed to users. Chosen: yes
  (`VariantChanceMult`, default 1.0, range 0.0-3.0). Documented in INI.

---

## 3. Variant difficulty package (what makes it harder)

Mechanic ID: **V-03 Variant perk/skill package.**

### L1 — Rule and numbers

- **V-03.1 Package contents.** Each variant package defines a named set of:
  1. `hpMult` (float, default 1.4): health multiplier applied to the actor's base
     health at spawn (additive with rank multiplier if rank 1: 1.5 × 1.4 = 2.1).
  2. `dmgMult` (float, default 1.2): outgoing damage multiplier.
  3. `armorBonus` (int, default 10): flat armor rating bonus (percent, applied via
     perk condition, not base record).
  4. `affixCount` (int, 0-2): number of additional monster affixes (from T-03 pool)
     applied on top of the rank's affix count. Capped so total affixes <= 4 (L1 T-03
     cap, T-08 readability).
  5. `perkEditorIds` (semicolon list): our PERK records granted via `AddPerk`
     (passive effects: regen, thorns, aura, etc.).
  6. `abilityEditorIds` (semicolon list): our SPEL/MGEF records granted via `AddSpell`
     (active abilities: summon minion, shock nova, etc.).
  7. `rewardMult` (float, default 2.0): rune yield multiplier (section 5).
  8. `dropChanceMult` (float, default 1.5): item drop chance multiplier.
  9. `materialTierBonus` (int, 0-2): bonus to material tier dropped (section 5).
- **V-03.2 Application at spawn (runtime).** The DLL applies:
  1. Health: `actor->SetActorValue(Health, base * hpMult * rankHpMult)` — wait, that
     mutates a base AV. Instead, apply health as a fortify effect via a perk condition
     or a MGEF, consistent with D-006/D-027 (no permanent base-AV writes). Actually
     for enemies, health scaling is commonly done via `SetActorValue` at spawn and
     re-applied on load; D-006 forbids *vanilla AV repurposing for the player's
     attributes*, not enemy health scaling (which is the established pattern for tier
     mods). Decision: enemy health/damage scaling uses the documented
     `SetActorValue`/perk-condition channel at spawn, re-applied on load, with caps.
     This is the same channel T-02 rank multipliers already use. No conflict with
     D-006 (which governs player attribute AVs).
  2. Perks: `actor->AddPerk(perk)` for each in `perkEditorIds`.
  3. Abilities: `actor->AddSpell(spell)` for each in `abilityEditorIds`.
  4. Monster affixes: roll `affixCount` distinct affixes from the T-03 pool not
     already on the actor; apply via MGEF/SPEL (TierEngine's existing affix path).
  5. Name prefix + keyword (V-01).
- **V-03.3 Caps (T-08, E-VAR-1).** Total monster affixes on any actor <= 4.
  Total HP multiplier (rank × variant) <= 6.0 (so a rank-1 variant maxes at ~2.1x,
  well below a rank-3 mini-boss at 3.5x). Total damage multiplier <= 2.5. Caps are
  enforced in `ApplyVariantPackage` and logged if clamped.
- **V-03.4 Failure modes.** Missing perk/ability EditorID → skip that entry, log,
  continue (do not abort the package). Perk grant fails (engine rejects) → log, skip.
  Health multiplier exceeds cap → clamp + log. Affix roll fails (no eligible affixes)
  → skip affixes, package still applies perks/stats.
- **V-03.5 Interaction with rank (0.1).** If the actor is rank 1, variant stats
  stack with rank stats (multiplicative for HP/dmg, additive for armor). If rank 0,
  variant stats apply alone. Rank 2+ never variant (0.1).

### L2 — Engine mapping

- **Health/damage scaling:** uses the same channel as T-02 rank multipliers
  (`SetActorValue` on Health/Stamina/Magicka or perk conditions; Phase 0 confirms
  which channel TierEngine already uses for rank scaling — variants reuse it, no new
  channel). Re-applied on load via `LoadedCellAttach` / load-scan fallback.
- **Perk/ability grant:** `RE::Actor::AddPerk` / `AddSpell` (documented SKSE APIs,
  same as L2 §4 "buff package: abilities/perks from our ESL applied at spawn/load").
- **Monster affixes:** reuse TierEngine's existing affix application path (MGEF/SPEL
  records from the base/Enemy pack). Variant affixes are drawn from the same T-03
  pool; no new affix records needed.
- **Hook:** `ActorSpawn` (0.2 ms). Application must fit the budget; perk/ability
  grants are O(package size) which is small (<= 4 perks, <= 2 abilities).
- **Dynamic Perk Framework (optional):** if `bUseDynamicPerkFramework = 1` and DPF
  is installed, perk grants go through DPF's API (which supports runtime perk
  granting with its own persistence). If DPF is absent, our own `AddPerk` path is
  used. The DLL checks DPF presence at `DataLoaded` and logs.

### L3 — Data schema

- `variant_packages.csv` columns (section 2) already define all package contents.
  No additional CSV. Perk/ability EditorIDs reference records in the Enemy pack's
  PERK (0xB00-0xB7F) and SPEL (0xA00-0xAFF) ranges.
- **New records (Enemy pack):** `RF_Perk_Var_*` (variant-only passives, ~8-12 PERK
  records) and reuse existing monster-affix SPELs. FormID ranges per D-035/D-040
  (Enemy pack owns 0x800-0xFFF).

### L4 — Code module

- **Owner:** TierEngine (extended). `ApplyVariantPackage(actor, packageId)` applies
  stats, perks, abilities, affixes, name, keyword. `RollVariant(actor, rfTier, seed)`
  selects the package (section 2).
- **Dependencies:** DataStore, FormResolver, HookManager, Compatibility.
- **Papyrus bridge:** `GetVariantPackage`, plus debug `int GetVariantHpMult(Actor)`,
  `int GetVariantDmgMult(Actor)` (returning percent for UI/debug).
- **Events:** `Runefall_OnVariantAssigned`.
- **UI surface:** No new page. Variant stats appear in the debug export.
- **Co-save:** None.

### Compatibility

- Perk grants are runtime on the reference; no base-record mutation (D-027).
- Caps prevent stacking from exceeding a mini-boss's difficulty (0.1, V-03.3).
- If an overhaul already grants the same perk, `AddPerk` is a no-op (no double
  effect beyond the first grant). Logged.
- DPF optional dep: if both are installed, our perks + DPF perks coexist; if
  DPF is preferred, disable our perk grants via INI `bVariantGrantPerks = 0`.

### Test entry

- Unit: `ApplyVariantPackage` with a valid package on a rank-0 actor → HP/dmg
  scaled, perks/abilities granted, keyword present, affixes applied (count <= 4).
- Unit: cap enforcement: package with hpMult 10.0 → clamped to 6.0 (with rank 1:
  1.5 × 4.0 = 6.0), logged.
- Unit: missing perk EditorID → skipped, logged, package still applies other
  contents.
- Integration: variant enemy has higher HP than a normal enemy of the same base;
  killing it takes measurably longer (E-09 balance gate).
- Performance: `ApplyVariantPackage` < 0.2 ms p99 including perk/ability grants.

### Open questions

- Q8: Whether enemy health scaling should use `SetActorValue` (mutates base AV at
  spawn, re-applied on load) or a fortify effect. Chosen: reuse TierEngine's
  existing rank-scaling channel (whatever Phase 0 proves). Document the channel in
  L2 once TierEngine's rank scaling is implemented (currently not implemented per
  FIX-PLAN P4.7). If TierEngine uses perk conditions, variants use the same.
- Q9: Whether variant affixes should be drawn from the full T-03 pool or a
  variant-specific subset. Chosen: full pool, excluding affixes already on the
  actor, capped at 4 total. Revisit if playtests show unreadable affix combos
  (T-08 "aura soup").
- Q10: Whether `dmgMult` applies to all damage types or only physical. Chosen: all
  outgoing damage (matches rank multiplier semantics). If the Combat pack (ailments)
  is installed, converted damage is also scaled.

---

## 4. Variant loot and reward scaling (RG-125c)

Mechanic ID: **V-04 Variant reward scaling.**

### L1 — Rule and numbers

- **V-04.1 Rune yield.** Variant enemies drop runes at `baseRankYield × runeMult`
  (default 2.0). A rank-0 variant drops 2x a normal enemy's rune yield; a rank-1
  variant drops 2x a rank-1 enemy's yield. RuneMult is per-package (default 2.0,
  range 1.0-4.0).
- **V-04.2 Item drop chance.** Variant enemies have `dropChanceMult` (default 1.5)
  applied to the base drop chance (L-04: 35% normal, 100% elite+). A rank-0 variant
  drops at 52.5% (35 × 1.5); capped at 100%.
- **V-04.3 Item rarity bias.** Variant enemies get `+variantRarityBonus` weight
  points to Exquisite and above (default +5, configurable), on top of the rank
  bonus (L-05: R1 +0, R2 +2, ...). A rank-0 variant rolls rarity as if it were
  rank 1 for rarity purposes.
- **V-04.4 Material drops.** Variant enemies drop materials at `materialTierBonus`
  (0-2) tiers above the area's base material tier (C-01, C-10), in addition to
  normal rank material drops (T-07). A variant with materialTierBonus 2 in a tier-2
  area drops tier-4 materials.
- **V-04.5 Variant-only bonus roll (optional).** Variant enemies have a
  `VariantBonusChancePct` (default 15%, INI) chance to drop one extra item from a
  small variant-only pool: glyphs (LE-03, if LE roll enabled), hunt materials of
  a random family, or a rune of +1 rank. This is a "champion pack bonus" and is
  independent of the normal drop roll.
- **V-04.6 XP.** Skyrim's own XP (from enemy level) is unchanged; we do not grant
  XP directly (D-006, no AV writes for XP). The Runefall reward for variants is
  runes + items + materials, not XP. This is the answer to Q6.
- **V-04.7 Failure modes.** RewardMult <= 0 → clamp to 1.0, log. Drop chance
  mult resulting in >100% → cap at 100%. Material tier bonus exceeding tier 4
  (Mythic) → cap at tier 4 (Mythic), log.
- **V-04.8 Loot hook read path (S-VAR-1).** At `ActorDeath`, LootEngine reads the
  variant package from the runtime map (or re-derives from seed if map miss). It
  does NOT re-roll the variant. Reward scaling uses the same packageId that was
  applied at spawn. This guarantees consistency (a variant enemy always drops
  variant-scaled loot, even if the runtime map was cleared).

### L2 — Engine mapping

- **Hook:** `ActorDeath` (0.5 ms budget). LootEngine already runs here (general
  table + rank table + race hunt rewards). Variant reward scaling is an additional
  step: if the dead actor is a variant, apply `runeMult`, `dropChanceMult`,
  `variantRarityBonus`, `materialTierBonus`, and the variant bonus roll.
- **Read path:** `TierEngine::GetVariantPackage(actor)` → packageId (from runtime
  map or re-derived). Then `DataStore::GetVariantPackage(packageId)` → package
  definition. O(1) lookup.
- **Rune grant:** `AddRunes(rank, amount * runeMult, "variant_kill")` through the
  rune wallet (co-save section 2).
- **Item drop:** LootEngine's drop chance check uses `dropChanceMult`; rarity roll
  uses `variantRarityBonus` added to the rank bonus.
- **Material drop:** HuntEngine/loot path uses `materialTierBonus` to bump the
  material tier (C-10).
- **Variant bonus roll:** independent roll using the loot RNG (L-03 seeded), with
  its own pity protection (L-06, shared counter).

### L3 — Data schema

- No new CSV. Variant reward parameters are columns in `variant_packages.csv`
  (section 2). Variant bonus roll pool is a small fixed list in INI
  `[Variants] BonusPool` (comma-separated item types: glyph, hunt_material, rune).
- **drops_variants.csv** (optional, RESERVE): if a dedicated variant drop table is
  wanted later, it follows the same format as drops_general.csv (rank × tier). For
  v1, variant rewards are multipliers on existing tables, not a separate table.

### L4 — Code module

- **Owner:** LootEngine (reward scaling) + TierEngine (variant package lookup).
  No new module.
- **Dependencies:** TierEngine, DataStore, HuntEngine, ItemRegistry.
- **Papyrus bridge:** `int GetVariantRuneMult(Actor)` → percent (for UI/debug).
- **Events:** `Runefall_OnLootRolled` already includes rarity/itemForm; variant
  status is available via `IsVariant(actor)`. No new event needed (or add
  `Runefall_OnVariantKilled (actorRef, packageId)` if UI wants it — optional).
- **UI surface:** No new page. Variant kills could show a notification ("Variant
  slain — bonus loot") via the notification channel (L2 §7).
- **Co-save:** None (variant identity re-derived; rune/material grants go through
  existing co-save sections 2 and 10).

### Compatibility

- Additive loot (D-008): variant rewards are added on top of normal drops, not
  replacing them. OWL and other loot mods keep their records.
- Variant bonus roll is independent of other mods' loot; no conflict.
- If another mod also scales enemy loot, both apply (multiplicative); caps in INI
  (`VariantDropChanceCap = 100`) prevent absurdity.

### Test entry

- Unit: `ActorDeath` on a variant actor → rune grant = base × runeMult (verify
  wallet increment). On a non-variant actor → base only.
- Unit: drop chance scaling: variant with dropChanceMult 1.5 on a 35% base →
  52.5% (verify over 10k rolls).
- Unit: material tier bump: variant with materialTierBonus 2 in tier-2 area →
  tier-4 material (verify).
- Integration: kill 100 variant enemies → rune income measurably higher than 100
  normal enemies (E-01 balance gate).
- Migration: N/A (no co-save state).

### Open questions

- Q11: Whether variant enemies should have a guaranteed unique drop (like bosses).
  Chosen: no guaranteed unique in v1 (variant bonus roll is probabilistic). Guaranteed
  variant uniques are RESERVE (Unique pack, 1.3+).
- Q12: Whether the variant bonus roll should include quest items. Chosen: no (quest
  items come from quest rewards, not random drops). Bonus pool is materials/glyphs/runes.
- Q13: Whether variant reward scaling should apply to containers the variant was
  guarding (chest in the same cell). Chosen: no (container loot is independent). Revisit
  if playtests show demand.

---

## 5. Variant as boss (Q7 resolution)

Mechanic ID: **V-05 Variant-boss mutual exclusion.**

### L1 — Rule and numbers

- **V-05.1 No stacking.** A variant cannot be applied to a boss (rank 4-5) or a
  mini-boss (rank 2-3) at runtime. Eligibility (V-02.1) excludes them. This is the
  resolution to audit Q7.
- **V-05.2 Boss templates with variant-like packages.** If a "boss variant" is
  desired (a boss with a champion-pack perk set), author it as a hand-authored boss
  template (D-026) in `Runefall - Bosses.esp` carrying the same perk/ability EditorIDs
  as a variant package. The boss's difficulty comes from its template, not from
  runtime variant application. This keeps boss mods' templates untouched and avoids
  runtime mutation of boss actors (which may be quest-critical).
- **V-05.3 No variant keyword on bosses.** Bosses never receive `RF_KW_Variant`.
  Loot scaling for bosses uses boss drop tables (drops_bosses.csv), not variant
  multipliers.
- **V-05.4 Failure modes.** If a boss actor somehow passes eligibility (bug),
  `ApplyVariantPackage` checks rank and aborts if rank >= 2, logging a warning.
  Double guard: eligibility + application-time rank check.

### L2 — Engine mapping

- **Guard in `ApplyVariantPackage`:** if `actor->GetLevel()` ... actually rank check:
  look up the actor's assigned rank (TierEngine's rank map, runtime). If rank >= 2,
  abort + log. This is a second guard after eligibility.
- **Boss templates:** authored in the Bosses pack (D-026, D-041). They reference
  the same PERK/SPEL records as variant packages, but are applied at spawn by the
  boss spawn-replacement logic, not by the variant system.

### L3 — Data schema

- No new schema. Boss templates are separate (D-026, D-036: 20 boss templates).

### L4 — Code module

- **Owner:** TierEngine (guard in `ApplyVariantPackage`). Boss spawn replacement is
  separate (D-026, not yet implemented per FIX-PLAN).
- **Co-save:** None.

### Compatibility

- Boss mods: their templates are never touched (no runtime variant application).
- Quest-critical bosses: no risk of variant perks breaking quest scripts (perks are
  additive, not quest-gating).

### Test entry

- Unit: `ApplyVariantPackage` on a rank-4 actor → aborts, no keyword, no perks.
- Integration: spawn a boss in a cell with variants enabled → boss has no variant
  keyword, no variant loot scaling.

### Open questions

- Q14: Whether a future "boss variant" mode (stacking) should be added as an
  optional INI toggle. Chosen: no for v1 (mutual exclusion). Revisit in v1.1 if
  playtests show demand for harder bosses without new templates.

---

## 6. Persistence and D-025 (no co-save state)

Mechanic ID: **V-06 Persistence model.**

### L1 — Rule and numbers

- **V-06.1 No co-save state.** Variant identity is NOT persisted. Re-derived at
  every spawn/load from the deterministic seed (0.6). No new co-save section, no
  schema bump, no migration.
- **V-06.2 Re-derivation on load.** When an actor loads (cell load, save load),
  `ActorSpawn`/`LoadedCellAttach` re-runs eligibility + selection + application.
  The variant keyword and runtime map entry are re-created. The player sees the
  same variant as before (deterministic seed).
- **V-06.3 Loot consistency (S-VAR-1).** At `ActorDeath`, LootEngine reads the
  variant package from the runtime map; if the map misses (e.g., map was cleared
  on cell unload but the actor's death event fires late), it re-derives from the
  seed. The re-derived packageId is guaranteed to match the spawn-time packageId
  (deterministic seed). No reward inconsistency.
- **V-06.4 Uninstall (N-10).** Removing the DLL leaves actors as base forms;
  variant perks disappear on load; no co-save state to clean up. Documented
  limitation: applied name prefixes (SetDisplayName) may persist in the save as
  engine data (same as dynamic item names, D-023). Acceptable.

### L2 — Engine mapping

- **Runtime map lifecycle:** entries added at spawn, erased on death and cell
  unload. On save/load, the map is empty initially and rebuilt as actors load.
- **Re-derivation:** `RollVariant(actor, rfTier, seed)` is deterministic and
  idempotent; calling it on every spawn produces the same result.
- **No co-save read/write for variants.** The existing co-save sections (1-10)
  are untouched.

### L3 — Data schema

- None new.

### L4 — Code module

- **Owner:** TierEngine. No co-save code. No migration.
- **Co-save:** None.

### Compatibility

- Save/load round-trip: variant enemies are identical before and after (deterministic
  seed). Verified by integration test.
- Uninstall: clean (no orphaned state).

### Test entry

- Unit: save, reload, spawn same actor → same packageId (deterministic).
- Integration: variant enemy killed before save → after reload, a new instance of
  the same base NPC at the same spawn point is the same variant.
- Migration: N/A (no state).

### Open questions

- Q15: Whether variant identity should be persisted for quest-critical enemies
  (e.g., a named variant that a quest references). Chosen: no in v1 (variants are
  generic only). If a quest needs a specific variant, author it as a boss template
  (D-026). Revisit if quest design demands it.

---

## 7. Performance (spawn hook 0.2 ms budget)

Mechanic ID: **V-07 Performance.**

### L1 — Rule and numbers

- **V-07.1 Budget.** `ActorSpawn` hook total p99 <= 0.2 ms per actor, including
  rank assignment (existing) + variant selection + application. Variant logic must
  not exceed 0.1 ms of that budget (leaving 0.1 ms for rank assignment and other
  spawn work).
- **V-07.2 O(1) lookup.** All package data is pre-built into hash maps at
  `DataLoaded`:
  - `variantRulesByTier`: array indexed by RF tier (O(1)).
  - `variantPackagesByRace`: `unordered_map<string, vector<int>>` (race EditorID →
    package indices). O(1) average.
  - `variantPackageById`: `unordered_map<int, VariantPackage>` (packageId → def).
    O(1) average.
- **V-07.3 No per-spawn file IO.** CSVs are loaded once at `DataLoaded`. No file
  reads at spawn.
- **V-07.4 No per-spawn allocation in the hot path.** Package definitions are
  pre-loaded; spawn only reads pointers/references. The runtime map insertion is
  the only allocation (amortized O(1)).
- **V-07.5 Caps on package size.** Perk/ability lists per package <= 4/2 items.
  Affix count <= 2. Application is O(package size) = O(6) worst case, negligible.
- **V-07.6 Failure modes.** If spawn hook exceeds budget (measured in Phase 0),
  disable variant selection (log + set `bEnableVariants = 0` for the session) rather
  than shipping a slow hook. Fail-closed (AGENTS §3.1 rule 9).

### L2 — Engine mapping

- **Pre-build at DataLoaded:** TierEngine builds all maps (100 ms budget, part of
  data load). Logged: "Variants: loaded N packages, M rules."
- **Hot path:** eligibility (O(1) checks) → density (O(1) map lookup) → seed
  (O(1) hash) → chance (O(1) RNG) → package lookup (O(1)) → apply (O(package size)).
- **Measurement:** Phase 0 records per-actor spawn time with variants enabled vs
  disabled. If variant path adds > 0.1 ms, investigate (likely perk grant overhead).

### L3 — Data schema

- None new (uses variant_rules.csv, variant_packages.csv from section 2).

### L4 — Code module

- **Owner:** TierEngine. Performance is part of the module's contract (D-055).
- **Co-save:** None.

### Compatibility

- N/A (performance is internal).

### Test entry

- Performance: spawn 1000 actors with variants enabled → measure p50/p99/p999 spawn
  time. Variant path p99 < 0.1 ms (of 0.2 ms total budget).
- Performance: spawn 1000 actors with variants disabled → baseline. Difference
  < 0.1 ms.
- Edge: package with 4 perks + 2 abilities → application < 0.05 ms.

### Open questions

- Q16: Whether perk grants (`AddPerk`) are the spawn-hook bottleneck. If Phase 0
  shows `AddPerk` is slow (> 0.05 ms each), move perk grants to `LoadedCellAttach`
  (2 ms budget, batched). Chosen: keep at spawn for simplicity; measure in Phase 0;
  fall back to cell-attach if needed.

---

## 8. Compatibility (enemy overhauls, boss mods)

Mechanic ID: **V-08 Compatibility.**

### L1 — Rule and numbers

- **V-08.1 Soft-detect overhauls (M-VAR-1).** At `DataLoaded`, the Compatibility
  module checks for known enemy overhaul DLLs/plugins (configurable list in INI
  `[Compatibility] EnemyOverhaulDlls`). If detected:
  - Log "Variants: enemy overhaul detected: <name>".
  - If `bAutoDisableOnOverhaul = 1` (default 1), set `bEnableVariants = 0` for the
    session and log "Variants: disabled due to overhaul conflict."
  - If `bAutoDisableOnOverhaul = 0`, keep variants enabled but log a warning
    ("Variants may conflict with <name>; caps still apply").
- **V-08.2 Caps as the safety net (I-09 pattern).** Even if an overhaul is not
  detected, global caps prevent runaway: HP mult <= 6.0, dmg mult <= 2.5, affixes
  <= 4. INI multipliers (`VariantHpCap`, `VariantDmgCap`) are tunable.
- **V-08.3 No NPC record edits (D-025, D-027).** Variants never edit base NPC
  records. Enemy overhauls' record edits are untouched. No conflict at the record
  level.
- **V-08.4 Boss mods.** Variants skip rank 2+ and bosses (0.1, 5.1). Boss mod
  templates are never touched. No conflict.
- **V-08.5 Fortunate Foes.** Its NPCs are boss templates (rank 4) → skipped by
  rank rule. Its SPID-distributed perks are separate FormIDs from ours → no
  collision. Its keyword-provider gap (no KYWD records) does not affect us.
- **V-08.6 OWL / loot mods.** Variant loot scaling is additive (D-008). OWL's
  leveled lists are untouched. If both are installed, both contribute loot
  (multiplicative drop chance); caps prevent absurdity.
- **V-08.7 Failure modes.** Overhaul detection false positive → variants disabled
  unnecessarily, but no crash. Overhaul detection false negative → variants may
  stack with overhaul perks; caps limit the damage. Documented as known limitation.

### L2 — Engine mapping

- **Compatibility module:** already planned (L4 §1, FIX-PLAN P4.1). Variant
  overhaul detection is a new check in that module, run at `DataLoaded`.
- **INI:** `[Compatibility] EnemyOverhaulDlls` (comma-separated DLL names),
  `bAutoDisableOnOverhaul` (default 1), `bEnableVariants` (default 1, master
  switch).
- **No new hook.** Detection runs at `DataLoaded` (100 ms budget).

### L3 — Data schema

- None new. Overhaul DLL list is in INI.

### L4 — Code module

- **Owner:** Compatibility (detection) + TierEngine (application-time rank guard).
- **Dependencies:** Config, Logger.
- **Papyrus bridge:** `int GetVariantsEnabled()` → 0/1 (for UI/debug).
- **UI surface:** Debug export shows variant status + detected overhauls.
- **Co-save:** None.

### Compatibility

- See above.

### Test entry

- Unit: overhaul detection with a known DLL name in the list → variants disabled.
- Unit: overhaul detection with an unknown DLL → variants stay enabled.
- Integration: install a known enemy overhaul alongside Runefall → variants
  auto-disable, log entry present.
- Integration: caps: variant with hpMult exceeding cap → clamped, logged.

### Open questions

- Q17: Whether the overhaul detection list should be user-editable or hardcoded.
  Chosen: user-editable INI list (default includes common overhauls). Hardcoded
  list is RESERVE for a compatibility patch pack.
- Q18: Whether variants should be disabled entirely when an overhaul is detected,
  or just capped. Chosen: auto-disable by default (safer), user can override via
  INI. Revisit if users want both.

---

## 9. Red-team coverage table

Challenges from REDTEAM-NEW-FEATURES.md §7 (Enemy variant system) mapped to
resolutions in this document:

| Challenge ID | Severity | Resolution section |
|---|---|---|
| C-VAR-1 (D-025 no permanent perk grants) | CRITICAL | §0.2 (re-derive at spawn, no co-save, idempotent keyword) |
| C-VAR-2 (D-005 no SkyPatcher) | HIGH | §0.3 (own PERK records + AddPerk; DPF optional dep) |
| A-VAR-1 (own PERK records suffice?) | HIGH | §0.3, §3 L2 (prototype 3 packages; DPF fallback) |
| E-VAR-1 (variant + boss stacking) | HIGH | §0.1, §5 (mutual exclusion; bosses use templates) |
| E-VAR-2 (idempotency on cell reload) | MEDIUM | §0.2 (exclusive keyword + idempotent AddPerk) |
| E-VAR-3 (naming reliability) | MEDIUM | §1 L1/L2 (SetDisplayName attempt + affix tell + TrueHUD optional) |
| S-VAR-1 (reward scaling consistency) | LOW | §4 L1 V-04.8, §6 (re-derive on map miss) |
| M-VAR-1 (compatibility with overhauls) | HIGH | §8 (soft-detect + auto-disable + caps) |
| P-VAR-1 (spawn hook 0.2 ms) | MEDIUM | §7 (pre-built maps, O(1) lookup, measured) |
| Q-VAR-1 (RESERVE, no world changes) | MEDIUM | §0.4 (no new NPCs/cells; optional pack) |

Additional red-team challenges inherited from the broader audit (applicable here):

| Challenge ID | Feature | Resolution |
|---|---|---|
| C-VAR-1 (D-025) | Variant persistence | §6 (no co-save state; re-derived) |
| E-AIL-2 (ailment-immune bosses) | Ailment × variant | §5 (variants don't apply to bosses; boss immunity is separate) |
| M-DUR-1 (conflict detection) | Durability × variant | §8 (same Compatibility module pattern) |

---

## 10. Co-save impact

**None.** Variant identity is re-derived at spawn (§6). No new co-save section, no
schema bump, no migration. Rune/material grants from variant kills go through
existing co-save sections 2 (rune wallet) and 10 (materials). This is a deliberate
choice to keep the variant system save-safe and uninstall-clean (D-025, D-028,
N-04).

---

## 11. Implementation ordering (when promoted)

1. **TierEngine variant extension** (sections 1-3): marker keyword, package map,
   selection, application. Depends on TierEngine's rank assignment being implemented
   (FIX-PLAN P4.7) and monster affixes (P3.10, P4.7). Blocked by P4.1/P4.2
   (HookManager + ActorSpawn hook).
2. **LootEngine variant reward scaling** (section 4): depends on TierEngine variant
   lookup + MaterialRegistry (C-10, FIX-PLAN P4.8).
3. **Compatibility overhaul detection** (section 8): depends on Compatibility module
   (FIX-PLAN P4.1).
4. **Enemy pack** (L3): KYWD + PERK records + CSV data + manifest. Depends on
   P3.1/P3.2 (category-aware FormID allocator + validator).
5. **Optional DPF integration** (section 3 L2): only if DPF is verified available
   in the target MO2 list; otherwise our own PERK records suffice.

---

## 12. Remaining open questions (consolidated)

1. `SetDisplayName` on actor references in 1.6.1170 (Q1) — Phase 0 spike.
2. Per-package keywords for interop (Q2) — RESERVE.
3. Runtime map pointer lifetime (Q3) — Phase 0 validates.
4. Variant chance scaling with player level (Q4) — RESERVE.
5. Per-NPC variant overrides (Q5) — RESERVE (1.2+).
6. Per-cell vs per-area density cap (Q6) — chosen per-cell.
7. Variant chance INI slider (Q7) — chosen yes.
8. Health scaling channel (Q8) — depends on TierEngine rank scaling implementation.
9. Variant affix subset vs full pool (Q9) — chosen full pool.
10. Damage type scoping for dmgMult (Q10) — chosen all damage.
11. Guaranteed unique drops for variants (Q11) — RESERVE.
12. Quest items in variant bonus pool (Q12) — chosen no.
13. Variant-guarded containers (Q13) — chosen no.
14. Variant-boss stacking (Q14) — chosen no for v1.
15. Quest-critical variant persistence (Q15) — chosen no.
16. `AddPerk` spawn-hook performance (Q16) — Phase 0 measures.
17. Overhaul detection list editability (Q17) — chosen user-editable INI.
18. Auto-disable vs cap on overhaul (Q18) — chosen auto-disable default.

---

RUNTIME STATUS: assistant-claimed (design document, no runtime verification)
SSEEDIT/CK: not launched
UNRESOLVED: Phase 0 proof spikes for SetDisplayName on actors, AddPerk performance,
TierEngine rank-scaling channel; register update (D-047) before any code; overhaul
detection list population; monster-affix system implementation (P3.10/P4.7) must
land before variants.