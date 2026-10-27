# Runefall - POC base status (2026-10-27)

**Not a release.** The current code is the validated POC base for the real mod. Release metadata is
0.1.0 so the first public build can use that version, but no Nexus upload, tag or push has been made.

## What is proven

| Area | Evidence |
|---|---|
| Static gates (DLL, generator, Papyrus, FOMOD, release tree) | 5/5 PASS, static-gates-20261027-231233/232716 |
| In-game batch | 33/33 PASS, 19/19 self-tests, no crash, batch-test-20261027-232958 |
| Form catalog | 554 rows packaged; catalog self-test PASS; archive verifier PASS |
| Container/actor/quest/rune/co-save paths | PASS in batch |
| Death drop + hunt first kill | PASS in batch |
| Compatibility smoke | OWL, Worlds Dawn, Fortunate Foes, DiabloRim PASS (targets verified loaded) |
| Uninstall matrix | pack-disable, dll-remove, all-remove PASS |
| Archive verifier | PASS, release-verify-20261027-233226 |

## What is still POC

- Affix/skill MGEF semantics are placeholders (all ValueModifier/Health).
- Item metadata is not instance-keyed; save/load loses registry identity.
- Progression writes GLOB mirrors only; no actor-value/perk application.
- UI is four read-only pages; no interactive forge/salvage/MCM.
- Content placement, monster affixes, boss encounters and quest content are not implemented.
- Only a 5-weapon base pool; no armor/jewelry/slot model.
- Compatibility is smoke-level on an isolated 87-plugin env, not the full 3,753-plugin Mantras list.
- Media gallery/tooltip capture is incomplete.

## Architecture

The binding structure, dependency rules, identity model, effect schema, co-save contract, POC debt
register and migration phases are in [ARCHITECTURE.md](ARCHITECTURE.md). Real development starts at
Phase B (item instance identity + co-save v3) and Phase C (domain/service split).

## Artifacts (not published)

- DLL 1,092,608 B sha256 D7F0912C9F8E18D5E3D30F8D817A809289F45567337B03EA3E29CEDDB2898A94
- Candidate Runefall-0.1.0.zip 530,036 B sha256 1A07C7939D548828C067DD11E2ABE04E474B1BCB6AB6EC083CF66FAB1549C859
- Local main is ahead of origin/main; not pushed.
