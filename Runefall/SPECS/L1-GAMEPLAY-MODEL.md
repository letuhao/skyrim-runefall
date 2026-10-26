# L1 - Gameplay model (engine-agnostic)

Status: draft for review, 2026-10-26
Depends on: GAME-DESIGN.md sections 1-2 (vision, targets, non-goals)
Feeds: L2 Skyrim mapping

This layer defines Runefall as a game first. It borrows the proven mechanisms of Diablo and
Torchlight and states them in engine-neutral terms with numbers. Nothing here is a Skyrim
implementation decision; those belong to L2 and later.

Mechanic IDs: R = rune/economy core, P = progression, I = item, L = loot, T = enemy tier,
C = crafting, E = economy/pacing. Reference IDs stay stable through all layers.

## 1. Design pillars and references

Four pillars: P progression attributes, I/L loot, T enemy tiers, C crafting and material
hunting - unified by the Rune resource (R).

Borrowed from Diablo:

- Rarity-colored itemization with prefix/suffix affixes and item levels (I).
- Monster ranks and monster affixes, difficulty "world tiers" that scale enemies and loot (T, E).
- Deterministic base crafting plus random affix outcomes (C).
- Salvage/transmute loops that turn duplicate loot into materials (C).

Borrowed from Torchlight:

- Transparent, small numbers: few stats, readable formulas, no hidden multiplicative soup (P).
- Socketable gems/rune economy and enchanting/transmutation (R, C).
- Champion packs with visible affixes, readable danger (T).
- Cheap respec and low friction in the early game (P).

Deliberately not borrowed: seasonal resets, class-locked items, auction/trading economy,
endgame rifts, pets, cartoony art direction, Diablo's exact damage formulas.

Player experience targets (event-based, never clock-based - see E-00):

- A meaningful power increase every 3-5 combat encounters in the first hours; every 5-10
  encounters mid-game.
- One notable upgrade per 5-10 elite-or-better kills, or per 2-3 loot events, whichever
  comes first.
- A clear answer within 5 seconds of seeing an item: keep, salvage, or compare.
- No mandatory grinding wall before the first boss tier; no one-shot deaths from off-screen.
- A long exploration with no combat is allowed to be quiet: it pays in materials, nodes and
  map progress, never in forced loot.

## 2. Core loop and progression axes

    Enemy tier kill  ->  loot roll (rarity + affixes)
          ^                    |
          |              keep / compare / salvage
          |                    v
    Rune spend + materials  ->  craft, socket, reroll, upgrade
          ^                    |
          |                    v
    Attribute points  ->  survive higher Runefall Tiers  ->  higher tiers, higher item levels

Two progression axes:

- Character level: vanilla leveling (XP from the game). Grants 2 attribute points and 1
  skill point per level (P-02, P-06). Unchanged from the base game except for the grants.
- Runefall Tier (RF Tier 0-5): the world/difficulty axis. Gates item levels, enemy ranks,
  affix magnitudes and rune ranks. Advanced by an attunement ritual at a forge, requiring a
  boss kill of the current rank and a rune cost (T-03). This is the Diablo/Torchlight
  "difficulty tier" analogue and is optional to advance (players can stay at a tier).

## 3. The Rune (R)

R-01 Single resource. The Rune is both the currency and the socketable power source. There
is one rune item family, ranks R1-R5; rank is the only attribute (no elemental families in
v1). This keeps the economy legible and the item data simple.

R-02 Value ladder. R1 = 1 unit, R2 = 5, R3 = 25, R4 = 125, R5 = 625. Values are exact
exchange units, used by all costs and by salvage rewards.

R-03 Sources.
- Ranked enemies drop runes of their rank: R1 elite (1-2), R2 champion (1-2), R3 mini-boss
  (2-4), R4 boss (3-6), R5 mythic (5-10). Exact tables in T-07.
- Salvage of Runefall items returns rune value equal to 10-25 percent of the item's rune
  value (C-02).
- Rare world caches and quest rewards (L-06) contain runes.
- Location discoveries and rare caches grant a fixed rune credit (5-25 units by area tier).
  Exploration is a real faucet that needs no combat (E-04).
- Conversion: 5 lower-rank runes craft into 1 next rank at a forge (C-06); the reverse
  (breaking a rune down) returns 4 lower-rank runes (loss of 20 percent).

R-04 Sinks.
- Attribute points: cost grows with the number already bought (P-04).
- Affix reroll and rarity upgrade (C-04, C-05).
- Socketing consumes the rune (I-05).
- Attunement to the next RF Tier (T-03).
- Respec (P-05).

R-05 Player-visible meaning. Runes are the reason to kill ranked enemies; they are spent at
forges, and socketed runes are the only way to raise an item's rune power. A player who
hoards runes is weaker but can reroll/craft on demand - a deliberate choice.

R-06 Enemy tiers are also rune ranks. An enemy's rank is the rank of the rune it carries
(T-01). This is lore-consistent and keeps a single vocabulary for players and code.

## 4. Progression: attributes and derived stats (P)

P-01 Nine attributes, three per build. The three builds are Warrior, Thief and Mage; each
owns three attributes and six skills, and each attribute gates exactly two skills. There is no
class lock: any character can spend points anywhere, but skill gates and attribute synergies
pull a character toward one build identity.

| Build | Attribute | Gates | Effect per point (non-combat anchors in bold) |
|---|---|---|---|
| Warrior | Might | Cleave, Ruinous Blows | +2 percent melee damage, **+2 carry weight**, +0.5 percent stagger resistance, **+0.5 percent ore/hide/wood gather yield** |
| Warrior | Endurance | Bulwark, Ironclad | +4 health, +1.5 percent armor, +0.5 percent block effectiveness, **+1 stamina and +0.5 percent stamina regeneration** |
| Warrior | Precision | Piercing Shot, Forgecraft | +1.5 percent ranged damage, +0.5 percent attack speed, **+1 percent crafted item power, +0.5 percent crafting material efficiency** |
| Thief | Agility | Shadowstrike, Evasive Step | +0.5 percent crit chance, +0.5 percent attack speed, +0.4 percent evasion, **+0.3 percent movement, +0.3 percent fall damage reduction** |
| Thief | Guile | Tumbler's Touch, Pilfer | **+1 percent lock/pickpocket effectiveness, +0.4 percent magic find, +0.3 percent rune find, +0.3 percent sneak detection reduction** |
| Thief | Cunning | Silver Tongue, Venomcraft | **+1 percent vendor value, +1.5 percent poison potency, +0.5 percent stealth damage, +0.5 percent persuasion/intimidation success** |
| Mage | Intellect | Runic Bolt, Rune Binding | +2 magicka, +1.5 percent destruction magnitude, **+0.5 percent enchantment power, +0.5 percent spell learning from books** |
| Mage | Wisdom | Mending Rune, Wardskin | +1.5 percent restoration magnitude, +1 percent ward/armor, **+0.5 magicka regen, +1 percent potion effectiveness** |
| Mage | Willpower | Runeforged Summon, Dread Mark | +1.5 percent conjuration/illusion magnitude, +1 percent summon duration, +0.5 percent cooldown reduction, **+0.5 percent intimidation chance** |

P-02 Attribute points. Two points per character level, plus optional purchase with runes
(P-07). Two points are needed because a full build saturates three attributes: 9 attributes x
soft cap 30 = 90 points, reachable around level 45-50. Points are permanent until a respec
(P-08).

P-03 Derived stats. The per-point effects are in the P-01 table. Every attribute carries at
least one offensive and one defensive/utility effect, so no attribute is a trap. Exact curves
in appendix A3.

P-04 Skills: 18 skill lines, six per build, three ranks each (54 nodes). Nine lines are active
abilities, nine are passive lines. Every line maps to one Skyrim skill for its prerequisite
gate, and to its paired attribute for a second gate.

| Build | Skill | Skyrim gate | Attribute gate | Type | Rank 1 / 2 / 3 |
|---|---|---|---|---|---|
| Warrior | Cleave | One-Handed | Might 10/20/30 | Active | arc strike / wider arc / bleed capstone |
| Warrior | Ruinous Blows | Two-Handed | Might 10/20/30 | Active | power slam / shockwave / stagger aura |
| Warrior | Piercing Shot | Archery | Precision 10/20/30 | Active | pierce / multi-shot / rune-tipped volley |
| Warrior | Bulwark | Block | Endurance 10/20/30 | Passive | block bonus / reflect / shield bash buff |
| Warrior | Ironclad | Heavy Armor | Endurance 10/20/30 | Passive | damage reduction / thorns / unstoppable |
| Warrior | Forgecraft | Smithing | Precision 10/20/30 | Passive | craft discount / bonus item power / runeforged craft |
| Thief | Shadowstrike | Sneak | Agility 10/20/30 | Active | backstab burst / vanish / execute |
| Thief | Evasive Step | Light Armor | Agility 10/20/30 | Passive | dodge chance / rune dust on dodge / mirror image |
| Thief | Tumbler's Touch | Lockpicking | Guile 10/20/30 | Passive | lock speed / bonus chest roll / chest reroll |
| Thief | Pilfer | Pickpocket | Guile 10/20/30 | Active | steal runes / steal item / boss cache pickpocket |
| Thief | Silver Tongue | Speech | Cunning 10/20/30 | Passive | vendor prices / rune find / reroll discount |
| Thief | Venomcraft | Alchemy | Cunning 10/20/30 | Passive | poison potency / poison on hit / toxic cloud |
| Mage | Runic Bolt | Destruction | Intellect 10/20/30 | Active | bolt / chain / rune detonation |
| Mage | Mending Rune | Restoration | Wisdom 10/20/30 | Active | heal / heal over time / revive ward |
| Mage | Wardskin | Alteration | Wisdom 10/20/30 | Passive | armor / spell ward / reflect |
| Mage | Runeforged Summon | Conjuration | Willpower 10/20/30 | Active | summon / empowered summon / double summon |
| Mage | Dread Mark | Illusion | Willpower 10/20/30 | Active | fear / loot mark / mass panic |
| Mage | Rune Binding | Enchanting | Intellect 10/20/30 | Passive | socket bonus / extra socket chance / reroll cost reduction |

Rank gates: rank 1 at Skyrim skill 20 and attribute 10, rank 2 at 50 and 20, rank 3 at 75 and
30. Rank 1 gives the base effect, rank 2 upgrades it, rank 3 is the capstone. A player can
reach rank 1 in every one of their build's six skills in the first tens of levels; ranks 2-3
are the long game.

P-05 Active/passive split. Nine active skills (three per build) are castable abilities shipped
as our own SPEL/MGEF records with native cooldown management. Nine passive lines are PERK
records with conditions on our GLOBAL records and skill state. Active abilities are limited on
purpose: Skyrim gives two hands and a power slot, so the player equips two or three actives
and the passives carry the build. The exact cast model (spell vs lesser power vs native
hotkey) is an L2 decision.

P-06 Skill points. One skill point per character level, plus optional rune purchase (P-07).
Rank costs escalate: rank 1 costs 1 point, rank 2 costs 2, rank 3 costs 3, so a fully maxed
skill line costs 6 points and all 18 lines would cost 108 - more than a level-81 character can
earn (81 + at most 20 bought). A character therefore maxes roughly one build (36 points) plus a
second build's core skills, and must specialise. Skill points and attribute points are
separate pools, so each level is a choice between raw stats and abilities. Skill respec has
its own cost (P-08).

P-07 Rune-bought points. Attribute points: base 25 rune units for the first bought point,
times 1.10 per already-purchased point on that attribute, capped at 15 bought points per
attribute. Skill points: base 40 rune units, times 1.15 per bought skill point, capped at 20
bought skill points in total. Both are roughly one to three reference combat encounters at RF
Tier 2 (E-02).

P-08 Respec. A single forge service resets both pools: attribute reset costs 100 rune units
plus 10 percent of runes spent on bought attribute points; skill reset costs 50 rune units
plus 5 percent of runes spent on bought skill points. Cheap enough to experiment, expensive
enough that builds are not free to swap mid-fight.

P-09 Soft caps. Soft cap at 30 points and hard cap at 60 per attribute: derived gains are
full to 30, halved from 31 to 60, and blocked above 60. Nine attributes with a 30 soft cap is
a deliberate budget: a build can saturate its three attributes and still have room for a
splashed fourth.

P-10 Build freedom and scaling. No class lock and no respec trap: off-build points still buy
utility, and skill gates reward commitment rather than forbid variety. Enemies scale with RF
Tier and player level (T-04); attributes and gear both matter, and neither substitutes for the
other.

P-11 Non-combat anchor rule. Every attribute must touch at least one non-combat gameplay
domain (crafting, economy, exploration, stealth, social, magic utility). This rule exists so
the system does not collapse into nine flavors of combat damage. Coverage is audited against
the vanilla actor value inventory in SPECS\L2A-ACTOR-VALUE-COVERAGE.md; every attribute now
carries at least one bolded non-combat anchor in the P-01 table, and each non-combat domain is
touched by at least two attributes. When an attribute effect changes, re-run that audit.

## 5. Items (I)

I-01 Item level (ilvl). Every Runefall item has an ilvl derived from its source: enemy ilvl =
enemy level + rank bonus, container tier + area tier, quest reward band (L-02). ilvl drives
affix magnitudes, not affix count.

I-02 Six rarity bands, and the rarity index is the enhancement count:
- Rarity 0 - Common: 0 enhancements, 0 sockets.
- Rarity 1 - Refined: 1 enhancement, 0-1 sockets.
- Rarity 2 - Exquisite: 2 enhancements, 1 socket.
- Rarity 3 - Radiant: 3 enhancements, 1-2 sockets.
- Rarity 4 - Mythic: 4 enhancements, 2 sockets.
- Rarity 5 - Runeforged: 5 enhancements (one per category, the hard maximum), 3 sockets,
  plus one optional unique "runeforged" effect that is not counted as an enhancement.

The rule is deliberately simple: rarity number = number of enhancements. There are exactly
five enhancement categories, so rarity 5 fills one of each and nothing can exceed it. "How
many enhancements does this item have?" is answered by its rarity number.

I-03 Enhancement model. Prefix and suffix pools separated by category; every enhancement has
a category (Offense, Defense, Utility, Economy, Elemental) and a tier T1-T5. An item holds at
most one enhancement per category, which is why five is the ceiling. Glossary: the player-facing
term is "enhancement"; "affix" is the internal and community synonym, and it is the name used
by the CSV table columns (affixId, prefix, suffix) so existing tables stay readable.

I-04 Enhancement budget. Enhancement count = rarity index (0-5). Enhancement tier and
magnitude come from ilvl and rarity bonus. No item can roll the same enhancement twice.
Rerolls keep rarity and socket count, and reroll all enhancements (C-04).

I-05 Sockets and rune power. Sockets accept runes; each socketed rank adds item power:
R1 +5 percent item stat, R2 +10, R3 +15, R4 +20, R5 +25 (additive to the item's base stat
package, capped). Socketing is reversible for a free extraction at a forge (R-04 sink is the
initial socket, not the extraction).

I-06 Item power score. A single number for comparison and UI: weighted sum of the base stat
package plus affix contributions plus socket contributions. The formula lives in appendix A2.

I-07 Naming. Item name = rarity marker + affix prefix + base name + affix suffix. Economy of
words: at most three words of affix text.

I-08 (amended 2026-10-27) No bind-on-pickup, no item degradation in core. Durability
and break is an optional module (I-10; see SPECS/DESIGN-ITEM-CRAFTING.md section 2) shipped
as a separate FOMOD module (default off); when it is off, items never degrade and
SetItemHealthPercent stays free for the tempering channel (D-023). Bind-on-pickup is never
used. Skyrim players expect to keep what they find.

## 6. Loot generation (L)

L-01 Drop sources: ranked enemy kills, containers (tiered chests and boss caches), quest
rewards, world caches, vendors (limited), salvage (materials only).

L-02 Item level from source: enemy ilvl = enemy level + 5 per rank; container ilvl = area
ilvl + container tier (0-2); quest reward ilvl = quest band. ilvl is clamped to the current
RF Tier band so a lucky low-level player cannot get endgame gear.

L-03 Roll pipeline (deterministic order, seeded):
1. Decide whether a Runefall item drops at all (drop chance, L-04).
2. Roll rarity (weights by RF Tier, enemy rank and magic find, L-05).
3. Roll ilvl within clamp (L-02).
4. Determine affix count from rarity (I-02).
5. Pick affix categories without replacement, then affixes within category.
6. Roll magnitudes and tiers from ilvl.
7. Roll sockets from rarity.
8. Roll name (I-07).
9. Apply the optional unique effect only at rarity 5 (Runeforged, I-02).

L-04 Drop chance per kill: base 35 percent for normal enemies, 100 percent for elite and
above (with rarity bias). Configurable; a "loot density" slider exists in the INI.

L-05 Rarity weights at RF Tier 2 (percent, six bands): Common 55, Refined 25, Exquisite 11,
Radiant 6, Mythic 2.5, Runeforged 0.5. Magic find shifts weight from Common toward higher
bands with diminishing returns (appendix A4). Enemy rank adds a flat bonus to Exquisite and
above (R1 +0, R2 +2, R3 +4, R4 +7, R5 +12 weight points).

L-06 Pity and dry-streak protection. Every kill without a Refined-or-better item adds 1 to a
hidden counter; at 20 kills the next drop is forced to Refined or better, at 80 to Exquisite
or better, and at 400 to Radiant or better. The counter resets on a qualifying drop. This
prevents the classic "three hours, no upgrades" failure while keeping rarity 5 a chase.

L-07 Smart drops (optional module). Bias rune-forged item categories toward equipment the
player has used recently (weapon type, armor slot). Default on for weapons only; toggleable.

L-08 Anti-abuse. Container loot is rolled once per container instance and stored until
looted, so closing/reopening cannot reroll. Save-scumming around a boss is mitigated by
rolling the boss drop at death and persisting it (L2 will define feasibility). If full
persistence is not feasible, document it as a known limitation rather than fake it.

L-09 Loot presentation. Rarity is visible in the name and via an optional color/marker
integration on top of the vanilla UI. No mandatory UI replacement.

L-10 General drop table. Every enemy kill rolls a baseline table indexed by rank (0-5) and RF
tier (0-5): chance to drop a Runefall item, item count, rarity bias, rune yield and a reserved
unique chance for rank 4-5. Containers, quests and bosses keep their own tables. The table is
data, never records.

L-11 Race hunt tables (purpose hunting). Hostile creature families each have a hunt identity:
one unique affix (guaranteed on the first kill of that family at or above its minimum RF tier)
and one hunt material (repeatable, chance rises with RF tier). Families are derived from the
official race inventory: a family qualifies at 10 or more NPC actors, which yields 19 families
for v1 (Draugr, Vampire, Skeleton, Daedra, Dragon, Falmer, Spider, Dwarven Automaton, Werewolf,
Riekling, Wolf, Dragon Priest, Hagraven, Troll, Spriggan, Chaurus, Giant, Sabre Cat, Mammoth).
Hunting a family is therefore the reliable way to obtain its unique affix and material, while
general tables still supply ordinary loot. Hunt uniques never enter the random pools.

## 7. Enemy tiers and monster affixes (T)

T-01 Ranks: Rune-Touched (1, elite), Runebound (2, champion), Runelord (3, mini-boss),
Runesworn (4, boss), Rune-Mythic (5, mythic boss). Rank is the enemy's rune rank (R-06).

T-02 Scaling multipliers (health / damage / armor / rune drop wealth):

| Rank | Health | Damage | Armor | Rune wealth |
|---|---|---|---|---|
| 1 | 1.5x | 1.2x | +10 percent | 2x |
| 2 | 2.2x | 1.4x | +20 percent | 4x |
| 3 | 3.5x | 1.7x | +30 percent | 8x |
| 4 | 6.0x | 2.0x | +40 percent | 16x |
| 5 | 10.0x | 2.5x | +50 percent | 32x |

T-03 Monster affixes (visible, readable, one line each). Pool of 10 for v1:
Volatile (explodes on death), Warded (elemental resistance), Frenzied (attack speed),
Leeching (heals on hit), Runed Shell (damage reduction), Charged (periodic shock nova),
Frostbound (slows on hit), Venomous (damage over time), Summoner (spawns one minion),
Phasing (brief invulnerability windows). Counts: rank 1 = 1 affix, rank 2 = 1-2, rank 3 =
2-3, rank 4 = 3-4, rank 5 = 4. No duplicate affix on one enemy.

T-04 Rank assignment. Determined at spawn by RF Tier, area difficulty and a roll; weight
tables in appendix A5. Rank 4-5 are usually hand-authored bosses; generic spawns cap at
rank 3 except in high-tier areas. Density caps per cell to avoid packs of 40 elites.

T-05 Bosses. Each RF Tier has one or more named bosses with a guaranteed rune and
material drop, a unique Runeforged item chance, and a lore entry. Bosses are the attunement
gate (T-03 in the loop; T-06).

T-06 Attunement. To advance from RF Tier N to N+1, the player kills the tier boss, then
performs the attunement ritual at a forge with a rune cost (25 x 5^N units). Attunement is
account-wide per character; the player can choose not to advance.

T-07 Rewards by rank: rune drops (R-03), item rarity bonus (L-05), material tier bonus
(C-01). Rank 3+ also drops a guaranteed material of at least the area's tier.

T-08 Failure modes to design against: unkillable elite at low level (cap multipliers by
player level), unreadable aura soup (max 4 affixes, all visible in a name line), spawn
camping (density caps and area gating).

## 8. Crafting, material hunting and hunting primacy (C)

Scope split (user direction, 2026-10-26):

- **v1 core - Rune Forge (item modification):** salvage, socket/extract, enhancement reroll,
  rarity upgrade, rune conversion, and material hunting. Gear acquisition in v1 comes from
  hunting and loot; materials feed the Rune Forge.
- **Expansion - Crafting Table:** deterministic base-item crafting, targeted affix crafting,
  Affix Shards, material-tier recipes and the crafting UI are reserved for a later
  **Runefall - Crafting.esp** expansion. They are designed after v1 ships.

This makes hunting the primary v1 progression mechanism: unique affixes come from race/boss/quest
hunting, and hunt materials are the currency of Rune Forge rerolls and upgrades.

C-01 Material tiers: Common, Refined, Exotic, Mythic. Sources: salvage (C-02), ranked enemy
drops (T-07), nodes and caches (C-07). Each tier has 3 families (Ore, Essence, Hide analogue
- names are content-layer decisions) used by recipes.

C-02 Salvage (core). Any Runefall item can be dismantled at a forge. Yield: 1-3 material units
of the item's tier, plus rune value equal to 10-25 percent of the item's rune value. The
10-percent-per-affix Affix Shard extraction is an **expansion feature** (C-09); v1 salvage yields
materials and runes only. Salvage destroys the item; no undo.

C-03 Craft a base item (EXPANSION - Crafting Table). Deterministic recipes: base item + N
materials of the tier = the item at ilvl = current RF Tier band midpoint. Not shipped in v1;
see C-09.

C-04 Reroll enhancements. Costs materials plus rune units; keeps rarity and socket count,
rerolls all enhancement slots. A "locked enhancement" option costs 3x and keeps one chosen
enhancement.

C-05 Upgrade rarity. Costs one rarity step in materials plus a chance component (60 percent
success, on failure the materials are consumed but the item is unchanged). Radiant to Mythic
requires one R3+ rune and a boss material; Mythic to Runeforged requires one R4+ rune and two
boss materials of the area tier.

C-06 Rune conversion. 5 runes of rank N to 1 of rank N+1; breaking down returns 4 lower
runes. Crafting stations are the only place runes convert.

C-07 Material hunting. Ore veins, plant nodes and creature parts yield tiered materials;
yield and tier depend on area tier (same axis as RF Tier) plus a small random. Node respawn
is long (longer than a typical play session) so hunting is a loop, not a farm.

C-08 Socket and extract (core). Socket a rune into an item socket (consumes the rune). Extract
a socketed rune for free at a forge. Socket count is fixed by rarity (I-02); no adding sockets
in v1 (the socket-add recipe is part of the Crafting Table expansion).

C-09 Crafting Table expansion (reserved). Adds: deterministic base-item crafting (C-03),
targeted affix crafting, Affix Shard extraction/usage (C-02 extension), material-tier recipe
chains, socket-add recipes, and the crafting station UI. Delivered as Runefall - Crafting.esp
(ESL) using the same data schemas and pack manifest conventions (D-040). Design work starts
after v1 ships; v1 must not depend on it.

## 9. Economy (E) - event-driven

E-00 No clock. Runefall is not an idle or session-timer game. Nothing is balanced on minutes
or hours: all income and costs are expressed per combat encounter, per loot event, per
discovery and per boss kill. Play speed is the player's business - a fast clear and a slow
exploration produce different income, and that is intended. Never add passive or idle
generation.

E-01 Reference combat encounter. One reference encounter is a fight with 8-12 enemies at the
current RF Tier, including 1-2 rank-1 or one rank-2 enemy. Rune income per reference
encounter by RF Tier (rank distribution in A5):
- Tier 0: 6-10 units. Tier 1: 12-18. Tier 2: 25-40. Tier 3: 50-80. Tier 4: 100-160.
  Tier 5: 200-320.
Income scales superlinearly with tier because gear costs and expectations scale with it.
Heavy combat therefore pays much more per minute than careful exploration - by design.
Conversely, an hour of exploration with no combat can legitimately yield zero rune income;
it pays in materials, nodes and discoveries instead (E-04).

E-02 Costs as encounter multiples (RF Tier 2 baseline; roughly x1.5 per tier):
- Bought attribute point: 1-2 encounters.
- Enhancement reroll: 1-2 encounters; locked-enhancement reroll 3-4.
- Rarity upgrade: 4-8 encounters.
- Socketing a rune: the rune itself, no extra fee.
- Attunement to the next RF Tier: 10-15 encounters.
- Respec: 3-5 encounters.
No cost is expressed in minutes. A typical combat session affords a meaningful spend every
2-4 encounters; exploration sessions are expected to bank materials more than runes.

E-03 Loot events are their own faucets. A chest at area tier grants 0-2 items and 2-6 rune
units; a boss cache grants one guaranteed item and 15-30 units; a quest reward grants one
item and 5-15 units; a rare cache grants 10-25 units. These are event counts, never time
gates.

E-04 Exploration faucet. Discovering a new location grants 5-15 units and one material of
the area's tier; harvesting a node grants materials (C-07); rare caches grant extra units.
Exploration therefore has its own reward axis, but it never guarantees loot and never
punishes a player who spends an hour wandering.

E-05 Anti-inflation and anti-abuse. Costs scale with RF Tier; no passive generation of any
kind (no idle income, no daily rewards); salvage returns less than item value and destroys
the item; chest loot is rolled once per container instance so reload-farming does not work
(L-08); rune conversion is lossy in both directions (C-06) so it cannot be looped for profit.

E-06 Currency display. Rune count and rank breakdown are visible in the inventory and in
the crafting UI. No hidden wallet.

E-07 Trade-off. Because runes are both currency and sockets, hoarding is a real choice; the
design targets "spend 60-70 percent, socket 30-40 percent" of rune income as healthy play,
measured per playthrough rather than per hour.

## 10. Pacing and difficulty

Pacing is expressed in encounters, loot events and tier unlocks (E-00), never in minutes or
hours. The only clock-like numbers in this design are combat durations (E-09), which are
gameplay targets rather than economy pacing.

E-08 RF Tier bands (item level / player level guidance):
Tier 0: levels 1-10, ilvl 1-10 - tutorial and first affixes.
Tier 1: levels 8-20, ilvl 11-25 - first elite packs, first reroll.
Tier 2: levels 18-35, ilvl 26-40 - champion packs, crafting loop online.
Tier 3: levels 30-50, ilvl 41-60 - mini-bosses, rarity upgrades.
Tier 4: levels 45-70, ilvl 61-80 - bosses, Mythic chase.
Tier 5: levels 65+, ilvl 80-95 - Runeforged chase, endgame tuning.

E-09 Combat targets: normal enemy 3-6 seconds, elite 10-20 seconds, champion 20-40 seconds,
mini-boss 60-120 seconds, boss 2-5 minutes at tier-appropriate gear. These are balance gates
for playtesting, not hard rules.

E-10 Gear checks: each RF Tier expects the player to have at least Refined gear in every
slot and two sockets active by the end of the tier. If playtests show the wall is too hard,
tune enemy multipliers before adding loot.

E-11 Difficulty settings. RF Tier is the primary knob; a separate INI multiplier scales all
enemy health and damage for personal preference. Do not tie loot to the INI multiplier.

## 11. Appendix: default numbers

A1 Rune ladder: R1 1, R2 5, R3 25, R4 125, R5 625.

A2 Item power score: sum over base stats normalized to percent plus 40 percent of affix
contribution plus 60 percent of socket contribution. Final formula lives in L2/L3 once the
Skyrim stat mapping exists.

A3 Attribute curves: linear per point up to the soft cap (30); half gains from 31-60; hard
cap 60. No exponential curves in v1.

A4 Magic find: effective rarity weight = base weight x (1 + MF percent x 0.01), applied to
Refined and above; Common weight is reduced by the same factor to keep the total constant.
Diminishing returns: MF contributes at 100 percent up to 100 percent MF, then 50 percent,
then 25 percent.

A5 Rank weights by RF Tier (generic spawns): Tier 0: 85/14/1/0/0; Tier 1: 75/22/3/0/0;
Tier 2: 65/28/6/1/0; Tier 3: 55/32/10/3/0; Tier 4: 45/33/15/6/1; Tier 5: 35/32/20/10/3.
Values are percent, ordered rank 1 to 5.

A6 Drop rarity weights: see L-05.

A7 All numbers above are defaults in the INI/CSV content layer (L3). The shape is design;
the values are tuning.

## 12. Open questions for L2 (Skyrim mapping)

1. Damage model: how Skyrim's armor/resist/damage formulas absorb our percent bonuses without
   becoming multiplicative soup. Which derived stats map to actor values, which to perk
   conditions, which to runtime modifiers.
2. Attack speed and movement: Skyrim weapon speed and movement multipliers are engine-owned;
   what is safe to modify and what breaks animations or physics.
3. Critical hits: does Skyrim expose a usable crit channel, or do we implement crit as a
   separate damage instance; how does it interact with sneak attacks.
4. Per-item metadata: how rarity/affixes/sockets persist for an item instance; extra data vs
   serialized mapping; how to survive save/load and uninstall.
5. Loot timing and anti-abuse: exact container/vendor/boss drop hooks; can a boss drop be
   rolled at death and persisted.
6. Enemy spawn hooks and rank assignment: which hook sees an actor before the player does; how
   to avoid double application; how to scale health safely.
7. Rune item delivery: one MISC record per rank vs one record plus metadata; pickup, stacking,
   weight and UI behavior.
8. UI surfaces: what the chosen framework can render (character sheet, salvage, crafting,
   comparison tooltip) and where vanilla UI must be touched.
9. Performance budgets: per-kill and per-container hook cost, max tracked actors, co-save size
   per 1000 items.
10. Which mechanics must be cut or reshaped because the engine cannot express them (e.g.
    deterministic boss-drop persistence, smart drops).
11. Actor value safety: which AVs the attribute effects may write (see L2A classification),
    which stay read-only gates, and how to reach armor rating, lockpicking/pickpocket checks,
    persuasion/intimidation and alchemy potency without overwriting perk or survival mods.
12. Does AVDamageResist drive real armor rating in 1.6.1170 (1.7.x deferred), or is armor a computed
    stat from equipment and perks? This decides how the Endurance/armor bonus reaches combat.
13. How do the non-combat anchors (gather yield, crafting efficiency, potion effectiveness,
    spell learning, fall reduction, sneak reduction) hook into the game: native hook, Papyrus
    event, or condition on vanilla values? Which are safe on existing saves?

## 13. Review checklist for this layer

- [ ] Every mechanic has an ID, a rule, and a default number or an explicit tuning note.
- [ ] No mechanic requires unbounded multiplicative stacking.
- [ ] Economy has faucets and sinks with target rates.
- [ ] Loot has pity protection and anti-abuse rules.
- [ ] Enemy ranks have readable affixes and caps.
- [ ] Crafting has deterministic and random paths, plus a rune trade-off.
- [ ] Attributes and skills are budgeted consistently: 9 attributes (3 per build), 18 skill
      lines (6 per build), 3 ranks each, paired attribute and Skyrim-skill gates.
- [ ] Build identity is encouraged (gates, synergies) but not class-locked.
- [ ] Open questions for L2 are complete and none hides a design decision.
