# Reference-game feature gap register

Status: 2026-10-26. Companion to `SPECS\GAP-REGISTER-FUTURE.md` (v1 scope lock). Purpose: compare
Runefall's feature set against the Monster Hunter series, Last Epoch, Path of Exile and the Diablo
series, and record every feature those games have that Runefall does not, so nothing already
proven in the genre is silently dropped or forgotten.

Rules:
- Every gap gets an ID (`RG-xxx`), a decision code (`NOW` / `RESERVE` / `WATCH` / `OUT`), a
  priority, and a destination pack or "not applicable".
- `NOW` items are cheap hooks or v1-worthy features; they may enter v1 only after updating this
  register and `GAP-REGISTER-FUTURE.md` (D-047).
- `RESERVE` items are deliberately out of v1 and go to a named expansion pack.
- `WATCH` items are risks or metrics, not features.
- `OUT` items are out of scope (multiplayer, console, platform, monetization) and are recorded so
  the decision is explicit, not accidental.
- A gap already covered by an existing `F-xx` / `N-xx` ID is cross-referenced, not duplicated.
- Reference games are cited by family: **MH** = Monster Hunter series, **LE** = Last Epoch,
  **PoE** = Path of Exile, **D** = Diablo series (D2/D3/D4 unless noted).

---

## 0. Runefall v1 feature baseline (what is already covered)

| System | What Runefall v1 already has |
|---|---|
| Progression | 9 attributes (3 builds), 18 skill lines x 3 ranks, 2 attr + 1 skill point per level, rune-bought points, respec, soft/hard caps, RF tiers 0-5 with attunement. |
| Items | 6 rarity bands (0-5), rarity = enhancement count, 5 enhancement categories, affix CSV tables, sockets (rune rank), item power score, no durability, no bind. |
| Loot | Drop sources, ilvl from source, roll pipeline, drop chance, rarity weights, magic find, pity/dry-streak, smart drops, anti-abuse (container roll-once), general drop table, race hunt tables (19 families), hunt uniques (59). |
| Enemy tiers | Ranks 1-5, 10 monster affixes, scaling multipliers, assignment at spawn, density caps, bosses per tier, attunement gate. |
| Crafting | Rune Forge: salvage, socket/extract, reroll (all + locked), rarity upgrade (60% success), rune conversion, material hunting. Crafting Table reserved. |
| Materials | 4 tiers (Common/Refined/Exotic/Mythic), 3 families each, salvage yield, node yields. |
| Economy | Single Rune currency R1-R5 (1/5/25/125/625), event-driven (no clock), sources/sinks, conversion, anti-inflation. |
| UI | SKSE Menu Framework pages (Character, Rune Forge, Hunt Codex, Item Detail), hotkeys, Papyrus bridge, localization keys. |
| Runtime | Native SKSE DLL, ESL content, multi-pack (D-040), co-save TLV, diagnostics, performance budgets, uninstall matrix. |
| Compatibility | Additive loot, no vanilla overrides, caps, INI multipliers, optional UI/Patches/Takeover modules. |

---

## 1. Combat and progression gaps

| ID | Feature gap | Reference | Runefall status | Priority | Retrofit cost | Destination | Notes |
|---|---|---|---|---|---|---|---|
| RG-001 | Mastery / ascendancy choice at a level threshold (specialize a build into a sub-class with its own passive cluster) | LE, PoE, D3 | Missing (3 flat builds, no specialization) | RESERVE | High | Progression pack (F-06) | F-06 already reserves subclass trees; ascendancy is the framing. Needs level gate + own passive cluster. |
| RG-002 | Per-skill skill trees (each active skill has its own small tree modifying it) instead of 3 fixed ranks | LE, PoE | Missing (fixed ranks only) | RESERVE | High | Progression pack | Large content multiplier; v1 ranks are the cheap version. |
| RG-003 | Passive skill tree / grid (large branching node web) | PoE, LE (passives) | Missing | RESERVE | High | Progression pack (F-06) | Out of scope for v1; attributes are the v1 passive layer. |
| RG-004 | Damage type system with conversions (physical/fire/cold/lightning/poison/shadow/void/arcane/holy) and damage conversion rules | D, LE, PoE, MH | Partial (Elemental category exists; no typed damage or conversion) | RESERVE | Med | Affix/Effect pack | Enhancement "Elemental" is a category, not a damage type. Needs MGEF damage types + conversion rules. |
| RG-005 | Ailment system (bleed, burn, chill, freeze, shock, poison, stun, slow, etc.) with stacking rules | D, LE, PoE | Missing (Venomcraft poison only) | RESERVE | Med | Combat pack | Needs ailment registry, stacking caps, cleanse. |
| RG-006 | Penetration / resistance system (resistances with caps and penetration stats) | D, LE, PoE | Missing | RESERVE | Med | Combat pack | Vanilla resistances exist; our bonuses are percent. Needs design. |
| RG-007 | Ward / energy shield mechanic (secondary health pool that regenerates, separate from HP) | LE, PoE | Missing | RESERVE | High | Combat pack | New resource; save/UI impact. |
| RG-008 | Block, dodge, evasion as distinct defensive layers with ratings | MH, PoE, D3 | Partial (Evasion passive, block effectiveness) | RESERVE | Med | Combat pack | Could be a defensive affix family. |
| RG-009 | Critical strike chance and multiplier as first-class stats with crit-specific affixes | D, LE, PoE, MH | Partial (Agility +crit chance; no crit damage multiplier) | NOW (cheap) | Low | Core affixes | Add crit damage multiplier affix + item power contribution. Hook: affix category. |
| RG-010 | Leech (life/mana/rune on hit) and life-on-hit mechanics | D, PoE, LE | Missing | RESERVE | Low | Affix pack | New affix family; needs hit hook. |
| RG-011 | Thorns / reflect damage | D, PoE | Missing | RESERVE | Low | Affix pack | New affix family. |
| RG-012 | Area of effect scaling and AoE-specific affixes | D, PoE, LE | Missing | RESERVE | Low | Affix pack | New affix family. |
| RG-013 | Cooldown recovery / reduction as a stat | D3, LE | Partial (Willpower -0.5% cooldown) | RESERVE | Low | Affix pack | Already an attribute effect; needs affix + cap. |
| RG-014 | Resource cost reduction (magicka/stamina cost) | D3, PoE | Missing | RESERVE | Low | Affix pack | New affix family. |
| RG-015 | Movement speed as a tunable stat with caps | D3, PoE, MH | Partial (Agility +movement) | RESERVE | Low | Affix pack | Needs cap to avoid physics break (L2 open question). |
| RG-016 | Attack/cast speed scaling with animations respected | D, PoE | Partial (Agility attack speed) | WATCH | - | - | L2 open question; must not break animations. |
| RG-017 | Charges / stacks on items and abilities (e.g., flask charges, skill charges) | PoE, D3 | Missing | RESERVE | Med | Item pack | New item state; co-save impact. |
| RG-018 | Weapon sharpness / condition system (degradation affecting damage) | MH | Missing (no durability by design, I-08) | OUT | - | - | Explicitly non-goal (I-08). Recorded as rejected. |
| RG-019 | Combo system / multi-hit scaling | MH, D3 | Missing | RESERVE | Med | Combat pack | Needs animation/combat overhaul. |
| RG-020 | Perfect dodge / guard / counter windows | MH, D3 | Missing | RESERVE | High | Combat pack | Needs input/timing hooks. |
| RG-021 | Mounts and mount-based combat/movement | MH, D4 | Partial (Skyrim mounts exist) | OUT | - | - | Vanilla Skyrim mounts; not our system. |
| RG-022 | Pet / companion scaling and hunt bonuses | MH (Palico), D3 (follower) | Missing (F-16 reserved) | RESERVE | Med | Companion pack (F-16) | Already reserved. |
| RG-023 | Damage conversion (change physical to elemental etc.) | PoE, LE | Missing | RESERVE | Med | Affix pack | Needs damage type system first (RG-004). |

---

## 2. Itemization and loot gaps

| ID | Feature gap | Reference | Runefall status | Priority | Retrofit cost | Destination | Notes |
|---|---|---|---|---|---|---|---|
| RG-024 | Item quality tiers (0-20% base stat multiplier) affecting base item power | PoE, D3 | Missing | NOW (cheap) | Low | Core items | Quality as a small multiplier on base stats; stored in metadata (N-06 tags). |
| RG-025 | Implicit affixes on base items (base item grants a fixed implicit mod) | PoE, D, LE | Missing | RESERVE | Low | Item pack | New field in item base data. |
| RG-026 | Corrupted items (powerful implicit, locked, cannot be modified) | PoE, D3 (ancient) | Missing | RESERVE | Med | Item pack | Needs corrupt action + lock flag. |
| RG-027 | Influenced items (item gains an influence tag granting special affix pool) | PoE | Missing | RESERVE | Med | Item pack | New affix pools per influence. |
| RG-028 | Fractured / synthesised items (guaranteed affix + special mods) | PoE | Missing | RESERVE | Med | Item pack | Special base items. |
| RG-029 | Veiled mods (hidden affix revealed by NPC, choice of 3) | PoE | Missing | RESERVE | Med | Item pack | Needs unveil UI + NPC. |
| RG-030 | Anointed items (oil-based affix on amulet/ring) | PoE (blight) | Missing | RESERVE | Low | Item pack | New consumable + slot. |
| RG-031 | Enchantment on a single affix (reroll one affix instead of all) | D4, D3 (enchantress) | Partial (reroll all; locked option 3x) | NOW (cheap) | Low | Rune Forge | Add "reroll one slot" action cheaper than full reroll. |
| RG-032 | Tempering (add a specific affix from a tempering manual, limited uses) | D4 | Missing | RESERVE | Med | Crafting pack | New manual item + use counter. |
| RG-033 | Masterworking (incremental affix value upgrades with ranks, crit masterwork) | D4 | Missing | RESERVE | Med | Crafting pack | Needs upgrade ranks + crit roll. |
| RG-034 | Imprinting (copy an item's affixes to another) | D4 | Missing | RESERVE | Med | Crafting pack | Needs item copy action. |
| RG-035 | Runewords (specific rune sequence in sockets yields a named powerful effect) | D2, D4 | Missing (sockets hold generic rank runes) | RESERVE | High | Rune pack (F-03) | Strong fantasy; needs sequence detection + named effects. F-03 already reserves rune families. |
| RG-036 | Aspect extraction / codex of power (extract legendary power, apply to any item) | D4 | Missing (unique affixes are fixed to hunt items) | RESERVE | Med | Unique pack | Needs aspect registry + apply action. |
| RG-037 | Legendary gems with ranks (socketed gem that levels and grants a powerful effect) | D3 | Missing (runes are currency, not gems) | RESERVE | High | Rune pack (F-03) | New item class + leveling. |
| RG-038 | Threshold jewels (jewel that grants a bonus if a condition on the passive tree holds) | PoE | Missing | RESERVE | Med | Item pack | Not applicable without passive tree. |
| RG-039 | Abyss jewels (jewels with special mods in abyssal sockets) | PoE | Missing | RESERVE | Low | Item pack | New socket type. |
| RG-040 | Cluster jewels (jewels that add passive nodes) | PoE | Missing | OUT | - | - | Requires passive tree. |
| RG-041 | Timeless jewels (transform keystones) | PoE | Missing | OUT | - | - | Requires passive tree. |
| RG-042 | Unique items with fixed legendary affixes and special mechanics | D, PoE, LE, MH | Partial (40 hunt/boss/quest uniques) | RESERVE | Low | Unique II (F-09) | Already have 40; reserve remaining 11+28 mined candidates. |
| RG-043 | Set items and set bonuses (2-piece, 4-piece, etc.) | D, PoE, LE, MH | Missing (F-04 reserved) | RESERVE | High | Set pack (F-04) | Already reserved. |
| RG-044 | Mythic uniques (extremely rare, build-defining) | D4 | Missing | RESERVE | Low | Unique pack | New rarity tier above Runeforged? Or separate flag. |
| RG-045 | Ancient / primordial / primal items (perfect or near-perfect rolls) | D3 | Missing | RESERVE | Low | Item pack | Drop flag + roll logic. |
| RG-046 | Item level clamping by world tier (already have) | D | Covered | - | - | - | L-02. |
| RG-047 | Smart loot (bias to player's used weapon/armor type) | D3 | Partial (L-07 weapons only) | NOW (cheap) | Low | Core loot | Extend L-07 to armor slots; toggle already exists. |
| RG-048 | Loot filter (rules-based hide/show/color/sound/icon/beam) | PoE, LE | Missing (F-21 reserved) | RESERVE | Med | QoL pack (F-21) | Already reserved; high value. |
| RG-049 | Auto-salvage / auto-pickup rules | MH (auto-craft), LE | Missing (F-21) | RESERVE | Med | QoL pack (F-21) | Already reserved. |
| RG-050 | Loot beams / map icons / ground markers for rarity | D4, PoE | Partial (optional color/marker) | RESERVE | Low | UI pack | Visual polish; needs UI framework. |
| RG-051 | Item comparison (DPS/defense score, detailed diff) | D4, LE | Missing (F-22 reserved) | RESERVE | Med | UI pack (F-22) | Already reserved. |
| RG-052 | Stash tabs / search / sort | D, PoE | Missing (vanilla inventory) | OUT | - | - | Vanilla Skyrim inventory; not our scope. |
| RG-053 | Armory / build loadouts (save and swap gear+skills) | D4 | Missing (F-13 reserved) | RESERVE | Med | UI pack (F-13) | Already reserved. |
| RG-054 | Transmog / layered armor / wardrobe | MH, D4, D3 | Missing (F-20 reserved) | RESERVE | High | UI/Asset pack (F-20) | Already reserved. |
| RG-055 | Item sockets with colors/links (skill gem socketing) | PoE | Missing | OUT | - | - | Skyrim has no skill gems; our sockets are rune ranks. |
| RG-056 | Quality upgrades on base items (whetstones, armor spheres) | MH | Partial (material hunting yields tiers) | RESERVE | Low | Crafting pack | Deterministic defense upgrade path (MH armor spheres) vs random rarity upgrade. |
| RG-057 | Treasure goblins / special loot carriers | D3 | Missing | RESERVE | Low | Enemy pack | New NPC type with special drop table. |
| RG-058 | Strongboxes / ambush containers with mods | PoE | Missing | RESERVE | Low | Container pack | New container type. |
| RG-059 | Cursed chests / shrines (risk/reward) | D3 | Missing | RESERVE | Low | World pack | New world object. |
| RG-060 | Pylons (temporary powerful buffs in dungeons) | D3 | Missing | RESERVE | Low | World pack | New world object. |
| RG-061 | Map/item drop from specific bosses (boss-specific uniques) | D, PoE | Partial (boss uniques exist) | - | - | - | Covered by D-041. |

---

## 3. Crafting and economy gaps

| ID | Feature gap | Reference | Runefall status | Priority | Retrofit cost | Destination | Notes |
|---|---|---|---|---|---|---|---|
| RG-062 | Forging potential / crafting resource consumed per craft (risk of fracture) | LE | Missing | RESERVE | Med | Crafting pack | Adds risk layer to crafting. |
| RG-063 | Fracture on failed craft (item breaks or loses affix) | LE | Missing | RESERVE | Med | Crafting pack | Needs failure outcome design. |
| RG-064 | Glyphs that modify crafting outcomes (chaos/creation/protection) | LE | Missing | RESERVE | Med | Crafting pack | New consumable items. |
| RG-065 | Runes of shattering/extraction/ascendance/refinement (break item into shards, extract affix, upgrade rarity, reroll values) | LE | Partial (reroll + conversion exist) | RESERVE | Med | Crafting pack | Map to Rune Forge v2. |
| RG-066 | Affix shards (extracted affix as currency, used in targeted crafting) | LE, PoE (essences) | Missing (C-09 reserved) | RESERVE | Med | Crafting Table (F-01) | Already reserved in C-09. |
| RG-067 | Targeted affix crafting (choose base + one affix family) | LE, D | Missing (C-09) | RESERVE | Med | Crafting Table (F-01) | Already reserved. |
| RG-068 | Deterministic base-item crafting (base item + materials = item) | MH, LE, D | Missing (C-09) | RESERVE | High | Crafting Table (F-01) | Already reserved. |
| RG-069 | Socket-add recipes (add a socket to an item) | LE, PoE | Missing (C-09) | RESERVE | Low | Crafting Table (F-01) | Already reserved. |
| RG-070 | Material recipe chains (tier up/down, family conversion) | MH, LE | Missing (C-01 basic) | RESERVE | Low | Crafting pack | New recipes. |
| RG-071 | Legendary potential (unique item can absorb an exalted item's affixes) | LE | Missing | RESERVE | High | Unique pack | Needs unique+exalted merge. |
| RG-072 | Exalted items (tier 6/7 affixes above normal) | LE | Missing | RESERVE | Med | Item pack | New rarity/affix tier. |
| RG-073 | Sealed affixes (lock an affix permanently so it survives crafts) | LE | Missing | RESERVE | Low | Crafting pack | New affix state. |
| RG-074 | Currency-based crafting with granular orbs (chaos, exalted, annulment, scouring, regal, etc.) | PoE | Missing (single Rune currency) | RESERVE | High | Economy pack | Different economy philosophy; Runefall uses one Rune. Watch only. |
| RG-075 | Harvest / horticrafting (plant seeds, grow crafts) | PoE | Missing | OUT | - | - | Not applicable to Skyrim loop. |
| RG-076 | Fossils / resonators (modify affix pools) | PoE | Missing | RESERVE | Med | Crafting pack | New consumable. |
| RG-077 | Essences (guaranteed affix on craft) | PoE | Missing | RESERVE | Low | Crafting pack | New consumable. |
| RG-078 | Recombinators (combine two items' affixes) | PoE | Missing (F-02 reserved) | RESERVE | Med | Crafting II (F-02) | Already reserved. |
| RG-079 | Affix transfer / extraction between items | LE, PoE | Missing (F-02) | RESERVE | Med | Crafting II (F-02) | Already reserved. |
| RG-080 | Enchanting reroll one affix (occultist style) | D4 | Partial (RG-031) | NOW (cheap) | Low | Rune Forge | Same as RG-031. |
| RG-081 | Imprinting (copy item) | D4 | Missing (RG-034) | RESERVE | Med | Crafting pack | Same as RG-034. |
| RG-082 | Gambling (spend currency for random item of a type) | D3, D4 (obols) | Missing | RESERVE | Low | Economy pack | New vendor action. |
| RG-083 | Vendor recipes / Horadric cube (combine items into new items) | D2 | Missing | RESERVE | Med | Crafting pack | New recipe system. |
| RG-084 | Transmutation (3 items → 1 of higher tier) | D2, F-17 | Missing (F-17 reserved) | RESERVE | Low | Economy pack (F-17) | Already reserved. |
| RG-085 | Rune gambling vendor | F-17 | Missing | RESERVE | Low | Economy pack (F-17) | Already reserved. |
| RG-086 | Salvage returning affix shards | LE | Missing (v1 salvage = materials + runes only) | RESERVE | Low | Crafting Table (F-01) | C-02 explicitly defers shard extraction. |
| RG-087 | Node respawn timers and hunting loop | MH, LE | Partial (C-07 long respawn) | - | - | - | Covered. |
| RG-088 | Tiered node yields by area | MH | Covered | - | - | - | C-07. |
| RG-089 | Material families (ore/essence/hide) | MH | Covered | - | - | - | C-01. |

---

## 4. Enemy, encounter and boss gaps

| ID | Feature gap | Reference | Runefall status | Priority | Retrofit cost | Destination | Notes |
|---|---|---|---|---|---|---|---|
| RG-090 | Monster affixes visible on name (already have 10) | MH, D3 | Covered | - | - | - | T-03. |
| RG-091 | Champion packs with visible affixes | Torchlight, D3 | Covered | - | - | - | T-03. |
| RG-092 | Monster rank scaling by player level and world tier | D, MH | Covered | - | - | - | T-04, A5. |
| RG-093 | Boss phases and arena mechanics | MH, D | Missing (F-14 reserved) | RESERVE | High | Boss pack II (F-14) | Already reserved. |
| RG-094 | Encounter modifiers (extra monsters, elemental hazards) | PoE (map mods), D4 (dungeon affixes) | Missing | RESERVE | Med | Dungeon pack | New modifier system. |
| RG-095 | Dungeon affixes / sigils (nightmare dungeons with modifiers) | D4, PoE (maps) | Missing | RESERVE | High | Dungeon pack | New dungeon system. |
| RG-096 | Map device / atlas (tiered maps with modifiers and bosses) | PoE | Missing | RESERVE | High | Endgame pack | Endgame loop; F-11. |
| RG-097 | Delve (infinite depth, darkness, flares, fossils) | PoE | Missing | RESERVE | High | Endgame pack | Endgame loop. |
| RG-098 | Heist (contracts, blueprints, rogues) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-099 | Expedition (logbooks, explosives, remnants) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-100 | Ritual (tribute, defer, favors) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-101 | Ultimatum (risk/reward trials) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-102 | Sanctum (roguelike, relics, boons, afflictions) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-103 | Blight (tower defense) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-104 | Metamorph (organs, catalysts) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-105 | Incursion (temple with room choices) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-106 | Betrayal (syndicate, safehouses) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-107 | Legion (time bubbles, emblems) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-108 | Breach (breaches, domains) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-109 | Abyss (cracks, depths, lich bosses) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-110 | Harbinger (shards, beachheads) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-111 | Essence (monsters, essences) | PoE | Missing | RESERVE | Low | Enemy pack | Could be "rune-essence" enemies dropping special materials. |
| RG-112 | Beyond (demons) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-113 | Torment (spirits) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-114 | Bloodlines (packs) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-115 | Domination (shrines) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-116 | Invasion (invaders) | PoE | Missing | OUT | - | - | Not applicable. |
| RG-117 | Nemesis (rare monsters with mods) | PoE | Partial (monster affixes) | - | - | - | Covered by T-03. |
| RG-118 | Ambush (strongboxes) | PoE | Missing (RG-058) | RESERVE | Low | Container pack | Same as RG-058. |
| RG-119 | Rampage (kill streaks) | PoE | Missing | RESERVE | Low | Combat pack | New mechanic. |
| RG-120 | Onslaught (speed) | PoE | Missing | RESERVE | Low | Combat pack | New mechanic. |
| RG-121 | Turf wars / monster interactions | MH | Missing | OUT | - | - | Not applicable to Skyrim engine. |
| RG-122 | Mounting monsters | MH | Missing | OUT | - | - | Not applicable. |
| RG-123 | Wirebug movement / seikret | MH | Missing | OUT | - | - | Not applicable. |
| RG-124 | Investigations / tempered monsters | MH | Partial (RF tiers + boss hunts) | RESERVE | Med | Hunt II (F-07) | F-07 hunt contracts cover targeted hunts; tempered = RF tier scaling. |
| RG-125 | Variant / risen / frenzied / afflicted monsters | MH | Missing | RESERVE | Med | Enemy pack | New monster variants. |
| RG-126 | Anomaly quests / coalescence | MH | Missing | RESERVE | Med | Endgame pack | New quest type. |
| RG-127 | Siege monsters | MH | Missing | RESERVE | Med | Boss pack | New encounter type. |
| RG-128 | Arena challenges | MH, D3 | Missing | RESERVE | Med | World pack | New arena cell. |
| RG-129 | World bosses (rotating) | D4, MH | Missing | RESERVE | Med | World pack | New boss type. |
| RG-130 | Legion events (mass spawns) | D4 | Missing | RESERVE | Med | World pack | New event. |
| RG-131 | Helltide (area event with chests/cinders) | D4 | Missing | RESERVE | Med | World pack | New event. |
| RG-132 | Tree of whispers (bounty board → caches) | D4 | Missing (F-07) | RESERVE | Med | Hunt II (F-07) | F-07 hunt contracts. |
| RG-133 | Strongholds (capture for renown) | D4 | Missing | RESERVE | Med | World pack | New world content. |
| RG-134 | Endemic life buffs (environmental buffs from creatures) | MH | Missing | RESERVE | Low | World pack | New world objects. |
| RG-135 | Environmental traps (boulders, vines, flash bugs) | MH | Missing | RESERVE | Low | World pack | New world objects. |
| RG-136 | Cursed chests / shrines (RG-059) | D3 | Missing | RESERVE | Low | World pack | Same as RG-059. |
| RG-137 | Frenzy virus (infection mechanic) | MH | Missing | RESERVE | Med | Enemy pack | New status. |
| RG-138 | Blights (elemental blight ailments) | MH | Missing | RESERVE | Low | Enemy pack | New ailment. |
| RG-139 | Status ailments: poison/paralysis/sleep/stun/exhaust/blast | MH | Partial (poison) | RESERVE | Med | Combat pack | Needs ailment system (RG-005). |

---

## 5. Endgame and scaling gaps

| ID | Feature gap | Reference | Runefall status | Priority | Retrofit cost | Destination | Notes |
|---|---|---|---|---|---|---|---|
| RG-140 | Infinite scaling beyond tier 5 (corruption / torment beyond 5) | PoE, D3, D4, LE | Missing (F-11 reserved) | RESERVE | High | Endgame pack (F-11) | Already reserved. |
| RG-141 | Endless dungeon (pit / greater rifts) | D3, D4 | Missing | RESERVE | High | Endgame pack | New dungeon. |
| RG-142 | Pinnacle bosses (shaper, maven, sirus, uber variants) | PoE, D3, D4 | Missing | RESERVE | High | Boss pack | New bosses. |
| RG-143 | Torment difficulty tiers (I-VI) | D3, D4 | Missing (RF tiers 0-5 only) | RESERVE | Med | Endgame pack | Could extend RF tiers to 10+. |
| RG-144 | Paragon levels / boards (post-max-level progression) | D3, D4 | Missing (F-06) | RESERVE | High | Progression pack (F-06) | Already reserved as subclass trees. |
| RG-145 | Glyphs (socketed into paragon board) | D4 | Missing | RESERVE | Med | Progression pack | Needs paragon board first. |
| RG-146 | Renown (account-wide unlocks: skill points, potion charges, paragon) | D4 | Missing | RESERVE | Med | Progression pack | New progression layer. |
| RG-147 | Pantheon (god blessings, major/minor) | PoE | Missing | RESERVE | Med | World pack | New buff system. |
| RG-148 | Bandit choice (quest choice with permanent reward) | PoE | Missing | RESERVE | Low | Quest pack | New quest pattern. |
| RG-149 | Prophecies (random future events with rewards) | PoE | Missing | RESERVE | Med | World pack | New system. |
| RG-150 | Seasons / seasonal realm (reset + season mechanic) | D3, D4, PoE, LE | Missing (F-12 reserved modes) | RESERVE | High | Modes pack (F-12) | Already reserved. |
| RG-151 | Season journey / conquests / challenges | D3, D4 | Missing | RESERVE | Med | Modes pack | Already reserved. |
| RG-152 | Hardcore mode (death = delete) | D2, D3, D4, PoE | Missing (F-12) | RESERVE | Med | Modes pack (F-12) | Already reserved. |
| RG-153 | SSF / self-found mode | PoE | Missing | RESERVE | Low | Modes pack | New mode flag. |
| RG-154 | Ruthless mode (extreme scarcity) | PoE | Missing | RESERVE | Low | Modes pack | New mode flag. |
| RG-155 | Leaderboards | D3, PoE | Missing | OUT | - | - | Not applicable to a mod. |
| RG-156 | Challenge rifts / trial | D3 | Missing | RESERVE | Med | World pack | New content. |
| RG-157 | Set dungeons | D3 | Missing | RESERVE | Med | World pack | New content. |
| RG-158 | Greater rift keystone / trial | D3 | Missing | RESERVE | Low | World pack | New item. |

---

## 6. Exploration and world gaps

| ID | Feature gap | Reference | Runefall status | Priority | Retrofit cost | Destination | Notes |
|---|---|---|---|---|---|---|---|
| RG-159 | Waypoints / fast travel between discovered locations | D, MH | Covered (Skyrim) | - | - | - | Vanilla. |
| RG-160 | Town portals / recall | D | Covered (Skyrim) | - | - | - | Vanilla. |
| RG-161 | Camps / safe zones in the field | MH | Covered (Skyrim) | - | - | - | Vanilla. |
| RG-162 | Gathering (herbs, ore, leather) | MH, D4 | Covered (Skyrim) | - | - | - | Vanilla + our material hunting. |
| RG-163 | Fishing | MH, D4 | Covered (Skyrim) | - | - | - | Vanilla. |
| RG-164 | Mining / bone collecting | MH | Covered (Skyrim) | - | - | - | Vanilla. |
| RG-165 | Bug catching / endemic life collection | MH | Missing | RESERVE | Low | World pack | New collection. |
| RG-166 | Photo mode | MH, D4 | Covered (Skyrim/mods) | - | - | - | Not our scope. |
| RG-167 | Gestures / emotes | MH | Covered (Skyrim) | - | - | - | Vanilla. |
| RG-168 | Map with fog of war / discovery rewards | MH | Partial (location discovery grants runes) | - | - | - | E-04 covers discovery rewards. |
| RG-169 | Dynamic events (random encounters) | D3, D4 | Missing | RESERVE | Med | World pack | New event system. |
| RG-170 | Bounty board / hunt contracts | MH, D4 (whispers) | Missing (F-07 reserved) | RESERVE | Med | Hunt II (F-07) | Already reserved. |
| RG-171 | Investigation board (targeted monster hunts) | MH | Missing (F-07) | RESERVE | Med | Hunt II (F-07) | Already reserved. |

---

## 7. UI, UX and QoL gaps

| ID | Feature gap | Reference | Runefall status | Priority | Retrofit cost | Destination | Notes |
|---|---|---|---|---|---|---|---|
| RG-172 | Loot filter (F-21) | PoE, LE | Missing | RESERVE | Med | QoL pack (F-21) | Already reserved. |
| RG-173 | Auto-salvage / auto-pickup (F-21) | LE, MH | Missing | RESERVE | Med | QoL pack (F-21) | Already reserved. |
| RG-174 | Colorblind-safe markers (F-24) | - | Missing | RESERVE | Low | QoL pack (F-24) | Already reserved. |
| RG-175 | Text scaling / input remapping (F-24) | - | Missing | RESERVE | Low | QoL pack (F-24) | Already reserved. |
| RG-176 | HUD cooldowns / rarity beams / rank badge animation (F-23) | D3, D4 | Missing | RESERVE | Med | UI pack (F-23) | Already reserved. |
| RG-177 | Tooltip deepening / item comparison metrics (F-22) | D4, LE | Missing | RESERVE | Med | UI pack (F-22) | Already reserved. |
| RG-178 | Diablo-style grid inventory (F-20) | D | Missing | RESERVE | High | UI pack (F-20) | Already reserved. |
| RG-179 | Full character sheet art (F-20) | D | Missing | RESERVE | High | UI pack (F-20) | Already reserved. |
| RG-180 | Loadouts / gear presets (F-13) | D4 | Missing | RESERVE | Med | UI pack (F-13) | Already reserved. |
| RG-181 | Radial menu / quick slots | MH | Covered (Skyrim) | - | - | - | Vanilla. |
| RG-182 | Diagnostic export / debug command (N-08) | - | Covered (planned) | - | - | - | N-08. |
| RG-183 | Localization keys for all strings (N-05) | - | Covered (planned) | - | - | - | N-05. |
| RG-184 | Performance budgets recorded (N-09) | - | Covered (planned) | - | - | - | N-09. |

---

## 8. Social, multiplayer and platform (out of scope)

| ID | Feature gap | Reference | Runefall status | Priority | Notes |
|---|---|---|---|---|---|
| RG-185 | Multiplayer / co-op scaling | MH, D3, D4, PoE | OUT | Console/multiplayer excluded (non-goals). |
| RG-186 | Trading / auction house / Bazaar | PoE, D2, D3 | OUT | Single-player mod; no multiplayer economy. |
| RG-187 | Clan / guild systems | D4, PoE | OUT | Not applicable. |
| RG-188 | Party play / group loot | D, PoE | OUT | Not applicable. |
| RG-189 | PvP (fields of hatred) | D4 | OUT | Not applicable. |
| RG-190 | Console support | D | OUT (F-33) | SKSE excludes consoles. |
| RG-191 | VR support | - | RESERVE (F-31) | CommonLibSSE-NG multi-runtime; needs VR test hardware. |
| RG-192 | Legacy 1.5.97 support | - | RESERVE (F-32) | Not in v1. |
| RG-193 | Skyrim Together / multiplayer safety | - | WATCH (F-34) | Loot/co-save not multiplayer-safe; documented unsupported. |
| RG-194 | Battle pass / monetization | D4 | OUT | Free mod; no monetization. |
| RG-195 | Leaderboards | D3, PoE | OUT | Not applicable to a mod. |

---

## 9. Audio, visual and presentation gaps

| ID | Feature gap | Reference | Runefall status | Priority | Retrofit cost | Destination | Notes |
|---|---|---|---|---|---|---|---|
| RG-196 | Rune pickup sounds / UI SFX (F-25) | D | Missing | RESERVE | Med | Asset pack (F-25) | Already reserved. |
| RG-197 | Unique affix VFX (F-25) | D, MH | Missing | RESERVE | Med | Asset pack (F-25) | Already reserved. |
| RG-198 | Rarity beams / ground glows | D4 | Missing (RG-050) | RESERVE | Low | UI pack | Same as RG-050. |
| RG-199 | Rank badge animation (F-23) | MH | Missing | RESERVE | Med | UI pack (F-23) | Already reserved. |
| RG-200 | Localization packs beyond English (F-26) | - | Missing | RESERVE | Low | Localization packs (F-26) | Already reserved. |
| RG-201 | Expanded asset set / alternate skins (F-27) | - | Missing | RESERVE | Low | Asset pack (F-27) | Already reserved. |

---

## 10. Consolidated priority summary

### NOW (cheap hooks or v1-worthy; require register update before work)
- RG-009 Crit damage multiplier affix.
- RG-024 Item quality tiers.
- RG-031 / RG-080 Enchanting reroll single affix.
- RG-047 Extend smart loot to armor slots.
- (All map to existing systems; no new dependencies.)

### RESERVE (future expansion packs; already cross-referenced where an F-xx exists)
- Progression: RG-001, RG-002, RG-003, RG-004, RG-005, RG-006, RG-007, RG-008, RG-010, RG-011, RG-012, RG-013, RG-014, RG-015, RG-017, RG-019, RG-020, RG-022, RG-023, RG-144, RG-145, RG-146.
- Itemization: RG-025, RG-026, RG-027, RG-028, RG-029, RG-030, RG-032, RG-033, RG-034, RG-035, RG-036, RG-037, RG-038, RG-039, RG-042, RG-043, RG-044, RG-045, RG-048, RG-049, RG-050, RG-051, RG-053, RG-054, RG-056, RG-057, RG-058, RG-059, RG-060, RG-061.
- Crafting/Economy: RG-062, RG-063, RG-064, RG-065, RG-066, RG-067, RG-068, RG-069, RG-070, RG-071, RG-072, RG-073, RG-076, RG-077, RG-078, RG-079, RG-081, RG-082, RG-083, RG-084, RG-085, RG-086.
- Enemy/Encounter: RG-093, RG-094, RG-095, RG-096, RG-097, RG-111, RG-124, RG-125, RG-126, RG-127, RG-128, RG-129, RG-130, RG-131, RG-132, RG-133, RG-134, RG-135, RG-136, RG-137, RG-138, RG-139.
- Endgame: RG-140, RG-141, RG-142, RG-143, RG-147, RG-148, RG-149, RG-150, RG-151, RG-152, RG-153, RG-154, RG-156, RG-157, RG-158.
- Exploration/World: RG-165, RG-169, RG-170, RG-171.
- UI/QoL: RG-172..RG-180 (mostly already F-xx).
- Audio/Visual: RG-196..RG-201 (mostly already F-xx).

### WATCH (risks/metrics, not features)
- RG-016 Attack/cast speed animation safety.
- RG-193 Multiplayer safety (documented unsupported).

### OUT (explicitly out of scope)
- RG-018 Durability (rejected by I-08).
- RG-021 Mounts (vanilla).
- RG-040/RG-041 Cluster/timeless jewels (require passive tree).
- RG-052 Stash (vanilla).
- RG-055 Skill-gem sockets (not applicable).
- RG-074 PoE orb economy (philosophy mismatch; watch only).
- RG-075 Harvest (not applicable).
- RG-098..RG-110, RG-112..RG-116 PoE league mechanics not applicable to a Skyrim mod.
- RG-121, RG-122, RG-123 MH traversal/combat not applicable.
- RG-155, RG-185..RG-195 Social/platform/multiplayer/monetization.

---

## 11. Cross-reference to existing registers

- `GAP-REGISTER-FUTURE.md` F-01..F-35 (reserved gameplay/presentation/compatibility) and N-01..N-10 (cheap hooks) already cover most RESERVED items here. New gaps from this register that are NOT already in that file: RG-004..RG-020, RG-024..RG-041, RG-044..RG-060, RG-062..RG-086, RG-093..RG-097, RG-111, RG-119, RG-120, RG-125..RG-139, RG-141..RG-143, RG-146..RG-149, RG-153, RG-154, RG-156..RG-158, RG-165, RG-169. These should be appended to `GAP-REGISTER-FUTURE.md` section 1/2/3 when the user wants them tracked there (D-047 requires the register to be the single source of truth). Until then, this file is the authoritative reference-game gap source.
- Decision codes and rules (D-047, D-048) apply unchanged: RESERVE never enters v1 code; only hooks (N-01..N-10 and the NOW items above) may.

## 12. Recommended next actions (not implementation)

1. Review the NOW list (5 items) and decide which enter v1; if any do, add them to `GAP-REGISTER-FUTURE.md` as new N-xx hooks and update L1/L3.
2. Decide whether to merge this register into `GAP-REGISTER-FUTURE.md` (single source of truth) or keep it as a companion. Recommendation: merge the RESERVED items as new F-xx rows with cross-reference RG- IDs, keeping this file as the detailed provenance.
3. For each RESERVED item that turns out to need a v1 hook, update section 4 (cheap hooks) of `GAP-REGISTER-FUTURE.md` and D-048 before any work (D-047 rule 4).
4. Do not let reference-game parity drive scope creep: the v1 scope lock (D-047) stands; this register is a backlog, not a commitment.
