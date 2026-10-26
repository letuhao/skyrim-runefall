# Reference-game feature gap register (Skyrim mod framing)

Status: 2026-10-26 (revised: mod-appropriate framing). Companion to
`SPECS\GAP-REGISTER-FUTURE.md` (v1 scope lock).

## 0. Framing correction (read first)

Runefall is a **Skyrim SE/AE mod**, not a standalone game. The four reference series
(Monster Hunter, Last Epoch, Path of Exile, Diablo) are **inspiration sources**, not a
feature checklist. Comparing a mod to a full standalone ARPG and treating every missing
feature as a "gap" is wrong: many of those features are game-level systems (multiplayer,
procedural dungeons, leagues, seasons, battle pass, leaderboards, mount riding, wirebug
movement) that do not belong in a Skyrim mod and must not be promised.

This register therefore lists only gaps that are **(a) relevant to a Skyrim mod** and
**(b) realistically implementable within Skyrim's engine** (SKSE + CommonLibSSE-NG + ESL
content + Papyrus + co-save). Game-level features are recorded once in Section 8 as
"not applicable to a mod" and are never tracked as expansion candidates.

Rules (unchanged from D-047/D-048):
- Every gap gets an ID (`RG-xxx`), a decision code (`NOW` / `RESERVE` / `WATCH` / `OUT`),
  a priority, and a destination pack or "not applicable".
- `NOW` items are cheap hooks or v1-worthy features; they may enter v1 only after updating
  this register and `GAP-REGISTER-FUTURE.md`.
- `RESERVE` items are deliberately out of v1 and go to a named expansion pack.
- `WATCH` items are risks or metrics, not features.
- `OUT` items are out of scope (engine limitation, or not a mod feature) and are recorded so
  the decision is explicit.
- A gap already covered by an existing `F-xx` / `N-xx` ID is cross-referenced, not duplicated.
- Reference games are cited by family: **MH** = Monster Hunter, **LE** = Last Epoch,
  **PoE** = Path of Exile, **D** = Diablo series.

---

## 1. Runefall v1 feature baseline (already covered)

| System | What Runefall v1 already has |
|---|---|
| Progression | 9 attributes (3 builds), 18 skill lines x 3 ranks, 2 attr + 1 skill point/level, rune-bought points, respec, soft/hard caps, RF tiers 0-5 with attunement. |
| Items | 6 rarity bands (0-5), rarity = enhancement count, 5 enhancement categories, affix CSV tables, sockets (rune rank), item power score, durability (optional module, ships), no bind. |
| Loot | Drop sources, ilvl from source, roll pipeline, drop chance, rarity weights, magic find, pity/dry-streak, smart drops (weapons), anti-abuse (container roll-once), general drop table, race hunt tables (19 families), hunt uniques (59). |
| Enemy tiers | Ranks 1-5, 10 monster affixes, scaling multipliers, assignment at spawn, density caps, bosses per tier, attunement gate. |
| Crafting | Rune Forge: salvage, socket/extract, reroll (all + locked), rarity upgrade (60% success), rune conversion, material hunting. Crafting Table reserved. |
| Materials | 4 tiers (Common/Refined/Exotic/Mythic), 3 families each, salvage yield, node yields. |
| Economy | Single Rune currency R1-R5 (1/5/25/125/625), event-driven (no clock), sources/sinks, conversion, anti-inflation. |
| UI | SKSE Menu Framework pages (Character, Rune Forge, Hunt Codex, Item Detail), hotkeys, Papyrus bridge, localization keys. |
| Runtime | Native SKSE DLL, ESL content, multi-pack (D-040), co-save TLV, diagnostics, performance budgets, uninstall matrix. |
| Compatibility | Additive loot, no vanilla overrides, caps, INI multipliers, optional UI/Patches/Takeover modules. |

---

## 2. Progression gaps (mod-relevant only)

| ID | Feature gap | Reference | Status | Priority | Destination | Notes |
|---|---|---|---|---|---|---|
| RG-009 | Critical strike damage multiplier stat (chance exists; damage multiplier missing) | D, LE, PoE | Partial | NOW (cheap) | Core affixes | Add crit damage multiplier affix + item power contribution. |
| RG-013 | Cooldown recovery / reduction as a stat (beyond Willpower) | D3, LE | Partial | RESERVE | Affix pack | Already an attribute effect; needs affix + cap. |
| RG-014 | Resource cost reduction (magicka/stamina cost) | D3, PoE | Missing | RESERVE | Affix pack | New affix family. |
| RG-015 | Movement speed as a tunable stat with caps | D3, PoE | Partial | RESERVE | Affix pack | Needs cap to avoid physics break (L2 open question). |
| RG-016 | Attack/cast speed scaling (animation-safe) | D, PoE | Partial | WATCH | - | L2 open question; must not break animations. |
| RG-022 | Pet / companion scaling and hunt bonuses | MH, D3 | Missing (F-16) | RESERVE | Companion pack (F-16) | Already reserved. |
| RG-004 | Damage type system with conversions (physical/fire/cold/lightning/poison/shadow) | D, LE, PoE | Missing | RESERVE | Affix/Effect pack | Enhancement "Elemental" is a category, not a typed damage system. |
| RG-005 | Ailment system (bleed, burn, chill, freeze, shock, poison, stun, slow) with stacking | D, LE, PoE, MH | Missing (poison only) | RESERVE | Combat pack | Needs ailment registry, stacking caps, cleanse. |
| RG-006 | Penetration / resistance system (resistances with caps and penetration stats) | D, LE, PoE | Missing | RESERVE | Combat pack | Vanilla resistances exist; our bonuses are percent. |
| RG-007 | Ward / energy shield (secondary regenerating health pool) | LE, PoE | Missing | RESERVE | Combat pack | New resource; save/UI impact. |
| RG-008 | Block, dodge, evasion as distinct defensive layers | MH, PoE, D3 | Partial | RESERVE | Combat pack | Could be a defensive affix family. |
| RG-010 | Leech (life/mana/rune on hit) | D, PoE, LE | Missing | RESERVE | Affix pack | New affix family; needs hit hook. |
| RG-011 | Thorns / reflect damage | D, PoE | Missing | RESERVE | Affix pack | New affix family. |
| RG-012 | Area of effect scaling and AoE-specific affixes | D, PoE, LE | Missing | RESERVE | Affix pack | New affix family. |
| RG-017 | Charges / stacks on items and abilities | PoE, D3 | Missing | RESERVE | Item pack | New item state; co-save impact. |
| RG-023 | Damage conversion (physical to elemental etc.) | PoE, LE | Missing | RESERVE | Affix pack | Needs RG-004 first. |
| RG-024b | Rune XP and Rune Level as real actor values (via Actor Value Generator) | Runefall (new) | Missing | RESERVE | Progression pack | AVG creates new AVs usable in MGEF/conditions/scripts. Enables rune progression as a native AV instead of co-save-only. |
| RG-024c | Rune progression system (level up via rune XP, granting perks/points) | Runefall (new) | Missing | RESERVE | Progression pack | Builds on RG-024b; new progression axis beyond character level and RF tier. |

Dropped as not mod-relevant: mastery/ascendancy (RG-001), combo system (RG-019), perfect
dodge/guard (RG-020), mount combat (RG-021).

Now shippable (design decision changed):
- RG-002 Per-skill skill trees: ships via Custom Skills Framework (41780). RESERVE.
- RG-003 Passive skill tree: ships via Custom Skills Framework (41780). RESERVE.
- RG-018 Durability and break: ships as optional module via EquipmentDurabilitySystem NG
  (109899). RESERVE (optional module, not core).

---

## 3. Itemization and loot gaps (mod-relevant only)

| ID | Feature gap | Reference | Status | Priority | Destination | Notes |
|---|---|---|---|---|---|---|
| RG-024 | Item quality tiers (0-20% base stat multiplier) | PoE, D3 | Missing | NOW (cheap) | Core items | Quality as small multiplier on base stats; stored in metadata (N-06). |
| RG-031 | Enchanting reroll single affix (instead of all) | D4, D3 | Partial (reroll all; locked 3x) | NOW (cheap) | Rune Forge | Add "reroll one slot" action cheaper than full reroll. |
| RG-047 | Extend smart loot to armor slots (weapons only today) | D3 | Partial (L-07) | NOW (cheap) | Core loot | Extend L-07 to armor; toggle exists. |
| RG-042 | Unique items with fixed legendary affixes (remaining mined candidates) | D, PoE, LE | Partial (40 exist) | RESERVE | Unique II (F-09) | 11 boss + 28 quest candidates reserved. |
| RG-043 | Set items and set bonuses (2-piece, 4-piece) | D, PoE, LE, MH | Missing (F-04) | RESERVE | Set pack (F-04) | Already reserved. |
| RG-044 | Mythic uniques (extremely rare, build-defining) | D4 | Missing | RESERVE | Unique pack | New rarity flag above Runeforged or separate flag. |
| RG-045 | Ancient / primordial / primal (perfect or near-perfect rolls) | D3 | Missing | RESERVE | Item pack | Drop flag + roll logic. |
| RG-025 | Implicit affixes on base items | PoE, D, LE | Missing | RESERVE | Item pack | New field in item base data. |
| RG-026 | Corrupted items (powerful implicit, locked, unmodifiable) | PoE, D3 | Missing | RESERVE | Item pack | Needs corrupt action + lock flag. |
| RG-027 | Influenced items (special affix pool by influence tag) | PoE | Missing | RESERVE | Item pack | New affix pools per influence. |
| RG-028 | Fractured / synthesised items | PoE | Missing | RESERVE | Item pack | Special base items. |
| RG-029 | Veiled mods (hidden affix, choose of 3 on unveil) | PoE | Missing | RESERVE | Item pack | Needs unveil UI + NPC. |
| RG-030 | Anointments (oil-based affix on amulet/ring) | PoE (blight) | Missing | RESERVE | Item pack | New consumable + slot. |
| RG-032 | Tempering (add specific affix from a manual, limited uses) | D4 | Missing | RESERVE | Crafting pack | New manual item + use counter. |
| RG-033 | Masterworking (incremental affix value upgrades, crit masterwork) | D4 | Missing | RESERVE | Crafting pack | Needs upgrade ranks + crit roll. |
| RG-034 | Imprinting (copy an item's affixes) | D4 | Missing | RESERVE | Crafting pack | Needs item copy action. |
| RG-035 | Runewords (specific rune sequence in sockets = named effect) | D2, D4 | Missing (F-03) | RESERVE | Rune pack (F-03) | Strong fantasy fit for a "Rune" mod. |
| RG-036 | Aspect extraction / codex of power | D4 | Missing | RESERVE | Unique pack | Needs aspect registry + apply action. |
| RG-037 | Legendary gems with ranks (socketed gem that levels) | D3 | Missing (F-03) | RESERVE | Rune pack (F-03) | New item class + leveling. |
| RG-038 | Threshold jewels (conditional bonus) | PoE | Missing | RESERVE | Item pack | Conditional affix. |
| RG-039 | Abyss jewels (special mods in special sockets) | PoE | Missing | RESERVE | Item pack | New socket type. |
| RG-055b | Imbue feature (craft a material onto equipment for better stats, chance to break) | Runefall (new, replaces sockets) | Missing | RESERVE | Crafting pack | Alternative to socket-gem/runeword system, which is impossible in Skyrim. Material + equipment → stat boost with break risk. Uses Rune Forge + salvage + material tiers. |
| RG-048 | Loot filter (rules-based hide/show/color/sound/icon) | PoE, LE | Missing (F-21) | RESERVE | QoL pack (F-21) | Already reserved; high value. |
| RG-049 | Auto-salvage / auto-pickup rules | LE, MH | Missing (F-21) | RESERVE | QoL pack (F-21) | Already reserved. |
| RG-050 | Loot beams / ground markers for rarity | D4, PoE | Partial (optional color/marker) | RESERVE | UI pack | Visual polish. |
| RG-051 | Item comparison (DPS/defense score, detailed diff) | D4, LE | Missing (F-22) | RESERVE | UI pack (F-22) | Already reserved. |
| RG-053 | Armory / build loadouts (save and swap gear+skills) | D4 | Missing (F-13) | RESERVE | UI pack (F-13) | Already reserved. |
| RG-054 | Transmog / layered armor / wardrobe | MH, D4, D3 | Missing (F-20) | RESERVE | UI/Asset pack (F-20) | Already reserved. |
| RG-056 | Quality upgrades on base items (deterministic defense upgrade path) | MH | Partial (material hunting) | RESERVE | Crafting pack | MH-style armor spheres. |
| RG-057 | Treasure goblins / special loot carriers | D3 | Missing | RESERVE | Enemy pack | New NPC type with special drop table. |
| RG-058 | Strongboxes / ambush containers with mods | PoE | Missing | RESERVE | Container pack | New container type. |
| RG-059 | Cursed chests / shrines (risk/reward) | D3 | Missing | RESERVE | World pack | New world object. |
| RG-060 | Pylons (temporary powerful buffs in dungeons) | D3 | Missing | RESERVE | World pack | New world object. |
| RG-061 | Boss-specific uniques | D, PoE | Partial (boss uniques exist) | - | - | Covered by D-041. |

Dropped as not mod-relevant: stash tabs (vanilla inventory, RG-052), cluster/timeless
jewels (RG-040/RG-041, require passive tree — now shippable via Custom Skills Framework
but still deferred).

Socket-gem links/colors (RG-055) is impossible in Skyrim; replaced by the Imbue feature
(RG-055b), which crafts a material onto equipment for better stats with a chance to break.

---

## 4. Crafting and economy gaps (mod-relevant only)

| ID | Feature gap | Reference | Status | Priority | Destination | Notes |
|---|---|---|---|---|---|---|
| RG-062 | Forging potential / crafting resource consumed per craft (risk of fracture) | LE | Missing | RESERVE | Crafting pack | Adds risk layer to crafting. |
| RG-063 | Fracture on failed craft (item breaks or loses affix) | LE | Missing | RESERVE | Crafting pack | Needs failure outcome design. |
| RG-064 | Glyphs that modify crafting outcomes | LE | Missing | RESERVE | Crafting pack | New consumable items. |
| RG-065 | Runes of shattering/extraction/ascendance/refinement | LE | Partial (reroll + conversion) | RESERVE | Crafting pack | Map to Rune Forge v2. |
| RG-066 | Affix shards (extracted affix as currency, targeted crafting) | LE, PoE | Missing (C-09) | RESERVE | Crafting Table (F-01) | Already reserved in C-09. |
| RG-067 | Targeted affix crafting (choose base + one affix family) | LE, D | Missing (C-09) | RESERVE | Crafting Table (F-01) | Already reserved. |
| RG-068 | Deterministic base-item crafting (base item + materials = item) | MH, LE, D | Missing (C-09) | RESERVE | Crafting Table (F-01) | Already reserved. |
| RG-069 | Socket-add recipes (add a socket to an item) | LE, PoE | Missing (C-09) | RESERVE | Crafting Table (F-01) | Already reserved. |
| RG-070 | Material recipe chains (tier up/down, family conversion) | MH, LE | Missing (C-01 basic) | RESERVE | Crafting pack | New recipes. |
| RG-071 | Legendary potential (unique absorbs an exalted item's affixes) | LE | Missing | RESERVE | Unique pack | Needs unique + exalted merge. |
| RG-072 | Exalted items (tier 6/7 affixes above normal) | LE | Missing | RESERVE | Item pack | New rarity/affix tier. |
| RG-073 | Sealed affixes (lock an affix permanently so it survives crafts) | LE | Missing | RESERVE | Crafting pack | New affix state. |
| RG-076 | Fossils / resonators (modify affix pools) | PoE | Missing | RESERVE | Crafting pack | New consumable. |
| RG-077 | Essences (guaranteed affix on craft) | PoE | Missing | RESERVE | Crafting pack | New consumable. |
| RG-078 | Recombinators (combine two items' affixes) | PoE | Missing (F-02) | RESERVE | Crafting II (F-02) | Already reserved. |
| RG-079 | Affix transfer / extraction between items | LE, PoE | Missing (F-02) | RESERVE | Crafting II (F-02) | Already reserved. |
| RG-082 | Gambling (spend currency for random item of a type) | D3, D4 | Missing | RESERVE | Economy pack | New vendor action. |
| RG-083 | Vendor recipes / Horadric cube (combine items into new items) | D2 | Missing | RESERVE | Crafting pack | New recipe system. |
| RG-084 | Transmutation (3 items to 1 of higher tier) | D2, F-17 | Missing (F-17) | RESERVE | Economy pack (F-17) | Already reserved. |
| RG-085 | Rune gambling vendor | F-17 | Missing | RESERVE | Economy pack (F-17) | Already reserved. |
| RG-086 | Salvage returning affix shards | LE | Missing (v1 salvage = materials + runes only) | RESERVE | Crafting Table (F-01) | C-02 explicitly defers shard extraction. |
| RG-087 | Node respawn timers and hunting loop | MH, LE | Covered (C-07) | - | - | - |
| RG-088 | Tiered node yields by area | MH | Covered (C-07) | - | - | - |
| RG-089 | Material families (ore/essence/hide) | MH | Covered (C-01) | - | - | - |

Dropped as not mod-relevant: PoE orb economy (RG-074, philosophy mismatch, watch only),
harvest/horticrafting (RG-075, not applicable to Skyrim loop).

---

## 5. Enemy, encounter and boss gaps (mod-relevant only)

Constraint (user-directed 2026-10-27): **No world changes.** Runefall must not add new
worldspaces, cells, world bosses, helltide events, strongholds, or dynamic world events.
Skyrim already has many world-change mods; Runefall's job is to modify EXISTING enemies
into many variants for farming, with new perks/skills making them harder to fight, for
better rewards when hunted. This is essentially a monster-variant system (like MH's
variant/rising monsters or D3's elite affixes) built on top of Skyrim's existing NPC
records via runtime perk/spell package application (Fortunate Foes proves this pattern
with SPID/SkyPatcher).

| ID | Feature gap | Reference | Status | Priority | Destination | Notes |
|---|---|---|---|---|---|---|
| RG-093 | Boss phases and arena mechanics | MH, D | Missing (F-14) | RESERVE | Boss pack II (F-14) | Already reserved. |
| RG-094 | Encounter modifiers (extra monsters, elemental hazards) | PoE, D4 | Missing | RESERVE | Dungeon pack | New modifier system. |
| RG-095 | Dungeon affixes / sigils (nightmare dungeons with modifiers) | D4, PoE | Missing | RESERVE | Dungeon pack | New dungeon system. |
| RG-111 | Essence monsters (special enemies dropping special materials) | PoE | Missing | RESERVE | Enemy pack | Could be "rune-essence" enemies. |
| RG-119 | Rampage (kill streak bonuses) | PoE | Missing | RESERVE | Combat pack | New mechanic. |
| RG-120 | Onslaught (speed bonus) | PoE | Missing | RESERVE | Combat pack | New mechanic. |
| RG-124 | Investigations / tempered monsters | MH | Partial (RF tiers + boss hunts) | RESERVE | Hunt II (F-07) | F-07 hunt contracts cover targeted hunts. |
| RG-125 | Variant / risen / frenzied / afflicted monsters | MH | Missing | RESERVE | Enemy pack | New monster variants. |
| RG-127 | Siege monsters | MH | Missing | RESERVE | Boss pack | New encounter type. |
| RG-128 | Arena challenges | MH, D3 | Missing | OUT (world change) | - | No new cells; Skyrim has arena mods. |
| RG-129 | World bosses (rotating) | D4, MH | Missing | OUT (world change) | - | No new world bosses; modify existing bosses instead. |
| RG-130 | Legion events (mass spawns) | D4 | Missing | OUT (world change) | - | No new world events. |
| RG-131 | Helltide (area event with chests/cinders) | D4 | Missing | OUT (world change) | - | No new world events. |
| RG-132 | Tree of whispers (bounty board to caches) | D4 | Missing (F-07) | OUT (world change) | - | Hunt contracts are a menu, not a world system. |
| RG-133 | Strongholds (capture for renown) | D4 | Missing | OUT (world change) | - | No new world content. |
| RG-134 | Endemic life buffs (environmental buffs from creatures) | MH | Missing | RESERVE | World pack | New world objects. |
| RG-135 | Environmental traps (boulders, vines, flash bugs) | MH | Missing | RESERVE | World pack | New world objects. |
| RG-136 | Cursed chests / shrines (risk/reward) | D3 | Missing | OUT (world change) | - | No new world objects. |
| RG-137 | Frenzy virus (infection mechanic) | MH | Missing | RESERVE | Enemy pack | New status. |
| RG-138 | Blights (elemental blight ailments) | MH | Missing | RESERVE | Enemy pack | New ailment. |
| RG-139 | Status ailments (poison/paralysis/sleep/stun/exhaust/blast) | MH | Partial (poison) | RESERVE | Combat pack | Needs ailment system (RG-005). |
| RG-125b | Enemy variant system (modify existing NPCs into many variants with new perk/skill packages for harder fights and better rewards) | MH, D3 | Missing | RESERVE | Enemy pack | Core farming loop: take existing NPC, apply perk/spell package (Fortunate Foes pattern via SPID/SkyPatcher), create N variants with increasing difficulty and reward. No new NPCs or world changes. |
| RG-125c | Enemy variant reward scaling (better loot/runes/materials for harder variants) | MH, D | Missing | RESERVE | Enemy pack | Ties into loot/rune/material systems; harder variant = better drops. |
| RG-125d | Enemy variant naming/tagging (visual indicator of variant type) | MH, D3 | Missing | RESERVE | Enemy pack | Name prefix/suffix + optional badge. |
| RG-090 | Monster affixes visible on name | MH, D3 | Covered (T-03) | - | - | - |
| RG-091 | Champion packs with visible affixes | Torchlight, D3 | Covered (T-03) | - | - | - |
| RG-092 | Monster rank scaling | D, MH | Covered (T-04, A5) | - | - | - |

Dropped as not mod-relevant: all PoE league mechanics (heist, expedition, ritual, ultimatum,
sanctum, blight, metamorph, incursion, betrayal, legion, breach, abyss, harbinger, essence,
beyond, torment, bloodlines, domination, invasion, nemesis, ambush, rampage as a league,
onslaught as a league), turf wars, mounting monsters, wirebug movement, anomaly quests as
procedural, siege as procedural.

---

## 6. Endgame and scaling gaps (mod-relevant only)

| ID | Feature gap | Reference | Status | Priority | Destination | Notes |
|---|---|---|---|---|---|---|
| RG-140 | Infinite scaling beyond tier 5 (corruption / torment beyond 5) | PoE, D3, D4, LE | Missing (F-11) | RESERVE | Endgame pack (F-11) | Already reserved. |
| RG-142 | Pinnacle bosses (uber variants) | PoE, D3, D4 | Missing | RESERVE | Boss pack | New bosses. |
| RG-143 | Torment difficulty tiers (I-VI) | D3, D4 | Missing (RF 0-5 only) | RESERVE | Endgame pack | Could extend RF tiers to 10+. |
| RG-146 | Renown (account-wide unlocks: skill points, potion charges) | D4 | Missing | RESERVE | Progression pack | New progression layer. |
| RG-147 | Pantheon (god blessings, major/minor) | PoE | Missing | RESERVE | World pack | New buff system. |
| RG-148 | Bandit choice (quest choice with permanent reward) | PoE | Missing | RESERVE | Quest pack | New quest pattern. |
| RG-149 | Prophecies (random future events with rewards) | PoE | Missing | RESERVE | World pack | New system. |
| RG-150 | Seasons / seasonal realm (reset + season mechanic) | D3, D4, PoE, LE | Missing (F-12) | RESERVE | Modes pack (F-12) | Already reserved. |
| RG-151 | Season journey / conquests / challenges | D3, D4 | Missing | RESERVE | Modes pack | Already reserved. |
| RG-152 | Hardcore mode (death = delete) | D2, D3, D4, PoE | Missing (F-12) | RESERVE | Modes pack (F-12) | Already reserved. |
| RG-153 | SSF / self-found mode | PoE | Missing | RESERVE | Modes pack | New mode flag. |
| RG-154 | Ruthless mode (extreme scarcity) | PoE | Missing | RESERVE | Modes pack | New mode flag. |
| RG-156 | Challenge rifts / trial | D3 | Missing | RESERVE | World pack | New content. |
| RG-157 | Set dungeons | D3 | Missing | RESERVE | World pack | New content. |
| RG-158 | Greater rift keystone / trial | D3 | Missing | RESERVE | World pack | New item. |

Dropped as not mod-relevant: leaderboards (RG-155), atlas/map device (RG-096), delve
(RG-097), paragon boards as a full system (RG-144/145) — too game-level; attributes
already serve as progression.

---

## 7. Exploration and world gaps (mod-relevant only)

| ID | Feature gap | Reference | Status | Priority | Destination | Notes |
|---|---|---|---|---|---|---|
| RG-165 | Bug catching / endemic life collection | MH | Missing | OUT (world change) | - | No new world objects. |
| RG-169 | Dynamic events (random encounters) | D3, D4 | Missing | OUT (world change) | - | No new world events. |
| RG-170 | Bounty board / hunt contracts | MH, D4 | Missing (F-07) | OUT (world change) | - | No new world systems; hunt contracts are a menu, not a world system. |
| RG-171 | Investigation board (targeted monster hunts) | MH | Missing (F-07) | OUT (world change) | - | No new world systems. |
| RG-134 | Endemic life buffs | MH | Missing | OUT (world change) | - | No new world objects. |
| RG-135 | Environmental traps | MH | Missing | OUT (world change) | - | No new world objects. |

Dropped as not mod-relevant / vanilla-covered: waypoints, town portals, camps, gathering,
fishing, mining, photo mode, gestures, map discovery rewards (all vanilla or already
covered by E-04).

---

## 8. Not applicable to a Skyrim mod (OUT)

These are game-level or platform features. They are recorded so the decision is explicit,
not accidental. They must never be promised on the Nexus page.

- Multiplayer / co-op scaling, trading / auction house, clan / guild systems, party play /
  group loot, PvP (D, PoE, MH).
- Console support (SKSE requirement), VR (reserved separately, F-31), legacy 1.5.97
  (F-32), Skyrim Together safety (F-34).
- Battle pass / monetization, leaderboards.
- Mounts and mount-based combat, monster mounting, wirebug movement, turf wars
  (animation/engine limitations).
- Procedural world generation: map device / Atlas, Delve, Heist, Expedition, Ritual,
  Ultimatum, Sanctum, Metamorph, Incursion, Betrayal, Legion, Breach, Abyss, Harbinger,
  Beyond, Torment, Bloodlines, Domination, Invasion, anomaly quests as procedural, endless
  dungeon / greater rift / pit, siege as procedural.
- (Moved to 8.1: these are buildable, not engine-limited; rejected/deferred by design.)
- Stash tabs / sort (vanilla inventory), armory (could be built, tracked as B).
- Weapon sharpness (rejected by I-08; durability now ships as optional module — see 8.2).

### 8.1 Implementable but rejected by design (not engine limitations)

These are shippable (proven by existing mods) but Runefall deliberately excludes them by
design decision. Recorded so the rejection is explicit and reversible later:

- **Native flask/charge system**: buildable as custom item state + co-save, but potions
  already exist in Skyrim; rejected to avoid duplicating vanilla.
- **Native socketed-gem links/colors**: impossible in Skyrim (RG-055). Replaced by the
  **Imbue** feature (RG-055b): craft a material onto equipment for better stats with a
  chance to break — an alternative to sockets/runewords. See Section 3.
- **Stash tabs / sort**: buildable with Grid Inventory as optional dep; deferred.
- **Armory / loadouts**: buildable (tracked as B, RG-053).

### 8.2 Now shipping (design decision changed)

These were previously rejected by I-08 / deferred, but the user has directed they will ship:

- **Durability and break system** (RG-018): ships as an optional module, implemented via
  EquipmentDurabilitySystem NG (109899) patterns. I-08 is amended: durability is now an
  optional module, not a hard non-goal. Equipment can break; repair via forge/materials.
- **Passive tree / skill tree system** (RG-003, RG-002): ships via Custom Skills Framework
  (41780). New custom skills with their own trees, not limited to the 18 built-in lines.
- **Ailment / damage-type / penetration framework** (RG-004, RG-005, RG-006, RG-023):
  will be designed and shipped later (Combat pack). Elemental Reactions Framework (163378)
  proves the pattern; AVG provides the AVs.

---

## 9. UI, UX and QoL gaps (mod-relevant only)

| ID | Feature gap | Reference | Status | Priority | Destination | Notes |
|---|---|---|---|---|---|---|
| RG-172 | Loot filter | PoE, LE | Missing (F-21) | RESERVE | QoL pack (F-21) | Already reserved. |
| RG-173 | Auto-salvage / auto-pickup | LE, MH | Missing (F-21) | RESERVE | QoL pack (F-21) | Already reserved. |
| RG-174 | Colorblind-safe markers | - | Missing (F-24) | RESERVE | QoL pack (F-24) | Already reserved. |
| RG-175 | Text scaling / input remapping | - | Missing (F-24) | RESERVE | QoL pack (F-24) | Already reserved. |
| RG-176 | HUD cooldowns / rarity beams / rank badge animation | D3, D4 | Missing (F-23) | RESERVE | UI pack (F-23) | Already reserved. |
| RG-177 | Tooltip deepening / item comparison metrics | D4, LE | Missing (F-22) | RESERVE | UI pack (F-22) | Already reserved. |
| RG-178 | Diablo-style grid inventory | D | Missing (F-20) | RESERVE | UI pack (F-20) | Requires Grid Inventory mod (optional dep). |
| RG-179 | Full character sheet art | D | Missing (F-20) | RESERVE | UI pack (F-20) | Already reserved. |
| RG-180 | Loadouts / gear presets | D4 | Missing (F-13) | RESERVE | UI pack (F-13) | Already reserved. |

Dropped: radial menu (vanilla/SkyUI), diagnostic export/localization/performance budgets
(already planned N-05/N-08/N-09).

---

## 10. Audio, visual and presentation gaps (mod-relevant only)

| ID | Feature gap | Reference | Status | Priority | Destination | Notes |
|---|---|---|---|---|---|---|
| RG-196 | Rune pickup sounds / UI SFX | D | Missing (F-25) | RESERVE | Asset pack (F-25) | Already reserved. |
| RG-197 | Unique affix VFX | D, MH | Missing (F-25) | RESERVE | Asset pack (F-25) | Already reserved. |
| RG-198 | Rarity beams / ground glows | D4 | Missing (RG-050) | RESERVE | UI pack | Visual polish. |
| RG-199 | Rank badge animation | MH | Missing (F-23) | RESERVE | UI pack (F-23) | Already reserved. |
| RG-200 | Localization packs beyond English | - | Missing (F-26) | RESERVE | Localization packs (F-26) | Already reserved. |
| RG-201 | Expanded asset set / alternate skins | - | Missing (F-27) | RESERVE | Asset pack (F-27) | Already reserved. |

---

## 11. Consolidated priority summary

### NOW (cheap, v1-worthy; require register update before work)
- RG-009 Crit damage multiplier affix.
- RG-024 Item quality tiers.
- RG-031 Enchanting reroll single affix.
- RG-047 Extend smart loot to armor slots.

### NOW SHIPPABLE (design decision changed; not cheap hooks but now in scope)
- RG-018 Durability and break system (optional module, via EquipmentDurabilitySystem NG).
- RG-002 / RG-003 Passive & per-skill trees (via Custom Skills Framework).
- RG-004 / RG-005 / RG-006 / RG-023 Ailment & damage-type framework (ship later, Combat pack).
- RG-055b Imbue feature (material + equipment → stat boost, break risk; replaces socket-gem).

### RESERVE (future expansion packs; cross-referenced to F-xx where applicable)
All entries in sections 2-7 and 9-10 marked RESERVE. Each requires a dedicated pack and
a budget decision before work.

### WATCH
- RG-016 Attack/cast speed animation safety.

### OUT (engine limitation or not a mod feature)
Section 8 plus any item explicitly marked dropped in sections 2-7.

---

## 12. Cross-reference to existing registers

- `GAP-REGISTER-FUTURE.md` F-01..F-35 and N-01..N-10 already cover most RESERVED items here.
  New gaps from this register not already tracked there: RG-004, RG-005, RG-006, RG-007,
  RG-008, RG-009, RG-010, RG-011, RG-012, RG-013, RG-014, RG-015, RG-017, RG-023,
  RG-024b, RG-024c, RG-055b,
  RG-024, RG-025..RG-045, RG-048..RG-060, RG-062..RG-086, RG-093..RG-095, RG-111,
  RG-119, RG-120, RG-125, RG-127, RG-128, RG-129, RG-130, RG-131, RG-133, RG-136,
  RG-137, RG-138, RG-139, RG-140, RG-142, RG-143, RG-146..RG-149, RG-153, RG-154,
  RG-156..RG-158, RG-165, RG-169, RG-172..RG-180, RG-196..RG-201.
  These should be appended to `GAP-REGISTER-FUTURE.md` when the user wants them tracked there
  (D-047 requires the register to be the single source of truth). Until then, this file is
  the authoritative reference-game gap source.
- Decision codes and rules (D-047, D-048) apply unchanged.
- Section 15 adds 45 new gaps (RG-202..RG-246) from Grim Dawn, Titan Quest, Wolcen,
  Lost Ark, Torchlight, Borderlands, Risk of Rain 2, and Hades. These are tracked here
  until merged into `GAP-REGISTER-FUTURE.md`.

---

## 13. Skyrim framework feasibility (context from rpg-mechanism)

Context basis: `_context\rpg-mechanism\` (00-INDEX through 10-PROTEUS) and the Runefall
specs. The constraints below are what the Skyrim SE/AE engine, SKSE, Papyrus, and the
CommonLibSSE-NG runtime actually allow.

### 13.0 Frameworks in rpg-mechanism that EXPAND what is implementable

The original feasibility audit understated Skyrim's capability. Several installed mods prove
that features previously marked "not feasible" are in fact shippable:

| Framework mod | What it proves | Features it unlocks |
|---|---|---|
| **Actor Value Generator (AVG)** 84743 (1.9.5.6 / 2.3.6.0) | Creates brand-new actor values usable in MGEF, conditions, scripts, console commands. Ships `ActorValueExtension.dll` + TOML data. | Real AVs for Rune XP, Rune Level, Rune Progression, poise, etc. — no longer limited to co-save-only counters. Attribute effects can target native AVs. |
| **EquipmentDurabilitySystem NG** 109899 (5.1) | Working durability/break system in Skyrim SE/AE. | Durability and break system is implementable (was wrongly marked OUT by engine limitation). Runefall still rejects it by design (I-08), but it is not an engine blocker. |
| **SKSE Menu Framework** (installed in target list) | ImGui-based native menus from C++. | Custom menus for crafting, salvage, gambling/gacha, tier upgrade, material hunting — all the "new menu + DLL" features the user listed. |
| **CommonLibSSE-NG** (alandtse fork) | Multi-runtime DLL, hooks, task interface, form factory. | Runtime game-form creation, main-thread task enqueueing, save data, co-save. |
| **Dynamic Persistent Forms** 116001 | Runtime form creation that persists. | Supports runtime item creation for reroll/tier-upgrade flows. |
| **Dynamic Perk Framework** 189660 | Runtime perk granting/removal. | Supports dynamic perk assignment for enemy affixes and progression. |
| **Elemental Reactions Framework** 163378 | Elemental reaction system. | Ailment/damage-type system is buildable. |
| **Combo System** 181633 | Combo mechanics. | Combo system is buildable. |
| **TrueHUD** 62775 | HUD overlays. | HUD cooldowns, rarity beams, rank badges. |
| **Prisma UI** 148718 | Web-based UI. | Alternative UI surface. |
| **Custom Skills Framework** 41780 | Custom skill trees in Skyrim. | Passive tree / skill tree system is shippable (was wrongly marked as needing a full ImGui build). Enables RG-003 (passive skill tree) and RG-002 (per-skill trees) as real custom skills. |

Consequence: features previously classified as C (not feasible) because they needed "new
actor values", "new menus", "new item state", or "new durability" are in fact **B (hard but
possible)** or **A (implementable)**. The only true C items are those violating hard engine
constraints (procedural world generation, multiplayer, console, animation-engine features).

### 13.1 Hard constraints

1. **No vanilla/shared base-record mutation for gameplay state.** Per-instance state must
   live in extra data + co-save (D-023, D-027, D-028).
2. **No leveled-list, container, or cell edits in core.** Loot and ranks are applied at
   runtime by the DLL (D-004, D-024, D-025).
3. **No vanilla actor-value repurposing.** Worlds Dawn's 28-AV technique is rejected (D-006).
4. **Forms created at runtime are not persisted.** Dynamic enchantment + co-save metadata
   is the only safe route (D-023).
5. **No per-frame gameplay work.** Budgets are per-hook (L-04, D-055).
6. **No procedural world generation.** Skyrim's worldspace/cell graph is authored at build
   time; the engine cannot generate new worldspaces, cells, or navmesh at runtime.
7. **No multiplayer, trading, clans, PvP, leaderboards, or battle pass.** Single-player mod.
8. **No console support** (SKSE requirement); VR only via multi-runtime build + test hardware.
9. **UI is SKSE Menu Framework or MCM.** No custom Scaleform/Flash (D-022).
10. **Inventory is vanilla.** Replacing it with a grid requires a separate mod (Grid Inventory)
    as an optional dependency.
11. **No mount/monster-riding animation hooks.** Requires animation/behavior files we do not own.
12. **No native socketed-gem/links/colors, flask/charge, or passive-tree systems.** Buildable
    as custom state + UI (Actor Value Generator, SKSE Menu Framework). Not native, but
    shippable; tracked as B.
13. **No native ailment/damage-type/penetration framework.** Buildable on MGEF + co-save
    (Elemental Reactions Framework proves it). Tracked as B.

### 13.2 Feasibility classification of the gaps

- **A — Implementable**: fits Skyrim + SKSE + co-save + records; normal effort.
  RG-009, RG-024, RG-031, RG-046, RG-047, RG-090, RG-091, RG-092, RG-159, RG-160,
  RG-161, RG-162, RG-163, RG-164, RG-166, RG-167, RG-168, RG-181, RG-182, RG-183,
  RG-184, RG-087, RG-088, RG-089.
- **B — Hard but possible**: requires a new system/UI/data layer; high effort but no engine
  blocker. (All other RESERVE items in sections 2-7, 9-10.)
- **C — Not feasible in Skyrim**: violates a hard constraint; drop or reframe as fixed
  authored content. Section 8 lists them; the only ones that could be reframed as fixed
  content are RG-103 (Blight), RG-114 (Bloodlines), RG-115 (Domination), RG-116 (Invasion),
  RG-126 (Anomaly quests), RG-131 (Helltide), RG-129 (World bosses), RG-130 (Legion events).

### 13.3 Implications

- Section 8 items are **OUT** and must never be promised on the Nexus page.
- Section 8.2 items are now **in scope** (design decision changed): durability (optional
  module), passive/skill trees (Custom Skills Framework), ailment/damage-type (ship later).
- RG-055 (socket-gem) stays OUT; replaced by RG-055b (Imbue), which is B.
- The 4 NOW candidates are all A and cheap; they can enter v1 without new systems.
- Every B item requires a dedicated pack and a budget decision before work (D-047).

### 13.4 Provenance

The constraints in 13.1 are evidenced by:
- PROTEUS 3.4.0 analysis (10-PROTEUS.md).
- `_context\rpg-mechanism\01-09` (DiabloRim, Worlds Dawn, Fortunate Foes).
- AGENTS.md section 4 and SPECS L2/L4 (hook map, budgets, co-save TLV).
- Runefall decisions D-003, D-004, D-006, D-023, D-024, D-025, D-027, D-028, D-040,
  D-047, D-048, D-050, D-052, D-055.

---

## 15. Additional game comparisons (Grim Dawn, Torchlight, Wolcen, Lost Ark, Borderlands, Risk of Rain 2, Hades)

The user requested comparison against more games in the genre. Below are new gaps not
covered by the original 4-game comparison.

### 15.1 Grim Dawn

| ID | Feature gap | Reference | Status | Priority | Destination | Notes |
|---|---|---|---|---|---|---|
| RG-202 | Devotion system (constellation nodes granting celestial powers/procs) | GD | Missing | RESERVE | Progression pack | Like a passive tree but with proc-based celestial powers. |
| RG-203 | Monster Infrequent items (MIs — unique items dropped by specific monsters) | GD | Missing | RESERVE | Unique pack | Unique items tied to specific enemy variants (fits enemy variant system). |
| RG-204 | Nemesis system (bosses that hunt the player after reaching reputation) | GD | Missing | RESERVE | Enemy pack | Nemesis bosses spawn based on faction reputation. |
| RG-205 | Bounty system (wanted posters for specific monsters with rewards) | GD | Missing (F-07) | RESERVE | Hunt II (F-07) | Hunt contracts already reserved. |
| RG-206 | Faction reputation (renown with factions for rewards/perks) | GD | Missing | RESERVE | Progression pack | Renown system (RG-146). |
| RG-207 | Components (crafting materials socketed into items for stat boosts) | GD | Missing | RESERVE | Crafting pack | Similar to Imbue (RG-055b) but as socketed components. |
| RG-208 | Augments (powerful crafting upgrades from faction vendors) | GD | Missing | RESERVE | Crafting pack | New consumable from faction vendors. |
| RG-209 | Relics (powerful crafted items with unique effects) | GD, TQ | Missing | RESERVE | Crafting pack | New item class. |
| RG-210 | Transmutation (combine 3 items into 1 of next tier) | GD | Missing (F-17) | RESERVE | Economy pack (F-17) | Already reserved. |
| RG-211 | Shattered Realm (endless dungeon with shards) | GD | Missing | OUT (world change) | - | Procedural; OUT. |
| RG-212 | Crucible (wave-based arena with multipliers) | GD | Missing | RESERVE | World pack | Arena mode; no world changes. |
| RG-213 | Caravan stash (shared stash across characters) | GD, TQ | Missing | OUT | - | Skyrim has shared stash mods. |

### 15.2 Titan Quest

| ID | Feature gap | Reference | Status | Priority | Destination | Notes |
|---|---|---|---|---|---|---|
| RG-214 | Dual mastery (choose 2 masteries for hybrid builds) | TQ | Missing | RESERVE | Progression pack | Hybrid build system; Runefall has 3 builds, could allow splashing. |
| RG-215 | Relic/charm system (socket relics and charms into gear) | TQ | Missing | RESERVE | Crafting pack | Similar to Imbue (RG-055b). |
| RG-216 | Monster Infrequent items (TQ style) | TQ | Missing | RESERVE | Unique pack | Same as RG-203. |

### 15.3 Wolcen

| ID | Feature gap | Reference | Status | Priority | Destination | Notes |
|---|---|---|---|---|---|---|
| RG-217 | Gate of Fates (rotating passive tree) | Wolcen | Missing | RESERVE | Progression pack | Rotating passive tree; complex but buildable. |
| RG-218 | Apocalypse (ultimate ability with cooldown) | Wolcen | Missing | RESERVE | Combat pack | Ultimate ability system. |
| RG-219 | Expedition (endgame with modifiers and bosses) | Wolcen | Missing | OUT (world change) | - | Procedural; OUT. |
| RG-220 | Housing (player house decoration) | Wolcen | Missing | OUT | - | Skyrim has housing mods. |

### 15.4 Lost Ark

| ID | Feature gap | Reference | Status | Priority | Destination | Notes |
|---|---|---|---|---|---|---|
| RG-221 | Engraving system (character power from accessory engravings) | LA | Missing | RESERVE | Progression pack | Accessory-based power system. |
| RG-222 | Card system (collection of cards for stat bonuses) | LA | Missing | RESERVE | Progression pack | Card collection system. |
| RG-223 | Tripod (skill customization with 3 tiers of modifiers) | LA | Missing | RESERVE | Progression pack | Skill modifier system; builds on RG-002. |
| RG-224 | Awakening (ultimate ability) | LA | Missing | RESERVE | Combat pack | Ultimate ability system. |
| RG-225 | Gem system (socket gems into gear for stat boosts) | LA | Missing | RESERVE | Crafting pack | Similar to Imbue (RG-055b). |
| RG-226 | Chaos dungeon (wave-based endgame with rewards) | LA | Missing | RESERVE | World pack | Arena mode; no world changes. |
| RG-227 | Guardian raid (boss fight with mechanics) | LA | Missing | RESERVE | Boss pack | New boss encounter type. |
| RG-228 | Cube (rogue-like dungeon with modifiers) | LA | Missing | OUT (world change) | - | Procedural; OUT. |
| RG-229 | Tower (endless tower with floors) | LA | Missing | OUT (world change) | - | Procedural; OUT. |

### 15.5 Torchlight (already referenced; additional gaps)

| ID | Feature gap | Reference | Status | Priority | Destination | Notes |
|---|---|---|---|---|---|---|
| RG-230 | Pet (companion with inventory, can sell items) | TL | Missing | RESERVE | Companion pack (F-16) | Companion with inventory. |
| RG-231 | Fishing (mini-game for fish/items) | TL | Missing | OUT | - | Skyrim has fishing (AE). |
| RG-232 | Ember (socket gems into gear) | TL | Missing | RESERVE | Crafting pack | Similar to Imbue (RG-055b). |
| RG-233 | Fame system (level up fame for skill points) | TL | Missing | RESERVE | Progression pack | Alternative progression axis. |
| RG-234 | Retirement (pass down items to new character) | TL | Missing | RESERVE | Progression pack | New character inherits items. |
| RG-235 | Shared stash (account-wide stash) | TL | Missing | OUT | - | Skyrim has shared stash mods. |

### 15.6 Borderlands

| ID | Feature gap | Reference | Status | Priority | Destination | Notes |
|---|---|---|---|---|---|---|
| RG-236 | Gun parts (random weapon generation from parts) | BL | Missing | RESERVE | Item pack | Weapon generation from parts; fits dynamic item generation. |
| RG-237 | Anointments (conditional bonuses on items) | BL | Missing | RESERVE | Item pack | Conditional affix (e.g., "on action skill end"). |
| RG-238 | Mayhem levels (difficulty scaling with better loot) | BL | Missing | RESERVE | Endgame pack | Difficulty scaling; similar to RF tiers. |

### 15.7 Risk of Rain 2

| ID | Feature gap | Reference | Status | Priority | Destination | Notes |
|---|---|---|---|---|---|---|
| RG-239 | Item stacking (same item stacks for multiplicative effect) | RoR2 | Missing | RESERVE | Item pack | Stacking mechanic for items. |
| RG-240 | Lunar items (trade HP for power) | RoR2 | Missing | RESERVE | Item pack | Risk/reward item type. |
| RG-241 | Void items (corrupt existing items) | RoR2 | Missing | RESERVE | Item pack | Corruption mechanic (RG-026). |
| RG-242 | Proc chains (items trigger other items) | RoR2 | Missing | RESERVE | Item pack | Interaction system between items. |

### 15.8 Hades

| ID | Feature gap | Reference | Status | Priority | Destination | Notes |
|---|---|---|---|---|---|---|
| RG-243 | Boons (temporary power from gods during a run) | Hades | Missing | RESERVE | World pack | Temporary buff system; could be "rune boons" from shrines. |
| RG-244 | Keepsakes (equippable items granting passive bonuses) | Hades | Missing | RESERVE | Item pack | New item type. |
| RG-245 | Mirror of Night (permanent upgrade tree) | Hades | Missing | RESERVE | Progression pack | Permanent upgrade system; like attributes. |
| RG-246 | Pact of Punishment (difficulty modifiers for more rewards) | Hades | Missing | RESERVE | Endgame pack | Difficulty modifier system. |

## 16. Overlap audit — entries already covered (not real gaps)

Many entries in the tables above overlap with Skyrim's vanilla systems or with Runefall's
existing/reserved features. They are recorded for provenance but are **not real gaps** and
should not drive scope. Listed here so the true gap count is visible.

### 16.1 Covered by Skyrim vanilla

| RG ID | Feature | Why it overlaps |
|---|---|---|
| RG-159 | Waypoints / fast travel | Skyrim vanilla. |
| RG-160 | Town portals / recall | Skyrim vanilla. |
| RG-161 | Camps / safe zones | Skyrim vanilla. |
| RG-162 | Gathering (herbs, ore, leather) | Skyrim vanilla + our material hunting. |
| RG-163 | Fishing | Skyrim AE vanilla. |
| RG-164 | Mining / bone collecting | Skyrim vanilla. |
| RG-166 | Photo mode | Vanilla / mods. |
| RG-167 | Gestures / emotes | Skyrim vanilla. |
| RG-181 | Radial menu / quick slots | Vanilla / SkyUI. |
| RG-052 | Stash tabs / sort | Vanilla inventory (Grid Inventory mod optional). |
| RG-213 | Caravan stash (shared) | Skyrim has shared stash mods. |
| RG-220 | Housing | Skyrim has housing mods. |
| RG-231 | Fishing (Torchlight) | Skyrim AE has fishing. |
| RG-235 | Shared stash (Torchlight) | Skyrim has shared stash mods. |

### 16.2 Covered by Runefall's existing features

| RG ID | Feature | Covered by |
|---|---|---|
| RG-046 | Item level clamping by world tier | L-02. |
| RG-087 | Node respawn timers | C-07. |
| RG-088 | Tiered node yields by area | C-07. |
| RG-089 | Material families (ore/essence/hide) | C-01. |
| RG-090 | Monster affixes visible on name | T-03. |
| RG-091 | Champion packs with visible affixes | T-03. |
| RG-092 | Monster rank scaling | T-04, A5. |
| RG-168 | Map discovery rewards | E-04. |
| RG-182 | Diagnostic export / debug command | N-08. |
| RG-183 | Localization keys for all strings | N-05. |
| RG-184 | Performance budgets recorded | N-09. |

### 16.3 Already reserved as F-xx (cross-referenced, not duplicated)

| RG ID | Feature | Reserved as |
|---|---|---|
| RG-042 | Unique items (remaining mined) | F-09 (Unique II). |
| RG-043 | Set items and set bonuses | F-04 (Set pack). |
| RG-048 | Loot filter | F-21 (QoL pack). |
| RG-049 | Auto-salvage / auto-pickup | F-21. |
| RG-051 | Item comparison | F-22 (UI pack). |
| RG-053 | Armory / loadouts | F-13 (UI pack). |
| RG-054 | Transmog / layered armor | F-20 (UI/Asset pack). |
| RG-066 | Affix shards | F-01 (Crafting Table). |
| RG-067 | Targeted affix crafting | F-01. |
| RG-068 | Deterministic base-item crafting | F-01. |
| RG-069 | Socket-add recipes | F-01. |
| RG-078 | Recombinators | F-02 (Crafting II). |
| RG-079 | Affix transfer / extraction | F-02. |
| RG-084 | Transmutation (3→1) | F-17 (Economy pack). |
| RG-085 | Rune gambling vendor | F-17. |
| RG-086 | Salvage returning affix shards | F-01. |
| RG-093 | Boss phases and arena mechanics | F-14 (Boss pack II). |
| RG-124 | Investigations / tempered monsters | F-07 (Hunt II). |
| RG-140 | Infinite scaling beyond tier 5 | F-11 (Endgame pack). |
| RG-150 | Seasons / seasonal realm | F-12 (Modes pack). |
| RG-151 | Season journey / conquests | F-12. |
| RG-152 | Hardcore mode | F-12. |
| RG-170 | Bounty board / hunt contracts | F-07. |
| RG-171 | Investigation board | F-07. |
| RG-172 | Loot filter | F-21. |
| RG-173 | Auto-salvage / auto-pickup | F-21. |
| RG-174 | Colorblind-safe markers | F-24 (QoL pack). |
| RG-175 | Text scaling / input remapping | F-24. |
| RG-176 | HUD cooldowns / rarity beams / badge anim | F-23 (UI pack). |
| RG-177 | Tooltip deepening / comparison | F-22 (UI pack). |
| RG-178 | Diablo-style grid inventory | F-20 (UI pack). |
| RG-179 | Full character sheet art | F-20. |
| RG-180 | Loadouts / gear presets | F-13 (UI pack). |
| RG-196 | Rune pickup sounds / UI SFX | F-25 (Asset pack). |
| RG-197 | Unique affix VFX | F-25. |
| RG-199 | Rank badge animation | F-23. |
| RG-200 | Localization packs beyond English | F-26. |
| RG-201 | Expanded asset set / alternate skins | F-27. |

### 16.4 True gaps remaining (not covered by vanilla or existing/reserved features)

After removing overlaps, the genuine gaps that represent new work are:

**Progression:** RG-004 (damage types), RG-005 (ailments), RG-006 (penetration), RG-007 (ward),
RG-008 (block/dodge/evasion), RG-009 (crit damage), RG-010 (leech), RG-011 (thorns), RG-012 (AoE),
RG-013 (cooldown recovery), RG-014 (resource cost reduction), RG-015 (movement speed), RG-016 (attack/cast speed),
RG-017 (charges/stacks), RG-022 (pet/companion), RG-023 (damage conversion), RG-024b (Rune XP/Level AVs),
RG-024c (Rune progression), RG-202 (Devotion), RG-204 (Nemesis), RG-206 (Faction reputation),
RG-214 (Dual mastery), RG-217 (Gate of Fates), RG-221 (Engraving), RG-222 (Card system),
RG-223 (Tripod), RG-224 (Awakening), RG-233 (Fame system), RG-234 (Retirement), RG-245 (Mirror of Night).

**Itemization:** RG-024 (quality tiers), RG-025 (implicit affixes), RG-026 (corrupted), RG-027 (influenced),
RG-028 (fractured/synthesised), RG-029 (veiled), RG-030 (anointments), RG-031 (reroll single affix),
RG-032 (tempering), RG-033 (masterworking), RG-034 (imprinting), RG-035 (runewords), RG-036 (aspect extraction),
RG-037 (legendary gems), RG-038 (threshold jewels), RG-039 (abyss jewels), RG-044 (mythic uniques),
RG-045 (ancient/primordial), RG-047 (smart loot armor), RG-050 (loot beams), RG-055b (Imbue),
RG-056 (quality upgrades), RG-057 (treasure goblins), RG-058 (strongboxes), RG-059 (cursed chests),
RG-060 (pylons), RG-061 (boss-specific uniques), RG-203 (Monster Infrequent), RG-236 (gun parts),
RG-237 (anointments), RG-238 (Mayhem levels), RG-239 (item stacking), RG-240 (lunar items),
RG-241 (void items), RG-242 (proc chains), RG-244 (keepsakes), RG-225 (gem system), RG-232 (ember).

**Crafting/Economy:** RG-062 (forging potential), RG-063 (fracture), RG-064 (glyphs), RG-065 (rune shatter/extract),
RG-070 (material chains), RG-071 (legendary potential), RG-072 (exalted items), RG-073 (sealed affixes),
RG-076 (fossils), RG-077 (essences), RG-082 (gambling), RG-083 (vendor recipes), RG-207 (components),
RG-208 (augments), RG-209 (relics).

**Enemy:** RG-094 (encounter modifiers), RG-095 (dungeon affixes/sigils), RG-111 (essence monsters),
RG-119 (rampage), RG-120 (onslaught), RG-125 (variant monsters), RG-125b (enemy variant system),
RG-125c (variant reward scaling), RG-125d (variant naming), RG-127 (siege monsters), RG-137 (frenzy virus),
RG-138 (blights), RG-139 (status ailments), RG-204 (nemesis), RG-205 (bounty), RG-227 (guardian raid),
RG-243 (boons), RG-246 (pact of punishment).

**Endgame:** RG-142 (pinnacle bosses), RG-143 (torment tiers), RG-146 (renown), RG-147 (pantheon),
RG-148 (bandit choice), RG-149 (prophecies), RG-153 (SSF), RG-154 (ruthless), RG-156 (challenge rifts),
RG-157 (set dungeons), RG-158 (greater rift keystone), RG-212 (crucible), RG-226 (chaos dungeon),
RG-238 (Mayhem levels), RG-246 (pact of punishment).

**UI/QoL:** RG-050 (loot beams), RG-056 (quality upgrades), RG-057 (treasure goblins), RG-058 (strongboxes),
RG-059 (cursed chests), RG-060 (pylons), RG-176 (HUD cooldowns), RG-198 (rarity beams).

**Audio/Visual:** RG-198 (rarity beams/ground glows).

## 14. Recommended next actions (not implementation)

1. Review the NOW list (4 items: RG-009, RG-024, RG-031, RG-047) and decide which enter
   v1; if any do, add them to `GAP-REGISTER-FUTURE.md` as new N-xx hooks and update L1/L3.
2. Confirm Section 8 (OUT) is acceptable as the "never promise" list for the Nexus page.
3. Decide whether to merge this register into `GAP-REGISTER-FUTURE.md` (single source of
   truth). Recommendation: merge the B-feasibility RESERVED items as new F-xx rows with
   RG- cross-references, keeping this file as the detailed provenance.
4. For each RESERVED item that turns out to need a v1 hook, update section 4 (cheap hooks)
   of `GAP-REGISTER-FUTURE.md` and D-048 before any work (D-047 rule 4).
5. Do not let reference-game parity drive scope creep: the v1 scope lock (D-047) stands;
   this register is a backlog, not a commitment.
