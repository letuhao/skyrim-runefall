# Runefall - internal release candidate status (2026-10-28)

**Internal candidate only. No Nexus upload, git tag or push has been made.** The archive below is
verified locally; publication remains a user decision.

## Version

- CURRENT.txt: Runefall 0.1.0
- FOMOD info.xml: 0.1.0
- Archive: build/release/Runefall-0.1.0.zip (714,385 bytes)
- SHA-256: 27241B9D7D71A6BA00B5262FBE68CD9A570DA4DEDE5478A55B3CC7CF57B940F2
- Archive verifier: 25/25 PASS, build/evidence/release-verify-20261028-114412.md

## Verified

| Area | Evidence |
|---|---|
| Static gates (DLL, generator, JSON, strings, host, Papyrus, FOMOD, release tree) | 10/10 PASS, static-gates-20261028-114102 |
| In-game batch | 38/38 PASS, batch-test-20261028-113956 |
| Runtime content cache/manifest | W2.7 evidence |
| Co-save schema 3 + XP/class/skill/endgame fields | CoSave.SelfTest PASS |
| Compatibility smoke (DiabloRim + SPID/KID/SkyPatcher) | r6.1-compat-20261028 |
| Uninstall matrix (pack-disable / dll-remove / all-remove) | r6.2-save-uninstall-20261028 |
| Balance calibration (37.4h, PCR +10 1.16x, +20 1.35x) | r6.3-balance-20261028 |
| English strings (262 keys) | R5.6 validator + static gate |

## Not verified in this environment

- Full 3,700-mod Mantras of Mara MO2 profile; only a bounded external-mod smoke test was run.
- PrismaUI host visual rendering/screenshots; JSON contracts and shell source only.
- Scripted final-boss kill; endgame completion path is self-test/engine verified.
- English-only v1; other languages out of scope.

## Remaining work for public release

- User-side PrismaUI visual acceptance and final media gallery.
- Full MO2 profile compatibility pass, including OWL/SPID/KID conflict-heavy lists.
- Final boss encounter content pass and any remaining balance tuning in real play.
- Publication metadata/push decision.
