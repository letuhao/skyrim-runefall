# Runefall - game design and technical context

Version: 0.1.0 (design baseline)
Date: 2026-10-26
Status: context and design only; no implementation yet
Project: D:\Works\source\skyrim-moddings\Runefall
Related evidence: D:\Works\source\skyrim-moddings\_context\rpg-mechanism\ (architecture
analysis of DiabloRim, Grid Inventory, Summermyst Option, Worlds Dawn, Fortunate Foes)
Layered specifications: SPECS\00-SPEC-PROCESS.md. L1 (SPECS\L1-GAMEPLAY-MODEL.md)
is the authoritative gameplay model; where its numbers differ from sections 3-7 below, L1 wins.

## 1. Identity and targets

- Working name: Runefall. Public product name decision is open: "Runefall" is the exact
  name of a 2018-2019 casual game series (Playcademy / GC Games) and there is an
  "Angkor: Runefall" too. There is no Skyrim-side collision (no mod, plugin or record with
  that name in either local MO2 list). Unclaimed alternatives: Runegrasp, Runemark,
  Runesurge. The project folder keeps the name Runefall regardless.
- Product: a free public Skyrim SE/AE mod, four pillars, native SKSE DLL plus ESL content,
  optional UI and compatibility modules. One Nexus page, FOMOD modules.
- Release target: **Skyrim SE/AE 1.6.1170**, the MO2 list "Mantras of Mara" (3,711 mods enabled,
  3,753 plugins), built with CommonLibSSE-NG as a multi-runtime DLL. Skyrim 1.7.104 is deferred:
  SKSE 2.3.1 and Address Library v13 are gated and the plugin ecosystem is not ready (D-062).
- Non-goals: total conversion, new worldspaces, vanilla record overrides, 1.5.97 support in
  v1, multiplayer or console support, a Diablo-style replacement inventory grid (that is a
  separate optional module decision).

## 2. The four pillars and the core loop

     Enemy tiers (rune ranks)  ->  Loot (rarity + affixes)
            ^                              |
            |                       salvage / dismantle
            |                              v
     Progression (attributes)  <-  Crafting and materials (rune economy)

Kill a rune-tier enemy -> get rarity/affix loot -> salvage duplicates into tiered
materials -> craft, socket or reroll the item you need -> attributes and runes make higher
tiers survivable -> elites, mini-bosses and bosses drop the rare materials and rune caches.

Every pillar is optional and toggleable through INI configuration. None of them edits
vanilla records.

## 3. The Rune system (core economy)

One shared resource ties all four pillars together: the Rune Rank, plus rune currency.

> **Superseded 2026-10-28 (D-065/D-068):** the RF Tier 0-5 / five-rank model below is the POC
> model. The live world/balance ruler is the 100-stage, 10-tier ladder in
> SPECS/POWER-LADDER.md; the player Rune Rank is a 1-100 input source whose economy is deferred to
> PLAYER-PROGRESSION. Do not build new content on the 0-5 ranks.

- Rune Rank (1-100) exists conceptually on the ladder, not as a new actor value.
- Enemy rank/elite/boss overlays are consumer modules on top of the stage curve.
- Player progression: rune essence (currency) accumulates from kills, salvage, container
  caches and boss drops; it is spent on attribute points, rerolls, socketing and crafting.
- Equipment: Runefall items carry rarity and up to three rune sockets; socketed runes add
  affixes/ranks. Item power = base item + rarity band + socket ranks.
- "Everyone gets buffed by the rune": enemy rank grants its stat/ability package; equipment
  sockets grant item power; player rank scales attribute effects. All three are applied by
  the DLL at runtime from our own records, with caps and exclusive keywords. This is the
  intended design, and it is also why conflicts stay low: nothing is a vanilla record edit.
- Sources: elite/champion/boss kills, salvage, rare world caches. Sinks: attribute points,
  rerolls, socketing, crafting recipes, respec.
- Currency representation: a weightless stackable MISC item ("Rune") so it is visible and
  safe to remove, plus a native counter cache for performance and display. Pure native
  counter only (no item) is a possible alternative; item-first is the recommended default
  because it survives uninstall cleanly.
- Anti-exploit and pacing: **event-driven, never clock-driven** (see L1 section 9). All
  income and costs are expressed per combat encounter, per loot event, per discovery and per
  boss kill - never per minute or per hour. No idle/passive generation, no daily rewards.
  Diminishing returns per level band; salvage destroys the item and returns a fraction of its
  rune value; per-container caps prevent reload farming.

## 4. Pillar 1 - Progression (attributes and skills)

- **Nine attributes, three per build** (Warrior: Might, Endurance, Precision; Thief:
  Agility, Guile, Cunning; Mage: Intellect, Wisdom, Willpower). Each attribute gates two
  skills and grants a defined package of derived bonuses (melee/ranged/spell power, health,
  armor, crit, evasion, magic find, rune find, poison, vendor value, cooldown reduction).
- **Eighteen skills, six per build, three ranks each** (54 nodes). Nine active abilities and
  nine passive lines; every line maps to one Skyrim skill and one paired attribute as gates
  (rank gates at Skyrim skill 20/50/75 and attribute 10/20/30). No new skill entries and no
  class lock - builds emerge from gating and synergies.
- **Every attribute has at least one non-combat anchor** (crafting, economy, exploration,
  stealth, social or magic utility) so the system is not nine flavors of combat damage. The
  anchors were audited against the 149 vanilla actor value records in Skyrim.esm - see
  `SPECS\L2A-ACTOR-VALUE-COVERAGE.md` and L1 P-11.
- Point sources: **two attribute points and one skill point per character level**, plus rune
  spend. Separate respec costs. Soft cap 30 / hard cap 60 per attribute, configurable in INI.
  No vanilla actor-value repurposing (the Worlds Dawn approach is explicitly rejected; it
  collides with other RPG mods and is not reversible).
- Effect implementation: attribute values live in the native co-save and in our own GLOBAL
  records for condition-driven effects. Derived bonuses are applied through our own
  PERK/MGEF/SPEL records granted at runtime, or re-applied on load. No permanent base
  actor-value edits, so uninstall and cross-mod coexistence stay clean.
- UI: one allocation panel. Framework choice is an open decision (SKSE Menu Framework vs
  Prisma UI vs MCM Helper only).

## 5. Pillar 2 - Loot

- Rarity bands (six, 0-5): Common, Refined, Exquisite, Radiant, Mythic, Runeforged - with
  distinct name prefixes/suffixes, color treatment and weight tables.
- **Rarity index = enhancement count** (0-5). There are five enhancement categories
  (Offense/Defense/Utility/Economy/Elemental), so an item can hold at most one enhancement
  per category and rarity 5 is the hard maximum. Rarity 5 optionally carries one unique
  "runeforged" effect that does not count as an enhancement.
- Enhancement (affix) system driven by CSV tables shipped as loose data files (no rebuild
  needed to add content). Proposed schema, deliberately close to the DiabloRim grammar so
  community tables can be ported:
      affixId,name,category,tier,points,minValue,maxValue,unit,duration,area,slots,weight,minItemLevel,mgef,npcExclude,prefix,suffix
- Roll timing: native hook at loot generation (container/corpse/vendor/quest reward), before
  the player sees the item. **Generation is dynamic per instance** - dynamic enchantment, name
  and tempering on the item instance plus co-save metadata; there are no variant records and the
  drop tables are CSV/JSON data (millions of combinations cannot be forms).
- Affix count and magnitude scale with item level, rarity band and magic find (a player
  attribute/stat).
- Loot authority modes, configurable: additive (default; Runefall items are added on top of
  vanilla/OWL loot), hybrid, takeover (optional module for full loot-game feel).
- No leveled-list edits: runes and Runefall items are injected/rolled at runtime, so OWL,
  Leveled List Object Swapper and similar mods keep working.
- Salvage value and materials derive from rarity, item level and affix count.

## 6. Pillar 3 - Enemy tiers

- Ranks: Elite (1), Champion (2), Mini-boss (3), Boss (4), Mythic (5).
- Assignment: native runtime on actor spawn, deterministic per spawn with configurable
  density and per-area caps. No NPC record edits, so enemy overhauls and boss mods keep
  their records; stacking is controlled by caps and exclusive keywords.
- Buff package per rank: health/damage/resistance multipliers, 0-3 perks, 0-2 abilities,
  optional aura VFX and name prefix. All from our own records.
- Loot and materials scale with rank (see pillars 2 and 4).
- Boss and mini-boss handling: rank assignment plus our own bestiary/boss templates and
  drop tables; density separated from the existing boss mods in the target list and
  configurable.

## 7. Pillar 4 - Crafting and material hunting

- Material tiers: Common, Refined, Exotic, Mythic. Sources: salvage of Runefall items,
  elite/champion drops, boss drops, tiered ore/plant nodes and rare caches.
- Salvage (dismantle): any Runefall item can be dismantled at a forge/workbench for
  materials proportional to rarity, item level and affix count, with a chance to extract an
  affix shard. The native DLL validates and performs the transaction.
- Crafting: vanilla COBJ categories extended with our own recipes using our materials;
  targeted crafting lets the player choose a base item and one affix family while rarity
  remains rolled; socketing and rerolling consume runes/materials.
- Material hunting: tiered node yields and enemy-type/region material specialisation so
  hunting is intentional rather than passive.
- UI: forge activator and message flow in v1; a dedicated crafting/salvage panel later.

## 8. Technical architecture

### 8.1 Components

- Runefall.dll - native core, CommonLibSSE-NG, C++20/23, CMake/vcpkg, VS2022 (MSVC 14.44).
  Owns: config loading, content registries (affixes, tiers, materials, recipes), item
  metadata registry, loot hook, tier engine, salvage/crafting API, Papyrus bridge, co-save.
- Runefall.esp - ESL-flagged content plugin, new records only: keywords, MISC (rune,
  materials), MGEF, SPEL, PERK, ENCH, COBJ, quest stubs for the Papyrus bridge. No vanilla
  record overrides, no leveled-list edits, no world edits.
- Optional modules: Runefall - UI (SKSE Menu Framework or Prisma UI app), Runefall -
  Patches (SkyPatcher/INI compatibility patches only), Runefall - Takeover Loot (optional
  loot authority module).
- Data files: Runefall.ini (configuration), Data\Runefall\affixes\*.csv,
  Data\Runefall\tiers.json, Data\Runefall\materials.json (content, hot-reloadable).
- Papyrus scripts only for UI glue and optional integrations; core systems work with no
  scripts.

### 8.2 Draft hook surface (to be proven in Phase 0)

- Container open / loot resolution: roll affixes on generated items.
- Actor spawn / placement: assign rune rank and buff package.
- Item added: rune currency aggregation and salvage validation hooks.
- Save load / revert: rebuild runtime state from co-save and reapply derived effects.
- Menu open: optional salvage/crafting UI entry point.
- Exact engine entry points are a Phase 0 research item; DiabloRim's observed behavior
  (container, vendor and quest-reward flows, pricing hook) is the local reference.

### 8.3 Save format and uninstall

- Co-save with a magic header and schema version; every platform-version change is a
  versioned addition, never a reshuffle.
- Per-item metadata (rarity, affix list, item level, sockets) plus actor rank map, rune
  count and config snapshot.
- Clean uninstall promise: with the DLL removed, items fall back to their base forms and
  runtime buffs disappear on load; nothing permanent was written into vanilla records.
- Known research risk: per-item metadata identity and persistence (extra data vs mapping)
  must be proven in the Phase 0 spike before the loot pillar is designed in detail.

## 9. Shipping strategy - DLL, SkyPatcher or ESL (direct answer)

Short answer: ship new content as an **ESL plugin**, behavior as the **DLL**, and use
SkyPatcher only for optional compatibility patches. "Ship everything by DLL or SkyPatcher"
is not the maximum-compatibility route, and here is why:

- SkyPatcher cannot create new forms. It patches existing records at runtime from INI
  rules. The rune currency item, keywords, materials, affixes and recipes do not exist in
  the game, so they cannot be created by SkyPatcher.
- Runtime-created forms in a DLL are not a substitute: forms created at runtime are not
  persisted in the save and are unsafe for items the player owns.
- An ESL plugin with only our own new FormIDs is conflict-free by construction: it overrides
  nothing, edits no leveled lists, no cells. This is the lowest-conflict way to add forms.
- The DLL then applies everything at runtime: no vanilla record edits at all, which is what
  "maximum compatibility" actually requires.
- SkyPatcher in the core would make SkyPatcher a hard dependency and each patch operation
  version-specific - fewer compatible users, not more. It stays an optional patch layer
  (for example, adjusting vanilla records the user explicitly asks to tune) and core
  features must never depend on it.

Dependency tiers:

- Hard: SKSE + Address Library (implicit for any DLL mod).
- Optional UI: SKSE Menu Framework or Prisma UI (decision open); MCM Helper only as a
  fallback if neither is chosen.
- Optional compatibility: SkyPatcher, and small INI patches.
- Not used: PapyrusUtil/JContainers (unless the chosen UI needs them), SPID, KID, other
  distributor frameworks - the native core performs distribution itself.

## 10. Conflict and compatibility strategy

Rules that make the compatibility claim true:

1. New FormIDs only; no vanilla record overrides in the core plugin.
2. No leveled-list, container or cell edits; runtime application only.
3. All actor buffs temporary/derived and re-applied on load; nothing permanent baked into
   saves.
4. Exclusive keywords guarantee one rune rank per actor and no double application.
5. Additive loot default; takeover is a separately installed module.
6. Global caps and INI multipliers for every stat so stacking with other mods stays sane.
7. Every system independently disableable; clean uninstall path documented and tested.
8. Soft-detect other loot/leveling mods; ship explicit compatibility notes and, where
   needed, optional INI patches - never silent overrides.

Known interaction surfaces to test (from the target list):

- OWL + OWL Randomized Special Loot: additive loot mode, no leveled-list edits.
- Existing enemy overhauls and boss mods: rank stacking caps, density config.
- Enchanting/alchemy/armor overhauls: percentage caps, additive bonuses only.
- Economy and gold mods: runes are a separate currency, no gold replacement in v1.
- Progression mods (Adamant, Experience, Custom Skills Framework in the lists): we do not
  repurpose actor values, so record-level coexistence is expected; test balance stacking.
- DiabloRim / Worlds Dawn / Fortunate Foes: no dependency and no file overlap; if DiabloRim
  is installed, an optional patch disables its loot distribution (two loot engines at once
  is a balance conflict, not a file conflict).
- DynDOLOD/Occlusion and world mods: no world edits, so no interaction.

## 11. Initial balance parameters (tunable in INI)

- Rank multipliers (HP / damage): Elite 1.5 / 1.2, Champion 2.2 / 1.4, Mini-boss 3.5 / 1.7,
  Boss 6.0 / 2.0, Mythic 10.0 / 2.5.
- Rune drops: Champion 1-3, Mini-boss 5-10, Boss 15-25, Mythic 40-60; salvage returns a
  fraction of item value.
- Rarity odds at RF Tier 2: Common 55, Refined 25, Exquisite 11, Radiant 6, Mythic 2.5,
  Runeforged 0.5 percent (adjust by magic find; see L1 L-05).
- Enhancements per item = rarity index (0-5); magnitudes scale with item level and rarity.
- Attribute packages and caps: to be defined in the design pass with a spreadsheet.

## 12. Validation summary (full plan in VALIDATION.md)

- Implement-first strategy (D-063): static gates plus an automated in-DLL probe harness during
  development; no per-feature manual game tests.
- Build gates: DLL compiles; ESL records validate; Papyrus compiles; data/FormID cross-checks.
- Batch test T1: one game launch on J:\SkyrimTest\1.6.1170 runs the full probe suite and a short
  scripted walkthrough (loot roll, tier spawn, hunt first-kill, save/load/revert).
- Compatibility suite on a copied Mantras profile and a minimal profile is part of T1/T2.
- Performance: probe records loot/spawn hook timing; no per-frame work.
- 1.7.x is not a v1 target (D-062).

## 13. Open decisions

1. Public product name (Runefall vs Runegrasp/Runemark/Runesurge).
2. UI framework: SKSE Menu Framework vs Prisma UI vs MCM Helper only.
3. Closed: release target is 1.6.1170; 1.7.104 support deferred (D-062). No Steam install changes.
4. Attribute names, count and derived-stat formulas.
5. Default loot authority (additive vs takeover module) and whether takeover ships in v1.
6. Currency representation default (MISC item + native cache vs pure native counter).

## 14. Evidence references

- Reference mod architecture: _context\rpg-mechanism\00-INDEX.md, 01-09.
- Workspace rules and verified environment: AGENTS.md sections 4, 5, 5.1.
- CommonLibSSE-NG: alandtse fork v14.0.1 (2026-10-25); the older CharmedBaryon repository is not
  the build target.
- SKSE/Address Library for the supported target: SKSE 1.6.1170, AL bins versionlib-1-6-1170-0.bin
  and -0-1 (installed in Mantras and in the Phase 0 test env). 1.7.104 requires SKSE 2.3.1 + AL v13
  and is deferred (D-062).
- 2026 UI options: SKSE Menu Framework (SKSEMenuFramework.dll present in target list),
  Prisma UI (PrismaUI.dll present in target list).
