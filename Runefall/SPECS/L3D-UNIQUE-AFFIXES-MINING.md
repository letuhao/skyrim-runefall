# L3D - Unique affixes and data mining

Status: 2026-10-26. Gap found in the content audit: the 120-affix core model has no one-of-a-kind
rewards from boss fights and quests. This document defines the unique-affix content class, the
data-mining plan against the official game files, and the tool needed to execute it.

## 1. The gap

Core affixes are rollable, tiered and category-locked. They cannot express:

- a boss dropping its signature effect (e.g. a named dragon's elemental burst),
- a quest granting a unique rune-forged affix,
- a one-time reward that feels authored, not generated.

Unique affixes solve this: fixed effects, no tiers, tied to a specific source, never in the
random pools. They must be sourced from real game content (vanilla + official DLC) so each one
corresponds to a recognizable boss, quest or artifact.

## 2. Data-mining sources

Official masters only, from the target game folder (read-only):

    H:\Skyrim\MantrasOfMara\Stock Game\Data\
      Skyrim.esm
      Update.esm
      Dawnguard.esm
      HearthFires.esm
      Dragonborn.esm

Not mined in v1: Creation Club content, other mods, BSA interiors (records only). The same run
can later be pointed at another supported Data folder for parity checks; 1.7.104 is deferred (D-062).

## 3. What to mine

| Target | Why | Detection |
|---|---|---|
| Named unique WEAP/ARMO | Candidate unique item rewards and boss drops | record has a non-generic name; enchantment present; unique/quest flag where exposed; EditorID patterns (DA, MQ, TG, DB, CW, DLC) |
| ENCH records | Unique enchantment effects and their components | all ENCH with effects; flag those used by named items |
| MGEF records | Effect vocabulary for unique affixes | archetype, resistance/av links, names |
| Named unique NPCs | Boss sources and their equipment | unique flag if exposed, high level, unique name, outfit/inventory |
| QUST records | Quest sources and reward context | quest name/flags/stages; reward mapping is heuristic (quest item flags, named items, reward FLSTs) |
| FLST records | Reward and loot pools | names containing Reward/Boss/Artifact/Unique |
| CONT records | Boss/quest reward containers | named containers, boss chests |

Output is evidence, not content: FormKeys and EditorIDs are references; names/descriptions are
read for classification only and are not copied into the mod.

## 4. Mining tool specification

Runefall\tools\DataMiner (C#, net8.0, Mutagen.Bethesda.Skyrim 0.50.2+, read-only overlays).

    dotnet run --project Runefall\tools\DataMiner -- \
      --data-dir "H:\Skyrim\MantrasOfMara\Stock Game\Data" \
      --out "Runefall\SPECS\data\mining"

Outputs:

    unique-items.json      formKey, editorId, name, type, enchantment, effects, flags, plugin
    unique-enchants.json   editorId, name, effects (mgef/editorId, magnitude, area, duration), users
    unique-npcs.json       editorId, name, uniqueFlag, level, class, outfit, inventory items, plugin
    quests.json            editorId, name, flags, stages, aliases, plugin
    candidates.csv         sourceType, sourceName, editorId, plugin, confidence, reason
    MINING-REPORT.md       counts per plugin and category, top candidates, coverage by DLC

Detection and ranking:

1. Hard signals: unique flag (NPC), quest item flag (item) if exposed; enchantment on a named
   item; EditorID prefix patterns (DA, MQ, TG, DB, CW, DLC1, DLC2).
2. Soft signals: non-generic name, named container/outfit, reward/artifact/unique keywords.
3. Confidence: HIGH = hard signal + named + enchantment/unique NPC; MEDIUM = two soft signals;
   LOW = one soft signal.
4. Dedupe by FormKey; classify source type (item/npc/quest/container); group by DLC.

Provenance and policy:

- Read-only; the tool writes only under SPECS\data\mining.
- No Bethesda assets, strings or record data are shipped in the mod. Unique affixes are original
  effects and names; the mined FormKey/EditorID is stored only as a source reference in our data
  files (allowed: referencing vanilla records).
- The mining output is design evidence; review before adopting a candidate.

## 5. From mining to unique-affix content

Pipeline:

1. Mine -> candidates.csv with confidence and source type.
2. Review: accept HIGH confidence bosses/quests first, then MEDIUM; reject duplicates and
   artifacts that cannot be tied to combat/quest flow.
3. Map each accepted source to one unique affix:
   sourceType, sourceEditorId, affix name (original), prefix, suffix, MGEF template (ours),
   effect family, drop rule (guaranteed first kill/completion then disabled/rare), pack
   (Runefall - Unique.esp), localization key.
4. Count U = accepted boss sources + accepted quest sources; update L3B section 11 and the pack
   manifest. Target v1 U = 20-40 (bounded by mining results and review capacity).
5. Validate: no duplicate affix per source, no affix without a source, drop rule reachable,
   records inside the pack's FormID budget, localization keys present.

## 6. Open items

- Mutagen property names for NPC unique flag and item quest flag must be confirmed in the tool
  code (compile-time check); fallback is EditorID/name patterns.
- Quest reward mapping is heuristic; the first mining pass classifies quest sources, a manual
  review confirms them.
- Which DLC sources are in scope: all official DLC for v1; CC later.
- Unique affix drop rules must not conflict with boss respawn logic; resolved in L4.

## 7. Mining results (run 2026-10-26)

Tool: Runefall\tools\DataMiner (built, ran read-only over five official masters).
Data dir: H:\Skyrim\MantrasOfMara\Stock Game\Data
Output: Runefall\SPECS\data\mining\ (candidates.csv 1.0 MB, unique-items.json 2.0 MB,
unique-enchants.json 271 KB, unique-npcs.json 1.25 MB, quests.json 576 KB, MINING-REPORT.md).

| Record type | Mined | HIGH candidates | MEDIUM candidates |
|---|---|---|---|
| Weapons | 3,293 | 318 unique-name | part of 356 |
| Armors | 3,835 | 1,135 unique-name | part of 356 |
| Enchantments | 752 | effect vocabulary | - |
| NPCs | 6,525 | 31 unique+boss pattern | 1,322 unique-flag |
| Quests | 2,391 | - | 48 reward-pattern |

Quality notes (manual review required):

- The HIGH item pool (1,453) is a candidate list of named weapons/armors, not 1,453 unique
  designs. It is the effect/reward vocabulary for unique affixes; selection is manual.
- The 31 HIGH NPCs include real bosses (Morokei, Vulthuryol, Rahgot, Alduin) and false positives
  (Dorian, a goat) from name patterns. Boss selection is manual against the RF tier plan.
- The 48 quest candidates include handler/radiant quests (Thieves Guild handlers) and real
  reward quests (Shalidor's Insights Rewards). Manual acceptance is required.
- Enchantment records (752) supply the effect vocabulary (MGEF links, magnitudes) for unique
  affix design; no Bethesda strings are copied.

## 8. Unique-affix count derived from mining

    B_unique = 20 boss templates (5 RF tiers x 4 templates, L3C)
    Q_unique = 20 accepted quest sources (from 48 mined candidates, ~40% acceptance)
    U = B_unique + Q_unique = 40 unique affixes

This supersedes the earlier 20-40 placeholder: v1 ships exactly 40 unique affixes, 20 boss-bound
and 20 quest-bound. Each is one record MGEF + one prefix + one suffix + one source rule; no
tiers. Pack: Runefall - Unique.esp; ~40 MGEF + ~40 names + 40 data rows, far inside 2,048 IDs.
Acceptance rule: a source is accepted only if the review confirms it is a unique boss encounter
or a quest with a concrete reward moment; HIGH-confidence lists are reviewed first.

## 9. Race mining addition (G12)

DataMiner was extended to mine RACE records and NPC race usage:

- 204 RACE records mined; 161 unique race EditorIDs after merging.
- 29 thematic families; families with >= 10 NPC actors qualify: **F = 19**
  (Draugr 368, Vampire 229, Skeleton 107, Daedra 103, Dragon 79, Falmer 79, Spider 66,
  Dwarven Automaton 38, Werewolf 30, Riekling 28, Wolf 25, Dragon Priest 22, Hagraven 18,
  Troll 15, Spriggan 13, Chaurus 13, Giant 11, Sabre Cat 11, Mammoth 10).
- Output: SPECS\data\mining\races.json (editorId, name, plugin, npcCount).
- Hunt rewards: 19 unique affixes + 19 hunt materials in Runefall - Hunt.esp; general table is
  data-only (36 rows).

## 10. Tool execution plan (this milestone)

1. Build DataMiner, run against the five masters.
2. Produce outputs + MINING-REPORT.md with counts.
3. Feed accepted candidates into unique_affixes.csv schema and the Unique pack manifest.
4. Record the run hash and counts in VALIDATION.md.
