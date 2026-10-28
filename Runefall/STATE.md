# STATE - Runefall

STATUS: v0.2 remediation — S0 truth reset — audit in progress — NOT RELEASED.

Last updated: 2026-10-28

## Current program

- Active authority: PROGRAM.md (v0.2 Remediation Program) and build/slices/S0/spec.md.
- Baseline commit: a50b2b6.
- Baseline audit: 7 PASS, 30 PARTIAL, 11 FAIL (build/audit/plan-gaps.md, runtime-gaps.md).
- The previous all-slices PASS and verified release claim are RETRACTED.
- Current slice: S0 truth reset and audit harness — implementation, pending independent audit.

## What is actually proven

Only the 7 baseline PASS slices are accepted: F0.1, F0.4, C1.4, C1.5, R5.1, R5.4, R5.6.
Runtime evidence also confirms hook 0x104 installation, 49 tables/2,120 rows loaded,
schema-1 to schema-3 co-save migration, and packages/equipment applying once in-game.
Everything else is PARTIAL or FAIL until an independent slice audit says otherwise.

## Not verified / limitations

- PrismaUI host rendering and screenshots are not verified; UI slices are contract-only.
- Full 3,700-mod MO2 profile, KYE2 and OWL takeover behavior are not verified.
- Scripted final-boss kill, item-instance identity, full damage-category live coverage,
  nine classes, ambient spawns and co-save v2 fixture migration remain gaps.
- No public release, upload, tag or push has been made.

## Historical

The 0.1.0 internal candidate archive was built and locally verified, but the 2026-10-28 audit
invalidated its all-PASS acceptance claim. It is not a release and must not be published.
