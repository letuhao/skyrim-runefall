# DESIGN-PROGRESSION — Progression feature cluster (Rune XP/Level, Rune progression, CSF skill trees)

Status: design draft, 2026-10-27. Companion to SPECS/AUDIT-NEW-FEATURES-DESIGN-DEPTH.md and
SPECS/REDTEAM-NEW-FEATURES.md. Resolves three interconnected features: Rune XP and Rune Level
(RG-024b), Rune progression system (RG-024c), and Passive/skill trees via Custom Skills
Framework (RG-002, RG-003).

Layered per D-013: each feature is given L1 rule + numbers, L2 engine mapping, L3 data
schema, L4 code module, compatibility notes, test entry, and open questions. Mechanic IDs stay
traceable to L1-GAMEPLAY-MODEL.md; new IDs are introduced here and must be folded into L1 in a
follow-up doc pass.

Scope note (D-047): all three features are currently RESERVE (Progression pack). This document
is design only; promotion to v1 requires a register update first. AVG and CSF are optional
dependencies with graceful disable (see §0.1, §0.6).

---

## 0. Cross-cutting resolutions

### 0.1 D-006 compliance: no vanilla AV repurposing (resolves A-RXP-1, C-RXP-1, C-CSF-2)

D-006 forbids repurposing vanilla actor values. Resolution (two-tier, fail-closed, mirroring
DESIGN-COMBAT.md §0.1):

- **Primary (always):** Rune XP, Rune Level and Rune Points live in the native co-save
  (section 1, progression) as authoritative counters. The DLL reads/writes these. No vanilla
  AV is written by the core. MGEF conditions and UI that need a "level" value read the co-save
  through the Papyrus bridge or the MenuApi snapshot.
- **Optional enhancement (AVG):** if Actor Value Generator (AVG) is installed and its TOML
  config declares our AVs (`Runefall_RuneXP`, `Runefall_RuneLevel`, `Runefall_RunePoints`),
  the DLL mirrors the co-save values to AVG AVs on change for UI display, console access and
  MGEF condition authoring. If AVG is absent, the co-save counters remain authoritative and
  the UI reads them through the Papyrus bridge. No hard dependency.
- **AVG AVs are custom, not vanilla repurposing.** They are new AVs created by AVG under
  the `Runefall_*` namespace (D-006 compliant). Documented as such so other mods do not
  confuse them with vanilla AVs.
- **AVG AV type: Adaptive.** Per-actor, persisted by the game's own AV system at the same
  level as any vanilla AV (av-doc-notes §4: "manipulated as you would any other actor value,
  and with the same level of persistence"). Functional (formula) is rejected for XP/Level
  because it needs an unverified get/set pair (A-RXP-1). Exclusive is unimplemented (av-doc §5).
- **AVG dependency tier: optional with graceful disable** (M-RXP-1 resolution). If AVG is
  absent, Rune XP/Level progression still works through co-save counters; only the "real AV"
  benefits (console, MGEF conditions, external mod reads) are lost. Logged at startup.

### 0.2 D-027 compliance: no base-record mutation (resolves C-RXP-1, C-CSF-2)

All three features obey D-027: no vanilla or shared records are mutated.

- **Rune XP/Level:** AVG AVs are defined via AVG's TOML data files (new AVs, not mutations).
  No record edits. Co-save counters are runtime state.
- **Rune progression:** purchased nodes grant our own PERK records (in Runefall.esp) via the
  documented `AddPerk`/`RemovePerk` path (L2 §2). No vanilla PERK mutation.
- **CSF skills:** CSF skills are defined by CSF config files and use our own SPEL/PERK/MGEF
  records (in Runefall.esp or a pack ESP). CSF owns skill IDs and tree layout; we own the
  records they reference. No vanilla record mutation. CSF may use its own AVs (C-CSF-2
  resolution: prefer CSF's own AVs; verify at Phase 0; if CSF repurposes ours, conflict —
  namespaced IDs prevent this).

### 0.3 Co-save schema: reuse v2, add progression fields (resolves N-01 churn)

The v2 schema bump defined in DESIGN-ITEM-CRAFTING.md §8 and extended in DESIGN-COMBAT.md §0.3
is reused. No further bump beyond v2 for this cluster; all new fields are additive with defaults.

- **Section 1 (progression) new fields** (see §6):
  `runeXP (u32)`, `runeLevel (u8)`, `runePoints (u16)`, `runePointsSpent (u16)`,
  `runeNodes (array u16)`, `runeRespecCount (u16)`, `runeXPLifetime (u32)`,
  `csfInstalled (bool)`, `csfSkillSnapshot (string, compact)`.
- **AVG mirror** is not a co-save section; it is a runtime mirror updated on change.
- Migration (v2 → v2): all new fields default. Old saves load with Rune Level 1, 0 XP,
  0 points, empty node list. No crash.

### 0.4 Hook map: no new hooks (resolves P-RXP-1, P-CSF-1, C-AIL-1 pattern)

The 11-hook map (D-050) is unchanged. Rune XP grants are event-driven from existing hooks
(`ActorDeath`, `ContainerGeneration`, `MenuOpen`, `ItemAdded`, `LoadedCellAttach` for
discoveries). Rune progression transactions run on `MenuOpen`. CSF skill XP is granted from
the same event hooks (kill/craft/discovery) via `AdvanceSkill` calls. No per-frame work. No
polling. No Papyrus update loops.

### 0.5 Performance: event-driven only (resolves P-RXP-1, P-CSF-1, E-00)

- Rune XP writes: one `ModActorValue` (or co-save write) per XP-granting event, at most a few
  per combat encounter. Never per-frame. AVG AV mirror updated on level-up and on save, plus
  on XP gain if the UI needs it (single call, ~0.01 ms).
- Rune progression transactions: `MenuOpen` (0.2 ms budget) through the SKSE task interface.
- CSF: `AdvanceSkill` calls on events only; CSF's own rendering is separate and non-blocking.
  MenuApi snapshots are immutable and refreshed on game-thread ticks.

### 0.6 Dependency policy (resolves M-RXP-1, E-CSF-1, Q-RXP-1)

- **AVG: optional.** Hard dependency would exclude users without AVG. Graceful disable with
  log if absent. Recommended default: optional.
- **CSF: optional.** If CSF is absent, CSF skills are disabled; the 18 built-in skills
  (P-04) still work. Graceful disable with log.
- **Core progression (attributes, 18 skills, RF tier) never depends on AVG or CSF.** This
  preserves D-056 (optional mods are never required by Core) and D-047 (reserved features
  never enter v1 code).
- Detection: at `DataLoaded`, query for AVG AVs (ActorValueInfo by name) and for the CSF
  plugin (FormResolver / GetModuleHandle). Log the result. Store `csfInstalled` and
  `avgInstalled` in the co-save config snapshot for diagnostics.

---

## 1. Rune XP and Rune Level (RG-024b) — mechanic ID P-12

**Status:** RESERVE (Progression pack). Design only; no v1 code until register update (D-047).

### L1 — Rule and numbers

- **P-12 Rune XP and Rune Level.** A third progression axis alongside character level
  (P-02) and RF Tier (T-06). Rune XP accumulates from gameplay events (kills, loot,
  discoveries, crafting, first-kills). Rune Level is derived from accumulated Rune XP via a
  curve. Rune Level is **not** the same as RF Tier (T-06 is world difficulty; Rune Level is
  character progression) and **not** the same as character level (P-02 grants attribute/skill
  points; Rune Level grants Rune Points + minor stat bonus + unlocks).
- **P-12.1 Rune XP earn sources** (event-driven, E-00; no passive/idle):
  - Ranked enemy kill: `baseXP(rank) × rfTierMultiplier` where baseXP = {R1:5, R2:15,
    R3:40, R4:100, R5:250} and rfTierMultiplier = 1.0 + 0.25 × RF Tier (tier 0 = 1.0,
    tier 5 = 2.25). R5 mythic = 562 XP.
  - Boss kill (rank 4-5 hand-authored): flat +200 XP in addition to rank XP.
  - Loot event (Refined+ item acquired): +10 XP.
  - Discovery (new location): +15 XP.
  - First-kill hunt family (RG-125b): +50 XP.
  - Crafting action (reroll, upgrade, socket, imbue): +5 XP.
  - Quest completion (reward granted): +20 XP.
  - No XP from: normal (unranked) enemy kills, vendor purchases, idle time, waiting.
- **P-12.2 Rune XP vs rune currency (R-03).** Separate resources. Rune currency (R1-R5
  MISC items) is the economy currency (R-01). Rune XP is a progression counter. A ranked
  kill grants both rune currency (R-03) and Rune XP (P-12.1); they are not converted into
  each other. This avoids double-counting confusion (E-RXP-1 resolution): the two faucets
  are tracked independently in the co-save (section 2 wallet vs section 1 runeXP).
- **P-12.3 Rune Level curve.** Hard cap Rune Level 50. XP to reach level L (from level 1):
  `XP(L) = floor(500 × (L-1)^1.5)` cumulative. Examples: L2 = 500, L5 = 500×4^1.5 ≈ 4000,
  L10 = 500×9^1.5 ≈ 13500, L30 = 500×29^1.5 ≈ 77800, L50 = 500×49^1.5 ≈ 171500.
  Soft cap at Rune Level 30: XP cost ×1.5 beyond level 30 (diminishing returns, A3 pattern).
  Tunable via INI `[Progression] RuneXPBase=500, RuneXPExponent=1.5, RuneLevelSoftCap=30,
  RuneLevelHardCap=50, RuneXPSoftCapMultiplier=1.5`.
- **P-12.4 Rune XP death/revert (E-RXP-2).** XP is granted to a **pending counter** during
  gameplay. The pending counter is committed to `runeXP` (authoritative) on save (SaveWrite).
  On revert (death without save), the pending counter is discarded (revert restores loaded
  state). This prevents save-scumming XP gains and keeps AVG AVs consistent (mirror updated
  only on commit). AVG AV is updated immediately on level-up for UI responsiveness, but the
  co-save authoritative value is the committed one; on load, AVG AV is reset from co-save.
- **P-12.5 Grants per Rune Level.** Each Rune Level grants:
  - +1 Rune Point (currency for the Rune progression system, Feature 2).
  - A minor passive stat bonus: +0.5% to all derived stats per Rune Level, capped at +10%
    at Rune Level 20 (diminishing: levels 21-50 grant +0.25% each, capped +17.5% at 50).
    Applied through the Progression ability package (GLOBAL + MGEF), conditioned on
    `runeLevel` read from co-save/AVG.
  - Unlocks: Rune Level 5/10/15/20/25/30/40/50 each unlock a tier of Rune progression
    nodes (Feature 2).
- **P-12.6 Rune Level cap.** Hard cap 50 (P-12.3). No prestige/reset in v1. Soft cap at 30
  (XP cost multiplier). Beyond 50, XP is banked but no level-up (overflow counter stored in
  co-save for a future prestige system).
- **P-12.7 AVG AV definitions.** Three Adaptive AVs, namespaced `Runefall_*`:
  - `Runefall_RuneXP` (Adaptive) — current XP toward next level (0..threshold).
  - `Runefall_RuneLevel` (Adaptive) — current Rune Level (1..50).
  - `Runefall_RunePoints` (Adaptive) — unspent Rune Points.
  All are mirrored from co-save on change. AVG TOML ships under the candidate path (§0.1);
  exact schema UNVERIFIED (av-doc-notes §6).
- **P-12.8 AVG config (TOML, candidate).** Ship `Runefall_AVG.toml` with Adaptive entries.
  Example (schema UNVERIFIED — verify against installed AVG's shipped TOML before release):
  ```toml
  [ActorValues]
  Runefall_RuneXP = { Type = "Adaptive" }
  Runefall_RuneLevel = { Type = "Adaptive" }
  Runefall_RunePoints = { Type = "Adaptive" }
  ```
  Ship under `Data/SKSE/Plugins/ActorValueData/` (shipped location) and, if the docs path
  proves canonical, also under `Data/SKSE/Plugin/ActorValueData/`. Phase 0 resolves which.
- **P-12.9 AVG detection.** At `DataLoaded`, query `RE::ActorValueInfo::GetActorValueInfoByName`
  for `Runefall_RuneXP` (then `Runefall_RuneXP_AVG` if AVG appends the suffix — UNVERIFIED).
  If found, set `avgInstalled = true` and enable mirroring. If not found, set false, log
  "AVG AVs not found — Rune XP/Level running in co-save-only mode", and continue. Fail-closed:
  no crash, no partial mirroring.
- **P-12.10 Failure modes.** AVG absent → co-save-only mode, progression still works. AVG
  present but TOML invalid → AVG AVs not created; same fallback, logged. AVG AV write fails
  (null actor) → log, skip, retry on next event. XP grant overflow → clamp to u32 max, log.
  Level-up at hard cap → XP banked, no level-up, logged.

### L2 — Engine mapping

- **AV type (0.1):** Adaptive. Per-actor, persisted by the game's own AV system. We mirror
  co-save → AVG AV on change so the AVG AV never diverges from co-save (co-save wins on load).
- **Earn events → hooks:** `ActorDeath` (rank/boss/first-kill XP), `ContainerGeneration`
  (loot XP), `LoadedCellAttach` (discovery XP — when a new cell/location is first entered,
  check co-save discovery flag), `MenuOpen` (crafting XP on transaction), `ItemAdded`
  (loot XP for items entering inventory outside containers). All existing hooks; no new hook.
- **Commit on save:** `SaveWrite` commits pending XP → `runeXP`; computes level-ups; updates
  AVG AVs; writes co-save section 1. `SaveLoad/Revert` restores pending = 0 and resets AVG
  AVs from co-save.
- **Level-up detection:** on commit, while `runeXP >= XP(runeLevel+1)` and `runeLevel < 50`,
  increment `runeLevel`, grant +1 Rune Point, apply passive bonus. Send mod event
  `Runefall_OnRuneLevelUp(newLevel)`. Show notification via Papyrus.
- **Application of passive bonus:** Progression module reads `runeLevel` from co-save/AVG and
  applies `+0.5%/level` (capped) to all derived stats through the existing ability package
  (GLOBAL + MGEF), conditioned on `runeLevel`. Recomputed on level-up and on load.
- **Papyrus bridge:** new query `int GetRuneXP()`, `int GetRuneLevel()`, `int GetRunePoints()`.
  New mutation `int CommitRuneXP()` (debug/force commit). New event `Runefall_OnRuneLevelUp`.
- **UI surface:** Character sheet (L4 §7 #1) gets a "Rune Level" bar showing XP progress,
  level, passive bonus, and Rune Points. Reads MenuApi snapshot (runeXP, runeLevel,
  runePoints, nextLevelXP).
- **Uninstall:** AVG AVs persist as unknown AVs in the save; game ignores unknown AVs (AVG
  handles cleanup). Co-save section 1 fields are ignored by the game when the DLL is removed.
  Documented limitation (N-10).

### L3 — Data schema

- **No new CSV for v1.** Rune XP uses constants in INI (`[Progression]` section) and the
  curve formula (P-12.3). Earn amounts are code constants (P-12.1) tunable via INI overrides:
  `RuneXP_KillR1..R5`, `RuneXP_Boss`, `RuneXP_Loot`, `RuneXP_Discovery`, `RuneXP_FirstKill`,
  `RuneXP_Craft`, `RuneXP_Quest`.
- **AVG TOML:** `Runefall_AVG.toml` (§P-12.8). Shipped as loose data (no rebuild needed to
  add AVs — but AVG requires its own reload; document).
- **FormID range:** none new. Rune XP/Level uses existing GLOBAL records (one new GLOBAL
  `RF_GLOB_RuneLevel` for MGEF conditions) allocated in the 0x880-0x8FF range (globals).
  No new MGEF/SPEL/PERK records.

### L4 — Code module

- **Owner:** Progression (counter, level-up, grants, passive bonus application) + SaveSystem
  (commit/revert) + RuneForge (crafting XP hooks).
- **New module?** No new module; extend Progression. Add an AVG adapter (presence check +
  mirror functions) inside Progression or a small `AvgAdapter` helper.
- **Co-save:** section 1 fields (§0.3, §6). Pending counter is runtime-only (not co-save);
  committed on save.
- **Papyrus bridge:** queries/mutations/events (§L2). Add to the existing bridge surface.
- **Config (runefall.ini):** `[Progression]` keys: `EnableRuneXP` (bool, default true),
  `EnableAVGMirror` (bool, default true), `RuneXPBase`, `RuneXPExponent`, `RuneLevelSoftCap`,
  `RuneLevelHardCap`, `RuneXPSoftCapMultiplier`, earn amounts, passive bonus caps.
- **UI surface:** Character sheet Rune Level bar (§L2).

### Compatibility

- Additive: no vanilla record edits. AVG AVs are new (AVG-owned). Co-save counters are
  runtime state. Other mods' progression (Adamant, Experience) is untouched; stacking is
  documented as a known interaction (test in compatibility suite).
- Interaction with P-07 (rune-bought points): orthogonal. Rune XP does not grant
  attribute/skill points; it grants Rune Points (Feature 2). No overlap.
- Interaction with RF Tier (T-06): orthogonal. RF Tier gates content; Rune Level grants
  Rune Points + passive bonus. Both advance independently.

### Test entry

- Unit: XP grant with AVG absent → co-save counter increments, no crash. XP grant with AVG
  present → AVG AV mirrors value. Level-up at threshold → runeLevel increments, Rune Point
  granted, mod event fires. Revert → pending counter discarded, runeXP unchanged.
- Integration: kill ranked enemy → XP granted → level-up notification shown. Save/load
  round-trip: runeXP, runeLevel, runePoints persist. Migration: load v1/v2 save without
  Rune XP fields → defaults applied, no crash.
- Performance: XP grant (single event) < 0.05 ms; save commit (1000 XP gains batched) < 5 ms.

### Open questions

- Q1 (AVG path): `Data/SKSE/Plugin/ActorValueData` (docs) vs `SKSE/Plugins/ActorValueData`
  (shipped). Phase 0 resolves; ship in both candidate locations if ambiguous.
- Q2 (AVG suffix): does AVG append `_AVG` to AV names? Phase 0 queries both.
- Q3 (AVG persistence): does an Adaptive AV persist natively across save/load? Our design
  does not rely on it (co-save wins on load), but if it does not persist, the mirror is
  recomputed on load anyway. No blocker.
- Q4 (discovery detection): how to detect "new location discovered" reliably without a
  per-frame check. Candidate: hook `TESObjectREFR` activation / map marker discovery event
  via Papyrus `OnLocationChange` or SKSE's `LocationDiscovery` event. Phase 0 resolves.
- Q5 (overflow): banked XP beyond level 50 — future prestige system. Deferred.
- Q6 (AVG AV read by other mods): if an external mod reads `Runefall_RuneLevel`, does it
  get the AVG AV value? Yes if AVG is installed; document that external reads require AVG.
- Q7 (AVG absent but user installs later): co-save counters already tracked; on next load
  with AVG present, mirror is initialized from co-save. Works.

---

## 2. Rune progression system (RG-024c) — mechanic ID P-13

**Status:** RESERVE (Progression pack). Depends on P-12 (Rune XP/Level) being settled.
Design only; no v1 code until register update (D-047).

### L1 — Rule and numbers

- **P-13 Rune progression.** Rune Points (earned via Rune Level, P-12.5) are spent on
  Rune progression nodes — a separate tree of passive bonuses and unique abilities beyond
  the 18 skill lines (P-04) and attribute points (P-02). Rune progression is the
  "what does each Rune Level grant" answer: it grants Rune Points, which buy nodes here.
- **P-13.1 Two currencies, no overlap (C-RPR-2 resolution).** Rune Points (earned, not
  purchasable) buy Rune progression nodes. Attribute points and skill points remain
  purchasable with rune currency (P-07) and granted by character level (P-02). Rune Points
  are **not** purchasable with rune currency — this prevents a second rune-spend path for
  the same goal and keeps the axes distinct (Rune Level = earned progression; runes =
  economy).
- **P-13.2 Node structure.** Each node has: `nodeId (u16)`, `name`, `description`,
  `cost (Rune Points)`, `runeLevelGate (min Rune Level)`, `attributeGate (optional: attr +
  value)`, `effect (perk/ability/stat bonus)`, `prerequisites (list of nodeIds)`,
  `branch (Warrior/Thief/Mage/Universal)`. Nodes are PERK records in Runefall.esp (passive)
  or SPEL records (active abilities) granted via AddPerk/AddSpell.
- **P-13.3 Node budget (v1).** 24 nodes across 4 branches (6 per branch). Branches:
  - Warrior branch: melee damage, stagger, armor, crafting efficiency, health, a unique
    active "Rune Slam" (native hotkey).
  - Thief branch: crit, evasion, magic find, rune find, vendor value, a unique active
    "Rune Vanish".
  - Mage branch: spell magnitude, cooldown reduction, summon duration, mana regen, a
    unique active "Rune Detonation".
  - Universal branch: rune find, carry weight, potion effectiveness, magic find, a unique
    passive "Rune Attunement II" (+5% all stats, stacks with P-12.5).
- **P-13.4 Cost curve.** Node cost = 1-5 Rune Points. Tier 1 nodes cost 1, tier 2 cost 2,
  tier 3 cost 3, capstones cost 5. Total cost to max one branch ≈ 15 Rune Points ≈
  Rune Level 15 (one branch) or Rune Level 45 (all four, with overlap). Tunable.
- **P-13.5 Gates.** `runeLevelGate` as above (5/10/15/20/25/30/40/50 tiers). Attribute
  gate optional: some nodes require attr ≥ 10/20/30 (reuses P-04 gate pattern). Prerequisite
  nodes must be purchased first.
- **P-13.6 Unique actives (D-021).** Three unique active abilities (one per build branch)
  are cast through the native hotkey layer (D-021), with cooldowns/charges managed natively
  (Progression module). They are SPEL records in Runefall.esp, granted on node purchase and
  removed on respec. They do not occupy the 18-skill slots (P-04) — they are Rune progression
  abilities, a separate pool.
- **P-13.7 Respec (E-RPR-1).** A forge service resets all Rune progression nodes:
  cost = 75 rune units + 10% of total Rune Points spent (escalating: ×1.10 per respec,
  capped at 5 respecs then locked until next Rune Level). Refunds all Rune Points. Mirror
  P-08 structure. Respec does not reset Rune Level or Rune XP.
- **P-13.8 Failure modes.** Insufficient Rune Points → transaction rejected, no consumption.
  Prerequisite not met → rejected. Gate not met → rejected. Respec with insufficient runes →
  rejected. Module disabled (INI) → command returns error code.

### L2 — Engine mapping

- **Application channel:** purchased nodes grant PERK/SPEL records via `AddPerk`/`AddSpell`
  (L2 §2, documented path). Removed on respec via `RemovePerk`/`RemoveSpell`. No base-record
  mutation (D-027). Passive bonuses from nodes are PERK conditions on our GLOBAL state.
- **Transaction validation:** RuneForge validates (points available, gates met, prerequisites
  met) and deducts Rune Points; Progression applies the grant. Single code path through the
  Papyrus bridge.
- **Hook:** `MenuOpen` (forge/character sheet) for the spend UI. No new hook.
- **Papyrus bridge:** new query `int GetRunePoints()`, `string GetRuneNodes()` (compact list),
  `int GetRuneNodeCost(int nodeId)`. New mutation `int SpendRunePoint(int nodeId)`,
  `int RespecRuneProgression()`. New event `Runefall_OnRuneNodePurchased(nodeId)`.
- **UI surface:** Character sheet (L4 §7 #1) gets a "Rune" tab listing 24 nodes with costs,
  gates, prerequisites, and spend/refund buttons. Reads MenuApi snapshot.
- **Uninstall:** purchased PERKs/SPELs are granted at runtime; removing the DLL leaves them
  un-granted (they disappear on load). Co-save node list is ignored. Documented (N-10).

### L3 — Data schema

- **New data file `rune_progression.csv`** (or `rune_nodes.csv`):
  `nodeId,name,branch,tier,cost,runeLevelGate,attrGate,attrValue,prerequisites,effectType,effectKey,effectMagnitude,perkEditorId,spellEditorId`
  - `prerequisites`: semicolon-separated nodeIds or empty.
  - `effectType`: `stat_bonus`, `passive_perk`, `active_ability`, `unlock`.
  - `perkEditorId`/`spellEditorId`: EditorID of the granted record (or empty).
- **FormID range:** PERK records for Rune progression nodes allocated in the existing PERK
  range (0xB00-0xB7F per D-035; check capacity — 24 nodes need 24 PERKs + 3 SPELs for
  actives; ensure range has room or extend to a pack ESP per D-040). Recommend shipping Rune
  progression in `Runefall - Progression.esp` (pack) to keep base FormID budget clean.
- **No new MGEF needed** (uses existing fortify templates or node-specific PERKs).

### L4 — Code module

- **Owner:** Progression (node grants, respec, point tracking) + RuneForge (transaction
  validation) + DataStore (rune_progression.csv).
- **Co-save:** section 1 fields `runeNodes`, `runePointsSpent`, `runeRespecCount` (§0.3, §6).
- **Papyrus bridge:** queries/mutations/events (§L2).
- **Config (runefall.ini):** `[Progression]` keys: `EnableRuneProgression` (bool, default
  true), `RuneProgressionMaxRespecs`, respec costs, node cost multiplier.

### Compatibility

- Additive: no vanilla record edits. Rune progression PERKs/SPELs are our own records.
- Interaction with the 18 skills (P-04): orthogonal. Rune progression nodes are separate
  from skill ranks; a character can invest in both. Unique actives (P-13.6) do not occupy
  skill slots.
- Interaction with attribute gates: reuses P-04 gate pattern (read-only attribute values).
- Interaction with CSF skills (Feature 3): orthogonal. CSF skills are XP-based; Rune
  progression is point-based. Both can be invested in.

### Test entry

- Unit: SpendRunePoint with insufficient points → error, no consumption. With sufficient
  points and gates met → node purchased, PERK granted, points deducted. Prerequisite not
  met → error. Respec → all nodes removed, points refunded, cost deducted.
- Integration: purchase node → passive bonus applies → unequip/reload → persists. Unique
  active granted → hotkey casts it → cooldown works.
- Save/load round-trip: purchased nodes persist. Migration: load save without runeNodes →
  empty, no crash.

### Open questions

- Q1 (respec refund): refund all Rune Points or partial? Chosen: full refund (P-13.7).
  Revisit if playtests show respec abuse.
- Q2 (node count): 24 nodes (6 per branch) for v1 of the progression pack. Scale up later.
- Q3 (unique actives): 3 unique actives (one per build branch) + none for Universal.
  Revisit if demand for more.
- Q4 (Rune Level gate tiers): 5/10/15/20/25/30/40/50. Tunable.
- Q5 (cost escalation): respec cost ×1.10 per respec, capped at 5. Revisit.
- Q6 (interaction with P-07): Rune Points are not purchasable (P-13.1). Revisit if
  playtests show players want to buy Rune Points with runes.

---

## 3. Passive/skill trees via Custom Skills Framework (RG-002, RG-003) — mechanic ID P-14

**Status:** RESERVE (Progression pack). Design only; no v1 code until register update (D-047).

### L1 — Rule and numbers

- **P-14 CSF skill trees.** New custom skill lines beyond the 18 built-in skills (P-04),
  each with its own tree, shipped via Custom Skills Framework (CSF). CSF skills are
  **additional** skill lines, not replacements for the 18 built-in skills.
- **P-14.1 D-018 unchanged.** The 18 built-in skill lines (P-04, 6 per build, 3 ranks)
  remain the core skill system. CSF adds *more* skills; it does not change the 18-skill
  budget, the skill-point pool (P-06), or the rank gates (Skyrim skill 20/50/75 +
  attribute 10/20/30). The 18 built-in skills use skill points (P-06); CSF skills use
  XP-based leveling (like Skyrim skills) driven by our event hooks. Two complementary
  progression layers, not competing ones (C-CSF-1 resolution).
- **P-14.2 CSF skill count (v1).** Ship 3-5 CSF skills initially (not 18) to stay within
  unverified CSF capacity (Q18). Candidate skills:
  - `Runefall_RuneMastery` (Mage-flavored): Rune XP gain +, Rune Level passive bonus +.
  - `Runefall_Hunting` (Thief-flavored): hunt material drop chance +, first-kill XP +.
  - `Runefall_CraftingMastery` (Warrior-flavored): crafting efficiency +, reroll cost -.
  - `Runefall_Survival` (Universal): carry weight +, potion effectiveness +.
  - `Runefall_Lore` (Universal): magic find +, spell learning +.
  Scale up after CSF capacity is verified.
- **P-14.3 CSF skill leveling.** CSF skills level up via XP we grant through `AdvanceSkill`
  on gameplay events: kills (scaled by rank), crafting actions, discoveries, first-kills.
  CSF handles the XP→level curve internally (CSF's own formula). We do not override it.
  CSF skill level thresholds (e.g., level 25/50/75) unlock tree nodes (CSF's own node
  system, defined in CSF config).
- **P-14.4 CSF skill gates.** CSF skills have their own gates defined in CSF config:
  attribute gate (attr ≥ 10/20/30) and optionally a Rune Level gate. They do not use the
  18-skill Skyrim skill gates. Attribute values are read from co-save/AVG (read-only).
- **P-14.5 CSF skill actives (D-021).** CSF skills that grant active abilities use our
  native hotkey layer (D-021) for casting, not CSF's (CSF has no hotkey layer). At a CSF
  level threshold, the skill grants a SPEL record (our own, in Runefall.esp or a pack ESP)
  via AddSpell. Our hotkey layer casts it with native cooldowns. CSF provides the tree UI
  and leveling; combat casting is ours.
- **P-14.6 CSF as optional dependency (E-CSF-1).** If CSF is absent, CSF skills are
  disabled; the 18 built-in skills still work. Detection at `DataLoaded`. Graceful disable
  with log: "CSF not found — custom skill trees disabled." No crash, no partial state.
- **P-14.7 CSF skill IDs (M-CSF-1).** All skill IDs namespaced `Runefall_<Name>` (e.g.,
  `Runefall_RuneMastery`). Validate uniqueness at load; log and skip on collision.
  Never use unprefixed IDs (collision risk with other CSF mods).
- **P-14.8 CSF config schema (A-CSF-1).** UNVERIFIED. The CSF config schema (upstream
  `docs/schema`) has not been read. Design ships CSF config files under
  `Data/CustomSkills/` (CSF's expected location) or `Data/Runefall/csf/` (our location,
  copied by the build). Phase 0 must read the schema and record findings in av-doc-notes
  before any config is authored. Until then, the config fields below are *expected*, not
  verified.
- **P-14.9 Expected CSF config fields (unverified):** skill ID (string), display name
  (localization key), description (localization key), icon path, max level, XP curve
  (CSF-internal), tree nodes (id, name, description, required level, prerequisites,
  effects), and any CSF-specific flags. Exact keys TBD by Phase 0.
- **P-14.10 CSF persistence (A-CSF-2).** CSF persists its own skill state through its own
  save mechanism (unverified). Our co-save does **not** mirror CSF skill state (to avoid
  a double source of truth, D-028). We store only a `csfInstalled` flag and a compact
  `csfSkillSnapshot` string (skill IDs + levels) in section 1 for diagnostics. Orphan risk
  on CSF removal is documented (N-10, E-CSF-2). Provide a debug command "reset CSF skills"
  that calls CSF's reset API (if available) or logs instructions.
- **P-14.11 CSF performance (P-CSF-1).** CSF menu rendering is CSF's own (separate budget).
  Our DLL does not render CSF trees. We call `AdvanceSkill`/`GetSkillLevel` on events only
  (kill/craft/discovery), never per-frame. MenuApi snapshots include CSF skill levels for
  our own UI (read-only display).
- **P-14.12 CSF failure modes.** CSF absent → skills disabled, logged. CSF config invalid
  → that skill disabled, logged, others continue. CSF skill ID collision → skip, log.
  CSF API call fails (null) → log, skip, no crash. CSF removal mid-save → orphaned state
  in save; CSF handles cleanup; our snapshot is ignored.

### L2 — Engine mapping

- **CSF API (av-doc-notes §4):** `GetAPIVersion`, `OpenCustomSkillMenu(string)`,
  `AdvanceSkill(string, float)`, `GetSkillLevel(string)`, `ShowSkillIncreaseMessage(string, int)`,
  `DebugReload()`. All `global native`. We call these from the DLL via the Papyrus bridge
  or directly via SKSE's Papyrus VM. Phase 0 verifies the call path from C++ (SKSE's
  `RE::BSScript::IVirtualMachine` or a Papyrus event).
- **XP grant → hooks:** `ActorDeath` (kill XP scaled by rank), `MenuOpen` (craft XP on
  transaction), `LoadedCellAttach` (discovery XP). Same hooks as Rune XP (P-12.1) but
  calling `AdvanceSkill` instead of (or in addition to) adding Rune XP. A kill grants both
  Rune XP and CSF skill XP (different pools, no conflict).
- **Level threshold → grant:** when `GetSkillLevel` crosses a threshold (25/50/75), grant
  the associated SPEL via AddSpell and show a notification. Tracked in co-save snapshot.
- **Active ability casting (D-021):** granted SPELs are cast via the native hotkey layer.
  CSF does not provide casting; it provides leveling and tree UI only.
- **CSF menu:** `OpenCustomSkillMenu(skillId)` opens CSF's own tree UI. We can invoke it
  from our Character sheet ("Open CSF Tree" button) or let the player use CSF's console
  bridge (`csf` command). Our UI does not reimplement CSF's tree.
- **Papyrus bridge:** new query `int GetCSFSkillLevel(string skillId)`,
  `bool IsCSFInstalled()`. New mutation `int GrantCSFSkillXP(string skillId, float xp)`,
  `int OpenCSFSkillMenu(string skillId)`. New event `Runefall_OnCSFSkillLevelUp(skillId, level)`.
- **UI surface:** Character sheet (L4 §7 #1) gets a "Custom Skills" section listing
  installed CSF skills with levels and an "Open Tree" button per skill. Reads MenuApi
  snapshot (csfSkills array).

### L3 — Data schema

- **New data file `csf_skills.json`** (or `.csv`) defining our CSF skills in our format:
  `skillId, displayNameKey, descriptionKey, iconPath, branch, xpSource, levelThresholds,
  grantedSpells, attributeGate, runeLevelGate`. The build copies/transforms this into CSF's
  config format (exact transform TBD by Phase 0 schema verification).
- **CSF config files** under `Data/CustomSkills/` (CSF's expected location). Generated by
  the build from `csf_skills.json`. Schema UNVERIFIED.
- **FormID range:** SPEL/PERK records for CSF skill actives allocated in Runefall.esp or a
  pack ESP (`Runefall - Progression.esp`). Use existing ranges; ensure capacity.

### L4 — Code module

- **Owner:** Progression (CSF adapter, XP grants, threshold grants) + DataStore (csf_skills
  data) + Compatibility (CSF detection).
- **New module?** A `CsfAdapter` helper inside Progression (or a separate module if CSF
  integration grows). Detection, XP grants, level reads, menu invocation.
- **Co-save:** section 1 fields `csfInstalled`, `csfSkillSnapshot` (§0.3, §6).
- **Papyrus bridge:** queries/mutations/events (§L2).
- **Config (runefall.ini):** `[Progression]` keys: `EnableCSFSkills` (bool, default true),
  `CSFSkillCount` (int, default 5), `CSFXPMultiplier` (float, default 1.0).

### Compatibility

- Additive: no vanilla record edits. CSF skills use our own SPEL/PERK records.
- CSF as optional dep (§0.6). If absent, disabled, logged. Built-in 18 skills unaffected.
- Interaction with other CSF-using mods: namespaced IDs prevent collisions (M-CSF-1).
  Menu conflicts: CSF's own menu is separate; ours invokes it via `OpenCustomSkillMenu`.
- Interaction with the 18 skills (P-04): orthogonal. CSF skills do not consume skill points
  (P-06). They level via XP.
- Interaction with Rune progression (P-13): orthogonal. CSF skills can have Rune Level gates.
- Interaction with attribute gates: reuses read-only attribute values.

### Test entry

- Unit: CSF absent → `IsCSFInstalled()` false, skills disabled, no crash. CSF present →
  detection true. GrantCSFSkillXP → `AdvanceSkill` called, no crash. Level threshold →
  SPEL granted.
- Integration: kill enemy → CSF skill XP granted → level-up notification → tree node
  unlockable via CSF menu. Save/load → skill levels persist (via CSF). Migration: load save
  without csfSkillSnapshot → empty, no crash.
- Performance: CSF XP grant (single event) < 0.05 ms. No per-frame work.

### Open questions

- Q1 (CSF schema): upstream `docs/schema` not read. Phase 0 must read it and record in
  av-doc-notes before any config is authored.
- Q2 (CSF capacity): can CSF handle 18+ custom skills? UNVERIFIED. Ship 3-5 initially.
- Q3 (CSF call path from C++): does the DLL call CSF's Papyrus functions directly via
  SKSE's VM, or via a Papyrus script bridge? Phase 0 resolves.
- Q4 (CSF persistence): how does CSF persist skill state? If it uses its own save
  mechanism, our co-save does not mirror (D-028). If it uses JContainers, that's a
  dependency we avoid (D-056). Verify.
- Q5 (CSF orphan on removal): documented as known limitation (N-10). Provide debug command.
- Q6 (CSF XP curve): CSF's internal XP→level curve. We grant XP; CSF converts. Verify
  the amounts feel right in playtesting.
- Q7 (CSF node effects): how do CSF tree nodes apply effects? If they apply PERKs, we
  ship PERK records. If they apply AV changes, we need AVG AVs. Verify schema first.
- Q8 (CSF and our hotkeys): CSF skills' actives use our hotkey layer (D-021). Verify
  CSF exposes a "granted spell" hook or we grant SPELs at thresholds ourselves. Chosen:
  we grant SPELs at thresholds (AddSpell), CSF handles tree UI only.

---

## 4. Red-team coverage table

Maps the 22 questions from the task brief to resolutions in this document.

| # | Question | Resolution | Section |
|---|---|---|---|
| Q1 | Does AVG work on 1.6.1170? | UNVERIFIED; optional dep with graceful disable; Phase 0 spike | §0.1, §1 L1 P-12.9, Open Q1 |
| Q2 | Do AVG AVs persist across save/load? | Adaptive type persists natively; co-save wins on load (mirror reset); no reliance | §0.1, §1 L1 P-12.4, Open Q3 |
| Q3 | What is Rune XP? | Progression counter, separate from rune currency; event-driven | §1 L1 P-12, P-12.2 |
| Q4 | Rune Level curve? | `500 × (L-1)^1.5` cumulative, soft cap 30, hard cap 50 | §1 L1 P-12.3 |
| Q5 | What does each Rune Level grant? | +1 Rune Point, +0.5% stats (capped), unlocks Rune progression tiers | §1 L1 P-12.5 |
| Q6 | Rune Level cap? | Hard cap 50, soft cap 30 (XP ×1.5), overflow banked | §1 L1 P-12.6 |
| Q7 | Rune XP vs rune currency (R1-R5)? | Separate resources; both granted on ranked kill; tracked independently | §1 L1 P-12.2 |
| Q8 | AVG dependency: hard/optional/bundled? | Optional with graceful disable; not bundled | §0.6, §1 L1 P-12.9 |
| Q9 | AVG AV definitions: how defined? | TOML, Adaptive type, `Runefall_*` names, suffix `_AVG` (unverified) | §1 L1 P-12.7, P-12.8 |
| Q10 | Performance: AVG reads/writes per-frame? | Event-driven; writes on XP gain/level-up/save, never per-frame | §0.5, §1 L2 |
| Q11 | Three axes interaction? | Character level → attr/skill points; RF tier → world difficulty; Rune Level → Rune Points + passive + unlocks. Orthogonal. | §1 L1, §2 L1 P-13.1 |
| Q12 | Rune Level vs RF tier same? | No — RF tier is world difficulty, Rune Level is character progression | §1 L1 P-12, L1-GAMEPLAY §2 |
| Q13 | Rune Level gates what content? | Rune progression node tiers (P-13.5); passive stat bonus scales with level | §1 L1 P-12.5, §2 L1 P-13.5 |
| Q14 | Rune Level save compat? | Co-save section 1 fields; AVG mirror reset on load; revert discards pending | §0.3, §1 L1 P-12.4, §6 |
| Q15 | CSF integration: how create custom skills? | CSF config files + our SPEL/PERK records; CSF handles tree UI and leveling | §3 L2, §3 L1 P-14 |
| Q16 | CSF config schema? | UNVERIFIED; Phase 0 spike; expected fields listed | §3 L1 P-14.8, P-14.9, Open Q1 |
| Q17 | CSF skills vs attribute/skill gates? | CSF skills have own gates (attr + Rune Level); do not use 18-skill Skyrim gates; attributes read-only | §3 L1 P-14.4, §3 Compatibility |
| Q18 | CSF handle 18+ skills? | UNVERIFIED; ship 3-5 initially | §3 L1 P-14.2, Open Q2 |
| Q19 | CSF hotkeys: native or CSF's? | Native hotkey layer (D-021); CSF has no hotkey layer | §3 L1 P-14.5, §3 L2 |
| Q20 | CSF dependency: hard/optional? | Optional with graceful disable; built-in 18 skills work without CSF | §0.6, §3 L1 P-14.6 |
| Q21 | CSF tree UI per-frame? | CSF renders its own UI; our DLL calls CSF API on events only; no per-frame work | §0.5, §3 L1 P-14.11 |
| Q22 | CSF save compat: CSF's save or ours? | CSF owns its skill state; we store a diagnostic snapshot only (D-028) | §3 L1 P-14.10, §0.3 |

### Red-team challenge coverage (from REDTEAM-NEW-FEATURES.md §5, §6, §3)

| ID | Challenge | Resolution | Section |
|---|---|---|---|
| A-RXP-1 | AVG works on 1.6.1170? | UNVERIFIED; optional dep, graceful disable; Phase 0 spike | §0.1, §1 Open Q1 |
| A-RXP-2 | AVG Adaptive AVs persist? | Co-save wins on load; mirror reset; no reliance on native persistence | §1 L1 P-12.4, Open Q3 |
| C-RXP-1 | D-006 compliance (custom AVs) | Namespaced `Runefall_*`, documented as custom, not vanilla | §0.1 |
| C-RXP-2 | Three axes overlap? | Distinct grants (Rune Points vs attr/skill points vs RF tier) | §1 L1 P-12.5, §2 L1 P-13.1 |
| E-RXP-1 | Rune XP earn sources? | Explicit list, no double-count with R-03 | §1 L1 P-12.1, P-12.2 |
| E-RXP-2 | Death/revert with XP gains? | Pending counter committed on save; revert discards | §1 L1 P-12.4 |
| S-RXP-1 | AVG AVs orphan on uninstall? | Documented (N-10); game ignores unknown AVs | §1 L2 (Uninstall) |
| M-RXP-1 | AVG hard/optional? | Optional (§0.6) | §0.6 |
| P-RXP-1 | AVG writes per XP gain? | Single call per event; batched on save; no per-frame | §0.5 |
| C-RPR-1 | Depends on RG-024b | Blocked until P-12 settled; this doc designs both together | §2 (status) |
| C-RPR-2 | Replace or supplement P-07? | Supplement: Rune Points earned only, not purchasable | §2 L1 P-13.1 |
| E-RPR-1 | Respec interaction? | Defined (P-13.7): forge service, cost, refund | §2 L1 P-13.7 |
| S-RPR-1 | Co-save section 1 extension? | Additive fields in section 1 (§0.3, §6) | §0.3, §6 |
| A-CSF-1 | CSF config schema known? | UNVERIFIED; Phase 0 spike | §3 L1 P-14.8, Open Q1 |
| A-CSF-2 | CSF persistence vs co-save? | CSF owns skill state; we store diagnostic snapshot only (D-028) | §3 L1 P-14.10 |
| C-CSF-1 | CSF XP vs skill-point pool? | CSF uses XP-based leveling; built-in 18 use skill points (P-06). Complementary. | §3 L1 P-14.1 |
| C-CSF-2 | CSF AV usage? | Prefer CSF's own AVs; verify; namespaced IDs prevent conflict | §0.2 |
| E-CSF-1 | CSF absent → graceful disable? | Optional dep; disabled + logged; built-in 18 work | §0.6, §3 L1 P-14.6 |
| E-CSF-2 | CSF state on uninstall? | Orphan risk documented (N-10); debug reset command | §3 L1 P-14.10 |
| M-CSF-1 | CSF skill ID collisions? | Namespaced `Runefall_*`; validate uniqueness at load | §3 L1 P-14.7 |
| P-CSF-1 | CSF menu per-frame? | CSF renders own UI; our calls event-only | §0.5, §3 L1 P-14.11 |
| Q-CSF-1 | RESERVE status? | Remains RESERVE (Progression pack); register update required | Scope note |

---

## 5. Compatibility notes

- **AVG (optional):** if installed, Rune XP/Level/Points mirror to AVG AVs for console,
  MGEF conditions, and external mod reads. If absent, co-save counters remain authoritative.
  No hard dependency (D-056). Soft-detect at `DataLoaded`; log result.
- **CSF (optional):** if installed, CSF skill trees are enabled. If absent, disabled.
  Built-in 18 skills unaffected. Soft-detect at `DataLoaded`; log result.
- **Other progression mods (Adamant, Experience, etc.):** we do not repurpose vanilla AVs
  (D-006), so record-level coexistence is expected. Balance stacking (double XP, double
  attribute gains) is a known interaction; test in the compatibility suite (D-057).
- **OWL / DiabloRim:** orthogonal. Rune XP grants are additive (no loot changes). If
  DiabloRim is installed, our XP-from-loot still fires on item acquisition (additive).
- **Enemy overhauls / boss mods:** Rune XP-from-kills fires on `ActorDeath` for ranked
  enemies. If another mod changes rank assignment, our XP amounts may be off; tunable via
  INI. No record edits.
- **Uninstall (N-10):** removing the DLL + ESP leaves co-save fields ignored by the game;
  items fall back to base forms; granted PERKs/SPELs disappear on load; AVG AVs persist as
  unknown AVs (AVG handles cleanup); CSF skill state persists in CSF's own save (orphan
  risk documented). No permanent record changes (D-027).

---

## 6. Co-save schema additions (v2, batched)

Reuse the v2 migration (DESIGN-ITEM-CRAFTING.md §8, DESIGN-COMBAT.md §0.3). Additive fields
only; no new bump.

### Section 1 (progression) — new fields

| Field | Type | Default | Purpose |
|---|---|---|---|
| `runeXP` | u32 | 0 | Accumulated Rune XP (committed) |
| `runeLevel` | u8 | 1 | Current Rune Level |
| `runeXPPending` | u32 | 0 | Pending XP (committed on save) — runtime, but persisted for safety |
| `runePoints` | u16 | 0 | Unspent Rune Points |
| `runePointsSpent` | u16 | 0 | Lifetime Rune Points spent |
| `runeNodes` | u16[] | empty | Purchased Rune progression node IDs |
| `runeRespecCount` | u16 | 0 | Number of Rune progression respecs |
| `runeXPLifetime` | u32 | 0 | Lifetime Rune XP earned (stats) |
| `csfInstalled` | bool | false | CSF detected at last load |
| `csfSkillSnapshot` | string | "" | Compact "id:level;..." for diagnostics |

### Section 3 (item registry) — ItemMetadata

No new ItemMetadata fields from this cluster. (Imbue/durability/LE fields are defined in
DESIGN-ITEM-CRAFTING.md and DESIGN-COMBAT.md.)

### Migration (v2 → v2)

All new fields default. Old saves load with Rune Level 1, 0 XP, 0 points, empty node list,
empty CSF snapshot. No crash. Downgrades refused with a clear message (D-052).

---

## 7. Implementation ordering (when promoted)

1. **Phase 0 spikes (before any code):**
   a. AVG: verify config path, AV name suffix, Adaptive persistence, TOML schema. Record in
      av-doc-notes.
   b. CSF: read upstream schema, verify call path from C++, verify capacity (3-5 skills),
      verify persistence model. Record in av-doc-notes.
   c. Discovery detection: find a reliable "new location discovered" event/hook.
2. **Co-save + Progression module:** add section 1 fields, pending/commit/revert logic.
3. **Rune XP/Level core:** earn events, curve, level-up, passive bonus, AVG mirror.
4. **Rune progression:** node data, purchase/respec transactions, PERK/SPEL grants.
5. **CSF adapter:** detection, XP grants, threshold grants, menu invocation.
6. **UI:** Character sheet Rune Level bar, Rune tab, CSF section.
7. **Papyrus bridge:** queries/mutations/events.
8. **Config/INI:** keys, defaults, caps.
9. **Tests:** unit + integration + save/load + migration + performance.

---

## 8. Remaining open questions (consolidated)

1. AVG config path: `Data/SKSE/Plugin/ActorValueData` vs `SKSE/Plugins/ActorValueData`
   (av-doc-notes §6). Phase 0.
2. AVG AV name suffix: does AVG append `_AVG`? Phase 0 query both.
3. AVG Adaptive persistence: verified by co-save-wins design, but native persistence would
   enable simpler mirror. Phase 0.
4. CSF config schema: upstream `docs/schema` unread. Phase 0.
5. CSF capacity: 18+ skills unverified; ship 3-5. Phase 0.
6. CSF call path from C++: SKSE VM direct vs Papyrus bridge. Phase 0.
7. CSF persistence model: own save vs JContainers. Phase 0 (D-056 forbids JContainers dep).
8. CSF node effects: PERK-based vs AV-based. Phase 0.
9. Discovery detection for XP grants. Phase 0.
10. Rune progression node count: 24 (6 per branch) for v1 of the progression pack. Tunable.
11. Unique actives: 3 (one per build branch). Revisit.
12. Respec lock after 5 respecs: revisit if too restrictive.
13. Overflow XP beyond level 50: future prestige system. Deferred.
14. CSF orphan on removal: documented limitation; debug reset command. Phase 0 verifies
    CSF reset API availability.

---

## 9. D-018 / D-006 / D-027 compliance summary

| Decision | Compliance | Evidence |
|---|---|---|
| D-006 (no vanilla AV repurposing) | Compliant | AVG AVs are new (`Runefall_*`), namespaced; core uses co-save counters; no vanilla AV writes |
| D-027 (no base-record mutation) | Compliant | AVG AVs via TOML (new); PERK/SPEL grants via AddPerk/AddSpell; no vanilla mutation |
| D-018 (9 attrs, 18 skills) | Unchanged | CSF adds skills beyond 18; built-in 18 budget and skill-point pool (P-06) unchanged |
| D-028 (co-save authoritative) | Compliant | AVG/CSF are mirrors/diagnostics; co-save is authoritative |
| D-056 (optional deps not required by Core) | Compliant | AVG and CSF both optional with graceful disable |
| E-00 (event-driven, no clock) | Compliant | All XP grants event-driven; no passive/idle generation |