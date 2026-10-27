# Runefall - POC base status (2026-10-27)

**Not a release.** The current code is the validated POC base for the real mod. Release metadata is
0.1.0 so the first public build can use that version, but no Nexus upload, tag or push has been made.

## What is proven

| Area | Evidence |
|---|---|
| Static gates (DLL, generator, Papyrus, FOMOD, release tree) | 6/6 PASS, static-gates-20261027-234233 |
| In-game batch | 33/33 PASS, 19/19 self-tests, no crash, batch-test-20261027-234332 |
| Form catalog | 554 rows packaged; catalog self-test PASS; archive verifier PASS |
| Container/actor/quest/rune/co-save paths | PASS in batch |
| Death drop + hunt first kill | PASS in batch |
| Compatibility smoke | OWL, Worlds Dawn, Fortunate Foes, DiabloRim PASS (targets verified loaded) |
| Uninstall matrix | pack-disable, dll-remove, all-remove PASS |
| Papyrus policy | 4 allowlisted scripts, checker PASS, debug natives off by default |
| Archive verifier | PASS, release-verify-20261027-234307 |

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

- DLL 1,093,120 B sha256 95A6A444CDA9449BA2F42EC4F17C25F98545F34DDAD87E3C21D038E6E633264A
- Candidate Runefall-0.1.0.zip 530,166 B sha256 D3B7C97A0956C81BC2D6BE20807EBFBD274E618B4E4C45EF65AE8432A5B63B03
- Local main is ahead of origin/main; not pushed.
