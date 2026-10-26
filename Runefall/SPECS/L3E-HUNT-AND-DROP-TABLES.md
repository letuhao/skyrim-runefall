# L3E - General drop tables and race hunt tables

Status: 2026-10-26. Covers gaps G11 (general drop table) and G12 (race-unique drop tables).
Purpose: make drops purpose-driven, not just rank/level driven: hunting a specific race or
creature family should have a reason.

## 1. Design goals

- **General drop table**: the baseline for every enemy, container tier and RF tier. This is the
  "what normally drops" table, separate from boss/quest tables.
- **Race hunt tables**: each hostile creature family has a unique drop identity - one unique
  affix (guaranteed on the first kill of that family at its minimum RF tier) and one hunt
  material (repeatable). Players who want a specific affix/material hunt that family.
- No record edits, no leveled-list edits: both are data tables evaluated by the DLL at kill time.

## 2. Hunt family model (derived from data mining)

Source: DataMiner run 2026-10-26 (races.json), official masters only. 204 RACE records mined;
161 unique race EditorIDs after merging plugin duplicates; grouped into 29 thematic families.

Selection rule (math-derived, no hand-picking):

    F = families with >= 10 NPC actors in the official game
    -> F = 19 families

| Family | NPC actors | Family | NPC actors |
|---|---|---|---|
| Draugr | 368 | Dragon Priest | 22 |
| Vampire | 229 | Hagraven | 18 |
| Skeleton | 107 | Troll | 15 |
| Daedra (Dremora/Atronach/Seeker) | 103 | Spriggan | 13 |
| Dragon | 79 | Chaurus | 13 |
| Falmer | 79 | Giant | 11 |
| Spider | 66 | Sabre Cat | 11 |
| Dwarven Automaton | 38 | Mammoth | 10 |
| Werewolf | 30 | Wolf | 25 |
| Riekling | 28 | | |

Note: the exact final list is generated from races.json by threshold; the table above shows the
families that pass. Families below 10 actors (Gargoyle, Ice Wraith, Death Hound, Wisp, Horker,
Boar, Netch, Slaughterfish) are reserved for a Hunt expansion pack, not v1.

Each family maps one or more race EditorIDs (e.g. Draugr = DraugrRace, SkeletonArmorRace,
DLC2AshSpawnRace). The mapping ships in race_groups.csv and is validated against the mined
race inventory.

## 3. Rewards and rows

Per family (v1):

- 1 unique affix (firstOnly = 1): guaranteed on the first qualifying kill at or above the
  family's minRfTier; afterwards disabled.
- 1 hunt material (repeatable): 15 percent base drop chance, +5 percent per RF tier above
  minRfTier, capped at 40 percent.
- No tiered core affixes are added by race tables; general tables handle ordinary loot.

Derived counts:

    race groups          F = 19 rows
    race reward rows     19 x 2 = 38 rows (19 unique affixes + 19 materials)
    unique affixes       U_hunt = 19
    hunt materials       19 MISC items

## 4. General drop table

Rows cover every combination of rank and RF tier:

    ranks: 0 (normal), 1 Elite, 2 Champion, 3 Mini-boss, 4 Boss, 5 Mythic -> 6
    RF tiers: 0-5 -> 6
    rows = 6 x 6 = 36

Columns (drops_general.csv):

    rank,rfTier,itemChancePct,itemCountMin,itemCountMax,rarityBias,runeMin,runeMax,uniqueChancePct

- itemChancePct: probability a kill produces a Runefall item.
- itemCountMin/Max: how many items when it procs.
- rarityBias: added weight to rarity bands at or above Refined.
- runeMin/Max: rune units dropped (rank-based, scales with RF tier).
- uniqueChancePct: reserved for rank 4-5 kills to roll from the unique-affix source pool of
  that enemy (default 0; bosses use their own table).

Containers, quests and bosses keep their existing dedicated tables (L3 section 3.4); the general
table is the enemy-kill baseline.

## 5. Data schemas

    race_groups.csv: raceGroup,memberRaceEditorIds,minRfTier,uniqueAffixId,materialId,description
    - memberRaceEditorIds: pipe-separated vanilla RACE EditorIDs (mined references)
    - uniqueAffixId: EditorID of the hunt unique affix (pack record)
    - materialId: EditorID of the hunt material MISC record

    drops_races.csv: raceGroup,rewardType,rewardId,dropChancePct,firstOnly,note
    - rewardType: unique_affix or material
    - firstOnly: 1 for unique affix, 0 for material

    drops_general.csv: rank,rfTier,itemChancePct,itemCountMin,itemCountMax,rarityBias,runeMin,runeMax,uniqueChancePct

Example rows:

    Draugr,DraugrRace|SkeletonArmorRace|DLC2AshSpawnRace,1,RF_U_Hunt_Draugr,RF_MISC_Hunt_Draugr,Ancient grave goods
    Draugr,unique_affix,RF_U_Hunt_Draugr,100,1,first kill only
    Draugr,material,RF_MISC_Hunt_Draugr,15,0,+5 percent per tier above minRfTier

    rank,rfTier,itemChancePct,itemCountMin,itemCountMax,rarityBias,runeMin,runeMax,uniqueChancePct
    1,0,35,1,1,0,1,2,0
    2,0,100,1,1,2,1,2,0
    3,2,100,1,2,4,2,4,5
    4,4,100,2,3,7,3,6,25
    5,5,100,2,4,12,5,10,50

## 6. Runtime and persistence (handoff to L4)

- Race detection: the DLL reads the actor's base race FormKey, resolves its EditorID, and looks up
  the race group in race_groups.csv. Vampire/werewolf variants use their own race records.
- firstOnly state: per (player, family) flag in the co-save; never resets unless the player
  resets hunt progress (INI command).
- Repeatable hunt materials use a per-family drop chance with tier scaling; no time gates.
- A race table never overrides the general table: a kill can roll general loot and, separately,
  the race reward.
- Unique affixes from the hunt pack are excluded from all random affix pools (validator).

## 7. Validation gates (add to L3/D-039 bundle)

1. Every family in race_groups.csv has at least 2 member race EditorIDs and every member race
   exists in the mined vanilla+DLC race inventory.
2. Every family has exactly one unique_affix row and one material row.
3. drops_general.csv covers all 36 rank x rfTier combinations exactly once.
4. Drop chances are 0-100; hunt chances increase monotonically with RF tier above minRfTier.
5. firstOnly = 1 only for unique_affix rows.
6. Hunt unique affixes cannot appear in any affix/drop table (unique pool separation).
7. Every hunt material has a MISC record and a crafting/salvage use (no dead materials).

## 8. Budget update

Hunt materials are a v1 Rune Forge currency (reroll and rarity-upgrade costs); the Crafting
Table expansion later adds recipe-based uses (D-045).

| Item | Before | After |
|---|---|---|
| Unique affixes | 40 (20 boss + 20 quest) | **59** (20 boss + 20 quest + 19 hunt) |
| MISC items | ~20 | ~39 (12 base materials + 19 hunt materials + 5 runes + others) |
| MGEF | ~182 | ~201 (adds 19 hunt unique effect templates) |
| Records | ~440-470 | **~465-495** |
| Data rows | - | +93 (36 general + 19 groups + 38 race rewards) |
| Packs | Unique, Bosses, Affixes, Quests, Patches | + **Runefall - Hunt.esp** |

The Hunt pack is optional and independent (D-040): if absent, general tables work and race
rewards are skipped with a log line.