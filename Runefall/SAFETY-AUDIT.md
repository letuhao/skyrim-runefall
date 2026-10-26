# Runefall native plugin - safety audit

Scope: `Runefall/src/plugin/src` (C++/CommonLibSSE-NG). Rule: a logged warning or a disabled
feature is acceptable; **a player CTD is never acceptable**.

## Non-negotiable invariants

1. No game data in `SKSEPlugin_Load`; forms only from `kDataLoaded`; player/world/inventory and
   runtime game forms only after `kNewGame`/`kPostLoadGame` + readiness check + delay.
2. Every `GetSingleton`, `Lookup*`, `As<T>`, array index and game cast is null/validity checked
   and logged on failure.
3. No `delete`/`free`/`malloc` on game-owned memory; factory forms are never deleted.
4. No game mutation or file I/O inside event/input dispatch; FormID + `AddTask` + revalidate.
5. Only the main thread calls the game; workers sleep only and enqueue tasks.
6. Serialized data is size-capped/validated before allocation; collections are capped.
7. Every SelfTest restores real player state before returning.
8. File/config/JSON/path handling degrades to defaults with a log.
9. Unproven ownership/timing paths are disabled, not shipped.
10. Debug build + PDB + CrashLogger are required for runtime diagnosis.

## Current status (2026-10-27)

| Area | Status |
|---|---|
| Runtime form timing | PASS: gameplay-ready gate, default 300 s delay, player check, clamp >=5 s |
| Null/validity checks | PASS: EFID/baseEffect, deleted forms, arrays, base name, datahandler, framework |
| Ownership | PASS: runtime loot uses InventoryChanges::EnchantObject (verified in-game); AttachToEntry remains disabled |
| Event dispatch | PASS: death/input capture FormID and queue main-thread tasks |
| Threads | PASS: worker only sleeps; game calls on main-thread tasks |
| Save data | PASS: 16 MiB record cap, 10k item cap, malformed payload rejected |
| SelfTest state | PASS: RuneForge wallet + HuntEngine first-kill map restored via RAII |
| File/config input | PASS: PackManager null/try-catch, config defaults, missing framework guarded |
| Runtime testing | probe item + death drop (enchanted corpse item) verified in-game with EnchantObject; no crash |

## Residual / accepted gaps

- `PapyrusBridge::GetMenuSnapshot` and read-only natives still read state from the Papyrus VM
  path; mutating natives are queued. Keep all script API use on the main thread.
- `ExtraDataList` attachment for inventory entries is disabled until an owning-container API path
  is proven.
- Vanilla base-item FormIDs are master-only by design; any Runefall ESL record lookup must go
  through `TESDataHandler::LookupForm(localId, pluginName)`.
- Debug hotkeys F10/F11/F12 remain registered when input is enabled; their work is queued to the
  main thread, and F11 runs self-tests only in debug builds.
- Loading-menu readiness is approximated by player existence plus the 300 s delay; keep the
  delay generous for slow load orders.

## Mandatory self-audit after each implementation

```powershell
# 1. pattern audit
Get-ChildItem Runefall/src/plugin/src -Recurse -Include *.cpp,*.h |
  Select-String -Pattern 'delete |free\(|malloc\(|new RE::|AddFormToDataHandler|AddObjectToContainer|GetSingleton\(\)'
# 2. build with warnings
cmake --build --preset release-windows
# 3. review every hit against the invariants above
# 4. record result in VALIDATION.md; disable unproven paths
```