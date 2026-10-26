# MEMORY candidates - Runefall

Candidate lessons for the evidence registry. Not promoted automatically; promotion follows
the memory-promotion protocol.

1. Worlds Dawn's 28-repurposed-vanilla-AV attribute system is a conflict amplifier and is
   not reversible. Rejected for Runefall; keep attributes in native save data + own records.
   Evidence: _context\rpg-mechanism\05-WORLDS-DAWN-PAPYRUS.md sections 3.1, 6.1.
2. Fortunate Foes distributes custom keywords with no KYWD provider in its ESPs: distribution
   can silently no-op. Always verify keyword providers exist for distributor rules.
   Evidence: _context\rpg-mechanism\07-FORTUNATE-FOES-CONTENT.md, 06-FORTUNATE-FOES-ESP.md.
3. DiabloRim is closed source with no Papyrus API; its only extension surfaces are the CSV
   table, INI keys and Grid Inventory messaging. Do not plan a feature that needs more.
   Evidence: _context\rpg-mechanism\01-DIABLORIM.md.
4. SkyPatcher patches existing records at runtime; it cannot create new forms. Form creation
   belongs in an ESL plugin (our own FormIDs), never in a DLL at runtime (not persisted).
5. CommonLibSSE-NG's public 2023 release predates 1.7.x runtime support; pin a commit after
   the 2026-10-12 merge (issue #113) and handle Address Library Format 5 with fallback.
6. "Newest Skyrim version" must be verified from the PE version, not assumed: local MO2
   Stock Games are pinned at 1.6.1170 while the Steam install is 1.7.104.
7. Never test a native plugin in the live Wabbajack list; copy the MO2 profile or use a
   minimal profile, so a bad DLL cannot corrupt a 3,700-mod setup.
8. Runtime-applied and co-save-rebuilt effects preserve uninstallability; permanent NPC
   edits (perks/abilities) bake into saves and are irreversible.
