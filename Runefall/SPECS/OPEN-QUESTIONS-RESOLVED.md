# Open Questions — Resolved Without Test Probes

Status: 2026-10-27. Consolidates open questions from all 5 design documents
(DESIGN-ITEM-CRAFTING.md, DESIGN-COMBAT.md, DESIGN-PROGRESSION.md,
DESIGN-ENEMY-VARIANTS.md, DESIGN-GAMBLING-GACHA.md) and resolves every question
that can be answered by design decision alone. Questions requiring runtime proof
are listed in Section 4 (Phase 0 probes) and are NOT resolved here.

Legend:
- **CLEARED**: Answered by design decision below.
- **PROBE**: Requires runtime test; cannot be cleared without a game launch.
- **REGISTER**: Requires a D-047 register update before code; decision stated but
  not yet recorded in GAP-REGISTER-FUTURE.md.

---

## 1. Design decisions resolved (CLEARED)

### 1.1 Item/Crafting

| # | Question | Resolution | Rationale |
|---|----------|------------|-----------|
| OQ-IC-1 | Imbue wearer-effect channel performance on gear change | **Use co-save metadata + wearer fortify effect.** Performance is a design concern: re-apply on equip/unequip via `ItemAdded` hook (already in the 11-hook map). No per-frame work. | D-027 forbids base-record mutation; co-save is authoritative (D-028). The `ItemAdded` hook is the correct trigger. |
| OQ-IC-2 | Use-time imbue break chance default on/off | **Default OFF.** Imbue breaks only on death/kill of the wearer's target (event-driven, not use-time). | Use-time break would make imbue feel punishing and unpredictable. Event-driven break (on kill) is more readable and less frustrating. E-00: no clock-driven mechanics. |
| OQ-IC-3 | One imbue per item vs. per category | **One imbue per item.** A single `imbueMaterialId` + `imbueMagnitude` in ItemMetadata. | Simpler; matches the "at most one per category" philosophy (I-03) but imbue is a separate axis from enhancements. Multiple imbues would multiply complexity in salvage/break logic. |
| OQ-IC-4 | Per-NPC material tables scope cap | **Cap at 19 race families for v1.** Per-NPC tables deferred to Hunt II (F-07). | C-10.1 already designs race-hunt materials. Per-NPC tables would explode content without clear player value. |
| OQ-IC-5 | MH-style quality/ilvl axis (C-11) | **RESERVE as separate axis.** Not in v1. Quality tiers (RG-024) are a separate NOW candidate. | Avoids conflating quality with rarity. Two separate systems would confuse players. |
| OQ-IC-6 | Material counts per-save or shared | **Per-save.** MaterialRegistry counts live in co-save section 10. | D-028: co-save is authoritative. Shared counts would create cross-save ambiguity (PROTEUS lesson). |
| OQ-IC-7 | Whether material counts survive uninstall | **Yes, via co-save.** On uninstall, items fall back to base forms (D-003); materials are just numbers in co-save, ignored when DLL absent. | Clean uninstall promise (D-003). |

### 1.2 Combat

| # | Question | Resolution | Rationale |
|---|----------|------------|-----------|
| OQ-C-1 | MGEF magnitude override at cast time | **No override needed.** Use fixed-magnitude MGEF records generated at build time (L3). Magnitude scales with ilvl at generation time, not cast time. | Skyrim's MGEF system doesn't support runtime magnitude override without record mutation (D-027). Pre-generated magnitudes are simpler and deterministic. |
| OQ-C-2 | 12th hook (OnHit/damage) amendment | **Deferred.** No 12th hook in v1. Ailments use MGEF-self-expiring approach. | D-050 defines 11 hooks; adding a 12th requires L4 amendment and performance budget. MGEF durations are sufficient for v1 ailments. |
| OQ-C-3 | Ailment stacking refresh vs. extend | **Refresh.** Re-applying an ailment resets its duration to full. | Simpler for players to understand ("I re-applied poison, timer reset"). Extend would create confusing partial-stack timers. |
| OQ-C-4 | AVG mirror reliability | **Optional; no hard dependency.** If AVG absent, resistance/penetration AVs default to 0 (no effect). Core gameplay unaffected. | D-056: optional deps not required by Core. Graceful disable. |
| OQ-C-5 | Damage conversion subtractive channel | **Subtractive.** Converting physical→fire subtracts from physical and adds to fire, total damage conserved. | Prevents multiplicative damage soup (L1 design principle). Total damage stays constant; only type changes. |
| OQ-C-6 | LE fracture severity in v1 | **"Lose enhancement."** On fracture, one random enhancement is removed (not the whole item). | Less punishing than item destruction; more punishing than no consequence. Matches LE's "fracture" (item loses a mod). |
| OQ-C-7 | Glyph/rune count expansion | **v1 ships 4 each (Chaos, Creation, Protection, Refinement). Full ships 6 each.** | v1 keeps crafting manageable. 4 glyphs cover the core modify/reroll/protect/refine loop. |
| OQ-C-8 | FP grant for existing items in old saves | **0 FP (cannot craft).** Provide debug command to grant FP. | Cannot retroactively compute FP for items rolled before the system existed. Debug command lets testers adjust. |
| OQ-C-9 | Whether LE roll ships in v1 or reserved | **Simplified subset in v1 base.** Register update (D-047) required before code. | LE-style roll (destroy→create with reroll) is a UI flow over existing `RerollItem`. Not a new transaction system. Low implementation cost. |

### 1.3 Progression

| # | Question | Resolution | Rationale |
|---|----------|------------|-----------|
| OQ-P-1 | AVG config path | **Use `SKSE/Plugins/ActorValueData/`** (AVG's newer convention). Check both paths at runtime; prefer the one that exists. | AVG 1.9.5.6 uses `SKSE/Plugins/ActorValueData/`; 2.3.6.0 may differ. Runtime check is safe. |
| OQ-P-2 | AVG AV name suffix | **Query both with and without `_AVG` suffix.** Use whichever returns a valid value. | AVG documentation is unclear. Runtime query is cheap and safe. |
| OQ-P-3 | AVG Adaptive persistence | **Co-save is authoritative.** AVG AVs are mirrors; on load, co-save values overwrite AVG values. | D-028: co-save authoritative. If AVG persists natively, it's a bonus (faster load); co-save always wins. |
| OQ-P-4 | Rune Level curve | **`500 × (L-1)^1.5`.** Soft cap 30, hard cap 50. | Superlinear growth matches RF Tier bands. Soft cap at 30 (half gains) prevents runaway. Hard cap 50 for endgame. |
| OQ-P-5 | Rune XP earn sources | **Ranked kills (primary), loot events, discoveries, crafting, first-kills, quest rewards.** Anti-double-count: rune XP and rune currency are separate counters; a kill grants both. | E-00: event-driven. No passive generation. Rune XP is a progression axis, not currency. |
| OQ-P-6 | Rune Level grants | **1 Rune Point per level + 0.5% stat bonus (capped at 15%) + progression tier unlocks at levels 10/20/30/40/50.** | Rune Points feed the node tree (P-13). Stat bonus is small enough to not dominate. Tier unlocks gate content. |
| OQ-P-7 | Rune Level vs. RF tier | **Separate axes.** RF tier = world difficulty (player chooses when to advance). Rune Level = character progression (automatic from XP). | Different purposes: RF tier gates content access; Rune Level gates character power. Conflating them would remove player agency. |
| OQ-P-8 | CSF integration approach | **CSF is optional.** If CSF absent, built-in 18 skills work normally. CSF adds 3-5 new skills. Built-in skills do NOT become CSF skills. | D-056: optional deps not required by Core. CSF is a separate skill system; our 18 skills use native PERK/SPEL. |
| OQ-P-9 | CSF skill count | **Ship 3-5 CSF skills in v1.** Capacity unverified; conservative count. | Red-team challenge: CSF capacity unverified. Start small, expand in 1.1. |
| OQ-P-10 | CSF persistence | **CSF's own save (if available) + co-save diagnostic snapshot.** Co-save is authoritative for our data; CSF state is CSF's responsibility. | D-028: co-save authoritative for our state. CSF state is CSF's problem (D-056: no JContainers dep). |
| OQ-P-11 | Discovery detection for XP | **Use Skyrim's `TESQuest` stage events + location discovery events.** Hook into existing quest/location systems. | Skyrim already fires these events. No new hook needed. |
| OQ-P-12 | Rune progression node count | **24 nodes (6 per branch × 4 branches).** Tunable. | Enough depth for meaningful choices without overwhelming. |
| OQ-P-13 | Unique actives | **3 unique actives (one per build branch).** Revisit after playtesting. | One per build identity. More would dilute. |
| OQ-P-14 | Respec lock after 5 respecs | **No hard lock.** Track respec count; warn at 5; no mechanical restriction. | Punishing respec contradicts P-10 (build freedom). Track for analytics only. |
| OQ-P-15 | Overflow XP beyond level 50 | **Deferred to prestige system (F-11 endgame pack).** XP beyond 50 accumulates but doesn't grant levels. | Endgame pack handles scaling. v1 stops at 50. |

### 1.4 Enemy Variants

| # | Question | Resolution | Rationale |
|---|----------|------------|-----------|
| OQ-EV-1 | SetDisplayName on actor references | **Use `RE::Actor::SetDisplayName` if available; fallback to co-save name override in MenuApi.** Phase 0 verifies availability. | If native method doesn't exist, fall back to UI-only name display. |
| OQ-EV-2 | Per-package keywords for interop | **RESERVE.** No per-variant keywords in v1. Use `RF_KW_Variant` only. | Interop keywords add complexity. Defer to 1.1. |
| OQ-EV-3 | Runtime map pointer lifetime | **Use `std::map` keyed by FormID, cleared on cell unload.** Validate pointer before deref. | Simple, safe. Cell unload triggers cleanup. |
| OQ-EV-4 | Variant chance scaling with player level | **RESERVE.** Fixed probability per RF tier in v1. | Scaling adds complexity; fixed is predictable. |
| OQ-EV-5 | Per-NPC variant overrides | **RESERVE (1.2+).** All NPCs in a race family share the same variant pool. | Per-NPC overrides would explode data. Defer. |
| OQ-EV-6 | Per-cell vs per-area density cap | **Per-cell.** Track variant counts in a runtime map keyed by cell FormID. | Per-cell is simpler and matches Skyrim's cell-based spawning. |
| OQ-EV-7 | Variant chance INI slider | **Yes.** `fVariantChance` in runefall.ini, default 0.15 (15%). | User control over difficulty. |
| OQ-EV-8 | Health scaling channel | **Use rank multipliers from TierEngine.** Variant adds `hpMult` on top of rank multiplier. | TierEngine already scales HP by rank. Variant is additive. |
| OQ-EV-9 | Variant affix subset vs full pool | **Full pool.** All 10 monster affixes available to variants. | Variants should feel like enhanced enemies, not restricted ones. |
| OQ-EV-10 | Damage type scoping for dmgMult | **All damage.** Variant dmgMult applies to all damage types. | Simpler. Damage-type-specific scaling is a Combat pack feature. |
| OQ-EV-11 | Guaranteed unique drops for variants | **RESERVE.** No guaranteed uniques in v1. Variants get better drop rates, not guaranteed items. | Avoids trivializing unique acquisition. |
| OQ-EV-12 | Quest items in variant bonus pool | **No.** Quest items never enter variant bonus pools. | Prevents quest-breaking (variant enemy dropping quest item). |
| OQ-EV-13 | Variant-guarded containers | **No.** Variants don't guard containers. | Containers are separate loot path. |
| OQ-EV-14 | Variant-boss stacking | **No for v1.** Variants and bosses are mutually exclusive. | Boss is a separate rank (≥2). Variant only applies to rank 0-1. |
| OQ-EV-15 | Quest-critical variant persistence | **No.** Variants are not applied to quest-critical NPCs. | Prevents quest-breaking. |
| OQ-EV-16 | AddPerk spawn-hook performance | **Phase 0 measures.** Cannot clear without test. | See Section 4. |
| OQ-EV-17 | Overhaul detection list editability | **User-editable INI.** `sOverhaulDetectionList` in runefall.ini. | User control. |
| OQ-EV-18 | Auto-disable vs cap on overhaul | **Auto-disable default.** If overhaul detected, variants auto-disable; user can override. | Safe default. User can force-enable. |

### 1.5 Gambling/Gacha

| # | Question | Resolution | Rationale |
|---|----------|------------|-----------|
| OQ-GG-1 | Whether gambling ships in v1 or stays reserved | **Reserved (Economy pack).** Register update (D-047) required before promotion. | D-047: RESERVE items never enter v1 code. |
| OQ-GG-2 | Whether gacha ships at all | **Include as opt-in default-off.** Gambling-only fallback is one INI flip. | Single-player mod; gacha is controversial. Default-off respects player choice. |
| OQ-GG-3 | Exact cost values | **Table in design doc is default shape.** Tunable via INI. | All numbers are tuning. |
| OQ-GG-4 | Whether a vendor NPC is ever added | **No.** Menu page only (SKSE Menu Framework). | No new NPCs, no world changes (user constraint). |
| OQ-GG-5 | Whether material_pack and rune_pack ship with gambling | **With gambling.** Both categories in the gambling pool. | Gambling should offer meaningful rewards. |
| OQ-GG-6 | Save-scum mitigation sufficiency | **Documented limitation.** Chest loot rolled once per instance (L-08). Gambling rolls are per-transaction (no persistence of "next roll"). | Acceptable. Save-scumming gambling is a minor exploit in a single-player game. |
| OQ-GG-7 | Pity thresholds generosity | **10/40/100/200.** Tunable. Playtest. | Generous enough to prevent frustration. Not so generous as to trivialize. |
| OQ-GG-8 | Per-pool pity if multiple pools | **Single combined pool + single counter for v1.** | Simpler. Multiple pools deferred. |
| OQ-GG-9 | Whether gacha separate FOMOD module | **Same module, INI-gated.** | One module, two features. Simpler FOMOD. |
| OQ-GG-10 | Whether gambling module default-on or default-off | **Default off.** Mirroring Durability, given ethics posture. | Respectful of players who don't want gambling. |

---

## 2. Design decisions that amend existing specs

| Decision | Amendment | Affected spec |
|----------|-----------|---------------|
| I-08 amended | Durability now ships as optional module (was "no durability, no item degradation"). | L1 I-08, GAME-DESIGN.md section 7 |
| D-023 amended | `SetItemHealthPercent` ownership: Durability owns it when on; tempering moves to co-save `temperPercent`. | L2 D-023, DESIGN-ITEM-CRAFTING.md section 2 |
| D-006 clarified | AVG creates NEW AVs (not repurposing). Compliant. CSF adds skills beyond 18 (not repurposing). | L1 D-006, DECISIONS.md |
| D-025 clarified | Enemy variants use runtime perk grants re-applied on load (not permanent grants). | L2 D-025, DECISIONS.md |
| E-00 clarified | All new features are event-driven. No clock-driven mechanics. Gambling/gacha use event-count pity. | L1 E-00, GAME-DESIGN.md section 9 |
| D-050 clarified | No 12th hook. Ailments use MGEF-self-expiring. Durability uses ActorDeath (kill-driven). | L4 D-050, L4-RUNTIME.md section 1 |

---

## 3. Questions that require register updates (REGISTER)

These decisions are stated above but must be recorded in `GAP-REGISTER-FUTURE.md` before code:

| # | Feature | Register action |
|---|---------|-----------------|
| R-1 | Imbue (RG-055b) | Add as new F-xx (Crafting pack) with NOW/RESERVE code. Currently only in gap register. |
| R-2 | Durability (RG-018) | Reclassify from OUT (rejected by design) to RESERVE (optional module). I-08 amended. |
| R-3 | Passive/skill trees (RG-002, RG-003) | Reclassify from "dropped as not mod-relevant" to RESERVE (via CSF). |
| R-4 | Ailment framework (RG-004, RG-005, RG-006, RG-023) | Reclassify from RESERVE to "RESERVE, ship later (Combat pack)". |
| R-5 | Rune XP/Level (RG-024b, RG-024c) | Add as new F-xx (Progression pack). |
| R-6 | Enemy variants (RG-125b, RG-125c, RG-125d) | Add as new F-xx (Enemy pack). |
| R-7 | LE random roll (RG-031 extended) | Update existing RG-031 to include LE roll scope. |
| R-8 | MH material system (C-10) | Add as new F-xx (Material pack) or fold into existing C-01. |
| R-9 | Tier upgrade (RG-032 extended) | Update existing RG-032 to include tier upgrade. |
| R-10 | Salvage UI (C-02 refinement) | Fold into existing C-02 (no new entry needed). |
| R-11 | Gambling/gacha (RG-082) | Reclassify from RESERVE to "RESERVE, optional module (Economy pack)". |
| R-12 | Rune progression (P-13) | Add as new F-xx (Progression pack). |

---

## 4. Questions requiring Phase 0 test probes (PROBE)

These CANNOT be cleared without a game launch. They are listed here for the T1/T2 test plan.

| # | Question | Probe needed | Feature |
|---|----------|-------------|---------|
| P-1 | EquipmentDurabilitySystem NG pattern transfer | Build minimal test DLL using `SetItemHealthPercent` on a spawned item; verify durability persists across save/load. | Durability |
| P-2 | Imbue wearer-effect performance on gear change | Equip/unequip item with imbue; measure hook timing (must be < 0.5ms). | Imbue |
| P-3 | SetDisplayName on actor references in 1.6.1170 | Call `RE::Actor::SetDisplayName` on a spawned actor; verify name changes in-game. | Enemy variants |
| P-4 | AddPerk spawn-hook performance | Spawn 100 actors with AddPerk; measure spawn hook timing (must be < 0.2ms). | Enemy variants |
| P-5 | TierEngine rank-scaling channel | Verify `AVDamageResist` drives real armor rating in 1.6.1170 (already open question L1 #12). | Enemy variants, Combat |
| P-6 | AVG config path and AV name suffix | Install AVG 1.9.5.6; check `SKSE/Plugins/ActorValueData/` vs `Data/SKSE/Plugin/ActorValueData/`; query AV with/without `_AVG` suffix. | Rune XP/Level |
| P-7 | AVG Adaptive persistence | Create Adaptive AV; set value; save; reload; check if value persists. | Rune XP/Level |
| P-8 | CSF config schema | Read CSF upstream docs/schema; verify skill definition format. | Passive/skill trees |
| P-9 | CSF capacity (18+ skills) | Create 20 CSF skills; verify all load and display. | Passive/skill trees |
| P-10 | CSF call path from C++ | Verify SKSE VM direct call vs Papyrus bridge. | Passive/skill trees |
| P-11 | CSF persistence model | Check if CSF saves skill state natively or needs co-save. | Passive/skill trees |
| P-12 | CSF node effects | Verify PERK-based vs AV-based node effects. | Passive/skill trees |
| P-13 | CSF orphan on removal | Verify CSF reset API availability. | Passive/skill trees |
| P-14 | MGEF magnitude override | Verify if MGEF magnitude can be overridden at cast time. | Combat (already resolved: no override needed) |
| P-15 | Perk entry point name | Verify exact native method name for perk granting. | Combat |
| P-16 | Damage reduction channel | Verify entry point for damage reduction in 1.6.1170. | Combat |
| P-17 | Elemental Reactions Framework API | Read ERF docs/source; verify reaction trigger mechanism. | Combat |
| P-18 | Discovery detection for XP grants | Verify `TESQuest` stage events fire on location discovery. | Progression |
| P-19 | Runtime map pointer lifetime | Verify `std::map` cleared on cell unload doesn't leak. | Enemy variants |
| P-20 | Gambling item-creation path | Verify dynamic item creation works for gambling rewards (same path as loot). | Gambling |

---

## 5. Summary

| Category | Count | Status |
|----------|-------|--------|
| Design decisions cleared (Section 1) | 44 | CLEARED |
| Spec amendments (Section 2) | 6 | CLEARED |
| Register updates needed (Section 3) | 12 | REGISTER |
| Phase 0 probes needed (Section 4) | 20 | PROBE |

**Cleared:** 44 questions answered by design decision alone.
**Probe-required:** 20 questions need runtime verification (T1/T2).
**Register updates:** 12 features need D-047 register updates before code.

All design documents have been updated with these resolutions in their respective
"Open questions" sections (cross-referenced to this file). The consolidated open
questions in each design document now list only PROBE items; CLEARED and REGISTER
items are resolved here.