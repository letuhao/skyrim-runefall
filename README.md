# Runefall

> **A whole new Rune Mastery system for Skyrim.**
>
> New progression. New enemies at every rank. New gear and rune crafting.
> New hunts, raids and quests. Runes empower every part of the game - and not
> a single vanilla record is touched along the way.

## The Runefall

Some years before the Dragonborn's story begins - on the coldest night in
living memory - the sky over Skyrim cracked open, and runes fell. Nothing
burned. Nothing changed. The runes sank quietly into everything they
touched, and the land carried them the way a tide carries salt.

Then the wild began to change first. The beasts grew stronger, stranger,
and much harder to kill: hunts failed, roads emptied, walls went up, and
life got harder for everyone. And when hunters finally started winning
again, they found the reason the wild had gotten so strong sealed inside
its victims: **rune fragments** - the fallen power, warm and awake and
loose.

The mages studied them and made the discovery that changed Skyrim: a
fragment's power can pass into a *willing* body too. They named the
practice **Rune Mastery** - and taught it, so that anyone could fight
back. Smiths learned to bind fragments into steel and armor, and the
**Rune Forge** was born. Enchanters and alchemists opened doors of their
own - because the runes had fallen on *everything*, and every craft
answered them in kind.

*Master the runes - or be mastered by them.* The full chronicle - the
night it fell, the beasts, the fragments, the first forge, and the land it
made - is told in [LORE.md](LORE.md).

---

**Status: in active development, pre-release.** No public release, upload, tag or push has
been made yet. An earlier internal build's ready-claim was retracted by an independent
audit (2026-10-28) and retired; a remediation program is completing the work one audited
slice at a time - live status in [STATE.md](Runefall/STATE.md), audit trail in
[VALIDATION.md](Runefall/VALIDATION.md). "Runefall" is the working name.

---

## Rune Mastery - the foundation

Runes are power that can be carried - and in Runefall, everything that matters carries
them. The enemy standing in front of you. The gear on your back. Your own rank, grown
from mastery. The land you walk, which answers runes with threats and treasures of its
own.

- **You master runes** - progression, attributes, skill trees.
- **Monsters carry runes** - threat ranks, stage design, new behavior.
- **Gear is runeforged** - crafting, sockets, salvage, rerolls.
- **The world answers runes** - hunts, raids, quests, threat bands.

One core loop, every pillar feeding the next:

```text
     Enemy tiers (rune ranks)    ->    Loot (rarity + affixes)
              ^                                |
              |                          salvage / dismantle
              |                                v
     Progression (attributes)    <-    Rune Forge: reroll / socket / upgrade
```

Kill a rune-ranked enemy, loot what it drops, break the gear you don't need into
materials at the Rune Forge, spend those runes and materials on the piece you actually
want, and bank the attribute points that make the next tier survivable. Elites,
mini-bosses, bosses and the hunt itself are where the rare materials, unique rune power
and caches live.

And the economy runs on **play, not clocks**: every rune is earned per encounter, per
loot event, per discovery, per boss kill. No idle income, no daily timers, no
"wait 24 hours" tricks. Heavy fighting pays. Patient exploring pays a different way.

## The player: mastery earned, not chosen

Skyrim has a fixed growth curve. Runefall replaces it with a rune mastery progression:

- **Rune Rank 1-100** - your own rung on the same ladder your enemies walk, rising
  through ten tiers from *Runetouched* to *Runefall Ascendant*.
- **A class ladder, not a starting menu** - three base classes drawn straight from
  Skyrim's three stones (Warrior, Mage, Thief), nine advanced classes as rune
  mastery deepens, and **18 master classes, one for every Skyrim skill**. Every
  rung is earned by playing and mastering runes, not picked once at character
  creation and never touched again.
- **Nine attributes** across Warrior, Thief and Mage families - Might, Endurance,
  Precision, Agility, Guile, Cunning, Intellect, Wisdom, Willpower. Two attribute
  points and one skill point per level; separate respec cost; soft cap 30 / hard cap
  60 per attribute, all INI-configurable.
- **Eighteen passive skill lines and nine active abilities**, three ranks each - 54
  nodes of ladder. Gates are Skyrim skill levels (20/50/75) and attribute thresholds
  (10/20/30), so builds emerge from gates and synergies instead of a class lock.
- **Power with an honest price**: every attribute opens at least one non-combat door -
  economy, carrying, exploration, stealth or magic utility - so points buy a
  playstyle, not just bigger damage numbers.
- **Uninstall-safe by design**: no vanilla actor values are repurposed; mastery lives
  in Runefall's own records and co-save, applied and re-applied at runtime.

## The monsters: threats that carry runes

Every enemy can answer the rune on your back with one of its own:

- **Five rune ranks** - Elite, Champion, Mini-boss, Boss, Mythic - assigned
  deterministically at spawn, with density caps and per-area limits.
- A rank brings its package: health, damage and resistance multipliers, up to three
  perks, up to two abilities, an optional aura and a name prefix. Applied once, from
  Runefall's own records, guarded by exclusive keywords - nothing double-stacks, and
  enemy overhaul mods keep their NPC records untouched.
- **One ladder rules them all**: 100 stages across ten tiers with a 15x power curve.
  A Runetouched bandit and an Ascendant beast are measured by the same system, which
  is why the challenge curve never lies to you.
- **Combat that respects the runes** - threat protection, penetration and damage
  categories mean a stage gap is a real tactical fact, not diagonal health inflation.
- **Anomalies** - rare enemies that break the host curve entirely, with their own
  families, abilities and rewards. When the codex matters, that's why.

## The gear economy: loot, hunt, forge

Gear in Runefall is not a reward screen - it is an economy with three interlocking
mechanisms, and it borrows its spine from the Monster Hunter series: kill the thing,
pull its parts, forge your next best piece. Every loop above feeds the next one, and
every step below is a designed default tuned in INI - shaped by play, not hardcoded
by fate.

### Loot: every drop is a roll you get to read

Loot is rolled the instant it comes into being - *before you ever see it*:

- **Six rarity bands**: Common, Refined, Exquisite, Radiant, Mythic, Runeforged.
  Rarity *is* the affix count (0-5), drawn from five families - Offense, Defense,
  Utility, Economy, Elemental - with at most one per family. Reading a new item is
  reading a decision. A Runeforged piece can carry one extra fixed effect on top.
- A designed pool of **120 affixes across 600 tier rows**, scaled by item level,
  rarity band and magic find, plus **59 fixed unique affixes** - 20 from bosses, 20
  from quests, 19 from first-kill creature hunts.
- **Per-instance generation**: dynamic enchantment, display name and tempering are
  applied to the item instance, metadata kept in the co-save. Affix space lives in
  data files: no variant records, no record bloat, no leveled-list edits.
- **Luck you can feel** - magic find shifts the odds upward, and a pity ladder
  (20/80/400 kills) makes a dry streak end at *something* instead of nothing.

### The hunt: every monster is a rich vein

This is the Monster Hunter loop, transplanted to Skyrim:

- **Your next upgrade has an address.** Nineteen hostile creature families each own
  **their own primary material** - no two families share one, so every species has
  its reward and every farm has a target.
- **Nothing you carry is junk.** The content build enforces a dead-material gate:
  if a material had no use in any recipe, cost or craft, the build fails instead of
  shipping filler.
- **The same beast pays twice.** A family's first kill yields its unique affix;
  repeat kills roll its hunt material with odds that climb as your mastery does.
  Double the reason to learn a monster's routes instead of skipping it once.
- **Four faucets feed one registry**: salvage, ranked-kill drops (tier bonus by
  rank), tiered nodes and caches by region, and the race hunts. Whatever you play -
  dungeon, farm, expedition - the material bag grows.

### The Rune Forge: everything becomes everything

- **Salvage** any Runefall item into one to three material units of its own tier
  plus ten to a quarter of its rune value. Broken gear produces the same yield -
  losing a fight costs durability, not items (and a confirmation dialog guards
  every destruction, because salvage is final).
- **Reroll** goes for fresh enhancements while your sockets, rarity and progress
  stay yours. A bad roll is raw material, not heartbreak.
- **Upgrade rarity, one band at a time** - base 60% success, tipped upward by the
  Precision attribute (+0.5% per point, capped +15%). The two apex steps are the
  gamble of the ladder: Radiant-to-Mythic wants a high-grade rune and a boss
  material; Mythic-to-Runeforged wants a top-tier rune, two boss materials and
  beats you with 40% odds - and pays back with the unique runeforged effect. When
  an upgrade fails, the bill is lost - the weapon never is.
- **Socket up to three runes**; higher rarity carries more sockets.
- **Imbue** one tier material into a piece for a flat, honest boost - ore leans
  offense, essence leans spell power, hide leans defense - scaled by the material's
  tier (+3% to +15%, hunt materials up to +20%). One imbue per item; overwriting
  costs a deliberate surcharge, so choices stay meaningful.
- The forge ships in stages: the core transactions come first, and the full
  Crafting Table - Monster Hunter-style gear trees, targeted affix crafting, affix
  shard extraction - is its own expansion module by design. No half-built systems
  hang off the core.

## The world: hunts, raids and rune-driven content

- **Threat bands** - regions, encounters and spawns draw difficulty from the same
  ladder, so where you go decides what meets you. A quiet valley stays a valley; a
  dragon mound doesn't.
- **Monster hunting** - nineteen hostile creature families each hide a unique affix on
  your first kill and drop hunt materials whose odds climb with mastery. The hunt is a
  progression path, not an errand.
- **Boss raids** - the top rungs of the ladder are held by tiered mini-boss and boss
  encounters with dedicated drop tables and the rarest materials and rune power in
  the game. Built to be read, planned and beaten - not cleared off the D-pad.
- **Rune-driven quests** - quests and completions feed the rune economy directly:
  unique affixes, caches and drops are tied to what you finish, so the story of your
  save IS the gear story.

## Built to live beside vanilla

The Rune Mastery system changes what the game *does*, not what it *is*:

- **New FormIDs only** - every Runefall record is its own. No vanilla record is
  overridden, no leveled list edited, no container, cell or world touched.
- **Runtime application** - enemy buffs and item logic are applied by the native DLL
  and re-applied on load; nothing permanent is baked into your save. Remove the mod
  and the game falls back to vanilla, cleanly.
- **Enemy overhauls keep their NPCs** (ranks are runtime overlays), loot and
  leveled-list mods like OWL keep working (additive loot by default), progression
  mods coexist (no actor values are repurposed).
- **It runs at engine speed** - loot, ranks, crafting and progression logic live in
  the native SKSE plugin. No Papyrus polling loops, no script lag, no save bloat.
- **Every pillar is optional** - toggle systems by INI; optional content packs disable
  cleanly if they're missing or incompatible.

## Requirements (hard)

Runefall **will not work without all four**:

1. **Skyrim SE/AE 1.6.1170** - the only supported runtime. Skyrim 1.7.x is currently deferred
   (the SKSE/Address Library/plugin ecosystem is not ready); 1.5.97 and consoles are unsupported.
2. **SKSE 1.6.1170** - the SKSE build matching 1.6.1170 (skse.silverlock.org).
3. **Address Library for SKSE Plugins** - the 1.6.1170 database
   (`versionlib-1-6-1170-0.bin` and `-0-1.bin`) placed in `Data\SKSE\Plugins`.
4. **SKSE Menu Framework 3.18+** (Nexus mod 120352) - release build for player installs.
   Copy its `SKSE` folder into `Data\` (DLL, fonts, ini, strings and themes). The DEV build is
   only for debug DLL builds and must not be used with the release DLL.

Not required: SkyUI, MCM Helper, JContainers, PapyrusUtil, .NET runtime, or any Creation Club
content beyond what Skyrim itself ships.

## Installation (v1 test build)

1. Install Skyrim 1.6.1170 + SKSE 1.6.1170 + Address Library 1.6.1170.
2. Install SKSE Menu Framework 3.18 (release) over `Data\`.
3. Install the Runefall files:
   - `Data\Runefall.esp` (base ESL)
   - `Data\Runefall - Unique.esp` (ESL, optional)
   - `Data\Runefall - Hunt.esp` (ESL, optional)
   - `Data\Runefall\Data\` (data tables, manifest, ini)
   - `Data\Scripts\RunefallNative.pex`, `RunefallProbe.pex`
   - `Data\SKSE\Plugins\Runefall.dll`
4. Load order: Skyrim.esm and DLC/CC masters first, then `Runefall.esp`, then its packs. All
   Runefall plugins are ESL-flagged; keep the base before the packs.
5. Launch through SKSE. In debug mode the plugin writes `Data\SKSE\Plugins\RunefallProbe.log`
   and `Runefall.log` under `Documents\My Games\Skyrim Special Edition\SKSE`.

## Development

Contributors and dev environments: see [DEVELOPMENT.md](DEVELOPMENT.md) for the toolchain,
native and content builds, data authoring rules, the module map and debug tooling.
Target runtime: Skyrim SE/AE 1.6.1170 only (D-062). Do not advertise 1.7.x support.

## Repository map

| Path | Contents |
|---|---|
| [LORE.md](LORE.md) | The full in-world chronicle of the Runefall |
| [Runefall/GAME-DESIGN.md](Runefall/GAME-DESIGN.md) | The Rune Mastery design: pillars, economy, balance |
| [Runefall/SPECS/](Runefall/SPECS/) | Layered design specs L1-L5, plus mining and balance studies |
| [Runefall/SPECS/POWER-LADDER.md](Runefall/SPECS/POWER-LADDER.md) | The 100-stage ladder and world threat bands |
| [Runefall/ARCHITECTURE.md](Runefall/ARCHITECTURE.md) | Technical architecture baseline |
| [DEVELOPMENT.md](DEVELOPMENT.md) | Build, toolchain, authoring and debug guide for devs |
| [Runefall/src/plugin/](Runefall/src/plugin/) | Native CommonLibSSE-NG plugin (C++, CMake + vcpkg) |
| [Runefall/src/papyrus/](Runefall/src/papyrus/) | Papyrus native bridge and quest probe |
| [Runefall/tools/DataMiner/](Runefall/tools/DataMiner/) | Official-master data miner (C#, Mutagen) |
| [Runefall/tools/Runefall.Build/](Runefall/tools/Runefall.Build/) | ESL + CSV generator/validator (C#, Mutagen) |
| [Runefall/PROGRAM.md](Runefall/PROGRAM.md) | Active remediation program (audited slices S0-S15) |
| [Runefall/STATE.md](Runefall/STATE.md) | Current status and proven evidence |
| [Runefall/DECISIONS.md](Runefall/DECISIONS.md) | Architecture decision records (D-001..D-068) |
| [Runefall/VALIDATION.md](Runefall/VALIDATION.md) | Validation trail and build proof |

## License

GPL-2.0 - see [LICENSE](LICENSE). Runefall is open source; build on it, study it, extend it.
