# DECISIONS - Runefall

Architecture decision records. Status: accepted / pending / rejected.

## D-001 Own mod instead of extending DiabloRim (accepted)
DiabloRim is a closed compiled DLL with no Papyrus API and no source; its extension surface
is only the affix CSV, INI keys and Grid Inventory messaging. Progression and enemy tiers
cannot live there. Its ideas are used as design references, never as code.
Evidence: _context\rpg-mechanism\01-DIABLORIM.md, 02-GRID-INVENTORY.md.

## D-002 Native core on CommonLibSSE-NG (accepted; target amended by D-062)
One DLL built from the active **alandtse fork v14.0.1** (commit 723cfda480ae, 2026-10-25), MIT
library, MIT-compatible linking. Multi-runtime capability remains, but the supported/tested v1
target is 1.6.1170 with its SKSE 1.6.1170 and Address Library 1.6.1170 bins (D-062).

## D-003 One ESL content plugin, new forms only (accepted)
The rune currency, materials, keywords, affixes, recipes and effect records ship in
Runefall.esp (ESL-flagged). No vanilla records, leveled lists, containers or cells edited.

## D-004 Runtime application, not record overrides (accepted)
Buffs, tiers and loot rolls are applied by the DLL at runtime. This is the foundation of the
maximum-compatibility claim, together with exclusive keywords and caps.

## D-005 SkyPatcher is an optional patch layer only (accepted)
SkyPatcher cannot create forms; using it for core content would create a hard dependency and
version-specific operations. Core must never depend on it.

## D-006 No vanilla actor-value repurposing (accepted)
Worlds Dawn's 28-AV technique is rejected as a conflict bomb. Attribute values live in the
native co-save plus our own GLOBAL records; derived effects use our own PERK/MGEF/SPEL
records and are re-applied on load.

## D-007 Currency as MISC item plus native cache (pending confirmation)
Visible, uninstall-safe, and fast through a native counter cache. Alternative: pure native
counter (no item) - less transparent, harder to recover on uninstall.

## D-008 Additive loot default, takeover as a separate module (accepted)
Avoids double-loot and economy conflicts with OWL and similar mods in the target list.

## D-009 Version targets (accepted; superseded in part by D-062)
Release and test target: 1.6.1170 (Mantras of Mara MO2). 1.7.104 deferred/unsupported for v1.
1.5.97 out of scope for v1.

## D-010 UI framework (pending)
Options: SKSE Menu Framework (installed in target list, C++ ImGui pages), Prisma UI
(installed in target list, web UI), or MCM Helper only (fallback). Decide in M1.

## D-011 Name (pending)
Working name Runefall. Public name review needed: exact match with a 2018-2019 casual game
series; Skyrim ecosystem is free of collisions. Alternatives: Runegrasp, Runemark, Runesurge.

## D-012 Learn from reference mods, never copy them (accepted)
DiabloRim is closed-source and Nexus permissions are unverified; Worlds Dawn scripts are
legacy (2016) and buggy; Fortunate Foes INIs contain authoring defects. Reuse patterns and
grammar ideas only, with our own implementation.

## D-013 Design-first layered specs (accepted 2026-10-26)
No gameplay code or content forms are built before the layered specs are reviewed:
L1 gameplay model -> L2 Skyrim mapping -> L3 content/data -> L4 runtime -> L5 compatibility
and release. Mechanic IDs stay traceable across layers.

## D-014 One Rune resource (accepted, L1 design)
The Rune is both the currency and the socketable, with ranks R1-R5 and a 5x value ladder.
One resource keeps the economy legible; the spend-versus-socket trade-off is intentional.

## D-015 New forms ship in one ESL plugin (accepted, user-confirmed)
All new forms (rune item, materials, keywords, effects, recipes) live in a single
ESL-flagged Runefall.esp with our own FormIDs. One plugin, light slot, no vanilla overrides.
The DLL applies behavior; SkyPatcher is not used to create forms.

## D-027 No mutation of vanilla or shared records (accepted, PROTEUS evidence 2026-10-26)
Runtime record mutation is available through SKSE native Papyrus methods and through our own
DLL, but PROTEUS shows the cost: base-form edits are shared by every copy and are session
scoped/leaky across saves. Runefall therefore never mutates vanilla or shared records. Keyword
mutation is allowed only on Runefall's own forms, and only where the effect is revertible or
reapplied. Per-instance state stays in extra data + co-save (D-023).

## D-028 Persistence is co-save-first (accepted, PROTEUS evidence 2026-10-26)
The co-save is authoritative for per-item, per-actor and progression state. External CSV/JSON/
INI files are content and configuration only. We do not adopt the PROTEUS model of external
JContainers presets as the source of truth (name-keyed, cross-save ambiguity), and we add no
JContainers/PO3/UILib dependency for it.

## D-021 Active skill cast model: native hotkey layer (accepted for L2)
Nine active skills are cast through a native hotkey layer owned by the DLL, with cooldowns and
charges managed natively. Fallback if the input layer is unstable in Phase 0: lesser-power
casting with the same cooldown state. Equipped-spell casting is not the default because two
hands cannot carry an ARPG rotation.

## D-022 UI framework: SKSE Menu Framework for v1 (accepted for L2)
Character sheet, Rune Forge and item detail pages use SKSE Menu Framework (already installed in
the target list). Prisma UI is an optional v2 module. MCM Helper is the fallback.

## D-035 Revised FormID map and record budget (accepted, L3A audit 2026-10-26)
The L3 map was too tight once 60 affixes, ~150 effect templates, skill effects, monster affixes
and boss templates were counted. Revised ranges: KYWD 0x800, GLOB 0x880, MGEF 0x900 (256),
SPEL 0xA00 (256), PERK 0xB00, MISC 0xB80, COBJ 0xC00, QUST/FLST 0xC80, NPC 0xD00, reserved
0xD80-0xFFF (640). Planned v1 total ~440-470 records inside one ESL.

## D-040 Multi-ESL base and pack architecture (accepted, user-directed 2026-10-26)
Runefall ships a base ESL plugin plus independent content expansion packs, all ESP files with the
ESL flag (light slots). Packs never master the base and never reference base FormIDs; cross-plugin
links go through EditorIDs resolved by the DLL from data files. Each plugin owns its own
0x800-0xFFF FormID window. Packs ship Data\Runefall\packs\<pack>.json manifests declaring format,
minimum base version, content files and record EditorIDs. A missing or incompatible pack disables
cleanly; the base keeps working.

## D-041 Unique affixes from bosses and quests (accepted, user-directed 2026-10-26)
A new content class: unique affixes with fixed effects and no tiers, tied to specific boss kills
or quest completions, shipped in Runefall - Unique.esp. Count is derived from mining:
U = B_unique + Q_unique = 20 boss templates + 20 accepted quest sources = **40 unique affixes**
(DataMiner run 2026-10-26: 31 HIGH boss candidates, 48 reward-pattern quest candidates, 752
enchantments catalogued). Sources come from Skyrim.esm + Update/Dawnguard/HearthFires/Dragonborn;
mined FormKeys/EditorIDs are references only, unique affixes are original effects and names.

## D-056 Dependency policy (accepted, L5 2026-10-26)
Hard: Skyrim 1.6.1170 (release and test target), SKSE 1.6.1170, Address Library 1.6.1170 bins, and
the DLL's pinned CommonLibSSE-NG commit. 1.7.104 is not a v1 target (D-062). UI module adds SKSE Menu Framework (MCM Helper fallback).
No JContainers, PO3 Papyrus Extender, PapyrusUtil or UILib dependency. Optional mods are never
required by Core.

## D-057 Compatibility pass matrix and patch policy (accepted, L5)
Every target (OWL, DiabloRim, Worlds Dawn, Fortunate Foes, perk/enchant/alchemy overhauls,
survival, boss overhauls, UI stacks, leveled-list mods) has an explicit status: PASS, PASS WITH
PATCH, KNOWN LIMITATION or FAIL. Untested targets are disclosed, never claimed compatible.
Patches live in the Patches pack; Core carries no target-specific data.

## D-058 FOMOD module matrix and packaging rules (accepted, L5)
Core is the only canonical copy; Hunt/Unique/Bosses/Affixes/Quests/Patches/UI/Balance contain
deltas only. Explicit module dependencies, UTF-8 XML with declaration, both XML files parsed,
every branch simulated, final archive root and member hashes verified.

## D-059 Release and version policy (accepted, L5)
Semantic versioning (MAJOR = data/save break, MINOR = packs/features, PATCH = fixes). Changelog
and all version strings must match. Beta on copied and clean profiles before public. After a game
update: 72-hour triage, rebuild against the pinned CommonLibSSE-NG commit, publish status.

## D-060 Ship gate adaptation and disclosure (accepted, L5)
The ship-gate checklist is mandatory per release, but houseCARL/Skyrim Forge are not installed in
this environment; the Mutagen build validator substitutes for forge plugin-info until they are.
The substitution and any unperformed test are disclosed in VALIDATION.md and on the mod page.

## D-061 Nexus publication deliverables (accepted, L5)
Page, media plan (hero, 4-8 images, one loop video/GIF), requirements split, install/update/
uninstall, compatibility status, known limitations, credits/permissions, AI disclosure, and
evidence-only claims. No fabricated media; other visual mods disclosed.

## D-049 Single-DLL module layout (accepted, L4 2026-10-26)
One DLL owns all gameplay state and hooks. Modules: Bootstrap, Config, PackManager, DataStore,
FormResolver, ItemRegistry, LootEngine, TierEngine, HuntEngine, RuneForge, Progression,
HookManager, InputManager, PapyrusBridge, MenuApi, Compatibility, Logger. All engine reads/writes
on the game thread; file parsing at data load; UI renders through SKSE Menu Framework.

## D-050 Hook map and budgets (accepted, L4)
Eleven hooks: DataLoaded, ContainerGeneration, ActorDeath, ActorSpawn, LoadedCellAttach, MenuOpen,
ItemAdded, SaveWrite, SaveLoad/Revert, Input, MenuRender. Each has a fixed responsibility and
p99 budget (0.5 ms loot/kill, 0.2 ms spawn, 5 ms save, 20 ms load, 100 ms data load, no per-frame
work). Exact engine entry points are Phase 0 research.

## D-051 Item metadata storage and identity (accepted, L4; Phase 0 chooses)
Primary: native per-item extra data serialized in the co-save with a stable instance key.
Fallback: metadata derived from the dynamic enchantment (effect set/magnitudes) plus a
name-encoded rarity/ilvl token and a co-save registry keyed by that token. The ItemRegistry
interface is fixed either way; Phase 0 proves one before the loot pillar is implemented.

## D-052 Co-save TLV format (accepted, L4)
Header magic RNFS + schema/save versions + section table. Nine sections: progression, rune
wallet, item registry, hunt state, container state, unique sources, pack state, config snapshot,
migration. Unknown sections are skipped; malformed sections are dropped with a log entry; additive
migration only; newer schema refuses load with a clear message.

## D-053 Versioned Papyrus bridge and events (accepted, L4)
Query/mutation functions plus mod events (Runefall_OnLootRolled, OnRankAssigned,
OnHuntFirstKill, OnTierChanged, OnItemMutated, OnPackChanged). API version reported at load; no
function blocks on IO or returns a guessed value.

## D-054 Pack runtime contract (accepted, L4)
Discover loaded Runefall plugins, read pack manifests, validate format/minBaseVersion/data files,
merge base then packs in load order (later pack wins duplicates with a log), resolve EditorIDs
once; invalid or missing pack disables its features and logs, never crashes.

## D-055 Performance budgets and no-per-frame rule (accepted, L4)
Data load 100 ms; loot 0.5 ms; kill 0.5 ms; spawn 0.2 ms; cell attach 2 ms; save 5 ms; load 20 ms;
co-save under 1 KB per 100 tracked items. No per-frame gameplay work; logging rate-limited.
Budgets are measured in Phase 0 and recorded in VALIDATION.md.

## D-063 Implement-first, batch-test strategy (accepted, user-directed 2026-10-26)
In-game manual testing is expensive and hard to repeat. v1 is implemented to completion under
static validation plus an automated in-DLL probe harness (debug mode writes RunefallProbe.log
with PASS/FAIL checks), then tested in batch sessions: T1 is one launch with the full probe suite
and a short scripted walkthrough, T2 is the fix batch plus a confirmation launch only if needed.
No feature is manually game-tested before T1. Phase 0 evidence already collected (build, plugin
load, Papyrus/co-save registration) remains valid. The goal is tracked in the session goal.

## D-062 Release target is 1.6.1170; 1.7.104 deferred (accepted, user-directed 2026-10-26)
The v1 release and validation target is Skyrim SE/AE 1.6.1170 with SKSE 1.6.1170 and Address
Library 1.6.1170 (Mantras of Mara setup). Skyrim 1.7.104 is ignored for v1: SKSE 2.3.1 and
Address Library v13 are Nexus-login gated and the plugin ecosystem does not support it yet. The
multi-runtime DLL remains a technical capability; it is not a v1 support claim. The 1.7.104 Steam
clone was removed to avoid an unsupported test environment. Revisit 1.7.x only when its
SKSE/Address Library/plugin ecosystem is stable, as a separate later release.

## D-047 Future gap register and v1 scope lock (accepted, user-directed 2026-10-26)
All non-v1 features are recorded in SPECS\GAP-REGISTER-FUTURE.md with an ID, a decision code
(NOW / RESERVE / WATCH) and a destination pack. 32 items are reserved or watched (18 gameplay,
8 presentation/QoL, 6 compatibility/platform), 10 cheap hooks must be covered in v1, and 8 risks
are measured during v1. Reserved features never enter v1 code; only their hooks (N-01..N-10) may.
Adding a reserved feature to v1 requires updating the register first.

## D-048 Cheap-now hooks (accepted, user-directed 2026-10-26)
Ten hooks ship in v1 because retrofitting them would break saves or reissue every pack: co-save
schema version + migration stub, data/pack format version negotiation, pack independence contract,
hunt firstOnly + pack-removal cleanup, full localization keys, loot tags in metadata, UI data API
separation, diagnostic logging/debug export, per-hook performance budgets, and the uninstall /
pack-disable test matrix. These are tracked as acceptance criteria in L4/L5.

## D-045 Crafting Table reserved as expansion (accepted, user-directed 2026-10-26)
v1 ships Rune Forge basics only: salvage, socket/extract, enhancement reroll, rarity upgrade, rune
conversion, material hunting (COBJ ~8: rune conversion + socket craft). The full Crafting Table -
deterministic base-item crafting, targeted affix crafting, Affix Shards, material recipe chains,
socket-add recipes and the crafting UI - is designed and shipped later as Runefall - Crafting.esp
(ESL, same data/pack conventions). v1 must not depend on it.

## D-046 Hunting primacy in v1 (accepted, user-directed 2026-10-26)
Because the Crafting Table is deferred, v1 gear acquisition is hunting and loot: general drop
table, race hunt uniques/materials, boss and quest uniques. Materials feed Rune Forge reroll and
rarity-upgrade costs in v1; recipe crafting arrives with the expansion. Hunting is therefore the
primary v1 progression mechanism, and the Hunt pack is a core-loop pack, not a side module.

## D-043 General drop table (accepted, user-directed 2026-10-26; gap G11)
Every enemy kill resolves drops_general.csv by rank (0-5) x RF tier (0-5): 36 data rows covering
item chance/count, rarity bias, rune yield and a reserved unique chance for rank 4-5. Containers,
quests and bosses keep their dedicated tables. Data only; no records and no leveled-list edits.

## D-044 Race hunt tables (accepted, user-directed 2026-10-26; gap G12)
Hostile creature families are derived from the mined official race inventory by a threshold rule
(family has >= 10 NPC actors -> F = 19 for v1). Each family grants one unique affix (first-kill
only, per player/family flag in co-save) and one hunt material (repeatable, chance rises with RF
tier). Race mapping is data (race_groups.csv, RACE EditorIDs); rewards ship in
Runefall - Hunt.esp. Hunt uniques are excluded from random pools. Unique affix total:
U = 20 boss + 20 quest + 19 hunt = 59.

## D-042 DataMiner tool and provenance policy (accepted 2026-10-26)
Runefall\tools\DataMiner (C#, Mutagen, read-only overlays) mines official masters for unique
items, enchantments, named NPCs, quests, reward lists and containers, producing candidates with
confidence levels under SPECS\data\mining. No Bethesda assets, strings or record data are
shipped; mined IDs are stored only as source references. Review gates: HIGH confidence first,
manual acceptance, no duplicate source, drop rule required.

## D-038 Math-derived affix matrix: 108 affixes, 540 rows (accepted, L3B 2026-10-26; revised to 120 by L3C/D-040 era audit)
Affix count was derived from coverage: 9 attributes x (5 categories x 2 choices + 2 skill
synergies) = 108. L3C added the benchmark constraint (beat 113 combined reference families), so
the accepted v1 budget is **120 affixes / 600 rows / ~182 MGEF / ~440-470 records**, category
pools 23/24/24/24/23, names 14,400, parity 40/40/40.
Affix count is derived from coverage, not chosen: 9 attributes x (5 categories x 2 choices + 2
skill synergies) = 108, which also equals 18 Skyrim skills x 6. Category pools: offense 21,
defense 22, utility 22, economy 22, elemental 21. Rows: 108 x 5 tiers = 540. Prefix and suffix
pools are 108 each, giving 11,664 distinct two-part names. Record effect budget: ~170 MGEF,
~420-450 total records. This supersedes D-036 for affix/MGEF/row counts.

## D-039 Coverage validation gates (accepted, L3B 2026-10-26)
The build fails unless: every attribute x category cell has >= 2 affixes; every one of the 18
Skyrim skills is referenced by >= 2 affixes; every one of the 10 slot groups has >= 18 valid
affixes; prefix and suffix pools each equal 108 with no duplicates and >= 10,000 distinct names;
category pools >= 18; every affix has 5 monotonic tier rows; per-build affix counts are within 2
of 36/36/36; FormID category caps hold. These gates make content gaps a build error, not a
review opinion.

## D-036 v1 content targets (accepted, L3A audit 2026-10-26; affix counts superseded by D-038)
20-25 recipes, 12-16 boss and mini-boss NPC templates (at least one mini-boss and one boss per RF
tier), 250-350 localization keys, drop tables sized to 6 container tiers / 5 quest bands / 5-6
boss tables. Reference comparison: DiabloRim 49+64 affixes, Worlds Dawn 742 records, Fortunate
Foes 415 records.

## D-037 v1 asset requirement (accepted, L3A audit 2026-10-26)
v1 ships a minimum asset set: 17 item icons (5 runes + 12 materials), 6 rarity badges/tints and
5 rank markers; optional UI frame and boss VFX. Fallback if art is late: tinted vanilla icon
paths plus UI rank badges, replaced before release. Assets load from Data\Runefall\assets.

## D-031 Fixed ESL FormID category ranges and EditorID convention (accepted L3)
Runefall.esp is ESL-flagged; FormIDs are allocated per category range (keywords 0x800, globals
0x840, MGEF 0x880, spells 0x900, perks 0x940, MISC 0x980, COBJ 0x9C0, quests 0x9E0, FLST 0x9F0,
boss NPCs 0xA00, rest reserved) and never renumbered. EditorIDs follow RF_<Type>_<Area>_<Name>.
Data files reference records by EditorID, never by hardcoded FormID. The build validator
enforces ranges, caps and uniqueness.

## D-032 Data files are versioned schemas (accepted L3)
CSV files under Data\Runefall\data are the authoring and balance surface for affixes, rarity,
ranks, drop tables, materials, recipes, skills and attributes. A single formats.json declares
schema versions and expected columns; the DLL and the build validator reject unknown or
malformed data. No balance change requires a plugin rebuild.

## D-033 Deterministic Mutagen build with integrated validator (accepted L3)
Runefall.Build (C#, Mutagen current release) validates data, generates the ESL deterministically,
copies data files, and fails on range/schema/reference/override violations. Same inputs produce
the same bytes; hashes are recorded in VALIDATION.md.

## D-034 Module split: Core / UI / Patches / Balance (accepted L3)
Core is required (ESL + data + DLL + core scripts). UI, per-mod patches and a data-only balance
pack are optional modules. FOMOD construction is an L5 task.

## D-023 Dynamic per-item generation is mandatory (L2, revised 2026-10-26)
Rarity, enhancements, item level and sockets are generated at runtime per item instance:
dynamic enchantment (CreateEnchantment/SetEnchantment), display name (SetDisplayName) and
tempering (SetItemHealthPercent) plus a co-save registry keyed by stable item identity. There is
no pre-generated variant fallback: the combinatorial item space cannot be expressed as records.
Reference: DiabloRim performs this natively; Worlds Dawn does it from Papyrus with SKSE's
WornObject/ObjectReference natives (verified in the installed 1.6.1170 sources).

## D-029 Drop tables are data, not records (accepted 2026-10-26)
Drop tables (per rank, container tier, quest band, boss) are CSV/JSON data evaluated by the DLL
at drop time. No record-per-variant generation and no leveled-list edits. Changing balance is a
data edit, not a plugin rebuild.

## D-030 Container-item mutation path (accepted 2026-10-26)
Items inside containers are not object references. The native inventory-entry path is the
primary mechanism; the Worlds Dawn-proven dummy-actor approach is the fallback. Phase 0 picks
the working path; both keep generation dynamic and require no records.

## D-024 Loot roll at container generation, persisted (L2)
The DLL rolls loot when a container/corpse/vendor/quest reward generates, before contents are
shown, and persists the roll per container instance. No leveled-list or container record edits.

## D-025 Enemy rank assignment: spawn hook primary, load-scan fallback (L2)
Exactly one rank per actor, applied by the DLL at spawn if the hook proves viable; otherwise a
deterministic load-scan. No NPC record edits and no permanent perk grants to generic NPCs.

## D-026 Boss content: native spawn replacement at fixed sites (L2)
Hand-authored boss templates spawn through the native tier engine at fixed sites; leveled
lists are never edited. Attunement runs through a Papyrus script and the native API.

## D-020 Actor values: use the existing data model, no engine re-investigation (accepted, user-directed 2026-10-26)
All actor values already exist in the Mutagen/Synthesis data model, and Bethesda has not
changed their behavior since release. The project therefore consumes the existing AV
definitions and documented usage patterns. It does not build AV dump tools, reverse-engineer
engine behavior, or spend tokens re-verifying stable facts. Implementation follows documented
Bethesda/community channels; anything undocumented is solved with our own DLL, without
editing vanilla records.

## D-019 Non-combat anchors and actor value safety classes (accepted, 2026-10-26)
Every attribute must touch at least one non-combat domain; coverage is audited against the
149 vanilla AVIF records (SPECS/L2A). Attribute effects may modify only the safe AV classes
(resources, carry weight, resistances, regen, intended skill modifiers within caps); the 18
skills are read-only gates; physics/timing multipliers, AI values, quest Favor* values and
unused Variable* values are never written. This is enforced by L1 P-11 and the L2A audit.

## D-018 Nine attributes and eighteen skills grouped by build (accepted, user-directed 2026-10-26)
Three builds own three attributes and six skills each: 9 attributes total, 18 skill lines,
3 ranks each (54 nodes). Each attribute gates exactly two skills; each skill is gated by one
Skyrim skill and its paired attribute. Two attribute points and one skill point per level.
Build identity comes from gates and synergies, not a class lock.

## D-017 Six rarity bands, rarity index = enhancement count (accepted, user-directed 2026-10-26)
Rarity 0-5: Common, Refined, Exquisite, Radiant, Mythic, Runeforged. An item has exactly its
rarity number of enhancements, capped at 5 by the five enhancement categories. Rarity 5 may
carry one unique runeforged effect that does not count as an enhancement. This replaces the
earlier five-band/mixed-count model, which was harder to reason about.

## D-016 Event-driven economy, no time-based pacing (accepted, user-directed 2026-10-26)
Runefall is not an idle or session-timer game. All income and costs are expressed per combat
encounter, loot event, discovery and boss kill. Heavy combat pays much more than slow
exploration by design; an hour of exploration may legitimately yield no rune income and is
rewarded through materials, nodes and discoveries instead. No passive/daily generation.
