# F7 diagnostics crash hotfix (out-of-S1-scope safety fix)

Crash: 2026-10-29 04:26:47, crash-2026-10-28-21-26-46.log. Probable stack:
Runefall.dll!RE::ConsoleLog::Print -> SkyrimSE.exe+08F9519 -> SkyrimSE.exe+010AC53.

Cause: InputManager F7 handler passed the multi-kilobyte diagnostics JSON to ConsoleLog::Print("[Runefall] diagnostics: %s", ...).
This is a debug-only hotkey, but a player pressing F7 could CTD.

Fix (commit cda5dbe): remove the ConsoleLog::Print call; keep the full JSON in Runefall.log. F7 retest after the fix logged diagnostics with the game process alive and no new crash.

Scope note: InputManager.cpp is outside the frozen S1 allowlist. This is a crash-safety hotfix, not co-save scope; it is recorded as follow-up F7-CONSOLE-THREAD and must be reviewed by the independent S1 auditor. It does not alter co-save behavior.
