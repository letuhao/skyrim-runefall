# L3B - Content math plan (affixes, names, skills, records)

Status: 2026-10-26. Supersedes the ad-hoc sizing in L3A for affix counts and record budgets.
Every number below is derived from the game model, not chosen by feel.

Model inputs:

- B = 3 builds: Warrior, Thief, Mage.
- A = 9 attributes, 3 per build.
- S = 18 Skyrim skills, 6 per build; each attribute gates exactly 2 skills.
- C = 5 enhancement categories: offense, defense, utility, economy, elemental.
- R = 6 rarity bands (0-5); enhancement count = rarity index.
- T = 5 enhancement tiers (T1-T5) per affix.
- G = 10 equipment slot groups: 1H, 2H, bow, body armor, head, hands, feet, shield, ring, amulet.

## 1. Build / attribute / skill matrix (the thing affixes must cover)

| Build | Attribute | Gated skills (2 per attribute) |
|---|---|---|
| Warrior | Might | One-Handed, Two-Handed |
| Warrior | Endurance | Block, Heavy Armor |
| Warrior | Precision | Archery, Smithing |
| Thief | Agility | Sneak, Light Armor |
| Thief | Guile | Lockpicking, Pickpocket |
| Thief | Cunning | Speech, Alchemy |
| Mage | Intellect | Destruction, Enchanting |
| Mage | Wisdom | Restoration, Alteration |
| Mage | Willpower | Conjuration, Illusion |

Consequences:

- Every attribute must be able to fill every category, or a build cannot build a 5-enhancement
  item without dead slots.
- Every Skyrim skill must appear in at least one affix, or the "18 skills" identity is cosmetic.
- Each build must have roughly equal affix support (36 each), or one build is under-served.

## 2. Affix count derivation (two independent ways, same result)

### 2.1 Per attribute x per category coverage

Requirement: every attribute must offer at least two choices in every category (so a build has a
choice, not a single mandatory roll).

    cells = A x C = 9 x 5 = 45 attribute-category cells
    minimum affixes from coverage = 45 cells x 2 = 90

### 2.2 Per attribute skill synergy

Each attribute gates exactly two skills. Give each attribute two dedicated skill-synergy affixes
(one per gated skill), which also make the skill identity visible on gear:

    skill synergy affixes = A x 2 = 9 x 2 = 18

### 2.3 Total and cross-checks

    N = 90 + 18 = 108 affixes

Independent cross-checks:

- Per build: 3 attributes x 12 affixes (10 coverage + 2 synergy) = 36; 3 builds x 36 = 108.
- Per skill: 18 dedicated skill affixes + attribute affixes tagged to skills; coverage rule
  requires every skill to be tagged by at least 2 affixes, satisfied with margin.
- Per category base pool: 90/5 = 18 attribute affixes + the 18 skill affixes assigned to
  categories (3-4 each) gives 21-22 per category.

### 2.4 Rows and effect records

    affix rows = N x T = 108 x 5 = 540 rows
    affix effect templates = N = 108 MGEF (one per affix; magnitudes vary by tier)
    skill effects = 27 MGEF/SPEL (9 active skills x 3 ranks) + ~20 passive/utility effects
    monster-affix effects = 20
    utility/visual/condition effects = 15
    total MGEF estimate = 108 + 27 + 20 + 15 = 170

MGEF capacity in the revised FormID map is 256, so 170 = 66% used, leaving room for patches.

## 3. Affix category allocation math

The 18 skill affixes are assigned one per Skyrim skill, keeping category pools balanced:

| Category | Attribute affixes (9 attrs x 2) | Skill affixes | Pool size |
|---|---|---|---|
| Offense | 18 | One-Handed, Two-Handed, Archery (3) | 21 |
| Defense | 18 | Block, Heavy Armor, Light Armor, Alteration (4) | 22 |
| Utility | 18 | Sneak, Lockpicking, Pickpocket, Illusion (4) | 22 |
| Economy | 18 | Speech, Alchemy, Smithing, Enchanting (4) | 22 |
| Elemental | 18 | Destruction, Restoration, Conjuration (3) | 21 |
| Total | 90 | 18 | 108 |

### 3.1 Loadout combinatorics

A rarity-5 item has one enhancement from each category (I-03). Minimum category pools from the
table (offense 21, defense 22, utility 22, economy 22, elemental 21):

    rarity-5 loadouts = 21 x 22 x 22 x 22 x 21 = 4,695,768 distinct enhancement sets

Lower rarities choose fewer categories:

    R1 = 108; R2 = 108 x 107 = 11,556; R3 = C(5,3) pools product ~ up to 10,164;
    R4 = 4-category products; R5 = 4.7M.

With tier magnitudes (5 choices per enhancement) a single R5 loadout has 5^5 = 3,125 magnitude
variants. Total possible R5 instances: 4,695,768 x 3,125 = 1.467e10. This is why records cannot
represent the item space and why dynamic generation is mandatory (D-023).

## 4. Prefix and suffix naming math

Requirement: every affix carries both a name prefix and a name suffix (the user requirement), and
the item name should be varied enough that duplicate names are rare.

    prefix pool P = N = 108
    suffix pool S = N = 108
    distinct two-part names = P x S = 108 x 108 = 11,664

Naming rule (deterministic):

- Item name = prefix from the highest-tier enhancement + base item name + suffix from the
  highest-tier enhancement in a different category.
- Rarity marker is a separate rarity token, not part of the prefix/suffix pools.
- R0 items have no affixes and no affix name parts; R1 uses one prefix or one suffix; R2+ use
  one prefix and one suffix.

Minimum diversity target: 10,000 distinct two-part names. Achieved: 11,664 (17% margin). If a
later content pack adds 20 affixes, names grow to 128 x 128 = 16,384 without rework.

## 5. Skill coverage math

Dedicated skill affixes (one per skill) are assigned to categories as section 3. In addition,
attribute affixes are tagged with the skills they support (a weapon affix can tag One-Handed;
an armor affix can tag Light Armor and Sneak; etc.).

    required skill tags = 18 skills x 2 affixes = 36 tag assignments minimum
    available tag capacity = 90 attribute affixes x 2 skill tags = 180

Coverage is therefore guaranteed with 5x margin; the validator enforces it (section 8).

## 6. Slot coverage math

Requirement: a player using any equipment slot must find at least 18 valid affixes (about 3-4 per
category), otherwise some slots become dead loot.

    slot groups G = 10
    minimum per slot = 18 affixes
    required affix-slot assignments = 10 x 18 = 180

Each affix declares a slot mask; average mask size is 2-3 slots (weapon affixes cover 1H/2H/bow;
armor affixes cover body/head/hands/feet; jewelry covers ring/amulet; shields separate). With 108
affixes and average mask 2.5, capacity = 270 assignments, above the required 180. Validator gate:
every slot group has at least 18 affixes and every affix has at least one slot.

## 7. Record, data and asset budget (derived)

| Artifact | Derivation | Count | Est. size |
|---|---|---|---|
| Affix CSV rows | N x T | 540 rows | ~65 KB |
| Affix MGEF | N | 108 | - |
| Skill SPEL/PERK/effects | 9 actives x3 + 9 passives x3 + effects | 27 + 27 + 27 | - |
| Monster affix effects | fixed pool | 20 | - |
| Utility effects | fixed pool | 15 | - |
| Keywords | rarity 6 + rank 5 + material 12 + 18 skill + 10 slot + misc | ~60 | - |
| Globals | 9 attributes + state + modes | ~40 | - |
| MISC | rune 5 + material 12 | 17-20 | - |
| COBJ recipes | 4 rune convert + 5 rarity steps + 1 extract + 1 reroll + 4 base craft + 4 socket + 6 utility | 25 | - |
| QUST/FLST | hosts + pools | ~10-15 | - |
| NPC/CLAS bosses | 5 RF tiers x (1 mini-boss + 1 boss) + 2 variants | 12 | - |
| Total records | sum | ~410-440 | 80-140 KB |
| Data files (all CSV + INI) | tables below | - | ~90-110 KB |
| Assets | 17 item icons + 6 rarity + 5 rank markers | 28 | 0.5-3 MB |
| Localization keys | affix 108 + rarity 6 + skills 54 + attributes 9 + materials 12 + recipes 25 + UI 40 | ~254 | ~20-25 KB |
| Package total | ESL + data + DLL 1-3 MB + assets | - | ~3-7 MB |

Data-file row counts derived from content (no arbitrary numbers):

    affixes.csv        540 rows
    rarity.csv           6 rows
    rarity_weights.csv  36 cells (6 rarities x 6 RF tiers)
    enemy_ranks.csv      5 rows
    spawn_weights.csv   30 cells (5 ranks x 6 RF tiers)
    drops_containers.csv 6 rows (container tiers)
    drops_quests.csv     5 rows (quest bands)
    drops_bosses.csv     6 rows (5 RF tiers + mythic)
    materials.csv       12 rows
    recipes.csv         25 rows
    skills.csv          18 rows
    attributes.csv       9 x effects rows (about 27-36)
    strings.csv        ~254 keys

## 8. Validation gates (build fails if not met)

1. Attribute coverage: for every (attribute, category) cell, at least 2 affixes exist.
2. Skill coverage: every one of the 18 Skyrim skills is referenced by at least 2 affixes.
3. Slot coverage: every one of the 10 slot groups has at least 18 valid affixes.
4. Name coverage: prefix and suffix pools each equal N (108) and have no duplicates; distinct
   two-part names >= 10,000.
5. Category pools: each category has at least 18 affixes (attribute affixes alone), 21+ with
   skill affixes.
6. Tier completeness: every affix has exactly 5 tier rows with monotonic magnitudes.
7. Build parity: per-build affix counts within 2 of each other (36/36/36).
8. Record budget: each FormID category within its cap; MGEF <= 256; total planned records
   reported.
9. Data references: every effectEditorId, skill rank EditorID, material and recipe reference
   resolves.
10. Prefix/suffix requirement: every affix row has a non-empty prefix and suffix.

## 9. What changed against the L3A sizing

| Item | L3A (superseded) | L3B (math-derived) |
|---|---|---|
| Affixes | 60 (12/category) | **108** = 9 attributes x (5 categories x 2 + 2 skill synergies) = 18 skills x 6 |
| Affix rows | 300 | **540** |
| MGEF | ~150 | **~170** |
| Category pools | 12 | 21-22 |
| R5 loadouts | 12^5 = 248,832 | 4,695,768 (19x more) |
| Name combinations | 60 x 60 = 3,600 | 108 x 108 = 11,664 |
| Skill coverage | implicit | explicit: every skill >= 2 affixes |
| Slot coverage | not checked | explicit: every slot >= 18 affixes |
| Build parity | not checked | explicit: 36/36/36 |

## 10. Revision to 120 affixes (benchmark constraint, 2026-10-26)

L3C added a benchmark constraint: beat the strongest combined local affix reference
(DiabloRim 49 + Summermyst 64 = 113 families).

    N = max(coverage 108, ceil(113 / 12) x 12) = 10 x 12 = 120

Deltas from the 108 plan:

| Item | 108 plan | 120 plan |
|---|---|---|
| Affixes | 108 | **120** |
| Affix rows (x5 tiers) | 540 | **600** |
| Per-attribute affixes | 12 | **13.33 -> 12 base + 12 benchmark split (40 per build)** |
| Category pools | 21-22 | **23, 24, 24, 24, 23** |
| R5 enhancement sets | 4,695,768 | **7,312,896** |
| Name combinations | 11,664 | **14,400** |
| Affix MGEF | 108 | **120** |
| Total MGEF estimate | ~170 | **~182** |
| Total records | ~420-450 | **~440-470** |
| Build parity | 36/36/36 | **40/40/40** |

The 120 target is the accepted budget (D-038 revised). It still uses the same coverage gates;
the extra 12 affixes are allocated by the validator to the category pools, not hand-picked.

## 11. Unique affixes (new content class, count pending mining)

Core affixes are rollable and tiered. **Unique affixes** are one-of-a-kind effects attached to
specific boss kills or quest completions; they never roll from drop tables.

Model:

    U = B_unique x u_b + Q_unique x u_q
    u_b = unique affixes per unique boss source (default 1)
    u_q = unique affixes per unique quest source (default 1)

- B_unique = hand-authored boss templates (20 in v1 after the boss-variety revision).
- Q_unique = number of mined quest sources accepted for unique rewards (derived in L3D).
- If mining yields more sources than the pack budget allows, sources are ranked by confidence
  and the best ones are accepted; the rest become candidates for content packs.

Per unique affix: 1 MGEF (or reused template), 1 prefix, 1 suffix, 1 source rule (boss/quest
EditorID), 1 drop rule (guaranteed first kill/completion, then disabled or rare), 0 tiers.
Pack budget: U records in Runefall - Unique.esp, well inside 2,048.

Rows: U rows in unique_affixes.csv (no tier multiplication).
Mining result (L3D, 2026-10-26): 31 HIGH boss candidates, 48 reward-pattern quest candidates,
7,128 weapons/armors scanned and 752 enchantments catalogued.

    B_unique = 20 (5 RF tiers x 4 boss templates)
    Q_unique = 20 (accepted from 48 quest candidates)
    U = 40 unique affixes, no tiers

Rows: 40 unique_affixes.csv rows. Records: ~40 MGEF + 40 names in Runefall - Unique.esp.
Unique affixes never enter drop tables; they are guaranteed first-kill / first-completion rewards
with a source rule, validated by a build gate (every unique affix has exactly one source).

## 12. Hunt families and drop tables (G11/G12, 2026-10-26)

Data mining (DataMiner, races.json) found 204 RACE records, 161 unique race EditorIDs, grouped
into 29 thematic families. Selection rule: a family qualifies at >= 10 NPC actors.

    F = 19 qualifying families
    U_hunt = F = 19 unique affixes (one per family)
    hunt materials = F = 19 MISC items

Drop-table rows:

    drops_general.csv = ranks (0-5) x RF tiers (0-5) = 6 x 6 = 36 rows
    race_groups.csv   = F = 19 rows
    drops_races.csv   = F x 2 rewards = 38 rows
    total new data rows = 93

Budget deltas:

| Item | Before | After |
|---|---|---|
| Unique affixes U | 40 | **59** (20 boss + 20 quest + 19 hunt) |
| MISC items | ~20 | **~39** (5 runes + 12 base + 19 hunt + 3 spares) |
| MGEF | ~182 | **~201** (+19 hunt unique effects) |
| Records | ~440-470 | **~480-510** |
| Packs | 5 | **6** (+ Runefall - Hunt.esp) |

Hunt uniques never enter random pools; firstOnly flag in co-save; hunt materials repeatable.
F is recomputed from the mined inventory by the same threshold, so a future DLC or pack grows F
without redesign.

## 13. Crafting Table reserved as expansion (2026-10-26)

v1 ships the hunting-first loop and **Rune Forge basics** only:

- Core: salvage, socket/extract, enhancement reroll, rarity upgrade, rune conversion, material
  hunting. COBJ in base: ~8 (rune conversion + socket craft + utilities).
- Reserved expansion (Runefall - Crafting.esp): deterministic base-item crafting, targeted affix
  crafting, Affix Shards, material-tier recipe chains, socket-add recipes, crafting UI. Estimated
  +17 COBJ and craft records in its own ESL.

Budget effect on v1:

| Item | With full crafting | v1 (crafting reserved) |
|---|---|---|
| COBJ records | 20-25 | **~8** |
| Records total | ~480-510 | **~465-495** |
| recipes.csv rows | 25 | **~8** |
| Data rows (all files) | - | -17 |

Hunting priority consequence: with the Crafting Table out of v1, gear acquisition is hunting/loot,
and hunt materials feed Rune Forge reroll/upgrade costs rather than recipe crafting. This is
intentional: hunting is the primary v1 progression mechanism (D-046).
