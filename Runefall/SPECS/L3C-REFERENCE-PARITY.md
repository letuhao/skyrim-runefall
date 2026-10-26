# L3C - Reference parity check (is 108 affixes "better" content?)

Status: 2026-10-26. Compares the math-derived L3B budget against measured reference content.
Purpose: answer whether Runefall's content size is ahead of the references, behind, or equal -
per dimension - and derive any adjustment instead of guessing.

## 1. Head-to-head content comparison

| Dimension | Runefall (L3B) | DiabloRim + Summermyst (combined) | Worlds Dawn | Fortunate Foes |
|---|---|---|---|---|
| Affix families | **108** | 113 (49 + 64) | none (attribute/proc system) | none (perk/spell library) |
| Affix tier rows | **540** (5 tiers) | 328 (3 tiers) | - | - |
| Tiers per affix | 5 | 3 | - | - |
| Rarity bands | 6 | 5 (white..red) + base | n/a | n/a |
| Prefix/suffix naming | both on every affix; 108x108 = **11,664** names | both; ~49x49 = 2,401 (base), 113x113 = 12,769 if merged | n/a | n/a |
| Rarity-5 enhancement sets | 4,695,768 | points-based, no category lock (not directly comparable) | n/a | n/a |
| Records | ~420-450 | 0 | 742 | 415 |
| MGEF | ~170 | runtime effects inside DLL | 341 | 13 |
| Attributes / skills | 9 attributes, 18 skills x 3 ranks | none | 7 attributes (28 AV slots) | none |
| Enemy ranks | 5 ranks, 10 monster affix types, ~20 effects | none | none | 4-tier content, 108 boss NPC variants |
| Hand-authored NPC records | 20 boss templates | 0 | 3 dummy NPCs | 108 |
| Data volume | ~90-110 KB | 27 KB + 30 KB = 57 KB | ~24 KB strings/translation | 27.5 KB configs |
| Assets | 28 files, 0.5-3 MB | 0 | 241 files, 28 MB | 2 BSAs, 42.8 MB |
| Scripts | 5-10 Papyrus | 0 | 46 | 0 |
| Package size | 3-7 MB | 1.7 MB | 28.3 MB | 43 MB |

## 2. Verdict per dimension

Ahead:

- **Affix rows and granularity:** 540 rows vs 328 (+65%); five tiers vs three.
- **Coverage engineering:** 108 families are derived to cover all 9 attributes x 5 categories and
  all 18 Skyrim skills; the reference tables are thematic, not coverage-complete.
- **Naming:** 11,664 two-part names vs ~2,401 in the DiabloRim base table.
- **Progression content:** 9 attributes / 18 skills / 54 rank nodes vs Worlds Dawn's 7
  attributes with no skill layer.
- **Rarity granularity:** 6 bands vs 5.
- **Data-driven balance:** drop tables, rank curves and skill tables are data; references keep
  much of this in INI multipliers or FormLists.

Behind:

- **Affix families:** 108 vs 113 combined DiabloRim + Summermyst (-4.4%). Small, but it is the
  single reference metric where we are behind.
- **MGEF breadth:** ~170 vs Worlds Dawn 341. Ours is templated for runtime generation, so not a
  functional gap, but it is a raw content-volume gap.
- **Hand-authored enemy content:** 12-16 boss templates vs Fortunate Foes 108 NPC variants (which
  are variants of one archetype, not 108 unique designs).
- **Assets:** 28 files vs 241 files / 42.8 MB FaceGen BSAs. Ours is a deliberate v1 minimum.

Equal:

- **Total records:** ~420-450 vs Fortunate Foes 415 and Worlds Dawn 742; between them.
- **Package size:** mid-range.

## 3. Derived adjustment (benchmark parity constraint)

Add one constraint to the L3B model: **beat the strongest local affix reference combination**.
The strongest combined local table is DiabloRim 49 + Summermyst 64 = 113 families.

    N >= 113
    N must stay a multiple of 12 (12 affixes per attribute, 36 per build)
    N = ceil(113 / 12) x 12 = 10 x 12 = 120

N = 120 affixes gives:

    rows = 120 x 5 = 600
    per-category pools = 120/5 = 24 (attribute 18 + skill ~3-4 + parity 2-3)
    R5 loadouts = 23 x 24 x 24 x 24 x 23 = 7,312,896
    names = 120 x 120 = 14,400
    MGEF = 120 + 27 + 20 + 15 = 182
    records = ~465-495
    affix CSV = ~72 KB
    build parity = 40/40/40

This beats 113 by 6%, stays inside the MGEF cap (182/256), keeps build parity exact, and costs
12 extra MGEF records + 60 extra rows.

Second derived adjustment: **boss variety**. Fortunate Foes ships 108 NPC variants for one
archetype; we need per-tier variety, not 108 variants. Derive from our rank model: 5 RF tiers x
(1 mini-boss + 1 boss) = 10, plus one alternate per tier for variety = 20 templates. That is
+8 NPC records and a modest data entry.

## 4. Recommendation

Adopt **120 affixes / 600 rows / 182 MGEF / ~450 records / 20 boss templates** for v1:

- It wins every reference dimension except assets and MGEF breadth (MGEF is implementation
  granularity, not player-visible content).
- The change is cheap (+12 MGEF records, +60 CSV rows, +8 NPCs) and keeps all coverage gates.
- It preserves the same math: N = 12 x max(9 attributes, ceil(113/12)) = 120.
- Content packs remain data-only for affix variants; new effect types need records, so the 120
  base keeps headroom (MGEF 182 of 256).

If unchanged, 108 remains defensible (coverage-complete, ahead in rows/tiers/names), but it is
the only metric where the combined reference table is larger.

## 5. Gap closure status (all G items)

| Gap | Resolution | Status |
|---|---|---|
| G1 affix breadth | 120 affixes (benchmark constraint) / 600 rows | closed |
| G2 MGEF budget | ~182 MGEF, capacity 256 | closed |
| G3 FormID/record budget | per-plugin 1,408 capacity; base ~440-470; packs separate | closed |
| G4 boss roster | 20 templates (5 RF tiers x (1 mini-boss + 1 boss) + alternates) | closed |
| G5 assets | 17 icons + 6 rarity badges + 5 rank markers minimum | closed |
| G6 localization | ~254-300 keys | closed |
| G7 recipes | 20-25 | closed |
| G8 drop tables | 6 container tiers / 5 quest bands / 5-6 boss tables | closed |
| G9 unique boss/quest affixes | new content class; mined: 31 boss + 48 quest candidates -> U = 20 boss + 20 quest + 19 hunt = 59 unique affixes (L3D) | closed |
| G10 multi-ESL packaging | base + independent ESL packs with manifests | closed |
| G11 general drop table | drops_general.csv: 6 ranks x 6 RF tiers = 36 rows, data-only | closed |
| G12 race hunt tables | 19 qualifying families x (1 unique affix + 1 material); Hunt pack | closed |
| Crafting Table scope | reserved as Runefall - Crafting.esp expansion; v1 keeps Rune Forge basics; hunting primacy (D-045/D-046) | by design |

## 6. Evidence

- _context\rpg-mechanism\01-DIABLORIM.md (136 rows / 49 IDs / 3 tiers / prefix+suffix).
- _context\rpg-mechanism\03-SUMMERMYST-OPTION.md (192 rows / 64 IDs).
- _context\rpg-mechanism\04,05 (Worlds Dawn 742 records, 341 MGEF, 7 attributes, 46 scripts).
- _context\rpg-mechanism\06,07 (Fortunate Foes 415 records, 108 NPC variants, 214 DISTR entries).
- SPECS\L3B-CONTENT-MATH.md (108-affix derivation and coverage gates).