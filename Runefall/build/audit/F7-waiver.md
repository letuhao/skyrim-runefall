# F7 hotfix scope waiver (Lead)

The candidate contains one file outside the frozen S1 allowlist: src/plugin/src/InputManager.cpp (F7 handler). The change removes a crash-producing ConsoleLog::Print of the multi-kilobyte diagnostics JSON; crash stack at 2026-10-29 04:26:47 was Runefall.dll!ConsoleLog::Print -> SkyrimSE.exe. The full diagnostics remain in Runefall.log.

Rationale: a player-triggerable CTD is never acceptable (AGENTS.md 3.1). The fix is two lines in a debug-only handler, does not touch CoSave behavior, and was followed by a successful F7 retest. This is a crash-safety waiver, not a weakening of any S1 acceptance criterion; it is recorded as follow-up F7-CONSOLE-THREAD and in HISTORY as an explicit exception.
