# Dynamic Enemy Spawns SKSE — Deep Dive

**Mod:** Dynamic Enemy Spawns SKSE (Nexus 178556, v3.4, 2026-09-29)  
**Internal project name:** `AutoEnemySpawn` (v1.0.0)  
**DLL:** `SKSE/Plugins/AutoEnemySpawn.dll`  
**License:** GPL-3.0 with the modding linking exception (`EXCEPTIONS.md`)  
**Evidence base:** full C++ source + PDB shipped in `Source And PDB Files 178556 3.4 2026-09-29T14-44Z HZCMOyR1r.zip`  
**Read-only source root:** `D:\Works\source\skyrim-moddings\rpg-mechanism\`

This document is a research extraction for Runefall. Nothing here is shipped; the mod folder is read-only.

---

## 1. Mod identity & file layout

### Installed (deployed) tree
```
Dynamic Enemy Spawns SKSE 178556 3.4 2026-09-29T14-44Z 2Bej58NL8/
└── Dynamic Enemy Spawns SKSE/
    ├── COPYING.txt
    ├── EXCEPTIONS.md
    └── SKSE/Plugins/
        ├── AutoEnemySpawn.dll
        ├── AutoEnemySpawn.ini                # live config (see §9)
        └── AutoEnemySpawn_Translation.ini    # SKSE Menu Framework strings
```

### Source tree (extracted from `Source And PDB Files ... HZCMOyR1r.zip`)
```
Source and PDB files for both/
├── COPYING.txt
├── EXCEPTIONS.md
├── PDB/AutoEnemySpawn.pdb
└── src/
    ├── main.cpp          # SKSE plugin entry, message handler
    ├── PCH.h / PCH.cpp   # precompiled headers (RE/Skyrim.h, SKSE, SimpleIni)
    ├── Version.h         # project "AutoEnemySpawn", version "1.0.0"
    ├── Settings.h/.cpp   # INI-backed settings singleton
    ├── SpawnManager.h/.cpp # core spawn engine (~2.2k lines)
    ├── SaveHook.h        # vtable hooks on RE::Character (ShouldSaveGame/SaveGame)
    ├── Strings.h/.cpp    # localized UI strings + translation INI
    └── UI.h/.cpp         # SKSE Menu Framework panel
```

### Runtime dependencies
- SKSE runtime (plugin built against CommonLibSSE-style relocation IDs).
- SKSE Menu Framework (optional but recommended; UI degrades to a warning if absent).
- Vanilla dead-body cleanup cell `WIDeadBodyCleanupCell` (optional; corpse transport no-ops if missing).

---

## 2. High-level architecture

`AutoEnemySpawn` is a **cell-load-driven density expander**. It does not edit leveled lists or ESP records. Instead, when a cell finishes loading, it:

1. Scans the already-placed hostile actors in that cell.
2. Resolves each to a stable base NPC via EditorID (`_npcCache`).
3. Computes a target total count from a rolled multiplier (base × follower scaling, exterior cap).
4. Places additional clones of those base NPCs at the source actors' positions via `TESDataHandler::CreateReferenceAtLocation`.
5. Batches placement to avoid spikes, guards against stale sessions after load/save transitions.
6. Tracks every clone in `_spawnedActors` (ActorHandle → base FormID) so death/loot/cleanup logic can identify them.
7. Prevents clones from being saved to disk via vtable hooks on `RE::Character`.

The design is deliberately **stateless across sessions**: cell cache and pending counters are wiped on load game; only the `_spawnedActors` map is snapshotted and cleared at the end of a load transition.

---

## 3. Plugin lifecycle (`main.cpp`)

- `SKSEPlugin_Load`:
  - `InitializeLog(false)` → log file `Data/SKSE/Logs/AutoEnemySpawn.log`.
  - `Settings::GetSingleton().Load()` (reads `Data/SKSE/Plugins/AutoEnemySpawn.ini`).
  - `SKSE::Init`.
  - `SaveHook::Install()` (vtable hooks).
  - Registers `MessageHandler` with the SKSE messaging interface.
- `MessageHandler`:
  - `kPreLoadGame`: `SpawnManager::HandlePreLoadGame()` (enable load-transition guard, snapshot tracked actors, clear cell state).
  - `kDataLoaded`: `BuildNPCCache()`, `CacheCleanupCell()`, `SpawnManager::Register()` (event sinks), `Strings::Load()`, `UI::Register()`.

---

## 4. Settings subsystem (`Settings.h/.cpp`)

Singleton with INI path `Data/SKSE/Plugins/AutoEnemySpawn.ini`, section `[Settings]`.

| Field | INI key | Default | Clamp / notes |
|---|---|---|---|
| `enemySpawnMultiplierMin` | `enemySpawnMultiplierMin` | 1.2 | ≥ 1.0 |
| `enemySpawnMultiplierMax` | `enemySpawnMultiplierMax` | 1.2 | ≥ min |
| `followerMultiplier` | `numFollowerMultiplier` | 1.25 | ≥ 1.0 |
| `enemyMultiplierExteriorMax` | `enemyMultiplierExteriorMax` | 3.0 | ≥ 1.0 |
| `maxExteriorSpawnRadius` | `maxExteriorSpawnRadius` | 10000.0 | ≥ 1.0 (units) |
| `enemyLimitInterior` | `enemyLimitInterior` | 35 | ≥ 1 |
| `spawnsPerBatch` | `spawnsPerBatch` | 2 | ≥ 1 |
| `batchDelayMs` | `batchDelayMs` | 150.0 | ≥ 0 |
| `allowDuplicatingUniqueEnemies` | `allowDuplicatingUniqueEnemies` | false | mutually exclusive with no-respawn flag |
| `allowDuplicatingNoRespawnEnemies` | `allowDuplicatingNoRespawnEnemies` | false | mutually exclusive with unique flag |
| `enableCorpseLooting` | `enableCorpseLooting` | false | gates corpse activation |
| `transportDeadSpawnsToCleanupCell` | `transportDeadSpawnsToCleanupCell` | false | gates corpse transport |
| `copyEquipmentFromOriginal` | `copyEquipmentFromOriginal` | true | armor/weapons only |
| `debug` | `debug` | false | log level |
| `enemyBlacklist` | `enemyBlacklist` | `mammoth,dragon priest` | comma list, case-insensitive partial match on actor name + EditorID |
| `forceCountedFollowers` | `forceCountedFollowers` | (empty) | comma list, case-insensitive partial match on actor name |

`Clamp()` enforces ranges and the mutual-exclusion rule (`allowDuplicatingUniqueEnemies` wins; if both set, `allowDuplicatingNoRespawnEnemies` is forced false).

`Save()` writes all keys back to INI (used by the MCM-style SKSE Menu Framework UI). `ApplyLogLevel()` toggles spdlog level.

---

## 5. SpawnManager — core engine

### 5.1 State (`SpawnManager.h`)

```cpp
struct CellSpawnData { bool spawned{false}; bool pending{false}; };
struct EnemyEntry   { ActorHandle handle; std::string editorID; bool isUnique{false}; };
struct SpawnBatch    { vector<EnemyEntry> enemies; vector<int> spawnQueue;
                       unordered_set<uint32_t> reservedFurniture;
                       bool isInterior; FormID cellFormID; uint32_t sessionID; int batchOffset; };
```

Key members:
- `_cellCache`: `FormID → CellSpawnData` — per-cell "already processed / in-flight" guard.
- `_pendingInteriorSpawnsByCell`: `FormID → int` — in-flight interior spawn count for the interior cap.
- `_spawnedActors`: `ActorHandle → FormID` (base FormID) — identity map for mod-spawned clones, mutex-protected.
- `_loadTransitionTrackedActors`: snapshot of `_spawnedActors` handles taken during load transition.
- `_npcCache`: `EditorID → TESNPC*`; `_formIDToEditorID`: `FormID → EditorID`.
- `_cleanupCell`: cached `WIDeadBodyCleanupCell`.
- `_sessionID` (atomic), `_loadTransitionActive` (atomic), `_activeTaskCount` (atomic).
- `_currentPlayerCellFormID`, `_rng`.

### 5.2 Event sinks registered (`Register()`)

`RE::ScriptEventSourceHolder` sinks:
- `TESCellFullyLoadedEvent`
- `TESLoadGameEvent`
- `TESContainerChangedEvent`
- `TESDeathEvent`
- `TESActivateEvent`

### 5.3 NPC cache (`BuildNPCCache()`)

Iterates `RE::TESForm::GetAllFormsByEditorID()` under a read lock. For every `TESNPC` with a non-empty EditorID:
- Skips forms whose high byte is `0xFF` (dynamic/temporary forms).
- Fills `_npcCache[editorID] = npc` and `_formIDToEditorID[formID] = editorID`.

This cache is the **encounter identity** backbone: every candidate actor is resolved to an EditorID, then to a cached base NPC used for cloning.

### 5.4 EditorID resolution (`ResolveEditorID`)

Resolution order for a live actor:
1. `GetActorBase()` → TESNPC.
2. If leveled creature: `ExtraLeveledCreature::originalBase`, then `templateBase`.
3. `GetTemplateActorBase()`.
4. Each is looked up in `_formIDToEditorID`.

Debug logging dumps all candidate sources. Returns empty string if none resolve — such actors are skipped entirely (no clone).

### 5.5 Cell scan (`ScanAndSpawnForCell`)

Runs on the main thread (dispatched from the cell-loaded event). Steps:

1. Validate cell attached (`cellState == kAttached`); otherwise clear `pending` and abort.
2. Mark `data.pending = false`, then if `data.spawned` abort; else `data.spawned = true` (one-shot per cell).
3. Determine `isInterior = cell->IsInteriorCell()`.
4. Validate player + player cell attached.
5. Iterate cell references:
   - Skip if cell detached mid-iteration (sets `iterationSafe=false`).
   - Skip non-actors, player, FF-prefixed form IDs, invalid actors.
   - `IsActorValid`: not disabled/dead, 3D loaded, parent cell attached.
   - Hostility filter: `safeActor->IsHostileToActor(player)` must be true.
   - Exterior distance filter: `GetPosition().GetDistance(player) <= maxExteriorSpawnRadius` (only exterior).
   - Resolve EditorID (skipped during load transition → empty → skip).
   - Cache lookup: EditorID must exist in `_npcCache`; cached form must not be FF.
   - Uniqueness gating (see §6).
   - Blacklist filter (name + EditorID, case-insensitive partial).
   - Dedup by EditorID within the cell (`seenEditorIDs` set) — one entry per distinct EditorID.
   - Build `EnemyEntry { handle, editorID, isUnique }`.
6. If no spawnable (non-unique) enemies, abort.
7. Compute counts:
   - `baseCount = spawnableEnemies.size()`
   - `followerCount = GetFollowerCount()`
   - `targetTotal = ComputeSpawnCount(baseCount, followerCount, isInterior)`
   - `spawnCount = targetTotal - baseCount`
   - If ≤ 0, abort.
8. Interior cap:
   - `currentTotal = enemies.size()` (all hostile actors found, including unique)
   - `pendingTotal = _pendingInteriorSpawnsByCell[cell]` (default 0)
   - If `(currentTotal + pendingTotal) >= enemyLimitInterior` → skip.
   - `headroom = enemyLimitInterior - currentTotal - pendingTotal`
   - `spawnCount = min(spawnCount, headroom)`
   - `_pendingInteriorSpawnsByCell[cell] += spawnCount`
9. `PerformSpawns(spawnableEnemies, isInterior, cellFormID, spawnCount)`.

### 5.6 Count formula (`ComputeSpawnCount`)

```
rolledMultiplier = uniform(enemySpawnMultiplierMin, enemySpawnMultiplierMax)
multiplier       = rolledMultiplier
if followerCount > 0:
    multiplier *= 1 + followerCount * (followerMultiplier - 1)
if !isInterior:
    multiplier = min(multiplier, enemyMultiplierExteriorMax)
result = round(baseCount * multiplier)
```

Notes:
- Interior cells have **no** exterior cap — only the `enemyLimitInterior` hard cap.
- Follower scaling stacks multiplicatively on the rolled base multiplier.
- `enemySpawnMultiplierMin == Max` yields a deterministic multiplier.

### 5.7 Follower counting (`GetFollowerCount`)

- Requires player 3D-loaded and attached cell.
- Vanilla follower faction lookup `0x0005C84E`.
- Scans `ProcessLists::ForEachHighActor` and `middleHighActorHandles`:
  - Counts actors that are `IsPlayerTeammate() && IsInFaction(followerFaction)` and not waiting (`kWaitingForPlayer < 1.0`).
  - `forceCountedFollowers` patterns (case-insensitive partial on name) count any matching loaded actor regardless of faction.
- Dedups middle-high by FormID.

### 5.8 Batched placement (`PerformSpawns` / `ScheduleNextBatch`)

`PerformSpawns`:
- Builds a `SpawnBatch` with `enemies`, `isInterior`, `cellFormID`, `sessionID = _sessionID`, `batchOffset = 0`.
- Builds `spawnQueue`: for `a_spawnCount` entries, cycles through shuffled indices of `enemies` (reshuffle every `baseCount` entries) → spreads clones across distinct source actors.
- Calls `ScheduleNextBatch`.

`ScheduleNextBatch`:
- If `sessionID != _sessionID` → drop (stale).
- Increments `_activeTaskCount`.
- `AddTask` to main thread:
  - Session check again; drop if stale.
  - For each entry in the batch slice (`spawnsPerBatch`):
    - Resolve source actor from `entry.handle`; skip if invalid (decrement pending interior counter).
    - Resolve base NPC from `_npcCache` by EditorID (skip if FF).
    - Validate data handler + attached parent cell.
    - Furniture eligibility: `IsFurnitureEligibleActor` (race flags: Walks + CanPickupItems, not CantOpenDoors, not Flies). If eligible, find nearest available furniture within 600 units not occupied/reserved; reserve it.
    - Placement position/rotation = source actor's position/angle (clones spawn on top of source, not at furniture; furniture is only used to activate the clone onto a seat).
    - `CreateReferenceAtLocation(baseObj, spawnPos, spawnRot, parentCell, worldspace, ..., forcePersist=false, ...)` → new ref.
    - `TrackSpawnedActor(newActor, baseObj->GetFormID())`.
    - If `copyEquipmentFromOriginal`: `CopyEquipmentFromSource(source, clone, force=false)`.
    - If furniture selected: `furnitureRef->ActivateRef(clone, ...)`; on failure, schedule a 250 ms retry task (session-guarded).
    - Decrement interior pending counter (`cleanupPending`).
  - Update `batch->batchOffset`.
  - If remaining entries: detach a thread that sleeps `batchDelayMs` then calls `ScheduleNextBatch` again.
  - Else log completion.

Important: `CreateReferenceAtLocation` is called with `forcePersist = false` and `SetTemporary()` is NOT applied (per debug log comment) — clones are excluded from saves via the `SaveHook` instead.

### 5.9 Spawned-actor identity map

`TrackSpawnedActor(actor, spawnFormID)` inserts `actor->GetHandle() → spawnFormID` under mutex.  
`IsSpawnedByMod(actor)` looks up the handle. Used by:
- Save hook (exclude from save).
- Death handler (block activation, transport corpse).
- Activate handler (block loot).
- Container-changed logging.

---

## 6. Spawn safety filters

### 6.1 Candidate eligibility (cell scan)
| Filter | Location | Effect |
|---|---|---|
| Cell attached | `IsCellAttached` | abort scan if detached |
| Actor pointer sane | `IsPointerSane` | skip |
| Not player | `actor != player` | skip |
| FormID high byte ≠ 0xFF | `(formID >> 24) != 0xFF` | skip dynamic forms |
| `IsActorValid` | enabled + not dead + 3D loaded + attached parent cell | skip |
| Hostile to player | `IsHostileToActor(player)` | skip non-hostiles |
| Exterior radius | distance ≤ `maxExteriorSpawnRadius` | skip far exterior actors |
| EditorID resolvable | `ResolveEditorID` non-empty | skip unresolvable |
| Cached base NPC exists | `_npcCache.find` | skip unknown |
| Cached form not FF | high byte check | skip |
| Uniqueness gating | flags + settings | may exclude from spawn pool |
| Blacklist | name + EditorID partial | skip |
| EditorID dedup within cell | `seenEditorIDs` | one entry per distinct EditorID |

### 6.2 Uniqueness gating (critical safety)

```cpp
hasUniqueFlag = cachedNPC->actorData.actorBaseFlags has kUnique
if (!allowDuplicatingUniqueEnemies) {
    if (hasUniqueFlag) isUnique = true;
    else if (!allowDuplicatingNoRespawnEnemies &&
             !cachedNPC->actorData.actorBaseFlags has kRespawn) {
        // heuristic: boss-like if name contains "boss" OR no "lvl" in editorID
        isUnique = editorID contains "boss" OR editorID not contains "lvl";
    }
}
```

Then `spawnableEnemies` excludes `entry.isUnique && !allowDuplicatingUniqueEnemies`.

So by default:
- `kUnique` actors are never duplicated.
- Actors without `kRespawn` are treated as unique unless `allowDuplicatingNoRespawnEnemies` (and even then, `kUnique` still blocks).
- The two allow flags are mutually exclusive (`Clamp`).

### 6.3 Interior density cap
- `enemyLimitInterior` (default 35) caps total hostile actors in an interior cell.
- Counts both existing hostiles (`enemies.size()`) and in-flight spawns (`_pendingInteriorSpawnsByCell`).
- If at/above limit, spawn skipped entirely; otherwise `spawnCount` clamped to headroom.
- Pending counter decremented as each spawn is placed (or skipped).

### 6.4 Exterior density cap
- `enemyMultiplierExteriorMax` (default 3.0) caps the combined multiplier for exterior cells only.
- Interiors have no multiplier cap (only the hard count cap).

### 6.5 Load-transition guard
- `_loadTransitionActive` atomic set true on `kPreLoadGame` and on `TESLoadGameEvent`.
- During transition: `ResolveEditorID` returns empty (so no new candidates), and `SaveHook` returns false for tracked actors.
- Cleared on the first `TESCellFullyLoadedEvent` after load (clearing old tracked actors) or during `TESLoadGameEvent` main-thread task.
- Session ID incremented on load; all in-flight batch tasks check session and drop if stale.

### 6.6 Session staleness guards
- Every batch carries `sessionID`.
- `ScheduleNextBatch` and the task lambda check `_sessionID.load() != sessionID` and drop.
- Corpse transport thread checks session before moving.
- Furniture retry thread checks session before activating.

---

## 7. Encounter identity

Identity is **EditorID-based**, not FormID-based at runtime:
- `_npcCache` maps EditorID → `TESNPC*` (base form used for cloning).
- `_formIDToEditorID` maps FormID → EditorID (used to resolve live actors).
- `seenEditorIDs` dedups within a single cell scan (one candidate per distinct EditorID).
- Spawned clones are tracked in `_spawnedActors` by **ActorHandle** (not FormID), mapping to the base FormID for diagnostics.
- Candidate actors must have a non-FF FormID and resolve to a cached base NPC; FF-prefixed forms (dynamic) are always skipped.

This design means the mod keys off **stable, non-dynamic base NPC forms** and ignores anything created at runtime by other mods.

---

## 8. Reward dedup / save-bake prevention

The mod's central "reward dedup" is **preventing spawned clones from being saved**, so loot/rewards from clones don't persist as duplicate items in the save:

- `SaveHook::Install()` writes two vtable slots on `RE::Character`:
  - Slot `0x0D` `ShouldSaveGame`: returns `false` for tracked mod spawns (and during load transition); otherwise forwards to vanilla.
  - Slot `0x0E` `SaveGame`: returns `true` without calling vanilla for tracked spawns (so the engine thinks it saved but writes nothing); forwards for others.
- During load transition, tracked spawns are snapshotted (`SnapshotTrackedActorsForLoadTransition`) and erased from `_spawnedActors` at transition end (`ClearLoadTransitionTrackedActors`), so old-session clones are forgotten and won't be re-identified.
- `CreateReferenceAtLocation` is called with `forcePersist = false`.

Secondary reward/loot controls:
- `enableCorpseLooting` (default false): when false, activation of spawned dead bodies is blocked and a notification ("Nothing salvageable remains on this body.") is shown.
- `transportDeadSpawnsToCleanupCell`: when true, dead clones are moved to `WIDeadBodyCleanupCell` after 1000 ms (session-guarded), removing them from the world and making them unlootable. Placing items into a spawned corpse (before transport) causes that corpse to become save-baked — documented warning.
- `enableCorpseLooting` + `transportDeadSpawnsToCleanupCell` interact: transport overrides looting by removing the body.

---

## 9. Cleanup / disengage lifecycle

### 9.1 Cell transitions
- On `TESCellFullyLoadedEvent`:
  - Session check; if player cell changed, erase previous cell's `_cellCache` and `_pendingInteriorSpawnsByCell` entries.
  - Set `_currentPlayerCellFormID`.
  - Clear load-transition guard (and old tracked actors) on first cell load after a load.
  - If cell already `spawned` or `pending`, skip; else mark `pending` and call `ScanAndSpawnForCell`.
- `_cellCache` is `spawned`-flagged per cell → **each cell is processed at most once per session**.

### 9.2 Load game
- `TESLoadGameEvent` bumps `_sessionID`, sets load-transition guard, then (main thread) clears `_cellCache` and `_pendingInteriorSpawnsByCell`, resets player cell, and if an attached current cell exists, clears the guard and scans it immediately.

### 9.3 Pre-load (kPreLoadGame)
- Sets load-transition guard, snapshots tracked actors, clears cell state and player cell.

### 9.4 Death handling
- `TESDeathEvent`: `BlockSpawnedDeadBodyActivation(actor, "death-event")` and `TransportDeadSpawnToCleanupCell(actor)`.
- `BlockSpawnedDeadBodyActivation`: if not `enableCorpseLooting`, calls `SetActivationBlocked(true)` (idempotent).
- `TransportDeadSpawnToCleanupCell`: if enabled and actor is a tracked mod spawn and dead, spawns a detached thread sleeping 1000 ms, then (main thread task) re-validates session/settings/handle/dead/tracked and `MoveRefTo(actor, cleanupCell, nullptr, {0,0,0}, actor->GetAngle())`.

### 9.5 Activation handling
- `TESActivateEvent`: if player activates a dead tracked spawn and looting is disabled, block activation and show notification.

### 9.6 Container-changed logging
- `TESContainerChangedEvent`: logs (debug) when the player transfers items into a spawned dead body — a save-bake warning path.

---

## 10. Level scaling

**This mod performs no level scaling.** Clones are created from the cached base `TESNPC` at its native level; the source actor's level is not copied or adjusted. Follower count affects **density** (spawn count), not level.

Implications for reuse: any tier/level system must be layered on top of placement (e.g., adjust the clone's level/actor values after `CreateReferenceAtLocation`, or via a separate SKSE hook), because this mod's pipeline ends at placement + equipment copy.

---

## 11. Equipment copy (`CopyEquipmentFromSource`)

- Collects worn armor and equipped weapons (left + right hand) from the **source** actor.
- Records whether the **clone** already had armor/weapon worn.
- If not `force` (normal path): if clone already had armor, skip source armor; if clone already had weapon, skip source weapons.
- For each source armor: add to clone's container if not already carried, then `EquipManager::EquipObject`.
- For each source weapon: same.
- Does **not** copy spells, torches, ammo, or other items.
- Runs synchronously during placement (normal spawns) or ~0.5 s later for test clones (per UI strings; the code actually runs synchronously in `SpawnTestCloneNearPlayer`).
- `copyEquipmentFromOriginal` gates the whole step.

---

## 12. Furniture / placement details

- `FindAvailableFurnitureNear`: within 600 units of the source actor, finds the nearest `TESFurniture` ref that is not disabled/marked-for-deletion/3D-unloaded, not occupied (via actor occupancy scan), not reserved by this batch. Iteration is cell-detach-safe.
- Furniture is only used to seat the clone (`ActivateRef`); the clone's actual spawn position/rotation equals the **source actor's** position/rotation.
- Furniture eligibility requires a humanoid-ish race (Walks + CanPickupItems, not CantOpenDoors, not Flies).
- Delayed activation retry at 250 ms if initial `ActivateRef` fails.

---

## 13. UI (`UI.cpp`)

Registered with SKSE Menu Framework under section "Dynamic Enemy Spawns SKSE" → "Settings". Panel exposes all settings with live save-on-change, plus:
- Test Spawn Clone (no strip).
- Test Spawn Clone With Stripped Equipment (strips all armor/weapons from clone, then copies source gear) — verification tool.
- Blacklist editor (add/remove, case-insensitive).
- Force-counted followers editor (add/remove).

If SKSE Menu Framework is absent, `UI::Register()` logs a warning and no-ops.

---

## 14. Strings / localization

`Strings::Load()` populates an in-memory map with English defaults, then overrides from `Data/SKSE/Plugins/AutoEnemySpawn_Translation.ini` section `[Strings]` if present. `Strings::Get(key)` returns `const char*` (empty string if missing).

---

## 15. Compatibility & pitfalls

- **SKSE runtime version**: uses `RELOCATION_ID(56227, 56626)` for `MoveRefTo` and vtable slot hooks; must match the installed SKSE runtime. Mismatched runtime → crash on cell load or save.
- **Cell-state sensitivity**: all scans/placements check `cellState == kAttached`; detached cells abort. Fast travel / load transitions can leave batches dropped (intended).
- **No quest/city checks**: the mod does **not** detect cities, quest areas, or interior "safe" zones. It keys only on `IsInteriorCell()`, hostility, exterior radius, and the interior count cap. City interiors with many hostile NPCs will be expanded up to 35.
- **No cooldown timer between cell visits**: a cell is processed once per session (`spawned` flag); re-entering the same cell later in the same session does not re-spawn. Across sessions, clones are not saved, so the base count resets and spawns re-occur.
- **Exterior multiplier can explode density**: `enemyMultiplierExteriorMax` default 3.0 with follower scaling; large follower parties + high base counts can place many clones. Tune down for performance.
- **Batching cost**: `spawnsPerBatch`/`batchDelayMs` trade latency vs spikes; very low batch sizes slow fill-in.
- **Equipment copy is armor/weapons only** and runs after placement; clones may briefly appear un-equipped.
- **Clone identity**: clones are not tracked by FormID in a save-safe way beyond the handle map; the handle map is cleared across sessions, so post-load, previously spawned clones are indistinguishable from vanilla NPCs.
- **Loot/save-bake risk**: transferring items into a spawned corpse bakes it into the save; transport-to-cleanup-cell removes looting entirely.
- **Blacklist is name/EditorID partial match** (case-insensitive); broad patterns (e.g., "dragon") can over-filter.
- **Follower detection** relies on vanilla follower faction `0x0005C84E` plus optional force-counted names; custom followers not in that faction are missed unless force-counted.
- **Level scaling absent**: clones inherit base NPC level; no adaptation to player level beyond what the base NPC already does.
- **Concurrency**: detached threads + task interface; `_activeTaskCount` is decremented via RAII guards but is not used to gate shutdown.

---

## 16. Relevance to Runefall

Runefall's encounter-safety needs (caps/cooldowns/safety filters/identity/dedup/cleanup) map directly onto this mod's patterns. Recommended template:

### A. Encounter safety template (caps)
- **Hard interior cap** (`enemyLimitInterior`-style): maximum hostile actors in a cell before injection is skipped; count both existing and in-flight.
- **Exterior multiplier cap** (`enemyMultiplierExteriorMax`): clamp combined scaling for exterior cells to avoid density explosions.
- **Exterior radius gate** (`maxExteriorSpawnRadius`): only consider actors within N units of the player for exterior injection.
- **Hostility gate**: only expand encounters for actors hostile to the player.
- **Validity gate**: 3D-loaded, attached cell, non-disabled, non-dead, non-FF form.

### B. Cooldowns / one-shot-per-cell
- Mark each cell `spawned` after first processing; skip on re-entry within a session.
- Clear cell state on load game and pre-load; re-scan the loaded cell immediately.
- Session ID + staleness checks on every deferred task; drop stale work after load/save transitions.
- Load-transition guard that suppresses candidate resolution and save hooks until the first cell fully loads.

### C. Safety filters (uniqueness / identity)
- Resolve candidates by EditorID to a cached base form; skip unresolvable and FF/dynamic forms.
- Exclude `kUnique` actors and (by default) non-respawn actors; make both opt-in via mutually exclusive flags.
- Heuristic boss detection (`"boss"` in EditorID or no `"lvl"`) as a fallback uniqueness signal.
- Case-insensitive partial blacklist on name + EditorID.
- EditorID dedup within a cell (one candidate per distinct identity).

### D. Encounter identity
- Key identity on EditorID → base NPC, not runtime FormID.
- Track injected actors in a handle-keyed map (ActorHandle → base FormID) under a mutex; never by raw pointer.
- Snapshot and clear the map across load transitions to avoid stale references.

### E. Reward dedup
- Prevent injected actors from being saved via vtable hooks (`ShouldSaveGame`/`SaveGame` on `RE::Character`), so no duplicate loot persists.
- Block or gate corpse activation; optionally transport dead injected actors to the vanilla cleanup cell after a short delay (session-guarded).
- Warn when items are transferred into an injected corpse (save-bake risk).
- Create references with `forcePersist = false`.

### F. Cleanup / disengage
- Erase previous cell's cache/pending state when the player cell changes.
- Clear all cell state on pre-load and load game.
- Detach delayed work (corpse transport, furniture retry) must re-validate session, settings, handle, and liveness before acting.
- Batched placement with `spawnsPerBatch` + `batchDelayMs` to limit spikes; shuffle source indices to spread load.

### G. Equipment / loadouts
- This mod copies source armor/weapons only; Runefall can replace this step with tier-based loadouts (assign gear sets by tier after placement) while reusing the placement pipeline.
- Equipment copy respects "already equipped" categories — useful to avoid clobbering.

### H. What this mod does NOT provide (gaps Runefall must fill)
- No level scaling / virtual level adjustment (layer separately).
- No quest, city, or "safe zone" awareness (add filters if needed).
- No cooldown timer across sessions (spawns re-occur because clones aren't saved).
- No tier assignment or buff packages (add after placement using the handle map).
- No damage/protection integration (combine with KYE2/Calamity patterns separately).

---

## 17. Source paths (read-only)

Installed mod:
- `rpg-mechanism/Dynamic Enemy Spawns SKSE 178556 3.4 2026-09-29T14-44Z 2Bej58NL8/Dynamic Enemy Spawns SKSE/SKSE/Plugins/AutoEnemySpawn.dll`
- `.../SKSE/Plugins/AutoEnemySpawn.ini`
- `.../SKSE/Plugins/AutoEnemySpawn_Translation.ini`

Source (extracted):
- `rpg-mechanism/Source And PDB Files 178556 3.4 2026-09-29T14-44Z HZCMOyR1r/Source and PDB files for both/src/main.cpp`
- `.../src/PCH.h`, `PCH.cpp`
- `.../src/Version.h`
- `.../src/Settings.h`, `Settings.cpp`
- `.../src/SpawnManager.h`, `SpawnManager.cpp`
- `.../src/SaveHook.h`
- `.../src/Strings.h`, `Strings.cpp`
- `.../src/UI.h`, `UI.cpp`
- `.../PDB/AutoEnemySpawn.pdb`
- `.../COPYING.txt`, `EXCEPTIONS.md`
- Zip: `rpg-mechanism/Source And PDB Files 178556 3.4 2026-09-29T14-44Z HZCMOyR1r.zip`
- Zip: `rpg-mechanism/Dynamic Enemy Spawns SKSE 178556 3.4 2026-09-29T14-44Z 2Bej58NL8.zip`
