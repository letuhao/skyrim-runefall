# L4 - Runtime architecture

Status: draft 2026-10-26. Depends on L1-L3, L3E, D-023..D-048 and the future gap register
(N-01..N-10). Purpose: define the DLL modules, hook responsibilities, data/pack loading, co-save
format, Papyrus bridge, UI integration, performance budgets, failure modes and the Phase 0
checklist. This layer does not write code.

Runtime baseline:

- One native DLL, CommonLibSSE-NG, C++20, CMake + vcpkg, VS2022/MSVC 14.44, pinned to the
  active **alandtse fork v14.0.1** (commit 723cfda480ae, 2026-10-25), which carries 1.7.99+
  runtime support. The older CharmedBaryon repository is not the build target (corrected in
  Phase 0, 2026-10-26; build evidence in VALIDATION.md).
- Multi-runtime build: Skyrim AE 1.6.x and 1.7.x in one DLL; Address Library format 5 with
  versionl2b fallback; SE/VR reserved (F-31/F-32).
- SKSE + Address Library are hard requirements; SKSE Menu Framework is the v1 UI dependency
  (D-022); no JContainers, no PO3, no UILib, no PapyrusUtil unless a future pack adds them.

## 1. Module architecture

All modules live in one DLL. Names are conceptual; actual C++ class names can differ.

| Module | Responsibility | Depends on |
|---|---|---|
| Bootstrap | SKSE messaging, version checks, init order, crash-safe startup | SKSE |
| Config | runefall.ini parsing, typed defaults, per-feature toggles, caps | logger |
| PackManager | discover/validate packages, read pack manifests, merge data, version negotiation | FormResolver |
| DataStore | load/validate CSV/JSON schemas, hold affix/drop/rank/skill/attribute/race tables | Config, PackManager |
| FormResolver | build EditorID -> FormKey map for the loaded Runefall plugins plus required vanilla records; cache lookups | game data |
| ItemRegistry | per-item metadata (rarity, enhancements, ilvl, sockets, tags); identity and persistence | SaveSystem |
| LootEngine | general drop table, container/vendor/quest tables, rarity/enhancement rolls, pity, dynamic item mutation | DataStore, ItemRegistry |
| TierEngine | rank assignment, monster-affix packages, scaling caps, boss handling | DataStore, FormResolver |
| HuntEngine | race->family lookup, firstOnly rewards, hunt material rolls, hunt progress events | DataStore, SaveSystem |
| RuneForge | salvage, reroll, upgrade, socket/extract, rune conversion transactions | ItemRegistry, DataStore |
| Progression | attributes, skill points/ranks, respec, derived effect application, glob mirrors | DataStore, SaveSystem |
| HookManager | registers/unregisters all engine hooks, orders them, guards against duplicate application | all gameplay |
| InputManager | hotkeys, rebinding, cooldown state, gamepad/keyboard; sends cast events to Progression | Config |
| PapyrusBridge | versioned native functions and mod events for UI/other mods | gameplay modules |
| MenuApi | read-only data snapshot for the UI (never reads files directly) | all |
| Compatibility | loot authority modes, soft-detect other mods, patch hooks | Config |
| Logger | versioned log, categories, rate limiting, diagnostic export | bootstrap |

The stage/power-ladder module is specified in `SPECS/POWER-LADDER.md`: StageEngine,
WorldBandResolver, RaceClassifier, ProtectionService, PenetrationResolver and StageProfileStore.
All balance values (curves, bands, class tables, protection rules, world bands) are data generated
by Runefall.Build (D-066); the DLL owns policy, engine access and persistence.

Threading rule: game-thread only for engine reads/writes and hooks. File IO and CSV parsing happen
once at data load (game thread acceptable at kDataLoaded; if profiling shows a stall, parse on a
worker and publish under a lock). UI rendering runs on the render thread through SKSE Menu
Framework; MenuApi snapshots are immutable and refreshed on game-thread ticks.

## 2. Hook map (responsibilities, not addresses)

Exact engine entry points are Phase 0 research; this table fixes what each hook must do.

| Hook | When | Must do | Budget (p99) |
|---|---|---|---|
| DataLoaded | after all plugins load | validate config/data/packs, resolve EditorIDs, build caches, log summary; disable invalid packs | 100 ms total |
| ContainerGeneration | before container contents are shown | run container table, roll items/runes, mutate instances, persist per-container state | 0.5 ms per container |
| ActorDeath | at death, before loot is resolved | general table + rank table + race hunt rewards + boss/quest unique rolls; firstOnly flags | 0.5 ms per kill |
| ActorSpawn | when an actor is placed/loaded | assign one rank, apply monster-affix package with caps, mark runtime state | 0.2 ms per actor |
| LoadedCellAttach | cell load/reset | refresh ranks of loaded actors, expire old container states, handle cell reset rules | 2 ms per cell |
| MenuOpen | forge/workbench or menu framework page open | Rune Forge entry, UI state snapshot | 0.2 ms |
| ItemAdded | item entering player inventory | finalize metadata if the item was mutated outside our hooks, update registry | 0.1 ms |
| SaveWrite | SKSE co-save serialization | serialize all sections with version header | 5 ms per save |
| SaveLoad/Revert | co-save deserialization / revert | rebuild state, validate, drop orphaned pack state, migrate old versions | 20 ms |
| Input | gameplay input event | hotkey dispatch, cooldown check, cast request | <0.05 ms per event |
| MenuRender | menu framework render | draw pages from MenuApi snapshot | UI budget |

No per-frame gameplay work. No polling loops. No Papyrus update loops in the core.

## 3. Pack and data loading contract

1. Discover loaded plugins (base Runefall.esp plus Runefall - *.esp packs) from the data handler.
2. Read Data\Runefall\packs\*.json manifests; verify: pack present in load order, format version
   compatible, minBaseVersion satisfied, declared data files exist.
3. Load base data first, then packs in load order. Merge rules: append rows; duplicate keys are a
   validator error inside a pack and a warning across packs (later pack wins, logged).
4. Resolve every EditorID reference to a FormKey once; unresolved references disable the affected
   feature/pack and log the exact record/row, never crash.
5. A pack that is removed while save data references it: keep orphaned state, skip its features,
   log; a later reinstall restores it.
6. Hot reload of data files is developer-only (debug build/INI switch), never in release.

All of this implements N-02 (format/version negotiation) and N-03 (pack independence).

## 4. Item instance metadata and identity

Primary approach (Phase 0 proof): a per-item metadata record attached to the item instance through
a native extra-data channel, serialized in the co-save with a stable instance key. Fallback
(Phase 0 alternative): derive metadata from the dynamic enchantment (effect set + magnitudes) and
a name-encoded rarity/ilvl token, with a co-save registry keyed by that token.

Fixed interface either way:

    ItemMetadata {
      uint32  registryId;        // stable within save
      FormKey baseForm;
      uint8   rarity;            // 0-5
      uint16  ilvl;
      uint8   socketCount;
      uint32  socketRuneRanks[3];
      uint8   enhancementCount;
      Enhancement [5] { affixIdHash, magnitude, tier, category };
      uint16  tags;              // hunt family, source type, loot authority
    }

- The registry is the single source of truth for rarity/enhancements. The item's visible state
  (dynamic enchantment, display name, temper) is derived from it and re-applied when needed.
- The registry is pruned only when items are destroyed; cap and size are measured in Phase 0.
- Rarity/name/tooltip reads go through the registry; no UI parses names to infer data.
- This is the N-06 loot-tag hook: tags are stored now so future loot filters and UI packs work.

## 5. Co-save format

Header: magic RNFS, u16 schema version, u16 save serialization version, u32 flags, u32 section count.
Sections are TLV (id, length, payload); unknown sections are skipped (forward compatibility).
Known sections:

    1  progression       attributes, points, spent runes, respec state
    2  rune wallet       counts per rank, lifetime earned/spent
    3  item registry     entries (metadata above), free-list, generation counter
    4  hunt state        per-family firstOnly flags, kill counters, material drop stats
    5  materials         material counts / derived cache
    6  unique sources    boss/quest unique awarded flags (reserved)
    7  pack state        installed pack IDs + versions + data format versions
    8  config snapshot   hash of effective INI + data versions + base/pack versions
    9  container state   (ref FormKey, rolled flag, group id); TTL by cell reset
    10 quest state       rewarded quest IDs (schema 2)
    11 ladder state      player rank/stage, difficulty profile, manual stage overrides (cap 256)
    12 encounter state   reserved for the encounter module

    Header (v3): magic RNFS, u16 schema=3, u16 serialization version, u32 flags, u32 section count.
    This section map supersedes earlier L4/ARCHITECTURE maps; the authoritative table is
    SPECS/POWER-LADDER.md section 9.

Rules:

- Write order and load order are explicit; every load validates lengths and drops malformed
  sections with a log entry instead of failing the save.
- Revert (death without save) must restore the loaded state, not the serialized one.
- Additive schema changes only; a version bump requires a migration function and a rollback path
  (N-01). Downgrades are refused with a clear message.
- Uninstall: remove DLL and plugins; the game ignores unknown co-save data; items remain base items
  plus whatever engine-native enchantment/name was applied. Document that applied names/enchants
  may remain (engine data) but our logic is gone.

## 6. Papyrus bridge (versioned API)

> **Policy (D-064):** this bridge is a compatibility/query surface, not the gameplay
> implementation. Runefall gameplay is DLL-native. Papyrus scripts are limited to native
> declarations, debug/probe helpers and explicitly approved compatibility shims; polling loops and
> gameplay logic are prohibited and enforced by `tools/check-papyrus-policy.ps1`.

Expose a small, stable, versioned surface. Bridge version is reported at load; scripts check it.

Query functions:

    int    GetApiVersion()
    int    GetRuneCount(int rank)                 // 1-5
    int    GetRuneTotal()
    int    GetAttribute(string attribute)         // Might, Endurance, ...
    int    GetAttributePointPool()
    int    GetSkillRank(string skill)             // 0-3
    int    GetSkillPointPool()
    int    GetRuneTier()                          // RF Tier 0-5
    int    GetItemRarity(Form itemRefOrForm)
    string GetItemEnhancements(Form itemRefOrForm) // JSON-ish compact string or parallel arrays
    int    GetHuntFirstKillState(string raceGroup) // 0 none, 1 awarded
    int    GetLootAuthorityMode()
    string GetDiagnostics()                       // versions, pack list, co-save stats

Mutation functions (all validated, return code 0 = ok, nonzero = error code):

    int AddRunes(int rank, int amount, string reason)
    int SpendRunes(int rank, int amount, string reason)
    int SpendAttributePoint(string attribute)
    int SpendSkillPoint(string skill)
    int RespecAttributes()
    int RespecSkills()
    int SalvageItem(Form itemRefOrForm)
    int RerollItem(Form itemRefOrForm, int lockedIndex)
    int UpgradeItem(Form itemRefOrForm)
    int SocketItem(Form itemRefOrForm, int socketIndex, int runeRank)
    int ExtractSocket(Form itemRefOrForm, int socketIndex)
    int SetDebugLootRoll(string mode)             // debug only

Events (SendModEvent):

    Runefall_OnLootRolled        (rarity, itemForm)
    Runefall_OnRankAssigned      (actorRef, rank)
    Runefall_OnHuntFirstKill     (raceGroup, uniqueAffix)
    Runefall_OnTierChanged       (oldTier, newTier)
    Runefall_OnItemMutated       (itemRef, rarity)
    Runefall_OnPackChanged       (packId, state)

No function blocks the game thread on IO; no function returns a guessed value.

## 7. UI integration

v1 pages (SKSE Menu Framework; D-022):

1. Character: 9 attributes, 18 skill lines with ranks and gates, point pools, respec.
2. Rune Forge: rune wallet, salvage list, reroll/upgrade/socket controls, material list.
3. Hunt Codex: 19 families, first-kill state, material counts, general rates (read-only).
4. Item Detail: rarity, enhancements, sockets, item power, comparison.

Rules: pages read MenuApi snapshots, never CSV/INI directly; all actions go through the Papyrus
bridge or native callbacks (single code path, validation in the DLL); pages degrade gracefully if
a pack is missing. MCM Helper is the fallback if the framework fails in Phase 0.

## 8. Performance budgets

- Data load: under 100 ms; memory under 20 MB for tables/caches.
- Loot hook p99 0.5 ms; spawn hook 0.2 ms; kill hook 0.5 ms; cell attach 2 ms.
- Save write 5 ms; load/revert 20 ms; co-save under 1 KB per 100 tracked items (measure at 1,000
  and 10,000).
- No per-frame work except input dispatch and menu rendering; logging rate-limited and async.
- All budgets recorded in VALIDATION.md and re-measured in Phase 0 and at release.

## 9. Failure modes and safety

| Failure | Behavior |
|---|---|
| Invalid CSV/JSON, unknown column, bad enum | disable the affected feature or pack, log exact row/file; base continues |
| Missing pack plugin vs manifest | warn and skip; base continues |
| Unresolved EditorID | disable dependent feature, log full reference |
| Co-save section malformed | drop section, log, continue; never crash or wipe |
| Co-save newer schema | refuse load with a clear message (no silent downgrade) |
| Save with pack removed then re-added | orphaned state kept, features resume |
| Hook exception | catch, log, disable offending subsystem for the session |
| Duplicate hook registration | guard by subsystem ID; log and ignore |
| Data-file edit mid-session | ignored until reload (debug only) |

## 10. Multi-runtime and compatibility

- Build with CommonLibSSE-NG alandtse v14.0.1; dynamic runtime detection; log the detected
  runtime and Address Library files at startup. The supported and tested target is 1.6.1170 (D-062).
  The multi-runtime DLL still builds for 1.6.x/1.7.x, but 1.7.x is not a v1 support target.
- 1.6.1170 = release and test target (Mantras list); 1.7.x deferred until the SKSE/AL/plugin
  ecosystem supports it; SE/VR reserved.
- Compatibility: loot authority modes (additive/hybrid/takeover), soft-detect OWL/DiabloRim and
  log conflicts, optional patch packs. Never mutate their records.

## 11. Extension points reserved for future packs

The core exposes stable seams so 1.1+ packs do not fork the DLL:

- Pack registration: reward handlers, loot sources, data schema additions.
- Crafting Table expansion (1.1): recipe registry, craft UI page hook, transaction API.
- Hunt II (1.2): contract definitions, targeted reward tables, firstKill reset command.
- Unique II (1.3): source registration for additional mined candidates.
- UI pack: MenuApi already exposes all data; no core changes.
- Balance packs: data only.

## 12. N-01..N-10 acceptance mapping

| Hook | Where satisfied in L4 |
|---|---|
| N-01 co-save schema/migration | section 5 |
| N-02 data/pack version negotiation | section 3 |
| N-03 pack independence | sections 3, 10 |
| N-04 firstOnly + pack removal cleanup | sections 3, 5, 9 |
| N-05 localization keys | L3 strings.csv, enforced by build validator |
| N-06 loot tags in metadata | section 4 tags field |
| N-07 UI data API separation | section 7 |
| N-08 diagnostics | section 13 |
| N-09 performance budgets | section 8 |
| N-10 uninstall/pack-disable matrix | sections 5, 9; tested in L5 |

## 13. Diagnostics and debug

- Versioned log at Data\SKSE\Plugins\Runefall.log: startup versions (DLL, SKSE, runtime, AL
  format), pack list, data hashes, hook registrations, every error with file/row/record.
- Debug console commands and a /rf debug export: pack states, co-save stats, active hooks, last
  N loot rolls, per-hook timings. Off by default in release; enabled by INI debug flag.
- Deterministic debug RNG seed for reproducing a loot or rank roll.

## 14. Phase 0 spike checklist (L4 assumptions to prove)

1. DLL loads on 1.6.1170, logs versions, detects AL format; builds multi-runtime.
2. Data load hook fires once; CSV validation runs; invalid row disables feature not crash.
3. Pack manifest discovery works with base + one pack; disabling the pack logs and continues.
4. Container generation hook can roll and mutate an item instance before display.
5. Actor death hook can roll general/race rewards; firstOnly flag persists in co-save.
6. Actor spawn or load-scan assigns exactly one rank; applies a capped effect package.
7. Item metadata primary approach (native extra data) survives save/load/uninstall; fallback
   approach tested if primary fails.
8. Co-save write/load/revert round-trip with all sections; malformed section is dropped safely.
9. SKSE Menu Framework page registers, reads MenuApi, performs one action through the bridge.
10. Input hotkeys fire in gameplay, are rebindable, and respect cooldowns.
11. Save with 1,000 dynamically generated items: size and load time within budget.
12. 1.7.x ecosystem check (deferred; not a v1 target - D-062).

## 15. Decisions recorded by this layer

- D-049 Module layout and single-DLL ownership of gameplay state (accepted).
- D-050 Hook map and per-hook budgets (accepted as the Phase 0 contract).
- D-051 Item metadata via native extra data, co-save registry; derived-metadata fallback if Phase 0
  disproves it.
- D-052 Co-save TLV format, versioned sections, additive migration (accepted).
- D-053 Versioned Papyrus bridge API and mod events (accepted).
- D-054 Pack runtime contract: discover, validate, merge, disable-invalid, EditorID resolution
  (accepted).
- D-055 Performance budgets and no-per-frame rule (accepted).
