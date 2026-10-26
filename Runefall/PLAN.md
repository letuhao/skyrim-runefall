# PLAN - Runefall

Ordered milestones. Each milestone is complete only when its acceptance evidence exists.
Design details: GAME-DESIGN.md. Decisions: DECISIONS.md. Gates: VALIDATION.md.

## M0 - Context and design baseline (DONE 2026-10-26)

Deliverables: versioned workspace, GAME-DESIGN.md, PLAN.md, DECISIONS.md, VALIDATION.md,
STATE.md, CHANGELOG.txt, CURRENT.txt, MEMORY candidates.
Acceptance: files exist, target environment and architecture recorded, open decisions listed.

## Design phase - layered specs (IN PROGRESS)

Spec process: SPECS\00-SPEC-PROCESS.md. No code or forms before these layers are approved.

- D1 L1 gameplay model (DONE, in review): SPECS\L1-GAMEPLAY-MODEL.md.
- D2 L2 Skyrim mapping (DONE draft): SPECS\L2-SKYRIM-MAPPING.md plus L2A coverage; decisions
  D-021..D-026 recorded; Phase 0 validation list extracted.
- D3 L3 content and data architecture (DONE, gaps G1-G12 closed; crafting expansion reserved):
  SPECS\L3-CONTENT-DATA.md, L3B-CONTENT-MATH.md (120 affixes/600 rows/~201 MGEF; v1 ~465-495
  records), L3A reference comparison, L3C parity/gap closure, L3D unique-affix mining
  (DataMiner, U=59), L3E hunt/general drop tables (F=19 hunt families), D-040 multi-ESL packs,
  D-045/D-046 crafting expansion + hunting primacy. Build/validator design ready.
- D4 L4 runtime architecture (DONE draft): SPECS\L4-RUNTIME.md (D-049..D-055; 11 hooks, co-save
  TLV, bridge API, UI, budgets, failure modes, 12-item Phase 0 checklist), plus N-01..N-10
  acceptance mapping.
- D5 L5 compatibility and release (DONE draft): SPECS\L5-COMPAT-RELEASE.md (D-056..D-061;
  dependencies, compatibility passes, FOMOD matrix, adapted ship gates, test plan, release and
  Nexus policy).
- Design layers complete. Next: Phase 0 spike (M1) against the L4 12-item checklist.
- Future scope lock: SPECS\GAP-REGISTER-FUTURE.md (32 reserved/watched items, expansion roadmap).
- D3 L3 content and data architecture: records, IDs, schemas, generators, localization.
- D4 L4 runtime architecture: DLL modules, hooks, co-save, Papyrus API, UI integration.
- D5 L5 compatibility and release: dependency policy, compat matrix, FOMOD, test matrix,
  Nexus kit.
Acceptance: each layer reviewed and approved; no open question from a layer is silently
assumed by the next.

## Strategy change 2026-10-26 - implement first, batch test later (D-063)

Manual in-game testing is expensive: it needs a game launch, a save, console/test probes and
human play. v1 is therefore built to completion under static validation and an automated
in-DLL probe harness, then tested in **batch sessions** (one launch per batch), not feature by
feature. Phase 0's already-proven bits (build, plugin load, Papyrus/co-save registration) stay
valid evidence; the remaining Phase 0 unknowns are implemented directly as product code.

## I1 - Native core implementation (no game required)

Config (INI), pack manager (manifest discovery/validation/merge), data store (all CSV/JSON
schemas), form resolver (EditorID to FormKey), co-save (versioned TLV sections), logging,
lifecycle/bootstrap. Every module has static self-check functions.

Acceptance: build PASS; debug harness reports config/data/pack/co-save self-checks PASS without
the game.

## I2 - Data and content build pipeline (foundation DONE 2026-10-26; records in progress)

CSV data for affixes/rarity/ranks/hunt/drops/skills/attributes/materials/recipes/unique/strings;
Mutagen generator produces base ESL + content packs (Unique, Hunt, Bosses, Affixes, Quests,
Crafting reserved); deterministic FormID allocation, ESL flags, manifest files; validator
cross-checks all FormID/EditorID references.

Acceptance: generator + validator PASS; all packs open with Mutagen; no vanilla overrides.

## I3 - Gameplay systems implementation

Item metadata/registry, loot engine, tier/rank engine, hunt engine, progression, RuneForge,
effect application, dynamic item mutation (SKSE CreateEnchantment/SetDisplayName/SetItemHealth),
general/race/boss/quest drop handling, firstOnly state, caps and pity.

Acceptance: compile + unit-style probes inside debug harness (deterministic RNG fixtures);
no per-feature game test.

## I4 - Papyrus bridge, UI and input

Versioned Papyrus API/events, SKSE Menu Framework pages (Character, Rune Forge, Hunt Codex,
Item Detail), MenuApi snapshots, hotkeys/input mapping, INI surface, localization strings.

Acceptance: Papyrus source compiles with PapyrusCompiler; menu code builds; bridge self-test
probes PASS.

## I5 - Batch test harness and static validation suite

Debug mode: on kDataLoaded run every probe, write RunefallProbe.log with PASS/FAIL lines; test
quest/script exercises Papyrus calls; save/load/revert probe; deterministic RNG; log export.
Static gates: build, data validator, plugin header audit, Papyrus compile, FOMOD/archive checks.

Acceptance: one command produces a full static evidence bundle; probe harness ready for T1.

## I6 - Build and package v1 candidate

Generate final ESLs/data/scripts/DLL/FOMOD; validate; produce archive, hashes, docs, Nexus kit.

Acceptance: v1 candidate archive PASS under static gates.

## T1 - Batch in-game test session (single launch, user-assisted)

Launch once on the 1.6.1170 test env; run the automated probe suite and a short scripted
walkthrough; save/load/revert; capture logs. A feature is not "tested" before this session.

Acceptance: probe report + manual walkthrough evidence recorded in VALIDATION.md; failures
listed for the fix batch.

## T2 - Fix batch and release validation

Fix all T1 failures, rebuild, rerun static gates; one confirmation launch if needed; final
release archive and publish.

Acceptance: all gates PASS; release archive hash recorded; known limitations disclosed.
