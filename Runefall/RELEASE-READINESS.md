# Runefall — release readiness (0.2.0-dev, 2026-10-27)

## Gates that pass

| Gate | Evidence |
|---|---|
| Static gates (DLL, generator, Papyrus, FOMOD, release tree) | 5/5 PASS, static-gates-20261027-110226.md |
| In-game batch incl. MCP TapKey hotkeys | all checks PASS, batch-test-20261027-110306.md |
| Self-tests | 18/18 PASS |
| Release archive layout/members/XML/hash | 21/21 PASS, release-verify-20261027-110455.md |
| Runtime loot / death drop / hunt unique | verified on CleanSave, no crash |
| Menu pages | Character, Rune Forge, Hunt Codex, Item Detail captured |
| Compatibility baseline | 86-plugin test list; UI/USSEP/Alternate Start/CC/SkyUI/EngineFixes/SkyrimSoulsRE PASS |

## Remaining scope before a non-dev release

- P4.2 mutation hooks: container generation, actor-spawn buffs, cell/menu/item mutation paths.
- P3.7 runtime application of drops_containers.csv and drops_quests.csv (tables loaded, not consumed).
- P5.6/P5.7: uninstall/pack-disable matrix and untested compatibility targets (OWL, DiabloRim, Worlds Dawn, Fortunate Foes, overhauls).
- Media: loot/hunt gallery shots; tooltip close-up.
- Version promotion from 0.2.0-dev to the release version, then final Nexus upload.

## Current artifact

- DLL 1,021,440 B sha256 055950B21D91930871D794B79508871AF640712A83D7149EAE6D1EF48F48702C
- Candidate Runefall-0.2.0-dev.zip 495,667 B sha256 BAC5B815F8467F1EA4BC618D460556B56F1D86A0210ADA5672D8D286DF36A346
- Local main is 44 commits ahead of origin/main; not pushed.

## Recommendation

Treat this as a **beta candidate**, not a final release. The build is runtime-stable on the tested
1.6.1170 setup; the remaining items above are feature/media/compatibility completeness, not build
or crash blockers.