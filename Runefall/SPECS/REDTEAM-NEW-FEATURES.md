# Red-team audit — new features (Runefall)

Status: 2026-10-27. Read-only challenge of the 12 unique features discussed in the
session prompt against the Runefall spec stack (GAME-DESIGN.md, DECISIONS.md,
L1-GAMEPLAY-MODEL.md, L4-RUNTIME.md, FIX-PLAN.md, GAP-REGISTER-REFERENCE-GAMES.md,
GAP-REGISTER-FUTURE.md, AGENTS.md §3.1). Companion to
`SPECS/AUDIT-NEW-FEATURES-DESIGN-DEPTH.md` (design-depth audit). No code or records changed.

Challenge legend: A=assumption, C=contradiction, E=edge case, S=save compatibility,
P=performance, M=compatibility (mod interop), Q=scope creep, H=ethics/content.
Severity: CRITICAL / HIGH / MEDIUM / LOW.

---

## 1. Imbue (RG-055b) — material fused onto equipment, break risk

- **A-IMB-1 (A, CRITICAL):** Assumed "imbue" can be applied without mutating base records
  (D-027). No application channel is chosen (dynamic enchantment? extra data? new MGEF?).
  If the only viable channel is a dynamic enchantment slot, it collides with the socketed-rune
  power (I-05) and the reroll/upgrade channel (D-023) — both already consume the enchantment
  slot. **Resolution:** pick and prove one channel in a Phase-0 spike before any L1 rule.
- **A-IMB-2 (A, HIGH):** Assumed imbue state fits in ItemMetadata without a schema bump. L4 §4
  ItemMetadata has no imbue field; adding one is an N-01 migration with a rollback path.
  **Resolution:** define `imbueMaterialId`, `imbueStatHash`, `imbueMagnitude` in ItemMetadata
  and bump co-save schema version.
- **C-IMB-1 (C, HIGH):** I-05 says sockets are the only rune-power channel and socket count is
  fixed by rarity. Imbue introduces a *second* power channel on the same item with no rule for
  coexistence (does imbue consume a socket? stack with it?). **Resolution:** amend L1 I-05/I-09
  to state the interaction explicitly.
- **E-IMB-1 (E, HIGH):** Two imbue materials with conflicting stat directions (e.g. +melee
  damage vs +spell damage) — is stacking allowed, does the second overwrite, is there a cap?
  Undefined. **Resolution:** define "one imbue per item" or "one per category" rule and a
  replacement cost.
- **E-IMB-2 (E, MEDIUM):** Break outcome undefined (imbue lost? item destroyed? durability hit?).
  If "break" destroys the item, it conflicts with the salvage loop (C-02) expectations and
  ItemRegistry pruning. **Resolution:** define break = imbue lost, item survives, plus a
  "shattered" cosmetic state; no item destruction.
- **S-IMB-1 (S, MEDIUM):** Imbue state must survive reroll (C-04 says reroll keeps rarity and
  sockets). Does reroll preserve imbue? Undefined. **Resolution:** state explicitly in C-04
  amendment.
- **M-IMB-1 (M, MEDIUM):** Salvage (C-02) — does salvage recover the imbued material? If yes,
  imbue becomes a risk-free material sink reversal. **Resolution:** salvage returns base
  materials only; imbued material is consumed (break risk is the sink).
- **Q-IMB-1 (Q, MEDIUM):** RG-055b is RESERVE (Crafting pack). Promoting to v1 requires a
  register update first (D-047). **Resolution:** keep RESERVE until v1 ships; design in L1 now
  but do not enter v1 code.

## 2. Durability and break (RG-018) — optional module

- **C-DUR-1 (C, CRITICAL):** L1 I-08 still reads "no durability, no item degradation" while
  GAP-REGISTER §8.2 and §13.1 #12 say durability ships as an optional module. Internal
  contradiction in the spec stack. **Resolution:** amend L1 I-08 text to "no durability in
  core; optional module ships via EquipmentDurabilitySystem NG patterns" (P2-style doc fix).
- **C-DUR-2 (C, HIGH):** D-023 already uses `SetItemHealthPercent` as the tempering channel
  for dynamic item mutation. Durability also wants item health. Two consumers of one channel
  with no precedence rule. **Resolution:** decide ownership — durability owns health percent;
  tempering must move to a different channel or be disabled when durability module is on.
- **A-DUR-1 (A, CRITICAL):** Assumed EquipmentDurabilitySystem NG (109899) patterns transfer
  to our use. Its reverse-engineering is unverified (audit §2). No proof it uses the same
  channel we need. **Resolution:** Phase-0-style proof spike before design finalization; if
  unproven, disable path (fail-closed, AGENTS §3.1 rule 9).
- **A-DUR-2 (A, HIGH):** Assumed AVG/AV not needed; durability state tracked how? If per-item
  health percent is engine-native, it persists in the save natively — but our co-save is
  authoritative (D-028). Double source of truth. **Resolution:** decide whether durability
  state lives in ItemMetadata (co-save) and is re-applied to health percent on load, or
  engine-native only (uninstall-safe but lossy).
- **E-DUR-1 (E, HIGH):** Item breaks mid-combat — what is the player-visible consequence?
  (Disabled? Lost? Quest-item safety?) Undefined. If "disabled", the item still occupies an
  inventory slot and may still be equipped with 0 effect — confusing. **Resolution:** define
  broken = cannot be equipped, must be repaired at forge; quest items immune (flag in metadata).
- **E-DUR-2 (E, MEDIUM):** Repair cost and success chance undefined; if repair fails, materials
  consumed with no change (like C-05 upgrade failure) — needs explicit rule. **Resolution:**
  mirror C-05 failure semantics (materials consumed, item unchanged) unless a "break on
  failed repair" rule is chosen deliberately.
- **S-DUR-1 (S, HIGH):** Per-item durability in ItemMetadata = new field + schema bump N-01.
  Existing saves have no durability field; migration must default to "full durability" or
  "module disabled". **Resolution:** migration defaults durability to full when module enabled,
  or skips the field when module disabled (optional module contract).
- **P-DUR-1 (P, MEDIUM):** Per-hit degradation hook not in the 11-hook map (L4 §2). Either
  degradation is event-driven (on damage taken, needs a damage hook — none exists) or
  per-swing (needs input/attack hook). **Resolution:** either add a hook (changes D-050) or
  make degradation probabilistic on ActorDeath/kill events only (coarser but fits existing
  hooks).
- **M-DUR-1 (M, HIGH):** Compatibility with vanilla repair, other durability mods
  (EquipmentDurabilitySystem NG itself), OWL, and our dynamic name/enchant (D-023). Two
  durability systems writing health percent = last writer wins, invisible corruption.
  **Resolution:** detect other durability mods at load (Compatibility module, P4.1) and
  auto-disable our module with a log when a conflict is detected.
- **Q-DUR-1 (Q, LOW):** FOMOD module matrix (P5.2) lacks a Durability row. **Resolution:** add
  "Durability (optional)" row to ModuleConfig.xml and L5 matrix.

## 3. Passive / skill trees via Custom Skills Framework (RG-002, RG-003)

- **A-CSF-1 (A, CRITICAL):** Assumed CSF config schema is known. av-doc-notes §6 flags it
  UNVERIFIED. Without the schema, skill IDs, node layout, and XP source cannot be designed.
  **Resolution:** read upstream CSF docs/schema before any design; record findings in
  av-doc-notes.
- **A-CSF-2 (A, HIGH):** Assumed CSF persists skill state in a way compatible with our co-save
  (D-028: co-save is authoritative). If CSF owns its own persistence, there are two sources
  of truth and uninstall leaves orphaned CSF state (N-04). **Resolution:** decide ownership —
  our co-save section mirrors CSF state, or CSF is the sole owner and we accept the orphan risk
  documented in N-04.
- **C-CSF-1 (C, HIGH):** Progression module already owns 18 skill lines x 3 ranks (54 nodes)
  with rank gates (Skyrim skill 20/50/75 + attribute 10/20/30). CSF adds *new* skill lines
  beyond the 18. Does CSF skill XP consume the existing skill-point pool or a new one? If new,
  the "one skill point per level" economy (P-06) breaks. **Resolution:** CSF skills consume
  the same skill-point pool, with their own gates; document in P-06 amendment.
- **C-CSF-2 (C, MEDIUM):** D-006 says no vanilla AV repurposing; CSF skills may require new
  AVs via AVG (RG-024b). If CSF uses its own AVs, that's fine; if it repurposes ours, conflict.
  **Resolution:** verify CSF's AV usage; prefer CSF's own AVs.
- **E-CSF-1 (E, MEDIUM):** CSF absent (optional dependency) — graceful disable required.
  Current code has no optional-dep contract for CSF. **Resolution:** add a CSF adapter module
  with a presence check at DataLoaded; disable CSF features and log if absent.
- **E-CSF-2 (E, MEDIUM):** What happens to CSF skill state when the mod is uninstalled?
  CSF skills may remain as orphaned records in the save. **Resolution:** document as known
  limitation (N-10 uninstall matrix) and provide a "reset CSF skills" debug command.
- **M-CSF-1 (M, HIGH):** Conflict with other CSF-using mods (skill ID collisions, menu
  conflicts). CSF skill IDs must be namespaced (`Runefall_*`). **Resolution:** prefix all
  skill IDs and validate uniqueness at load.
- **P-CSF-1 (P, LOW):** CSF menu rendering on MenuRender budget — if CSF opens its own menu,
  our MenuApi snapshot must not block. **Resolution:** keep MenuApi reads non-blocking.
- **Q-CSF-1 (Q, MEDIUM):** RG-002/RG-003 are RESERVE (Progression pack). Promoting requires
  register update (D-047). **Resolution:** keep RESERVE; design first.

## 4. Ailment / damage-type / penetration framework (RG-004/005/006/023)

- **C-AIL-1 (C, CRITICAL):** The 11-hook map (L4 §2) has no damage/hit hook. Ailments need
  either (a) MGEF-self-expiring effects (no hook, but stacking/cleanse logic must live in
  MGEF conditions — unproven) or (b) a new hook, which changes D-050 (accepted hook map) and
  the performance budget. **Resolution:** decide MGEF-self-expiring first; if insufficient,
  propose a 12th hook amendment to D-050 with a budget.
- **A-AIL-1 (A, HIGH):** Assumed Elemental Reactions Framework (163378) API transfers. Its
  exact API is unverified (av-doc-notes). Building on an unverified framework risks a rewrite.
  **Resolution:** treat as pattern reference only (D-012); implement on our own MGEF + co-save.
- **A-AIL-2 (A, HIGH):** Assumed new AVs via AVG for resistance/penetration stats. AVG config
  path is UNVERIFIED (av-doc-notes §6). **Resolution:** verify AVG path on target MO2 list
  before design.
- **E-AIL-1 (E, MEDIUM):** Ailment interactions undefined (freeze cancels burn? cleanse
  sources? stacking caps?). Without rules, balance is undefined. **Resolution:** write an L1
  ailment registry with stacking caps, durations, cleanse sources, and interaction matrix.
- **E-AIL-2 (E, MEDIUM):** What happens when a variant enemy (enemy variant system) is also
  ailment-immune (boss)? Undefined interaction. **Resolution:** define immunity flags per
  actor rank; bosses immune to stun/freeze, vulnerable to others.
- **S-AIL-1 (S, MEDIUM):** Ailment state on actors — MGEF-driven self-expiring (no co-save
  needed) vs tracked in co-save (survives load). If MGEF, ailments reset on load (acceptable?).
  **Resolution:** MGEF-self-expiring; document that ailments do not persist across saves.
- **M-AIL-1 (M, HIGH):** Compatibility with vanilla resistances and other magic/ailment mods.
  Our penetration stat must not override vanilla resistances to negative values. **Resolution:**
  additive penetration with a floor (resistance cannot go below 0 or above 85%).
- **P-AIL-1 (P, MEDIUM):** If ailments need per-hit processing, that's per-hook overhead on
  ActorDeath/ContainerGeneration — must fit 0.5 ms budget. **Resolution:** keep ailment
  application in MGEF (engine handles timing); DLL only sets the MGEF, no per-frame work.
- **Q-AIL-1 (Q, LOW):** RG-004/005/006/023 are RESERVE (Combat pack, ship later). **Resolution:**
  keep deferred; do not start design until v1 ships.

## 5. Rune XP and Rune Level (RG-024b) — real AVs via AVG

- **A-RXP-1 (A, CRITICAL):** Assumed AVG works on 1.6.1170 target. AVG is installed in
  rpg-mechanism but its config path is UNVERIFIED (av-doc-notes §6). **Resolution:** verify
  AVG config path and Functional get/set pair on the target MO2 list before any design.
- **A-RXP-2 (A, HIGH):** Assumed AVG Adaptive AVs persist via the game's own AV system.
  If AVG AVs are not saved by Skyrim's save system, Rune XP/Level is lost on save/load.
  **Resolution:** test save/load round-trip with an AVG AV in Phase 0; if not persisted,
  mirror in co-save (D-028) — but that's a second source of truth.
- **C-RXP-1 (C, HIGH):** D-006 says no vanilla AV repurposing. AVG AVs are *custom*, so
  technically compliant — but the distinction must be documented. If Rune XP/Level uses AVG AVs
  that look like vanilla AVs to other mods, confusion. **Resolution:** document that AVG AVs
  are custom (not vanilla repurposing) and namespace them `Runefall_*`.
- **C-RXP-2 (C, MEDIUM):** Rune XP/Level as a *new* progression axis alongside character
  level (P-02) and RF Tier (T-06). Three progression axes with overlapping rewards risk
  confusion. **Resolution:** define what Rune Level grants that the other two don't (unique
  perks? points?) and ensure no overlap with P-07 rune-bought points.
- **E-RXP-1 (E, MEDIUM):** Rune XP earned from what? (kills? loot? crafting?) Undefined.
  If from kills, it double-counts with existing rune currency drops (R-03). **Resolution:**
  define earn sources explicitly; avoid double-counting with R-03.
- **E-RXP-2 (E, MEDIUM):** Death/revert with XP gains — if XP is granted on kill and the
  player dies before saving, does XP revert? If AVG AVs are engine-persisted, they may not
  revert (save-scumming). **Resolution:** grant XP only on save, or track pending XP in co-save
  and commit on save.
- **S-RXP-1 (S, HIGH):** If AVG AVs persist natively, uninstalling Runefall leaves orphaned
  AVs in the save (no cleanup). If mirrored in co-save, migration needed. **Resolution:**
  document orphan risk (N-10); provide cleanup on uninstall if feasible.
- **M-RXP-1 (M, MEDIUM):** AVG as hard dependency vs optional. If hard, users without AVG
  cannot play. If optional, Rune XP/Level silently disabled. **Resolution:** decide dependency
  tier; recommend optional with graceful disable.
- **P-RXP-1 (P, LOW):** AVG AV writes on every XP gain — if frequent, per-hook overhead.
  **Resolution:** batch XP writes; only write on level-up or save.

## 6. Rune progression system (RG-024c)

- **C-RPR-1 (C, HIGH):** Depends on RG-024b (Rune XP/Level). Cannot be designed until
  RG-024b is settled (audit §6). **Resolution:** block design until RG-024b verified.
- **C-RPR-2 (C, MEDIUM):** Does Rune progression *replace* or *supplement* rune-bought
  points (P-07)? If both exist, the economy has two rune-spend paths for the same goal.
  **Resolution:** decide — either Rune progression grants points (replaces P-07) or grants
  separate perks (supplements); document in P-07 amendment.
- **E-RPR-1 (E, MEDIUM):** Respec interaction — if Rune Level grants perks, can they be
  respecced? At what cost? **Resolution:** define respec rules; mirror P-08 cost structure.
- **S-RPR-1 (S, MEDIUM):** Progression state section (co-save section 1) must extend for Rune
  Level grants. Schema bump. **Resolution:** extend section 1 with Rune Level + granted perks.
- **Q-RPR-1 (Q, LOW):** RG-024c is RESERVE (Progression pack). **Resolution:** keep deferred.

## 7. Enemy variant system (RG-125b/c/d)

- **C-VAR-1 (C, CRITICAL):** D-025: no permanent perk grants to generic NPCs; variants must
  re-derive at spawn. If variant assignment is persisted in co-save and not re-derived,
  save bloat and orphaned state. **Resolution:** variants are re-derived at spawn from a
  deterministic seed (spawn position + cell + base NPC EditorID); no co-save state.
- **C-VAR-2 (C, HIGH):** D-005: core must not depend on SkyPatcher. Fortunate Foes uses
  SPID/SkyPatcher for perk/spell packages. Runefall must use its own PERK records + AddPerk
  at spawn (TierEngine). **Resolution:** implement variant perk packages via TierEngine at
  ActorSpawn hook; no SkyPatcher dependency.
- **A-VAR-1 (A, HIGH):** Assumed our own PERK records + AddPerk suffice for variant
  difficulty. Dynamic Perk Framework (189660) proves runtime perk granting but is an optional
  dep. If our own records don't cover all needed effects, variants are limited. **Resolution:**
  prototype 3 variant packages with our own PERK records; if insufficient, add Dynamic Perk
  Framework as optional dep.
- **E-VAR-1 (E, HIGH):** Variant enemy is also a boss — does the variant stack with boss
  rank (T-01)? If a rank-4 boss gets a variant, is it rank-4 + variant multipliers (multiplicative
  soup)? **Resolution:** variants replace rank multipliers for that enemy, not stack; or variants
  only apply to generic NPCs (rank 1-3), bosses use their own templates (D-026).
- **E-VAR-2 (E, MEDIUM):** Idempotency (D-025) — double application on cell reset/reload.
  ActorSpawn hook fires on load; if not guarded, variant applied twice. **Resolution:** use
  exclusive keywords (D-004 rule 4) to mark variant-applied actors; skip if keyword present.
- **E-VAR-3 (E, MEDIUM):** Naming (RG-125d) — per-instance actor names are not a reliable
  engine feature (L2 §4 notes UI badge fallback). If we can't set display names reliably,
  variants are indistinguishable. **Resolution:** use UI badge in Item Detail / name prefix via
  SetDisplayName where proven; fallback to a floating badge via TrueHUD (optional).
- **S-VAR-1 (S, LOW):** No co-save state needed if re-derived (C-VAR-1). But reward scaling
  (RG-125c) must be consistent — if variant tier is re-derived, loot must use the same
  derivation. **Resolution:** loot hook reads the variant keyword from the actor, not re-derives.
- **M-VAR-1 (M, HIGH):** Compatibility with enemy overhauls and boss mods. Variant perk
  packages may conflict with overhaul perks (double damage, conflicting auras). **Resolution:**
  caps on variant multipliers (I-09: global caps and INI multipliers); soft-detect overhauls
  and disable variants if conflict detected.
- **P-VAR-1 (P, MEDIUM):** ActorSpawn hook budget 0.2 ms per actor. Variant package
  application (perk grants, stat multipliers, name change) must fit. If variant count is high
  (N variants per base NPC), the package lookup must be O(1) (EditorID-keyed map). **Resolution:**
  pre-build variant package map at DataLoaded; ActorSpawn does a map lookup + keyword check.
- **Q-VAR-1 (Q, MEDIUM):** RG-125b/c/d are RESERVE (Enemy pack). "No world changes"
  constraint (§5 of GAP-REGISTER) means variants modify existing NPCs only — no new cells.
  **Resolution:** enforce in design; no new worldspaces/cells.

## 8. Last Epoch-style random roll (destroy → create new item)

- **C-LE-1 (C, HIGH):** L4 §4 says ItemRegistry is pruned only when items are destroyed.
  A "destroy → create new" flow creates a *new* registry entry but must prune the old one.
  If the old entry is not pruned, registry bloats and co-save grows unbounded. **Resolution:**
  specify atomic transaction: prune old ID, allocate new ID, in one step; rollback on failure.
- **C-LE-2 (C, HIGH):** If the new item is created while the old is equipped, destroying the
  equipped item mid-flow may crash or leave the player with no weapon. **Resolution:** require
  unequip before roll, or implement a swap (create new, then replace in slot, then destroy old)
  with a rollback on failure.
- **E-LE-1 (E, HIGH):** Transaction safety — if creation fails mid-flow, is the original item
  lost? Without atomicity, a crash mid-flow loses the item permanently. **Resolution:** use
  the task interface (SKSE::GetTaskInterface()->AddTask) to make the transaction atomic on
  the main thread; validate before mutating.
- **E-LE-2 (E, MEDIUM):** Does the new item's ilvl/rarity change? If yes, it's not a "reroll"
  but a "re-roll from scratch" with different economics. **Resolution:** define: destroy→create
  keeps rarity and ilvl (like C-04 reroll) but mints a new instance; cost is the same as C-04
  full reroll. Document as a UX variant, not a new transaction.
- **S-LE-1 (S, MEDIUM):** Registry ID handling — if the new item gets a new ID, all references
  to the old ID (equipped slot, quest tracking) must update. **Resolution:** if implemented as
  a UI flow over the existing RerollItem (audit recommendation), no new ID; if new transaction,
  specify reference updates.
- **M-LE-1 (M, LOW):** Other mods tracking item FormIDs — if we destroy and create, the FormID
  changes (new base form). Other mods holding references break. **Resolution:** prefer in-place
  reroll (no FormID change); document that destroy→create changes identity.
- **P-LE-1 (P, LOW):** If implemented as a new transaction, extra co-save write on every roll.
  **Resolution:** batch; only write on MenuClose or save.

## 9. Monster Hunter material system (per-race/NPC loot)

- **C-MH-1 (C, HIGH):** Race-hunt materials are designed (19 families, L-11) but
  MaterialRegistry does not exist yet (FIX-PLAN P4.8: materials loaded but not consumed). No
  material tracking in co-save. This blocks even the *designed* race-hunt materials, let alone
  a broader per-NPC system. **Resolution:** close P4.8 first (create MaterialRegistry + co-save
  section) before any MH-material work.
- **A-MH-1 (A, MEDIUM):** Assumed "MH material system" means per-NPC material tables. The
  scope is ambiguous (audit §9): is it (a) the already-designed race-hunt materials (then
  COMPLETE pending implementation) or (b) a broader per-NPC system? **Resolution:** decide scope
  before data work; recommend (a) for v1, (b) as RESERVE.
- **E-MH-1 (E, MEDIUM):** Duplicate/overlapping materials across races — if two races drop the
  same material, hunting one race is sufficient, reducing hunting diversity. **Resolution:**
  L3 gate #7 requires every hunt material has a use; add a uniqueness gate (no two races share
  a primary material).
- **E-MH-2 (E, MEDIUM):** Dead materials — if a material has no recipe/use, it's clutter.
  L3 gate #7 already requires a use; enforce in validator. **Resolution:** P3.6 cross-reference
  validation must include material→recipe/use linkage.
- **S-MH-1 (S, LOW):** Material counts in co-save — if MaterialRegistry is added, schema bump.
  **Resolution:** add material counts to co-save section 4 (hunt state) or a new section 10.
- **M-MH-1 (M, MEDIUM):** Compatibility with other loot mods — if another mod also adds
  per-NPC loot, our loot is additive (D-008) but may double-count. **Resolution:** additive
  default; detect other loot mods and log.
- **Q-MH-1 (Q, MEDIUM):** RG-203/207/215/225/232 are RESERVE (Unique/Crafting packs).
  **Resolution:** keep RESERVE; do not promote to v1 without register update.

## 10. Tier upgrade with tier materials (MH-style)

- **C-TU-1 (C, CRITICAL):** Ambiguous whether "tier upgrade" is C-05 (rarity upgrade, already
  designed: 60% success, materials consumed on failure) or a *new* MH-style quality/ilvl upgrade
  axis. If a new axis is invented without a decision, we get two upgrade systems. **Resolution:**
  decide explicitly; if new, write an L1 subsection with rule + defaults + failure mode + L3
  schema; if C-05, remove from "new features" list and track as implementation (FIX-PLAN P4.x).
- **A-TU-1 (A, MEDIUM):** Assumed "MH-style tier upgrade via DLL" implies a new system.
  The phrase is vague; no design exists. **Resolution:** clarify scope with the user before any
  data work.
- **E-TU-1 (E, MEDIUM):** If a new axis, failure mode undefined (downgrade? break? materials
  lost?). **Resolution:** define failure semantics; recommend mirror C-05 (materials consumed,
  item unchanged) for consistency.
- **E-TU-2 (E, MEDIUM):** If upgrade changes ilvl/affix magnitudes, does it re-roll magnitudes
  or scale them? Affects ItemRegistry metadata and co-save. **Resolution:** scale (not re-roll)
  to preserve player investment; document in L1.
- **S-TU-1 (S, LOW):** Item metadata (rarity/ilvl) update — handled by ItemRegistry if
  in-place. **Resolution:** in-place update; no new registry ID.

## 11. Gambling and gacha

- **C-GAM-1 (C, HIGH):** E-00 says no clock-based pacing. Pity counters must be event-count
  based, not time-based. If gacha uses daily/weekly limits, it violates E-00. **Resolution:**
  pity is event-count based (kills/rolls), no time gates.
- **C-GAM-2 (C, MEDIUM):** L-08 anti-abuse (container roll-once, boss drop persist at death)
  must apply to gambling rolls to prevent save-scumming. Undefined. **Resolution:** gambling
  rolls persist per-session (not per-save) to prevent reload farming; or use a deterministic
  seed from save state.
- **E-GAM-1 (E, HIGH):** Save-scumming around gambling rolls — if the player can reload and
  re-roll, the economy breaks (E-05 anti-inflation). **Resolution:** persist roll results in
  co-save; re-loading restores the same result.
- **E-GAM-2 (E, MEDIUM):** Economy inflation (E-05) — gambling as a rune sink must return
  less value than input (like salvage C-02 returns 10-25%). If gambling returns equal or
  better value, it's an exploit. **Resolution:** gambling expected value < 1.0 (e.g., 0.85)
  so it's a net sink.
- **H-GAM-1 (H, HIGH):** Ethics of gacha in a single-player mod. Gacha mechanics (rates/pity)
  in a free mod are not monetization, but they can still feel exploitative or addictive.
  Community backlash risk on Nexus. **Resolution:** decide scope — if gacha ships, make it
  cosmetic-only or low-stakes (materials/runes, not unique items); disclose rates transparently;
  consider whether it ships at all (still RESERVE).
- **H-GAM-2 (H, MEDIUM):** If gambling rewards unique items, it undermines the hunting loop
  (D-046: hunting is the primary v1 progression). **Resolution:** gambling rewards materials
  or runes only; unique items come from hunting/bosses.
- **M-GAM-1 (M, MEDIUM):** SKSE Menu Framework custom menu for gambling — if the framework
  fails (P5.5 test env issues), gambling UI breaks. **Resolution:** MCM Helper fallback (D-022).
- **Q-GAM-1 (Q, MEDIUM):** RG-082/RG-085 are RESERVE (Economy pack, F-17). **Resolution:**
  keep RESERVE; decide scope and ethics before any design.

## 12. Material salvage via new menu + DLL (COMPLETE, but verify)

- **C-SAL-1 (C, MEDIUM):** Audit says COMPLETE, but MaterialRegistry does not exist (P4.8).
  Design is complete; implementation is not. Salvage yield cannot be stored without it.
  **Resolution:** track as implementation work (P4.8); no new design needed.
- **E-SAL-1 (E, MEDIUM):** Salvage destroys the item with no undo (C-02). If the player
  salvages by mistake, the item is gone. No confirmation dialog designed. **Resolution:** add
  a confirmation step in the Rune Forge page (L4 §7).
- **S-SAL-1 (S, LOW):** ItemRegistry pruned on destroy (L4 §4) — salvage must prune. If
  salvage doesn't prune, registry bloats. **Resolution:** verify prune is called in SalvageItem
  transaction (P4.4).
- **M-SAL-1 (M, LOW):** Compatibility with item-tracking mods — salvage changes item count;
  other mods may hold references. **Resolution:** salvage is local to our registry; other mods
  tracking FormIDs are unaffected (base form remains).

---

## Summary

### Challenges by type

| Type | Count | Critical | High | Medium | Low |
|---|---|---|---|---|---|
| Assumption (A) | 12 | 3 | 6 | 3 | 0 |
| Contradiction (C) | 14 | 4 | 6 | 4 | 0 |
| Edge case (E) | 22 | 0 | 7 | 13 | 2 |
| Save compat (S) | 11 | 0 | 3 | 6 | 2 |
| Performance (P) | 7 | 0 | 0 | 4 | 3 |
| Compatibility (M) | 10 | 0 | 3 | 5 | 2 |
| Scope creep (Q) | 9 | 0 | 0 | 6 | 3 |
| Ethics (H) | 2 | 0 | 1 | 1 | 0 |
| **Total** | **87** | **7** | **26** | **42** | **12** |

### Top-5 most dangerous assumptions (could kill a feature if wrong)

1. **A-DUR-1 — EquipmentDurabilitySystem NG patterns transfer to our use.** If unproven, the
   entire durability module has no engine channel and must be disabled (fail-closed). This is
   the highest-risk assumption because it gates a feature the user has explicitly directed to
   ship (§8.2). Mitigation: Phase-0 proof spike before any design; disable path if unproven.

2. **A-IMB-1 — Imbue can be applied without mutating base records and without colliding with
   the socketed-rune channel (I-05) or the reroll/upgrade channel (D-023).** If no clean
   channel exists, imbue is infeasible as designed. Mitigation: prove one application channel
   (dynamic enchantment vs extra data vs new MGEF) in a spike.

3. **A-CSF-1 — CSF config schema is known and compatible.** If the schema is incompatible or
   undocumented, the entire passive/skill tree feature cannot be designed. Mitigation: read
   upstream CSF docs first; this is a documentation task, not a code task.

4. **A-RXP-1 — AVG works on the 1.6.1170 target and its AVs persist.** If AVG AVs are not
   saved by Skyrim's save system, Rune XP/Level is lost on save/load, killing the feature.
   Mitigation: save/load round-trip test in Phase 0 before design.

5. **A-AIL-1 — Elemental Reactions Framework API transfers.** If its API is unusable, the
   ailment framework must be built from scratch on MGEF + co-save, significantly increasing
   effort. Mitigation: treat as pattern reference only (D-012); implement on our own MGEF.

### Cross-cutting risks

- **Hook map completeness:** The 11-hook map (L4 §2, D-050) lacks a damage/hit hook. Ailments
  and durability degradation both need one. Either extend the hook map (amending D-050) or
  constrain both features to MGEF-self-expiring / event-driven degradation. This is an
  architectural decision that affects L4 §2 and the performance budget.
- **Co-save schema churn:** Imbue, durability, ailment state, Rune XP/Level, and MaterialRegistry
  all require ItemMetadata or progression section changes. Each is an N-01 migration with a
  rollback path. Batching these into one schema bump is cheaper than six separate bumps.
- **Scope lock (D-047):** All 7 STUB features are RESERVE. Promoting any to v1 requires a
  register update first. The red-team recommends keeping all new features RESERVE until v1
  ships, then designing promoted features in L1→L2→L3→L4 order (D-013).

RUNTIME STATUS: assistant-claimed (read-only audit, no runtime verification)
SSEEDIT/CK: not launched
UNRESOLVED: hook map extension decision; scope promotion decisions; AVG/CSF/EquipmentDurability
verification; imbue application channel; durability repair semantics.
