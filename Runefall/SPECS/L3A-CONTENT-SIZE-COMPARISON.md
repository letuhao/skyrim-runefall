# L3A - Content size comparison and gap resolution

Status: 2026-10-26. Companion to SPECS\L3-CONTENT-DATA.md. Purpose: compare Runefall's planned
content/data volume with the reference mods already analysed, find gaps, and fix them before L4.
**Content sizing in this document is superseded by SPECS\L3B-CONTENT-MATH.md** (108 affixes,
540 rows, ~170 MGEF, math-derived coverage gates). Keep L3A for the measured reference volumes;
use L3B for the budget.

Sources: _context\rpg-mechanism\01..10 (record dumps, CSV stats, asset inventories, script
counts, plugin sizes).

## 1. Reference content volumes (measured, not estimated)

| Reference | Records | Data rows / config | Scripts | Binary | Assets | Total package |
|---|---|---|---|---|---|---|
| DiabloRim 1.0.7 | 0 (no ESP) | affixes 136 rows / 49 IDs (16.4 KB); INI 25 keys (10.9 KB) | 0 (native only) | DLL 1.24 MB | 0 | 1.7 MB |
| Summermyst Option 1.0.0 | 0 | affixes 192 rows / 64 IDs (30.3 KB) | 0 | 0 | 0 | 30 KB |
| Worlds Dawn 2.14b | 742 (341 MGEF, 95 SPEL, 25 FLST, 30 GLOB, 169 VMAD) | strings.json 237 strings (7 KB); translation 173 tokens (17 KB) | 46 .pex / 46 .psc | 0 | 241 files / 28 MB (44 meshes, 73 textures, 29 sounds) | 28.3 MB |
| Fortunate Foes 7.62 | 415 (90 + 199 + 126) | SPID 214 entries (19.6 KB); SkyPatcher 8 lines (7.9 KB) | 0 | 0 | 2 BSAs 42.8 MB (108 FaceGen triples) | 43 MB |
| PROTEUS 3.4.0 | 1 core ESP + 39 patch ESPs (0.44 MB total) | JSON templates + presets | 25 .psc | DLL 556 KB | SWF 82 KB, page images 9.4 MB | ~11 MB + 39 patches |
| Grid Inventory 1.0.9 | base ESP not shipped | INI 71 keys | 0 | DLL 3.4 MB + PDB 48 MB | 0 | 14.6 MB |

Takeaways:

- A loot engine (DiabloRim) needs very few rows to work mechanically (136) but relies on a large
  native binary and a second author overlay (192 rows) for variety.
- A total RPG overhaul (Worlds Dawn) spends its volume on records (742) and assets (28 MB), not
  on data files.
- Enemy-content packs (Fortunate Foes) spend volume on records (415) and FaceGen BSAs (43 MB).
- None of them ships a combinatorial item set. The dynamic-generation decision (D-023) matches
  how the references actually work.

## 2. Runefall planned volume (L3 draft) vs references

| Our area | L3 draft | Comparable reference | Gap |
|---|---|---|---|
| ESL records | ~250-300 | WD 742; FF 415; PROTEUS core+patches | Under-budgeted once affixes, skill effects and monster affixes are counted |
| Affix pool | 40 affixes x 5 tiers = 200 rows | DiabloRim 49 + Summermyst 64 = 113 distinct; 328 tier rows | Too thin: with one enhancement per category, 8 per category leaves high-rarity items samey |
| MGEF templates | ~50 | WD 341 MGEF; DiabloRim runtime effects built-in | Under-budgeted: affix effects + skill effects + monster affixes + utility |
| Data files | ~70 KB total estimate | DiabloRim 27 KB; Summermyst 30 KB | Data volume fine; schemas are richer (drop tables), so slightly higher |
| Scripts | 5-10 Papyrus (UI glue) | WD 46; PROTEUS 25 | Fine - native core replaces most scripts |
| DLL | 1-3 MB estimate | DiabloRim 1.24; PROTEUS 0.56; Grid 3.4 | No gap |
| Assets | none planned in v1 | WD 28 MB; FF 43 MB; PROTEUS SWFs | Real gap: no item icons/rank badges/UI art means a flat Nexus presentation |
| Boss/enemy records | ~10 NPC templates | FF 108 boss NPCs; WD 3 dummy NPCs | Thin roster for five ranks; five tiers need at least one mini-boss and one boss each |
| Localization strings | not sized | WD 237 strings + 173 tokens | Must budget ~250-350 keys |

Estimated draft package: ESL 60-100 KB + data 70 KB + DLL 1-3 MB + scripts 30 KB = ~2-4 MB,
versus references at 1.7 MB (DiabloRim) to 43 MB (Fortunate Foes). Size is not the problem;
**content breadth and presentation art are**.

## 3. Gap list and resolution

### G1 - Affix breadth (blocking for loot quality)

Problem: 40 affixes = 8 per category; with max one enhancement per category (I-03), a rarity-5
item picks one of 8 per category, so the pool feels shallow versus DiabloRim's 49 + 64.
Resolution: raise to **60 affixes, 12 per category, 5 tiers = 300 rows** for v1. Content pack
format already supports overlay files (D-032), so the pool can grow post-release without a
plugin rebuild. Data size impact: ~36 KB affixes.

### G2 - Enhancement effect templates (MGEF) under-budgeted

Problem: L3 reserved 50 MGEF and assumed one template per effect; 60 affixes can share a smaller
number of effect archetypes, but skills and monster affixes also need templates.
Resolution: budget **120-150 MGEF**: ~60 affix effects, ~27 skill effects, ~20 monster-affix
effects, ~20 utility/visual/condition effects. Increase the MGEF FormID range to 256.

### G3 - Record budget and FormID map too tight

Problem: 250-300 planned total underestimates; category caps (e.g. 128 MGEF, 64 SPEL/PERK) leave
little headroom for patches or expansions.
Resolution: revise the map (section 4) to ~1,400 record capacity plus 640 reserved, with planned
~450 records.

### G4 - Enemy and boss roster too thin

Problem: five ranks with ~10 NPC templates cannot fill five RF tiers with variety.
Resolution: **12-16 boss/mini-boss templates** (one mini-boss + one boss per RF tier plus a few
variants) and rank data driven from enemy_ranks.csv; generic ranks stay runtime-applied (no NPC
records). This matches the reference pattern: content packed into NPC/LVLN records only where
hand-authored (Fortunate Foes), everything else distributed/runtime.

### G5 - Assets for public presentation

Problem: v1 had no assets. Reference mods ship 28-43 MB; a Nexus mod with no icons for five
runes, twelve materials and six rarity badges looks unfinished.
Resolution: minimum asset set in v1: **17 item icons** (5 runes + 12 materials), **6 rarity
badges/tints**, **5 rank markers** (Elite..Rune-Mythic), UI frame/background optional. Fallback
if art is not ready: reuse existing vanilla icon paths with tinted variants and ship ~0.2 MB;
replace before release. Budget: 0.5-3 MB, 25-35 DDS files.

### G6 - Localization budget

Problem: strings.csv not sized; reference has 237 record strings + 173 translation tokens.
Resolution: budget **250-350 keys** (60 affixes x name/prefix/suffix, 6 rarities, 18 skills x 3
ranks, 9 attributes, 12 materials, 20 recipes, UI labels, error messages). Strings file ~20-30 KB.

### G7 - Recipe and material depth

Problem: 15 recipes is thin once base craft, rune conversion (4 steps), socket craft, rarity
upgrade (4 steps), reroll, extraction are counted.
Resolution: **20-25 COBJ/data recipes**; materials stay 12 items (4 tiers x 3 families).

### G8 - Drop-table schema coverage

Problem: references do not ship drop tables (DiabloRim uses INI multipliers; WD uses FormLists),
so our tables have no direct size reference.
Resolution: size the tables from content: 6 container tiers, 5 quest bands, 5-6 boss tables,
6 x 5 spawn weights; ~50-80 data rows. Already covered by schema; add examples and validation
ranges in L3.

## 4. Revised FormID map (supersedes L3 section 1)

| Range | Category | Capacity | Planned |
|---|---|---|---|
| 0x800-0x87F | KYWD keywords | 128 | ~60 |
| 0x880-0x8FF | GLOB condition mirrors | 128 | ~40 |
| 0x900-0x9FF | MGEF enhancement/skill/monster templates | 256 | ~150 |
| 0xA00-0xAFF | SPEL active skill spells | 256 | ~40 |
| 0xB00-0xB7F | PERK passive skill perks | 128 | ~30 |
| 0xB80-0xBFF | MISC runes and materials | 128 | ~20 |
| 0xC00-0xC7F | COBJ recipes | 128 | ~25 |
| 0xC80-0xCFF | QUST/FLST | 128 | ~10 |
| 0xD00-0xD7F | NPC_/CLAS boss templates | 128 | ~16 |
| 0xD80-0xFFF | Reserved (patches, expansions, assets-only) | 640 | 0 |

Total capacity 1,408 + 640 reserved; planned ~440-470 records for v1. Still inside one ESL
(2,048 IDs), and patch/expansion plugins can be separate ESLs.

## 5. Revised v1 content targets (input to L3 update)

| Artifact | v1 target | Estimated size |
|---|---|---|
| Affixes (CSV) | 60 affixes x 5 tiers = 300 rows | ~36 KB |
| Rarity / weights | 6 rarities, 36 weight cells | ~3 KB |
| Enemy ranks / spawn | 5 ranks, 30 spawn-weight cells | ~4 KB |
| Drop tables | 6 containers, 5 quest bands, 6 boss tables | ~8 KB |
| Materials / recipes | 12 materials, 20-25 recipes | ~6 KB |
| Skills / attributes | 18 skills, 9 attributes, 54 rank gates | ~5 KB |
| Strings | 250-350 keys | ~25 KB |
| ESL records | ~440-470 across the map | 80-140 KB |
| Assets | 17 icons + 6 rarity + 5 rank markers (min) | 0.5-3 MB |
| Scripts | 5-10 Papyrus | 30-60 KB |
| DLL | native core | 1-3 MB |

Package estimate: **4-8 MB** (versus DiabloRim 1.7 MB, Worlds Dawn 28 MB, Fortunate Foes 43 MB).
Comparable in scope, lighter than asset-heavy overhauls, heavier in data than pure loot engines.

## 6. Workload comparison and effort note

- Affix authoring: 300 rows (DiabloRim base 136 + overlay 192 = 328) - equivalent to shipping
  both reference tables at once.
- Records: ~450 (between DiabloRim 0 and Fortunate Foes 415 / Worlds Dawn 742) - reasonable.
- Scripts: 5-10 (versus 25-46) because the native core owns loot/tiers/save.
- Assets: 25-35 files (versus 241 in Worlds Dawn) - minimal but present.
- The largest effort is not volume but balance and compatibility testing; the content volume is
  proven feasible by the references.

## 7. Resolutions applied

- L3 FormID map and record budget replaced with section 4 above (D-035).
- L3 affix target 40 -> 60 (12 per category), MGEF budget 50 -> 150 (D-036).
- L3 enemy/boss target 10 -> 12-16; recipes 15 -> 20-25; localization budget 250-350 keys.
- Assets are now a v1 requirement with a minimal set and a documented fallback (D-037).
- Drop-table schemas get concrete row targets and examples in L3.

## 8. Risks created by the resize

1. 60 affixes x 5 tiers is a real authoring/balance load; mitigate with a balance spreadsheet and
   data-only iteration (no rebuild).
2. 150 MGEF templates raise the chance of effect-name conflicts with other mods; keep our own
   names/EditorIDs and prefer generic effect archetypes with runtime magnitudes.
3. Asset dependency: icons need an artist or curated CC0 set; if late, the fallback keeps the
   mod functional but visually flat.
4. More records = more to validate; the L3 validator and deterministic build must land before
   content authoring starts.
