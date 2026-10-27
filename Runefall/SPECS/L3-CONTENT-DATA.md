# L3 - Content and data architecture

Status: draft 2026-10-26. Depends on L1 (gameplay), L2 (Skyrim mapping), D-023/D-029/D-030
(dynamic loot) and D-027/D-028 (no vanilla record mutation; co-save is authoritative).
Content budgets and the FormID map were revised after the reference-size audit
(SPECS\L3A-CONTENT-SIZE-COMPARISON.md) and then derived mathematically in
SPECS\L3B-CONTENT-MATH.md (D-038: 108 affixes = 9 attributes x 12; D-039 coverage gates).

Purpose: define exactly what ships as records, what ships as data files, how records get stable
IDs, how content is generated and validated, and how the runtime consumes it.

Core rule: **forms only where the engine requires a form; everything tunable is data**.

- Records: definitions that must exist as game forms (keywords, magic effects, spells, perks,
  items, recipes, quest hosts, boss templates, condition mirrors).
- Data files: everything the DLL evaluates at runtime (affix tables, drop tables, rank curves,
  material yields, skill tables, attribute tables, config).
- Instances: item rarity/enhancements are generated at runtime (D-023); they never become forms.

## 1. Plugin and pack scheme (multiple ESL plugins)

Runefall ships as a **base plugin plus independent content expansion packs**, all ESL-flagged
(ESP files with the ESL flag, so each costs a light slot and never a full plugin slot):

| Plugin | Role | Required | Content |
|---|---|---|---|
| Runefall.esp | Base | yes | Core schema: keywords, globals, 120 affix MGEF templates (~201 MGEF total), skills (SPEL/PERK), runes/materials (MISC), recipes (COBJ), quest hosts, boss templates, data files manifest |
| Runefall - Unique.esp | Pack | no | Unique affixes sourced from boss/quest mining (40: 20 boss + 20 quest) |
| Runefall - Hunt.esp | Pack | no | Race hunt content: 19 family unique affixes + 19 hunt materials + race tables (G12) |
| Runefall - Crafting.esp | Expansion (reserved) | no | Crafting Table expansion: base-item recipes, targeted affix crafting, Affix Shards, craft UI (D-045). Not shipped in v1. |
| Runefall - Bosses.esp | Pack | no | Expansion boss/mini-boss roster beyond the base templates |
| Runefall - Affixes.esp | Pack | no | Additional affix families (data + their MGEF templates) |
| Runefall - Quests.esp | Pack | no | Quest-bound unique affix rewards and their quest record glue |
| Runefall - Patches.esp | Pack | no | Per-mod compatibility records |

Rules that make this work without master chains:

1. **Each plugin is self-contained.** Packs do not master the base plugin and never reference
   base FormIDs. They reference vanilla records only, and they reference our cross-plugin
   content **by EditorID through data files**, which the DLL resolves across all loaded Runefall
   plugins at runtime.
2. **No cross-plugin FormLink.** If a pack needs shared behavior, it ships its own records and
   declares the dependency in its data manifest; the DLL validates that the base plugin is
   present and at a compatible data-format version.
3. **One FormID window per plugin.** Every ESL owns 0x800-0xFFF independently (2,048 IDs each).
   The category allocation below applies inside every plugin, so a pack can be added or removed
   without renumbering anything.
4. **Load order.** Base first, packs after it; pack order is free because packs are independent.
   The DLL logs loaded Runefall plugins and their pack manifests at startup.
5. **Pack manifest.** Each pack ships Data\Runefall\packs\<pack>.json declaring: plugin file
   name, pack id, data-format version, minimum base version, content files, record EditorID
   counts. The DLL refuses a pack whose base version is incompatible and falls back to base-only.

If a future pack truly needs to reference base records directly, the base would have to become
an ESM+ESL master; that is deliberately avoided now to keep packs removable.

### 1.1 FormID allocation inside every plugin

Light-plugin FormID window 0x800-0xFFF (2,048 objects maximum per plugin). Allocation is fixed
by category so records never move and external data references stay stable.

| Range | Category | Capacity | Planned |
|---|---|---|---|
| 0x800-0x87F | KYWD keywords (rarity, rank, material tier/family, socket markers) | 128 | ~60 |
| 0x880-0x8FF | GLOB condition mirrors (attributes, rank state, feature gates) | 128 | ~40 |
| 0x900-0x9FF | MGEF enhancement/skill/monster/hunt effect templates | 256 | ~201 (120 affix + 62 skill/monster/utility + 19 hunt) |
| 0xA00-0xAFF | SPEL active-skill spells | 256 | ~40 |
| 0xB00-0xB7F | PERK passive-skill perks | 128 | ~30 |
| 0xB80-0xBFF | MISC runes and material items | 128 | ~39 (5 runes + 12 base materials + 19 hunt materials + spares) |
| 0xC00-0xC7F | COBJ deterministic recipes (v1: rune conversion + socket only) | 128 | ~8 |
| 0xC80-0xCFF | QUST script/event hosts and FLST pools | 128 | ~10 |
| 0xD00-0xD7F | NPC_/CLAS boss and mini-boss templates | 128 | ~16 |
| 0xD80-0xFFF | Reserved (patches, expansions, future modules) | 640 | 0 |

Total capacity 1,408 + 640 reserved per plugin; base planned ~465-495 records for v1, inside
one ESL (2,048 IDs). The Crafting Table expansion adds its own records later (estimated ~17
COBJ + craft records) in Runefall - Crafting.esp. Each expansion pack has its own 2,048-ID budget; a unique-affix pack of
~40 unique effects is a small fraction of that. The ESL flag is mandatory; the validator rejects any record outside its category
range or above the cap.

EditorID convention (ASCII, stable, no spaces):

    RF_KW_<Category>_<Name>        e.g. RF_KW_Rarity_Mythic, RF_KW_Rank_Elite
    RF_GLOB_<System>_<Name>        e.g. RF_GLOB_Attr_Might, RF_GLOB_State_Attuned
    RF_MGEF_<Category>_<Effect>    e.g. RF_MGEF_Offense_MeleeDamage
    RF_SPEL_Skill_<Skill>_<Rank>   e.g. RF_SPEL_Skill_Cleave_2
    RF_PERK_Skill_<Skill>_<Rank>   e.g. RF_PERK_Skill_Ironclad_3
    RF_MISC_Rune_<Rank>            e.g. RF_MISC_Rune_3
    RF_MISC_Mat_<Tier>_<Family>    e.g. RF_MISC_Mat_Refined_Ore
    RF_COBJ_<Recipe>               e.g. RF_COBJ_RuneConvert_3
    RF_QUST_<Purpose>              e.g. RF_QUST_Core, RF_QUST_Debug
    RF_NPC_Boss_<Name> / RF_CLAS_<Role>

External data refers to records by EditorID (resolved by the DLL at load from the Runefall
plugins), never by hardcoded FormID. This keeps CSV/JSON authoring readable and survives load
order. The build validator enforces one-to-one EditorID existence and type.

### 1.2 Pack manifest schema

Each pack ships Data\Runefall\packs\<packId>.json:

    {
      "packId": "unique",
      "plugin": "Runefall - Unique.esp",
      "format": 1,
      "minBaseVersion": "0.1.0",
      "contentFiles": ["data/unique_affixes.csv", "data/unique_sources.csv"],
      "recordEditors": { "MGEF": ["RF_U_MGEF_..."], "ENCH": ["..."] },
      "conflicts": []
    }

The DLL loads the base first, then every pack manifest in load order, validates format and base
version, and merges pack data tables. A missing or incompatible pack is disabled with a log line
and the base continues to work. The FOMOD install matrix and the build validator both check that
a pack's declared records exist only in that pack.

## 2. Record set (what the ESL contains)

| Type | Purpose | Count |
|---|---|---|
| KYWD | Rarity bands, enemy ranks, material tiers/families, socket/rune markers, scratch markers | ~60 |
| GLOB | Condition mirrors for perks/UI: 9 attribute values, skill rank state, RF tier, loot authority mode, feature gates | ~40 |
| MGEF | Effect templates: affix effects (120), skill effects (~27), monster-affix effects (~20), hunt uniques (19), utility/visual/conditions (~15); magnitude/duration set at runtime | ~201 |
| SPEL | Active skill abilities, 3 ranks each (9 active lines) - about 27, plus variants | ~40 |
| PERK | Passive skill lines, 3 ranks each (9 passive lines) | ~30 |
| MISC | Rune R1-R5, 12 base materials, 19 hunt materials | ~39 |
| COBJ | v1 deterministic recipes: rune conversion (4) + socket craft (1-2) + utilities (2) | ~8 |
| QUST | Core host (events, mod-event bridge), debug host, attunement host | 3 |
| FLST | Small pools for UI/validation (rarely used) | ~5-10 |
| NPC_/CLAS | Mini-boss and boss templates across the five RF tiers | 20 |

Not generated: no ARMO/WEAP base items, no ENCH variants, no leveled lists, no cells or world
edits, no dialogue. Boss templates are NPC records only and spawn through the native engine at
fixed sites (D-026).

### 2.1 Assets (v1 minimum)

The reference audit (L3A) flagged presentation as the real gap. The v1 asset budget is:

- 17 item icons: 5 rune ranks + 12 materials (DDS, 256x256 or 512x512).
- 6 rarity badges or tint definitions.
- 5 rank markers (Elite, Champion, Mini-boss, Boss, Rune-Mythic) for the UI badge.
- Optional: UI frame/background, 3-5 boss aura/VFX choices reusing vanilla effects.

Fallback if art is late: reuse existing vanilla icon paths with tint variants and ship the rank
badges as UI-only markers; replace before release. Assets are loaded from Data\Runefall\assets.

## 3. Data files (what the DLL consumes)

Shipped loose under Data\Runefall\ so balance edits never require a plugin rebuild.

    Data\Runefall\
      runefall.ini
      data\
        formats.json          schema version + expected column sets per file
        affixes.csv
        rarity.csv
        rarity_weights.csv
        enemy_ranks.csv
        spawn_weights.csv
        drops_containers.csv
        drops_quests.csv
        drops_bosses.csv
        drops_general.csv     baseline enemy table: rank x RF tier (36 rows)
        race_groups.csv       hunt families and their member RACE EditorIDs (Hunt pack)
        drops_races.csv       race hunt reward rows (unique affix + material per family)
        materials.csv
        recipes.csv
        skills.csv
        attributes.csv
        unique_affixes.csv    unique pack: one row per unique affix (no tiers)
        unique_sources.csv    unique pack: source rules (boss/quest EditorID -> affix, drop rule)
        strings.csv           optional localization key map

All CSV files are UTF-8, comma-separated, first line is a header, '#' lines are comments, no
trailing spaces. Booleans are 0/1. Percent values are stored as numbers where the column name
ends in Pct (e.g. 12.5 = 12.5 percent). Every file declares its schema version in formats.json.

### 3.1 affixes.csv

    affixId,name,category,tier,unit,slots,minValue,maxValue,duration,area,weight,minItemLevel,effectEditorId,prefix,suffix,description

- affixId: stable string, lowercase, unique per affix family (rows repeat for tiers 1-5).
- name: display name; tier-specific wording allowed.
- category: offense, defense, utility, economy, elemental (one enhancement per category per item).
- tier: 1-5 (magnitude tier, gated by ilvl).
- unit: flat, pct, seconds, toggle.
- slots: pipe-separated slot tokens (WEAPON, ONEHANDED, ARMOR, HEAD, HANDS, FEET, RING, AMULET, SHIELD, BOW, STAFF).
- minValue/maxValue: roll range for the tier.
- duration/area: magic effect fields, 0 when unused.
- weight: selection weight within its category.
- minItemLevel: gate.
- effectEditorId: record in the ESL (RF_MGEF_...). Validator resolves it.
- prefix/suffix: name parts; empty allowed.
- description: optional tooltip text key.

V1 content target (derived in SPECS\L3B-CONTENT-MATH.md, benchmark-adjusted in L3C):
120 affixes = 12 x ceil(113/12), five tiers each => 600 rows; category pools 23-24. A further
set of **unique affixes** (no tiers, one-of-a-kind drops from bosses and quests) ships in the
Runefall - Unique pack; its count is derived from the data-mining results (L3D). Content authors
edit CSV only; the format supports overlay packs, so the pool can grow after release without a
plugin rebuild.

### 3.2 rarity.csv and rarity_weights.csv

    rarity.csv: rarityId,name,enhancementCount,socketMin,socketMax,colorToken,prefixStyle
    0,Common,0,0,0,white,none
    1,Refined,1,0,1,green,light
    ...
    5,Runeforged,5,3,3,gold,full

    rarity_weights.csv: rfTier,rarityId,weight
    0,0,55.0   0,1,25.0   ... 0,5,0.1
    5,0,20.0   ...          5,5,4.0

Weights are relative; the DLL normalizes. Magic find and enemy-rank bonuses are applied by the
DLL on top of these base weights (caps in runefall.ini).

### 3.3 enemy_ranks.csv and spawn_weights.csv

    enemy_ranks.csv: rank,name,hpPct,dmgPct,armorPct,affixCount,itemRarityBonus,runeRank,runeMin,runeMax,materialTierBonus
    1,Elite,50,20,10,1,2,1,1,2,0
    ...
    5,Rune-Mythic,900,150,50,4,12,5,5,10,3

    spawn_weights.csv: rfTier,rank,weight    (generic spawns; ranks 4-5 normally 0)

### 3.4 Drop tables

    drops_containers.csv: containerTier,itemChancePct,rarityBias,itemCountMin,itemCountMax,runeMin,runeMax,extraRollChancePct
    drops_quests.csv: questBand,itemCount,minRarity,runeMin,runeMax
    drops_bosses.csv: bossId,itemCount,guaranteedRarity,runeMin,runeMax,materialTier,uniqueTableId

Per-container instance roll state is persisted in the co-save (D-024); the tables are pure data.

### 3.4.1 General and race hunt tables (G11/G12)

    drops_general.csv: rank,rfTier,itemChancePct,itemCountMin,itemCountMax,rarityBias,runeMin,runeMax,uniqueChancePct
    rows = 6 ranks x 6 RF tiers = 36

    race_groups.csv: raceGroup,memberRaceEditorIds,minRfTier,uniqueAffixId,materialId,description
    rows = 19 families (families with >= 10 NPC actors in the official race inventory)

    drops_races.csv: raceGroup,rewardType,rewardId,dropChancePct,firstOnly,note
    rows = 19 families x 2 rewards = 38

Race groups are mined references (RACE EditorIDs); the Hunt pack supplies the unique affix and
material records. Full design and drop-rate rules: SPECS\L3E-HUNT-AND-DROP-TABLES.md.


### 3.5 materials.csv and recipes.csv

    materials.csv: materialId,name,tier,family,itemEditorId,salvageYieldMin,salvageYieldMax,source
    family: ore, essence, hide
    source: salvage, node, drop

    recipes.csv: recipeId,outputType,materials,runeCost,ilvlBand,requirementSkill,requirementRank
    v1 outputType: rune_convert, socket_craft, utility
    expansion outputType (reserved): base_item, affix_craft, shard_craft, socket_add
    materials: pipe list materialId:count
    v1 rows: ~8; expansion adds ~17 recipes in its own pack data (D-045)

Deterministic recipes are also mirrored as COBJ records for the vanilla crafting UI; the native
path validates costs for reroll/upgrade runs that COBJ cannot express.

### 3.5.1 unique_affixes.csv and unique_sources.csv (Runefall - Unique pack)

    unique_affixes.csv: uniqueId,name,prefix,suffix,category,effectEditorId,minValue,maxValue,duration,area,slots,description
    unique_sources.csv: uniqueId,sourceType,sourceEditorId,dropRule,firstOnly,note

    sourceType: boss or quest
    dropRule: guaranteed_first | guaranteed_first_then_rare
    firstOnly: 0/1 (default 1)
    sourceEditorId: vanilla NPC/QUST EditorID mined by DataMiner (reference only)

59 unique rows: 40 boss/quest uniques + 19 hunt uniques (sourceType boss, quest or race),
and 59 matching source rows. Unique affixes are never tiered and never appear in random drop
tables; the source rules are validated at build time.

### 3.6 skills.csv and attributes.csv

    skills.csv: build,skillName,skyrimSkill,attribute,type,rank1,rank2,rank3,gateSkill,gateAttribute,description
    type: active or passive
    rank1..rank3: EditorIDs of the SPEL/PERK for each rank (validator resolves)

    attributes.csv: attribute,build,effectKey,perPoint,cap,softCap
    effectKey is a stable key the DLL maps to an internal stat modifier; it is not a record ID.

These two files are the single source of truth for the character sheet; the DLL reads them at
startup, mirrors values into GLOBs for perk conditions, and the UI renders from them.

### 3.7 runefall.ini

Sections: [General] enable/disable, logging, loot authority, feature toggles; [Caps] per-attribute
caps and stat caps; [Loot] magic find curve, pity thresholds, container roll state TTL; [UI]
framework selection and layout; [Debug] developer commands. Defaults ship commented.

### 3.8 strings.csv (localization staging)

    key,english,context
    affix.health.name,of Health,affix
    rarity.5,Runeforged,rarity
    skill.cleave.name,Cleave,skill

English ships in v1. Record FULL strings live in the ESL; data-file text resolves through this
table so a later translation pack only swaps strings.csv.

## 4. Build pipeline (Mutagen generator + validator)

Project tool: Runefall\tools\Runefall.Build (C#, net8.0, Mutagen current release). One command
does everything deterministically:

    dotnet run --project Runefall\tools\Runefall.Build -- build --out <snapshot dir>

Steps:

1. Load and validate data files first (formats.json schema version, headers, types, enums,
   ranges, cross-references).
2. Load content manifests (content-src\*.json) that declare records to generate: keywords,
   globals, MGEFs, spells, perks, MISC, COBJ, quests, boss NPCs.
3. Resolve references: affix effectEditorId -> MGEF; skill ranks -> SPEL/PERK; materials ->
   MISC; recipes -> MISC/COBJ; boss tables -> NPC records.
4. Allocate FormIDs by category range in a stable order (manifest order + explicit sort key);
   fail if a category exceeds its cap.
5. Emit Runefall.esp (ESL flag) with deterministic content; write validation report
   (counts per range, unresolved refs, warnings).
6. Copy data files and runefall.ini into the snapshot layout; write hashes and the content
   manifest hash for reproducibility.
7. Run the validator bundle: FormID ranges, ESL flag, no vanilla overrides, no leveled lists/
   cells/dialogue, record count caps, EditorID uniqueness, data-file cross refs, localization
   coverage for player-facing strings, schema versions.

Build output is reproducible: same inputs -> same bytes (ordered generation, no timestamps in
records, fixed hash metadata).

## 5. Snapshot and module layout (multi-ESL)

    Runefall 0.1.x\
      Data\
        Runefall.esp                  base plugin (ESL)
        Runefall - Unique.esp         unique-affix pack (ESL, optional)
        Runefall - Bosses.esp         boss expansion pack (ESL, optional)
        Runefall - Affixes.esp        affix expansion pack (ESL, optional)
        Runefall - Quests.esp         quest unique-reward pack (ESL, optional)
        Runefall.esp?                 version check: base plugin identity is stable
        Runefall                     data files + ini + packs*.json manifests
        Scripts...                   Papyrus (L4)
        SKSEPluginsRunefall.dll
        Interface...                 optional UI resources
      fomod\
      docs\

FOMOD modules (defined here, built in L5):

- Core (required): base ESL + data + DLL + core scripts.
- Unique (optional): Runefall - Unique.esp + its data files.
- Hunt (optional): Runefall - Hunt.esp, race tables and 19 family uniques/materials.
- Crafting (expansion, reserved): Runefall - Crafting.esp + craft data; not part of v1.
- Bosses (optional): boss expansion pack.
- Affixes (optional): affix expansion pack.
- Quests (optional): quest-bound unique rewards.
- UI (optional): SKSE Menu Framework pages - the DLL exposes the API regardless.
- Patches (optional): per-mod compatibility files (OWL, DiabloRim coexistence, overhauls).
- Balance pack (optional): alternate data files only, no plugin changes.

Every module except Core is independent; removing it must not break the base (validated by the
DLL manifest check and by the FOMOD install matrix).

## 6. Validation rules (build fails on errors, warns on balance)

Errors: missing/invalid schema version, unknown column, type mismatch, out-of-range enum, missing
referenced EditorID, duplicate EditorID, record outside its FormID category range or over cap,
ESL flag missing, vanilla record override, leveled list/cell/dialogue record, data file missing,
localization key missing for a shipped FULL string, recipe referencing unknown material.
Warnings: affix values outside the expected band, weight sums far from expected, drop rates above
100 percent, rank multipliers creating near-unwinnable early tiers, sparse categories, unused
records.

Hunt/drop-table gates (L3E section 7): every race group has >= 2 member RACE EditorIDs that exist
in the mined official inventory; every family has exactly one unique affix and one material row;
drops_general covers all 36 rank x RF-tier cells exactly once; firstOnly = 1 only for unique
affixes; hunt uniques are excluded from all random affix pools; every hunt material has a MISC
record and a crafting/salvage use.

## 7. Mapping L1/L2 mechanics to L3 artifacts

| L1/L2 mechanic | L3 artifact |
|---|---|
| R-02 rune ladder | RF_MISC_Rune_1..5 + rune conversion COBJ records + recipes.csv |
| R-03 sources/sinks | enemy_ranks.csv rune columns, drops_*.csv, recipes.csv costs |
| I-02 rarity bands | rarity.csv + RF_KW_Rarity_* keywords |
| I-03 enhancement categories | affixes.csv category + slots columns, RF_MGEF templates |
| I-05 sockets | rarity.csv socket ranges, socket markers, socket COBJ |
| L-03/L-04 loot pipeline | affixes.csv, rarity_weights.csv, drop tables, DLL pipeline (L4) |
| T-01/T-02 ranks | enemy_ranks.csv + RF_KW_Rank_* |
| T-03 monster affixes | MGEF/SPEL templates + spawn data (L4 application) |
| T-06 attunement | RF_QUST_Core + recipe costs |
| C-01/C-02 materials | materials.csv + RF_MISC_Mat_* |
| C-03..C-06 crafting | recipes.csv + COBJ records + DLL validation |
| P-01 attributes | attributes.csv + RF_GLOB_Attr_* |
| P-04 skills | skills.csv + RF_SPEL_Skill_* / RF_PERK_Skill_* |
| E-01/E-02 economy | data tables above; DLL math |
| UI surfaces | strings.csv + UI module resources (L4/L5) |

## 8. Content authoring workflow

1. Editing balance or adding an affix/material/drop: edit CSV only; run the build validator in
   data-only mode (no plugin rebuild needed for runtime changes, but validator still checks).
2. Adding a new record type or effect: add manifest entry + generator code, rebuild ESL, bump
   data format version if columns change.
3. Every build records: input hashes, ESL hash, data hash, plugin record counts, unresolved
   warnings. Store in VALIDATION.md.
4. Third-party content packs: same schemas, different file prefix (e.g. affixes_mypack.csv) loaded
   after core, following the DiabloRim merge-overlay precedent; validator can check a pack
   against the core schema.

## 9. Decisions recorded by this layer

- D-031 Fixed ESL FormID category ranges and EditorID convention; validator enforces both.
- D-032 Data files are versioned schemas with a single formats.json; CSV is the authoring format,
  resolved by EditorID, never hardcoded FormIDs.
- D-033 Deterministic Mutagen build with an integrated validator; same inputs -> same bytes.
- D-034 Module split: Core / UI / Patches / Balance pack; balance pack ships data only.

## 10. Open items carried into L4/L5

- DLL load order: data validation at startup, failure behavior (disable feature vs disable mod),
  logging.
- Hot reload of data files (developer-only vs public), if any.
- UI reads tables through native API or through its own CSV parsing (prefer native API).
- Localization packaging: per-language strings.csv plus record string files.
- Compatibility patch format for OWL/DiabloRim (data-only vs patch plugin).
- Build machine reproducibility details (SDK versions, NuGet lock).

## 11. Dev tooling for this layer

- Mutagen (current release) for ESL generation and record validation - already proven with
  esp-explorer; upgrade from 0.50.2 when the build tool is created.
- Paperless checks: schema validator written in C# in the build tool; no external tools needed.
- Optional later: houseCARL/SkyrimForge for record QA on the target list (not runtime deps).