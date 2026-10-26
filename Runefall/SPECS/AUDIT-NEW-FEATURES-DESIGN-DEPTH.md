# Design-depth audit — new features discussed in Runefall sessions

Status: 2026-10-27. Audit of the 13 features listed in the session prompt against the
Runefall spec stack (GAME-DESIGN.md, DECISIONS.md, L1-L4, GAP-REGISTER-REFERENCE-GAMES.md,
GAP-REGISTER-FUTURE.md, FIX-PLAN.md). Each feature is rated COMPLETE / PARTIAL / STUB
against the design-depth checklist: clear rule; default numbers (or "tunable, default X");
failure mode + player-visible consequence; engine mapping (L2); data schema (L3); code module
(L4); hook point (L4); co-save section (if stateful); UI surface; compatibility note; test
entry; open questions listed. Note: "Durability and break" was listed twice (#2 and #13) in
the session prompt; it is audited once below.

Method: read-only cross-reference of the spec documents. No code or records were changed.
Implementation state (FIX-PLAN) is cited where relevant, but ratings reflect *design depth*,
not implementation status.

---

## Summary table

| # | Feature | RG IDs | Status |
|---|---|---|---|
| 1 | Imbue (material onto equipment, break risk) | RG-055b | STUB |
| 2 | Durability and break (optional module) | RG-018 | STUB |
| 3 | Passive / skill trees via Custom Skills Framework | RG-002, RG-003 | PARTIAL |
| 4 | Ailment / damage-type / penetration framework | RG-004, RG-005, RG-006, RG-023 | STUB |
| 5 | Rune XP and Rune Level (real AVs via AVG) | RG-024b | STUB |
| 6 | Rune progression system (new axis) | RG-024c | STUB |
| 7 | Enemy variant system | RG-125b, RG-125c, RG-125d | STUB |
| 8 | Last Epoch-style random roll (destroy → create reroll) | — (new flow) | PARTIAL |
| 9 | Monster Hunter material system (per-race/NPC loot) | RG-203, RG-207, RG-215, RG-225, RG-232 (related) | PARTIAL |
| 10 | Tier upgrade with tier materials (MH-style) | RG-056 (related) | PARTIAL |
| 11 | Gambling and gacha | RG-082, RG-085 | STUB |
| 12 | Material salvage (new menu + DLL) | C-02 (core) | COMPLETE |

Counts: 7 STUB, 4 PARTIAL, 1 COMPLETE (12 unique features; durability duplicate collapsed).

---

## Per-feature audit

### 1. Imbue (RG-055b) — STUB

**One-line design in sources:** "craft a material onto equipment for better stats, chance to
break" — alternative to the impossible socket-gem/runeword system (GAP-REGISTER §3, §8.1,
§11, §13.3; L1 I-05 sockets are rune-rank only).

**What's present**
- Named dependency on existing systems: Rune Forge, salvage, material tiers (RG-055b note).
- Feasibility class B (hard but possible) per §13.3; no engine blocker identified.
- Destination pack: Crafting pack (RESERVE).
- Replacement rationale documented (socket-gem links/colors impossible in Skyrim, §8.1).

**What's missing (specific questions)**
- Rule: what exactly is an "imbue" — a material fused into an item slot, or a stat boost
  applied to an item? Is there a cap on imbues per item? Does it occupy a socket or a new slot?
- Default numbers: stat boost magnitude (flat/pct) by material tier; break chance (%); cost in
  runes/materials; number of imbues allowed per item.
- Failure mode: what "break" means — imbue lost, item destroyed, item degraded (durability hit),
  or a curse? Player-visible consequence undefined.
- Engine mapping (L2): how the boost is applied without mutating base records (D-027) —
  dynamic enchantment (D-023), extra data, or a new MGEF on the item? Must not collide with
  existing socketed-rune power (I-05) or reroll/upgrade (C-04/C-05).
- Data schema (L3): imbue recipe rows (recipes.csv outputType?), material→stat mapping table,
  FormID category for any new records (currently 0xD80-0xFFF reserved).
- Code module (L4): which module owns imbue transactions — RuneForge (likely) or a new module?
  No module named for it.
- Hook point (L4): MenuOpen (forge) presumably, but not stated; no hook assigned.
- Co-save: imbue state must live in ItemMetadata (new field) — schema bump (N-01) not assessed;
  pruning/uninstall behavior undefined.
- UI surface: Rune Forge page mentioned generically, but no action/flow designed.
- Compatibility: interaction with reroll (does imbue survive reroll?), upgrade rarity, salvage
  (does salvage recover the imbued material?).
- Test entry: none.
- Open questions: none listed (silently assumed).

**Recommended next design step:** Write an L1 subsection "Imbue (I-09)" with rule + default
numbers + failure mode, then an L2 mapping (application channel) and an L3 data schema
(imbue recipe rows + imbue-state field in ItemMetadata), before any code. Decide whether
imbue is a v1.1 Crafting-pack feature or ships in v1 (currently RESERVE).

---

### 2. Durability and break (RG-018) — STUB

**One-line design in sources:** ships as optional module via EquipmentDurabilitySystem NG
(109899) patterns; I-08 amended ("no durability" → optional module); L1 I-08 still reads
"no durability, no item degradation" with the amendment only in §8.2/§13.1 (GAP-REGISTER).

**What's present**
- Feasibility proven by EquipmentDurabilitySystem NG (§13.0, §13.1 #12).
- Decision: optional module, not core (§8.2, D-047 RESERVE).
- Framework reference documented (§13.0 table).

**What's missing (specific questions)**
- Rule: how durability degrades (per hit? per swing? % chance on damage taken?), repair cost
  and location, whether a broken item is unusable or destroyed, whether repair restores to full.
- Default numbers: durability pool per item (by rarity/material?), degradation rate, break
  threshold, repair cost (materials/runes/gold), repair success chance.
- Failure mode: player-visible consequence of break (item disabled? lost? quest item safety?).
- Engine mapping (L2): how durability state is tracked (SetItemHealthPercent is the vanilla
  tempering channel used by L2 section 3; D-023 already uses it for tempering — collision?);
  EquipmentDurabilitySystem NG patterns need reverse-engineering (unverified).
- Data schema (L3): durability columns in materials.csv/items? new data file? FormID range.
- Code module (L4): no module owns it (not in L4 §1 module table); HookManager not yet created
  (FIX-PLAN P4.1).
- Hook point (L4): which of the 11 hooks — ActorDeath? ItemAdded? MenuOpen? not stated.
- Co-save: per-item durability in ItemMetadata (new field + schema bump N-01); revert-time
  restore behavior undefined.
- UI surface: item tooltip durability bar; repair action in Rune Forge? none designed.
- Compatibility: vanilla repair, other durability mods (EquipmentDurabilitySystem NG itself),
  OWL, our dynamic name/enchant (D-023).
- Test entry: none.
- Open questions: none listed (silently assumed). Also L1 I-08 text still says "no durability"
  — internal contradiction until I-08 is formally amended in L1 (P2-style doc fix).

**Recommended next design step:** Amend L1 I-08 and add an "I-09 Durability (optional module)"
subsection with rule + defaults + failure mode; add L2 mapping (SetItemHealthPercent channel
vs EquipmentDurabilitySystem NG); add ItemMetadata.durability field to L4 §4 with a migration
note; decide FOMOD module placement (P5.2 matrix currently lacks it).

---

### 3. Passive / skill trees via Custom Skills Framework (RG-002, RG-003) — PARTIAL

**What's present**
- Core skill system is fully designed: P-04 (18 skill lines x 3 ranks, 54 nodes), skills.csv
  schema (L3 §3.6), SPEL/PERK records (L3 §1.1 ranges), Progression module (L4 §1),
  rank gates (Skyrim skill 20/50/75 + attribute 10/20/30), Papyrus bridge (GetSkillRank etc.).
- CSF feasibility proven: av-doc-notes §4 (CustomSkills.psc API v3: OpenCustomSkillMenu,
  AdvanceSkill, GetSkillLevel, etc.); §13.0 table; §13.1 #12 notes CSF config schema is
  UNVERIFIED.
- Decision: ships via CSF (§8.2, §11, §13.3); RESERVE (Progression pack).

**What's missing (specific questions)**
- Rule: what trees ship — one passive tree? per-build trees? new skill lines beyond the 18?
  How do CSF skill IDs map to Runefall's attribute/skill gates?
- Default numbers: node count, point cost per node, XP curve, whether CSF skills consume the
  existing skill-point pool or a new one, cap.
- Failure mode: what happens if CSF is absent (optional dependency) — graceful disable?
- Engine mapping (L2): CSF skills are SPEL/PERK records created how? CSF owns record creation
  (needs verification); our Progression module must grant XP via AdvanceSkill — hook point
  not assigned.
- Data schema (L3): CSF config files (skill IDs, names) not in our CSV set; no schema defined.
- Code module (L4): Progression module would own it, but no CSF adapter module; no CSF calls
  in PapyrusBridge yet.
- Hook point (L4): not assigned (likely Input/MenuOpen).
- Co-save: CSF persistence model unverified (av-doc-notes §6); interaction with our co-save
  (double source of truth?) undefined; orphaned state on CSF removal undefined (N-04).
- UI surface: CSF provides OpenCustomSkillMenu — do we rely on it or add a Runefall page?
  Not decided.
- Compatibility: CSF as optional dep; base game without CSF must work; conflict with other
  CSF-using mods.
- Test entry: none.
- Open questions: CSF config schema UNVERIFIED (av-doc-notes §6); node layout; point economy.

**Recommended next design step:** Read the CSF config schema (upstream docs/schema) and write
an L1/L2 design note for "Custom Skills integration": skill IDs, node layout, XP source,
point economy, co-save ownership, and the optional-dependency contract. Then add a CSF
adapter to the Progression module plan and a data schema (csf_skills.csv or CSF config files).

---

### 4. Ailment / damage-type / penetration framework (RG-004, RG-005, RG-006, RG-023) — STUB

**What's present**
- Feasibility proven: Elemental Reactions Framework (163378) proves the pattern (§13.0, §13.1 #13);
  buildable on MGEF + co-save.
- Explicitly "will be designed and shipped later (Combat pack)" (§8.2, §11, §13.3).
- Dependency order noted: RG-023 (damage conversion) needs RG-004 first.
- Destination: Combat pack / Affix pack (RESERVE).

**What's missing (specific questions)**
- Rule: damage-type taxonomy (physical/fire/cold/lightning/poison/shadow) and conversion
  rules (RG-023); ailment registry (bleed, burn, chill, freeze, shock, poison, stun, slow)
  with stacking caps, durations, cleanse; penetration/resistance with caps.
- Default numbers: ailment stacking caps, durations, proc chances, penetration caps,
  resistance caps.
- Failure mode: how ailments interact (freeze cancels burn?); cleanse sources; player-visible
  feedback.
- Engine mapping (L2): MGEF + co-save (Elemental Reactions Framework); new AVs via AVG? Or
  MGEF conditions? Not decided; no L2 section.
- Data schema (L3): ailment CSV, damage-type table, resistance/penetration affixes — none.
- Code module (L4): no module; would need a new "Combat" or "AilmentEngine" module.
- Hook point (L4): not assigned (hit/damage hook not in the 11-hook map — gap).
- Co-save: ailment state on actors? Or MGEF-driven self-expiring effects? Undefined.
- UI surface: none specified (Item Detail damage type?).
- Compatibility: vanilla resistances, other magic/ailment mods.
- Test entry: none.
- Open questions: only "ship later"; no listed open questions (silently assumed).

**Recommended next design step:** This is explicitly deferred. When picked up, start with an
L1 "Combat pack" section defining the damage-type taxonomy and ailment registry + stacking
rules + penetration caps, then L2 mapping (MGEF channel + whether a hit hook is needed —
note the 11-hook map has no damage/hit hook, so that gap must be flagged), then L3 schemas.
Do not let it silently assume the Elemental Reactions Framework's exact API.

---

### 5. Rune XP and Rune Level (RG-024b) — STUB

**What's present**
- Feasibility proven by AVG (§13.0, §13.1 #1): creates new AVs usable in MGEF/conditions/scripts;
  ships ActorValueExtension.dll + TOML data.
- av-doc-notes §4 documents AVG config (TOML, suffix _AVG, Adaptive vs Functional types,
  formula vocabulary) and flags UNVERIFIED items (path, Functional get/set pair).
- Destination: Progression pack (RESERVE).

**What's missing (specific questions)**
- Rule: what Rune XP is (earned from what — kills? loot? crafting?), what Rune Level gates
  or grants.
- Default numbers: XP per source, XP curve, max Rune Level, points granted per level.
- Failure mode: what happens on death/revert with XP gains; save/load consistency.
- Engine mapping (L2): AVG Adaptive vs Functional AV choice (Adaptive is per-actor with
  persistence; Functional is formula) — not decided; AVG config path UNVERIFIED.
- Data schema (L3): rune_xp.csv / rune_level table — none.
- Code module (L4): Progression module would own it, but no AVG integration code.
- Hook point (L4): not assigned.
- Co-save: if AVG AVs persist via the game's own AV system, do we also mirror in co-save
  for safety/uninstall? Undefined (D-006 says no vanilla AV repurposing, but AVG AVs are
  custom — ownership model unverified).
- UI surface: character sheet Rune XP/Level display — not designed.
- Compatibility: AVG as hard dependency? Or optional? Not decided.
- Test entry: none.
- Open questions: AVG path UNVERIFIED; Functional get/set pair; persistence on uninstall.

**Recommended next design step:** Decide AVG dependency tier (hard vs optional) and AV type
(Adaptive), verify the config path on the target MO2 list, then write an L1 "Rune XP/Level"
subsection with earn rules + curve + grants, and an L2 mapping. This blocks feature #6.

---

### 6. Rune progression system (RG-024c) — STUB

**What's present**
- Named as a new progression axis beyond character level and RF tier (GAP-REGISTER §2).
- Builds on RG-024b (Rune XP/Level AVs).
- Destination: Progression pack (RESERVE).

**What's missing (specific questions)**
- Rule: what advancing Rune Level grants (perks, points, unlocks, stat bonuses) and how it
  differs from character level (P-02) and RF tier (T-06).
- Default numbers: grants per Rune Level, caps, cost.
- Failure mode: respec interaction; what happens if Rune Level is reset.
- Engine mapping (L2): how grants are applied (PERK/ability grants via AddPerk, native
  hotkey layer for actives) — not specified.
- Data schema (L3): rune_progression.csv — none.
- Code module (L4): Progression module.
- Hook point (L4): not assigned.
- Co-save: progression state section (new or extend section 1) — undefined.
- UI surface: character sheet — not designed.
- Compatibility: interaction with the 18 built-in skill lines and attribute gates.
- Test entry: none.
- Open questions: entirely unspecified; depends on #5.

**Recommended next design step:** Cannot be designed until #5 (Rune XP/Level) is settled.
Then define the grant table and respec rules, and decide whether this replaces or supplements
the rune-bought points (P-07).

---

### 7. Enemy variant system (RG-125b, RG-125c, RG-125d) — STUB

**What's present**
- One-line rule: modify existing NPCs into N variants with new perk/skill packages for harder
  fights and better rewards; no new NPCs or world changes (§5, constraint block).
- Pattern reference: Fortunate Foes (SPID/SkyPatcher runtime perk/spell package application).
- Dynamic Perk Framework (189660) proves runtime perk granting (§13.0).
- Destination: Enemy pack (RESERVE); reward scaling (RG-125c) ties to loot/rune/materials;
  naming (RG-125d) = prefix/suffix + optional badge.

**What's missing (specific questions)**
- Rule: how many variants per base NPC; deterministic roll at spawn vs authored list; whether
  variants stack with monster ranks (T-01) or replace them.
- Default numbers: variant count, difficulty multiplier per variant, reward multiplier
  (RG-125c), density caps.
- Failure mode: double application / stacking with other enemy overhauls; idempotency (D-025).
- Engine mapping (L2): runtime perk/spell package application at spawn — Fortunate Foes uses
  SPID/SkyPatcher, but Runefall uses native (D-005: no SkyPatcher dependency for core).
  Dynamic Perk Framework is an optional dep? Or our own PERK records applied via AddPerk?
  Not decided.
- Data schema (L3): variant definition table (base NPC EditorID → variant packages) — none;
  FormID range for variant markers (keywords).
- Code module (L4): TierEngine (likely) but no variant logic; ActorSpawn/LoadedCellAttach hooks
  exist (L4 §2) but variant application not specified.
- Hook point (L4): ActorSpawn / LoadedCellAttach (assigned by analogy, not explicitly).
- Co-save: variant assignment must be re-derived at spawn (D-025: no permanent perk grants
  to generic NPCs) — confirm and document; no co-save state needed if re-derived.
- UI surface: name prefix/suffix (RG-125d) — how applied (SetDisplayName? name badge? L2 §4
  notes per-instance actor names are not a reliable engine feature → UI badge fallback).
- Compatibility: enemy overhauls, boss mods, density caps, no NPC record edits (D-025).
- Test entry: none.
- Open questions: none listed (silently assumed).

**Recommended next design step:** Write an L1/L2 "Enemy variants" design: variant generation
rule, count, multiplier table, perk-package assignment, naming, and the re-derive-on-load
policy (D-025). Add a variant table schema to L3 and assign ownership to TierEngine in L4.
Decide whether Dynamic Perk Framework is a hard/optional dependency or whether our own PERK
records + AddPerk suffice.

---

### 8. Last Epoch-style random roll (destroy item → create new item with reroll) — PARTIAL

**What's present**
- Core reroll is designed: C-04 (reroll all enhancements, keeps rarity + socket count; locked
  enhancement option costs 3x), RuneForge module (L4 §1), RerollItem Papyrus bridge (L4 §6),
  Rune Forge UI page (L4 §7), roll pipeline L-03 (L1/L2).
- Dynamic item mutation channel documented (D-023: CreateEnchantment/SetEnchantment,
  SetDisplayName, SetItemHealthPercent + co-save registry).
- ItemRegistry interface defined (L4 §4) with pruning rule ("pruned only when items are
  destroyed").

**What's missing (specific questions)**
- Rule: when is the "destroy → create new" flow used vs the in-place reroll (C-04)? Is this
  just a UI flow over RerollItem, or a new transaction that discards the old item instance
  and mints a new one?
- Default numbers: cost difference (if any) from C-04; whether the new item's ilvl/rarity
  can change.
- Failure mode: if creation fails mid-flow, is the original item lost? (transaction safety.)
- Engine mapping (L2): re-applying dynamic enchantment/name/temper on the new instance;
  registry ID handling (prune old, allocate new) — L4 §4 says pruned on destroy, but the
  create-new path's registry allocation is not specified.
- Data schema (L3): none new (reuses affixes.csv/rarity.csv).
- Code module (L4): RuneForge + ItemRegistry; transaction must be atomic.
- Hook point (L4): MenuOpen (forge) — implied.
- Co-save: registry pruning + new entry; size impact (N-01 migration).
- UI surface: Rune Forge page action + confirmation dialog — not designed.
- Compatibility: item identity changes — does that break quest tracking / equipped-item
  references? (Important: destroying an equipped item.)
- Test entry: none.
- Open questions: none listed.

**Recommended next design step:** Decide whether this is a UI flow over the existing RerollItem
transaction or a new "destroy+mint" transaction. If new, specify atomicity, registry ID
handling, and equipped-item safety in L4; add a confirmation step to the Rune Forge page
spec. Likely just a UX variant of C-04 — confirm and document rather than inventing a
second transaction.

---

### 9. Monster Hunter material system (hunting + per-race/NPC loot tables for unique materials) — PARTIAL

**What's present**
- Race-hunt material system is fully designed: L1 L-11 (19 families, unique affix first-kill +
  repeatable hunt material, chance scales with RF tier); G12 (race_groups.csv,
  drops_races.csv); L2 §3 (race→family lookup, firstOnly flags in co-save); L3 §3.4.1
  (38 rows, 19 materials); L4 HuntEngine module; co-save section 4 (hunt state); Hunt Codex
  UI page (L4 §7); ActorDeath hook (L4 §2); validation gates (L3 §6).
- Base materials designed: C-01 (4 tiers x 3 families), materials.csv, RF_MISC_Mat_* records.
- Material hunting nodes: C-07 (node yields by area tier).

**What's missing (specific questions)**
- Rule: the "MH material system" as a broader feature — per-individual-NPC material tables
  (vs per-race families), carve vs capture mechanics, whether specific monsters yield unique
  materials beyond the 19 race families.
- Default numbers: drop chances for per-NPC tables; tier scaling; how many materials per
  monster.
- Failure mode: duplicate/overlapping materials across races; dead materials (L3 gate #7
  requires every hunt material has a use).
- Engine mapping (L2): ActorDeath hook already rolls race rewards; per-NPC override would
  need a lookup by base NPC EditorID — not designed.
- Data schema (L3): per-NPC material table (drops_npcs.csv?) — none; race_groups.csv covers
  families only.
- Code module (L4): HuntEngine (likely) but per-NPC logic not specified.
- Hook point (L4): ActorDeath (implied).
- Co-save: material counts tracked? MaterialRegistry does not exist yet (FIX-PLAN P4.8:
  materials loaded but not consumed; no material tracking in co-save). This is a real gap
  even for the designed race-hunt materials.
- UI surface: Hunt Codex shows material counts (designed) — but material inventory display
  not specified.
- Compatibility: other loot mods; additive loot mode (D-008).
- Test entry: none (FIX-PLAN P5.5 blocks T1).
- Open questions: scope (per-NPC tables are a large data effort); whether MH-style carve/
  capture adds gameplay value in Skyrim.

**Recommended next design step:** Decide whether "MH material system" means (a) just the
already-designed race-hunt materials (then it's COMPLETE and only needs implementation per
FIX-PLAN P4.8/P4.9), or (b) a broader per-NPC material system. If (b), write a design note
with a per-NPC table schema and a scope cap (e.g., only named/boss NPCs) before data work.
Also close the MaterialRegistry gap (FIX-PLAN P4.8) which blocks both readings.

---

### 10. Tier upgrade with tier materials (MH-style) — PARTIAL

**What's present**
- Rarity upgrade is designed: C-05 (costs one rarity step in materials + 60% success chance;
  on failure materials consumed, item unchanged; Radiant→Mythic needs R3+ rune + boss material;
  Mythic→Runeforged needs R4+ rune + two boss materials).
- RuneForge.UpgradeItem Papyrus bridge exists (L4 §6).
- recipes.csv supports upgrade output types.
- Materials + runes are the currency (C-01, R-02).

**What's missing (specific questions)**
- Rule: is "tier upgrade" the same as C-05 rarity upgrade, or a separate MH-style item
  quality/level upgrade (e.g., upgrading an item's RF-tier band or a quality level 1-20)?
  The prompt says "MH-style tier upgrade via DLL" — ambiguous vs C-05.
- Default numbers: if a new axis, upgrade cost curve, success chance, max tier, stat
  magnitude scaling.
- Failure mode: if distinct from C-05, what happens on failure (downgrade? break? materials
  lost?).
- Engine mapping (L2): if it changes ilvl/affix magnitudes, does it re-roll magnitudes or
  scale them? Affects ItemRegistry metadata.
- Data schema (L3): upgrade recipe rows (recipes.csv) or a new table — none for a new axis.
- Code module (L4): RuneForge (likely) but UpgradeItem is already wired to C-05.
- Hook point (L4): MenuOpen (forge) — implied.
- Co-save: item metadata (rarity/ilvl) update — handled by ItemRegistry if in-place.
- UI surface: Rune Forge page upgrade action — implied, not specified for a new axis.
- Compatibility: interaction with imbue, reroll, sockets.
- Test entry: none.
- Open questions: none listed.

**Recommended next design step:** Clarify whether this feature is just C-05 (already designed,
needs implementation) or a new quality/ilvl upgrade axis. If new, write an L1 subsection
with rule + defaults + failure mode and an L3 schema; if it's C-05, remove it from the
"new features" list and track it as implementation work (FIX-PLAN P4.x). Do not let
"MH-style" imply a second upgrade system without a decision.

---

### 11. Gambling and gacha — STUB

**What's present**
- F-17 (GAP-REGISTER-FUTURE): "rune gambling vendor, trade, transmutation" reserved for
  Economy pack; RG-082 (gambling: spend currency for random item of a type), RG-085 (rune
  gambling vendor).
- Framework reference: SKSE Menu Framework proves custom menus (§13.0).
- Destination: Economy pack (RESERVE).

**What's missing (specific questions)**
- Rule: what is gambled (random item of a chosen type? rune packs? materials? gacha banners
  with rates?), cost, and what the player receives.
- Default numbers: cost per roll, drop rates by rarity, pity threshold, pull counts.
- Failure mode: anti-abuse (save-scumming around gambling rolls — L-08 anti-abuse pattern),
  economy inflation (E-05).
- Engine mapping (L2): LootEngine roll + dynamic item creation (D-023) + a new UI page;
  no vendor record edits (D-003).
- Data schema (L3): gambling table (costs, pools, rates) — none.
- Code module (L4): no module; would need a GamblingModule or extension of LootEngine +
  MenuApi.
- Hook point (L4): MenuOpen/MenuRender (implied).
- Co-save: pity counters, pull history, daily/weekly limits? (E-00: no clock-based pacing —
  so no daily limits; pity is event-count based.)
- UI surface: custom menu page (SKSE Menu Framework) — not designed.
- Compatibility: economy balance with other mods; whether gambling is toggleable (INI).
- Test entry: none.
- Open questions: scope and ethics (gacha in a single-player mod — is it cosmetic-only or
  gameplay-affecting?); whether it ships at all (still RESERVE).

**Recommended next design step:** Keep as RESERVE. Before any design, decide scope and
whether gacha mechanics (rates/pity) belong in a single-player mod at all. If yes, write an
L1 "Gambling" subsection (rule + cost + rates + pity + anti-abuse) reusing L-06 pity
pattern, then L3 gambling.csv, then a UI page spec. E-00 (no clock) constrains pity to
event counts.

---

### 12. Material salvage via new menu + DLL — COMPLETE

**What's present**
- Rule: C-02 (salvage any Runefall item at a forge; yield 1-3 material units of the item's
  tier + rune value 10-25% of item's rune value; destroys item, no undo); C-09 defers
  Affix Shard extraction to expansion.
- Default numbers: yield 1-3 units, rune return 10-25%, tier from item.
- Failure mode: item destroyed, no undo (stated); validation in DLL (L2 §5).
- Engine mapping (L2 §5): native API invoked from Papyrus forge activator script; DLL
  validates transaction (item metadata, rune count, materials); salvage/craft transactions
  owned by RuneForge.
- Data schema (L3): materials.csv (materialId, tier, family, salvageYieldMin/Max, source),
  recipes.csv (v1 outputType includes utility).
- Code module (L4): RuneForge (salvage, reroll, upgrade, socket/extract, rune conversion).
- Hook point (L4): MenuOpen (forge/workbench open) 0.2 ms; ItemAdded (metadata finalize).
- Co-save: item registry pruned on destroy (L4 §4); material counts need MaterialRegistry
  (FIX-PLAN P4.8 — loaded but not consumed; gap in implementation, not design).
- UI surface: Rune Forge page includes salvage list (L4 §7 #2).
- Papyrus bridge: SalvageItem(Form, ...) mutation function (L4 §6).
- Compatibility: additive (no vanilla record edits); salvage returns less than item value
  (E-05).
- Open questions: whether salvage needs a dedicated page vs being part of Rune Forge (current
  design: part of Rune Forge). Note: "new menu" framing is already satisfied by the Rune
  Forge page.

**What's missing**
- Test entry: none explicitly (FIX-PLAN P5.5/P5.6 block T1).
- Implementation gap: MaterialRegistry not yet created (FIX-PLAN P4.8) — materials loaded
  but not tracked/consumed. This is implementation, not design, but it blocks the salvage
  yield from being stored.

**Recommended next design step:** No new design needed. Track as implementation work:
create MaterialRegistry (FIX-PLAN P4.8), wire SalvageItem transaction, add a smoke test
entry for salvage round-trip (destroy item → materials + runes granted → co-save persists).
If a dedicated salvage-only page is wanted, specify it as a Rune Forge sub-view, not a new
system.

---

## Summary

**Counts (12 unique features):** 1 COMPLETE, 4 PARTIAL, 7 STUB.

- COMPLETE: Material salvage (already core; only implementation + test entry remain).
- PARTIAL: Passive/skill trees (core skills designed; CSF integration not), LE-style random
  roll (roll pipeline + reroll exist; destroy→create flow not specified), MH material system
  (race-hunt materials designed; per-NPC + MH mechanics not), Tier upgrade (rarity upgrade
  C-05 exists; MH-style new axis ambiguous).
- STUB: Imbue, Durability and break, Ailment/damage-type framework, Rune XP and Rune Level,
  Rune progression system, Enemy variant system, Gambling and gacha.

**Top unanswered questions that block implementation** (ordered by dependency impact):

1. **Imbue vs. sockets/runes (RG-055b):** Is imbue a new item-state field, and how is the
   boost applied without mutating base records (D-027) or colliding with socketed-rune power
   (I-05)? No application channel is chosen; no ItemMetadata field defined.
2. **Durability application channel (RG-018):** EquipmentDurabilitySystem NG patterns are
   unverified against our use of SetItemHealthPercent for tempering (D-023) and dynamic
   enchantment/name. Need a Phase-0-style proof before design can be finalized; L1 I-08 still
   says "no durability" and must be formally amended.
3. **Ailment hit hook (RG-004/005/006/023):** The 11-hook map (L4 §2) has no damage/hit hook.
   Either the ailment framework must be MGEF-self-expiring (no hook needed) or the hook map
   must be extended. This is an architectural decision that affects L4 §2 and D-050.
4. **AVG dependency and path (RG-024b):** AVG config path is UNVERIFIED (av-doc-notes §6);
   Adaptive vs Functional AV type undecided; hard vs optional dependency undecided. Blocks
   Rune XP/Level and the Rune progression axis.
5. **CSF config schema (RG-002/003):** UNVERIFIED (av-doc-notes §6); co-save ownership
   (CSF's own vs ours) undefined; optional-dependency contract undefined. Blocks passive/skill
   tree design.
6. **Enemy variant perk application (RG-125b):** Runefall core must not depend on SkyPatcher
   (D-005); Dynamic Perk Framework is an optional dep. Whether our own PERK records + AddPerk
   suffice (and re-derive-on-load per D-025) is undecided.
7. **Tier upgrade vs. rarity upgrade (RG-056/C-05):** Ambiguous whether "MH-style tier upgrade"
   is already C-05 or a new quality/ilvl axis. Must be resolved before any data work to avoid
   a second upgrade system.
8. **LE random roll scope:** Unclear whether "destroy → create new" is a new transaction or a
   UI flow over the existing RerollItem. Affects ItemRegistry ID handling and equipped-item
   safety.
9. **MH material system scope:** Unclear whether it is the already-designed race-hunt materials
   (then COMPLETE, pending implementation) or a broader per-NPC system. Plus MaterialRegistry
   (FIX-PLAN P4.8) does not exist yet, blocking material tracking for any reading.
10. **Gambling/gacha scope and ethics:** Whether gacha (rates/pity) belongs in a single-player
    mod at all is undecided; E-00 (no clock-based pacing) constrains pity to event counts.

**Cross-cutting note:** All 7 STUB features are currently RESERVE (GAP-REGISTER decision codes)
and are not expected in v1 code. The audit's purpose is to surface that none of them have
even a first-pass L1 rule/numbers/failure-mode, so that when they are promoted (per D-047,
which requires a register update first), the design work is not starting from a one-line
description. Recommended sequencing: close the v1 implementation gaps in FIX-PLAN (P1 safety,
P3.1/P3.2 FormID+validator, P4.x modules) before investing in any new-feature L1 design;
then design promoted features in L1→L2→L3→L4 order per D-013.

RUNTIME STATUS: assistant-claimed (design audit, no runtime verification performed)
SSEEDIT/CK: not launched
UNRESOLVED: none beyond the open questions listed per feature above
