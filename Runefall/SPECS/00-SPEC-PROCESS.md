# Runefall - spec layers and process

Status: layer model established 2026-10-26. Layer 1 is in review; layers 2-5 are drafts.

## Why layers

The gameplay must be designed first as a game (Diablo/Torchlight-derived), then translated into
Skyrim's engine constraints, then into content and code. Skipping a layer means either designing
mechanics the engine cannot express, or writing a plugin before knowing what it must do.

Each layer is a document with: scope, inputs, decisions, numbers, open questions, review gate.
A layer is approved before the next layer is drafted in detail. A change in a lower layer
ripples upward - that is expected and recorded in CHANGELOG/DECISIONS.

## Layers

| Layer | File | Question it answers | Status |
|---|---|---|---|
| L1 Gameplay model | SPECS\L1-GAMEPLAY-MODEL.md | What is the game? Attributes, items, loot, tiers, crafting, economy - engine-agnostic. | in review |
| L2 Skyrim mapping | SPECS\L2-SKYRIM-MAPPING.md | How does each L1 mechanic map onto Skyrim systems, which need native hooks, which are data, what is impossible or must change? | draft next |
| L3 Content and data architecture | SPECS\L3-CONTENT-DATA.md | What records, schemas, config files and generators exist; naming, IDs, distributions, localization. | not started |
| L4 Runtime architecture | SPECS\L4-RUNTIME.md | DLL modules, hook points, save format, Papyrus API, UI surfaces, performance budgets, failure modes. | not started |
| L5 Compatibility and release | SPECS\L5-COMPAT-RELEASE.md | Dependency policy, compatibility matrix, FOMOD modules, test matrix, versioning, Nexus kit. | not started |

Layer 0 (vision, targets, non-goals, name) lives in GAME-DESIGN.md section 1-2 and feeds L1.

## Review gate for a layer

A layer is approved when:

1. Every mechanic has a rule and a number (or is explicitly marked "tunable, default X").
2. Every mechanic names its failure mode and the player-visible consequence.
3. Open questions that belong to the next layer are listed as inputs, not silently assumed.
4. No mechanic contradicts an earlier accepted decision (DECISIONS.md).
5. The user has reviewed the layer and either approved it or listed changes.

## Traceability

- Mechanics get stable IDs: P (progression), I (item), L (loot), T (tier), C (crafting),
  E (economy), R (rune). Example: L1-I-03 = item rarity bands.
- L2 maps each ID to an engine mechanism; L3 maps it to records/data; L4 maps it to code and
  hooks. Do not invent new mechanics inside L3/L4 without adding them to L1.

## Current next step

Review SPECS\L1-GAMEPLAY-MODEL.md. After approval, draft L2 (Skyrim mapping) using its
open-question list, then L3/L4, and only then start the Phase 0 spike (PLAN.md M1), which
remains a technical de-risking step, not gameplay implementation.
