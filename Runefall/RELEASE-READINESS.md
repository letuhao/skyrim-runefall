# Runefall - release readiness

STATUS: v0.2 remediation — S0 truth reset — audit in progress — NOT RELEASED.

**The 0.1.0 internal candidate release claim is RETRACTED (2026-10-28 audit).** No archive in
build/release is currently release-ready; no Nexus upload, git tag or push has been made.

## Current status

- Program: PROGRAM.md v0.2 remediation.
- Baseline: a50b2b6, audit 7 PASS / 30 PARTIAL / 11 FAIL.
- Current: S0 truth reset and audit harness, pending independent implementation audit.
- Release block: version 0.1.0, archive null, verified false, published false, retracted true.

## Verified baseline (not a release)

- F0.1, F0.4, C1.4, C1.5, R5.1, R5.4, R5.6 passed the prior audit.
- Runtime hook 0x104, 49 tables/2,120 rows, schema-1 to schema-3 migration, package/equipment
  application observed in-game.
- Everything else is PARTIAL/FAIL per the audit reports in build/audit/.

## Required before any release claim

- S0-S15 independently audited and PASS.
- Real PrismaUI host evidence, full MO2 compatibility decision, final-boss/endgame evidence,
  item identity, co-save v2 migration, damage-category live evidence.
- Reconciled version/changelog/FOMOD/archive and a fresh release audit.
