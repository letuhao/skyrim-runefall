# S1 A8 batch-rig blocker

Date: 2026-10-28, after repeated test launches.

Observed: run-batch-test.ps1 starts Skyrim 1.6.1170 and MCP connects, but the batch reports
Player loaded name= empty, save load requested FAIL, gameplay readiness FAIL, co-save write/load
FAIL, while Runefall plugin load and portfolio self-tests (38/38) pass. No new crash log is written.

Repository/rig facts:
- clean save exists: Save3_EAEC9073...Tamriel... .ess/.skse (3,088,989 B / 2,924 B).
- Runefall.log ends with probe pass=38 and no co-save load line for the failed attempts.
- Repeated full process kills, setup-test-env.ps1, and 8 s settle do not restore player/save load.
- This began after many consecutive batch launches; earlier same-day runs passed at 17:41/17:58/18:13/18:33/18:39.

Conclusion: A8 runtime re-verification is BLOCKED by the test rig's main-menu/load state, not by a
new Runefall crash frame. The last batch-passing candidate remains 576a840; its evidence is retained.
The rig needs a clean restart/relaunch (Steam/SKSE/main menu verified, save file visible) before any
new runtime claim. Do not mark S1 A8 PASS from stored logs for a later candidate.


## 2026-10-29 12:55 — rig hang after startup (reverted build)

The pure codec split compiled and its host tests passed (31 assertions / 5 cases), but the in-game build
hung after startup: 38/38 self-tests passed, then MCP get_player_info/get_cell_info timed out and save
load never completed. The split was reverted, the previous source rebuilt (DLL 01F7B92F..., 12:48:50),
and a fresh launch still reached self-tests then hung at the main menu with the same MCP timeout.
The clean save files are byte-identical to _context/clean-save backups. No new crash log.
Conclusion: the current blocker is the test rig/main-menu/game-thread state, not proven to be the codec
split. A clean Steam/SKSE/game restart is required before another runtime attempt. Do not claim S1 A8.
