# Lessons learned — 2026-10-29 (build/test integrity failure)

## What happened

1. A SelfTest edit introduced a duplicate `emptyVersion` variable. The build failed.
2. The build was launched in a background command whose failure was not surfaced; the batch
   script was then invoked, but it necessarily ran no game because the build stopped earlier.
   The result was reported as a "test attempt" when no test had run.
3. The batch harness did not require proof that the target save existed, that `load_save`
   was accepted, or that the player had left the main menu. Main-menu state could satisfy
   plugin-load/self-test checks, producing misleading PASS lines.
4. Pressing F7 crashed the game inside `RE::ConsoleLog::Print` while passing the multi-kilobyte
   diagnostics JSON. This was a real Runefall crash path, not an environment crash.

## Root causes

- Build result was not treated as a hard gate before runtime testing.
- The batch report wrote gameplay checks even when no save had loaded.
- A debug console print passed an unbounded/large string through the game console API.
- Stale test-environment DLLs were confused with the current source; unverified token edits
  were built once, reverted in source, but not rebuilt before the next run.

## Corrective actions implemented

- `run-batch-test.ps1` now **fails before gameplay checks** unless all are true:
  save `.ess` and `.skse` exist, MCP reports connected, `load_save` is accepted,
  `get_player_info` returns a name other than `Prisoner`, and `get_cell_info` has a formId.
  It writes a preflight-failure report and exits 2 instead of emitting a passing report.
- F7 no longer calls `ConsoleLog::Print` with full diagnostics; the JSON is logged only.
- `AGENTS.md` rule 3.1.12 now requires a foreground build, exit-code check, and DLL
  existence/newer-than-source check before any game launch or batch.
- This workspace's `skyrim-mcp-control` skill documents the mandatory load preflight and
  distinguishes a connected pipe from a loaded game.

## Rules for future slices

1. Build foreground; paste the last lines and store `buildExit=0` in evidence.
2. Verify the DLL copied into the test environment has a newer timestamp/hash.
3. Never run a batch after an unreported build failure.
4. Never accept a batch PASS report that lacks save/load/player/cell preflight evidence.
5. After any crash, stop and isolate; do not continue into feature checks.
