# DESIGN-GAMBLING-GACHA — Economy feature cluster (gambling + gacha)

Status: design draft, 2026-10-27. Companion to SPECS/AUDIT-NEW-FEATURES-DESIGN-DEPTH.md and
SPECS/REDTEAM-NEW-FEATURES.md. Resolves two interconnected features: gambling (spend currency
for a random item of a type, RG-082) and gacha (random reward from a pool with a pity system,
RG-085), plus the rune gambling vendor (F-17). Layered per D-013: each feature is given L1 rule
+ numbers, L2 engine mapping, L3 data schema, L4 code module, compatibility notes, test entry,
and open questions. Mechanic IDs stay traceable to L1-GAMEPLAY-MODEL.md; new IDs are introduced
here (E-12, E-13) and must be folded into L1 in a follow-up doc pass, same convention as the
sibling DESIGN-*.md files.

Scope note (D-047): both features are currently RESERVE (Economy pack, F-17). This document is
design only; promotion to v1 (or to a 1.4 Economy pack) requires a register update first. If
promoted, gambling ships as an optional FOMOD module "Gambling & Gacha" (default off in core),
mirroring the Durability module pattern (DESIGN-ITEM-CRAFTING.md §2). Core economy (E-01..E-07)
never depends on it; gambling is an extra sink, not a required one. D-046 (hunting primacy) is
preserved: gambling rewards are never unique items.

---

## 0. Cross-cutting resolutions

### 0.1 Design decisions (resolved, answering the task brief)

| # | Question | Decision | Rationale |
|---|---|---|---|
| 1 | v1 feature or reserved? | **RESERVED** (Economy pack). Design now; cheap hooks only in v1. | D-047: reserved features never enter v1 code. Economy sinks are only meaningful once v1 faucet data exists (F-17). Promotion requires a register update. |
| 2 | Currency: runes, separate token, or gold? | **Runes (R-01/R-02), spent in rune units.** No separate token, no gold. | A second currency (gambling token) adds a faucet + inventory clutter and breaks the "one resource" rule (D-014). Gold is not Runefall's currency (GAME-DESIGN §10: "runes are a separate currency, no gold replacement in v1"). Runes keep the economy legible and make gambling a real rune sink (E-07). |
| 3 | Pity system? | **Yes, for gacha only (E-13). Event-count based, transparent, never clock-based (E-00).** Gambling (E-12, kadala-style) has no pity. | E-00 forbids clock pacing; L-06 already defines event-count pity for loot. Kadala (D3/D4) has no pity by design — you reroll. Gacha (banner-style) is where pity belongs. Keeping them distinct is the cleanest answer to Q3. |
| 4 | Gacha included at all, or gambling only? | **Include gacha, but only as a low-stakes, transparent, opt-in, pity-protected layer with an independent toggle (default off).** Gambling (E-12) can ship without gacha (`bEnableGacha = 0`). | H-GAM-1/H-GAM-2: gacha in a free single-player mod is not monetization, but exploitation/backlash risk is real. Mitigation: no real money, no FOMO, transparent rates, pity, low stakes (never uniques), opt-in. If the community rejects it, gacha is one INI flip away from gone and gambling remains. |
| 5 | UI: SKSE Menu Framework page or MCM? | **SKSE Menu Framework page (D-022), MCM Helper fallback (D-010/D-022).** | D-022 is the accepted v1 UI framework. MCM is the fallback if the framework fails in Phase 0 (P5.5). |
| 6 | Should gambling be a sink, not a faucet? | **Sink. Expected economic value (salvage return) ≤ 0.85 × cost.** | E-07 trade-off and E-05 anti-inflation. Gambling converts surplus runes into items whose salvage value is a fraction of cost; the gap is the sink. |

### 0.2 D-006 compliance: no vanilla AV repurposing

Gambling/gacha introduces no new actor values. Currency is the rune wallet (section 2, R-02 ladder) and the
MISC rune item (D-007). Pity counters and roll counts are co-save integers, not AVs. No vanilla AV is
written. No AVG dependency. D-006 is satisfied trivially; no AVG mirror is needed (unlike DESIGN-PROGRESSION.md §0.1).

### 0.3 D-027 compliance: no base-record mutation

All gambling/gacha state obeys D-027: no vanilla or shared records are mutated.

- **Items minted by gambling are new instances**, not record edits. They are generated through the
  standard loot pipeline (L-03) with gambling-specific weights, then mutated per instance via the dynamic
  item channel (D-023: CreateEnchantment/SetEnchantment, SetDisplayName, SetItemHealthPercent) plus a
  co-save registry entry (ItemRegistry, L4 §4). No base forms are edited; no leveled-list or container
  records are touched.
- **Pool membership is data-driven**: the pool is computed at roll time from the ItemRegistry + current
  RF Tier + selected category. No pool records, no record per variant (D-029).
- **No vendor NPC, no cell edits, no leveled-list edits.** The entry point is a menu page (UI only),
  which avoids vendor-record ownership questions and maximizes compatibility (see §Compatibility).
- **Pity/rates are data + co-save**, never records.

### 0.4 Co-save schema: reuse v2, add section 11 (resolves N-01 churn)

The v2 schema bump defined in DESIGN-ITEM-CRAFTING.md §8 (batched ItemMetadata fields + section 10
materials) is reused. No further bump beyond v2 for this cluster; all new fields are additive with
defaults. One new section is added:

- **Section 11 "gambling"** (see §5): pity counter, roll counts, roll nonce, module snapshot.
- No new ItemMetadata fields: gambled items use the existing metadata shape (rarity, enhancements, ilvl,
  sockets, tags). Tags already carry loot source (N-06), so a gambled item is tagged `source = gambling`.

Migration (v2 → v2): section 11 defaults empty (pity 0, counts 0, nonce 0). Downgrades refused with a
clear message (D-052). If gambling module is absent, section 11 is ignored.

### 0.5 Hook map: no new hooks (resolves D-050)

The 11-hook map (D-050) is unchanged. Gambling/gacha is purely on-demand:

- Roll happens on a user action (button click in the menu page), dispatched via the Papyrus bridge or a
  native callback, executed on the game thread through the SKSE task interface (AGENTS §3.1 rule 4).
- No per-frame work. No polling. No Papyrus update loops. MenuRender draws from a MenuApi snapshot.
- MenuOpen (forge/menu) handles page entry and state snapshot (0.2 ms budget). Roll cost is paid on
  action, not on open.

### 0.6 Performance: on-demand only (resolves E-00, N-09)

- Roll (LootEngine roll + dynamic item creation + inventory add + pity update): target < 5 ms on user
  action. Measured and recorded in VALIDATION.md.
- Pity update + co-save serialization of section 11: < 0.1 ms (in-memory; disk write happens at
  SaveWrite, 5 ms budget).
- MenuRender: draws from snapshot; no CSV/INI reads; no roll computation. UI budget applies.
- No per-kill, per-container, or per-frame gambling work. Gambling never runs unless the player opens
  the page and clicks.

### 0.7 Dependency policy (resolves D-056)

- **SKSE Menu Framework (D-022)** is the UI dependency (already required by v1 UI). MCM Helper fallback.
- **No new hard dependencies.** Gambling reuses LootEngine, ItemRegistry, RuneForge, DataStore, SaveSystem,
  MenuApi, PapyrusBridge — all existing modules. No AVG, no CSF, no JContainers, no PO3.
- **Optional module**: if promoted, ships as FOMOD module "Gambling & Gacha" (default off). Core never
  depends on it (D-047, D-056).

---

## 1. Gambling (E-12) — kadala-style, mechanic ID E-12

**Status:** RESERVE (Economy pack, F-17). Design only; no v1 code until register update (D-047).
Promotion requires D-047 register update + Phase 0 probe (item creation path, already proven by loot).

### L1 — Rule and numbers

- **E-12.1 Rule.** At a forge or via the gambling menu page, the player spends runes to receive a
  single random Runefall item of a chosen category and ilvl band matching the current RF Tier. The
  item is generated through the standard loot pipeline (L-03) with gambling-specific rarity weights
  (E-12.4) and an ilvl clamp (E-12.5). The transaction is a **sink**: the expected economic value
  (rune-value returned if salvaged) of the reward is ≤ 0.85 × cost (E-GAM-2). No unique affixes
  (hunt/boss/quest) enter the pool (E-12.6, H-GAM-2, D-046).
- **E-12.2 Currency and cost.** Runes (R-01/R-02), spent in rune units. Cost table by RF Tier
  (encounter multiples per E-02; mid-encounter income per E-01):

  | RF Tier | Cost (rune units) | ≈ reference encounters |
  |---|---|---|
  | 0 | 8 | ~1.0 |
  | 1 | 15 | ~1.0 |
  | 2 | 30 | ~0.9 |
  | 3 | 65 | ~1.0 |
  | 4 | 130 | ~1.0 |
  | 5 | 260 | ~1.0 |

  Tunable via INI `[Gambling] CostMult` (default 1.0) scaling the whole table, and per-tier overrides
  `CostTier0..5`. Cost is paid in rune units converted from the wallet per R-02 (highest rank first
  for efficiency, per P4.14 guidance). No separate token, no gold.
- **E-12.3 Categories (player selects one per roll).** The player picks a category before rolling:
  - `weapon` — any weapon base form in the ItemRegistry.
  - `armor` — any armor slot.
  - `accessory` — rings/amulets if present in the registry; if none, the category is disabled in UI.
  - `material_pack` — 3-5 material units of the area's tier, weighted toward common/refined families.
  - `rune_pack` — 2-4 rune units of rank ≤ current RF Tier.
  Default: all enabled. INI `[Gambling] EnableCategory_*` (bool) can disable a category.
  The pool is computed at roll time from the ItemRegistry + tier + category, so packs that add items
  automatically expand the gambling pool (no separate pool data, D-029).
- **E-12.4 Rarity weights (gambling, percent, six bands):** Common 40, Refined 30, Exquisite 18,
  Radiant 8, Mythic 3, Runeforged 1. (Compared to normal drops L-05: 55/25/11/6/2.5/0.5 — gambling
  is slightly more generous in high bands, but the sink property holds via the salvage gap: salvage
  returns only 10-25% of item rune-value (C-02), so expected economic return ≈ 0.15 × cost even at
  Mythic. Gameplay power is bounded by ilvl clamp and the no-uniques rule.) Tunable via INI
  `[Gambling] RarityWeight_*`.
- **E-12.5 ilvl clamp.** Item ilvl is rolled within the current RF Tier band (L-02), clamped to the
  player's tier. No lucky low-level endgame gear (L-02 rule).
- **E-12.6 No unique affixes (H-GAM-2, D-046).** Hunt (L-11), boss (D-041) and quest (D-041) unique
  affixes are excluded from the gambling pool. Gambling items carry ordinary affixes only. Unique
  items come from hunting/bosses/quests only; gambling never substitutes for hunting.
- **E-12.7 Anti-abuse (C-GAM-2, E-GAM-1, L-08).** The roll is generated on the game thread and
  applied immediately: item added to inventory, runes deducted, pity N/A (gambling has no pity),
  `rollNonce` incremented. The pity/rates are not involved (gambling is kadala-style: pure random,
  no pity). Transaction persistence follows the same rules as all forge transactions: on revert
  (death without save) the transaction reverts with inventory and wallet. Save-scumming mitigation:
  `rollNonce` (u64, monotonic per roll attempt, section 11) is mixed into the RNG seed; on reload
  the nonce resets to the saved value, so the sequence differs only if the player saved. Documented
  as a known limitation (L-08): gambling is save-scummable to the same degree as all forge
  transactions; full prevention requires per-roll disk writes, which is out of budget. Do not fake
  persistence.
- **E-12.8 Failure modes.** Insufficient runes → transaction rejected, no roll, no consumption,
  error code returned. Invalid/disabled category → rejected. Module disabled (INI) → command returns
  error code, no consumption. Item creation failure → rollback (runes refunded, no item), logged
  (E-LE-1 pattern from DESIGN-ITEM-CRAFTING.md §5). Pool empty for tier/category → rejected + log.
- **E-12.9 Gacha vs gambling distinction.** Gambling has no pity and no displayed "rates table"
  beyond the rarity weights (which are shown as "approximate odds"). Gacha (E-13) has pity and a
  full transparent rates table. This keeps the two mechanics legible and distinct (Q3).

### L2 — Engine mapping

- **Owner:** `GamblingModule` (new, optional, fail-closed, follows the DurabilityModule pattern from
  DESIGN-ITEM-CRAFTING.md §2) + `LootEngine` (roll functions `RollGambling(category, tier)`,
  `RollGacha(tier)`) + `RuneForge` (transaction: cost deduction, item creation, pity update,
  rollback) + `MenuApi` (snapshot) + `PapyrusBridge`.
- **Module gating:** `GamblingModule` registers with HookManager only when `bEnableGambling = 1`
  (INI) or the FOMOD module is installed. When disabled, all gambling/gacha commands return an error
  code and the menu page shows "Gambling disabled." No partial state.
- **Hook:** `MenuOpen` (forge/menu) for page entry and state snapshot (0.2 ms). No new hook. Roll
  happens on user action via the Papyrus bridge or native callback, executed on the game thread via
  `SKSE::GetTaskInterface()->AddTask` (AGENTS §3.1 rule 4); re-resolve and revalidate before
  mutating (no game mutation inside event dispatch).
- **Item creation path:** `LootEngine` rolls rarity/ilvl/affixes/sockets per L-03 with gambling
  weights; `DynamicEnchantment` (D-023) mints the instance; `ItemRegistry` allocates a registry entry
  tagged `source = gambling`; item is added to the player inventory via the same path as loot drops.
  On failure, rollback (runes refunded, no registry entry, no item).
- **Rune deduction:** `RuneForge` converts the cost to rune units and deducts from the wallet
  (section 2) per R-02, highest rank first. Validates sufficient balance before rolling.
- **Papyrus bridge (new):**
  - Query: `int GetGamblingRollCount()`, `string GetGamblingRates()` (compact "cat:common,refined,..."),
    `int GetGachaPity()`, `string GetGachaRates()`, `bool IsGamblingEnabled()`, `bool IsGachaEnabled()`.
  - Mutation: `int Gamble(int categoryIndex)` → 0 = ok, nonzero = error code (insufficient runes,
    disabled, invalid category, creation failure). `int GachaRoll()` → 0 = ok, nonzero = error.
  - Event: `Runefall_OnGambleRolled(rarity, itemForm)`, `Runefall_OnGachaRolled(rarity, itemForm)`.
- **UI surface:** new page "Gambling" (SKSE Menu Framework, D-022) with: category selector, cost
  display (current runes vs cost), roll button, result panel (item name/rarity/power), gacha rates
  table + pity progress (if gacha enabled), enable/disable status (read-only, INI-driven). MCM Helper
  fallback (D-010/D-022). Pages read MenuApi snapshots, never CSV/INI directly (N-07).
- **Uninstall:** gambling state is co-save only (section 11); removing the module leaves items as
  base forms (engine data remains, logic gone). Documented (N-10).

### L3 — Data schema

- **No new CSV for v1.** Costs, rates, pity thresholds and category toggles live in INI
  (`[Gambling]`, `[Gacha]` sections), matching the Rune XP pattern (DESIGN-PROGRESSION.md §1 L3).
  Pool is computed at roll time from the ItemRegistry + tier + category, so content packs that add
  items automatically expand the pool (D-029, no pool records).
- **Optional data file (if promoted to a pack):** `Data/Runefall/gambling.json` declaring rates/pity/
  costs as a versioned schema (D-032) for pack-level overrides. For v1 core, INI suffices.
  Validator enforces that rates sum to 100% and pity thresholds are positive integers.
- **FormID range:** none new. Gambling uses existing MISC (rune currency), item base forms, and
  MGEF templates already allocated (D-035). No new records.
- **Co-save:** section 11 (see §5). No new ItemMetadata fields.

### L4 — Code module

- **Owner:** `GamblingModule` (optional, fail-closed) + `LootEngine` (roll) + `RuneForge`
  (transaction) + `MenuApi` (snapshot) + `PapyrusBridge`. Follows the module-architecture table in
  L4-RUNTIME.md §1; add a row "GamblingModule | optional gambling/gacha rolls, pity, rates | LootEngine, RuneForge, SaveSystem".
- **Config (runefall.ini):**
  - `[Gambling]` `bEnableGambling` (bool, default 1 within module, 0 in core), `CostMult` (float, 1.0),
    `CostTier0..5` (int, table above), `EnableCategory_Weapon/Armor/Accessory/MaterialPack/RunePack`
    (bool, default 1), `RarityWeight_Common..Runeforged` (int, table above), `MaterialPackMin/Max`
    (int, 3/5), `RunePackMin/Max` (int, 2/4).
  - `[Gacha]` `bEnableGacha` (bool, default 0), `CostMult` (float, 1.0), `RarityWeight_*` (int,
    E-13.2 table), `PityRefined` (int, 10), `PityExquisite` (int, 40), `PityRadiant` (int, 100),
    `PityMythic` (int, 200).
- **Co-save:** section 11 (§5).
- **Papyrus bridge:** functions/events above.

### Compatibility

- **Additive, no vanilla record edits.** No vendor NPC, no cell edits, no leveled-list edits. The
  entry point is a menu page; maximum compatibility (GAME-DESIGN §9).
- **Rune wallet (section 2):** gambling spends runes through the existing wallet, consistent with
  R-02. No duplicate currency logic.
- **Other economy mods:** our rune currency is separate from gold and from other mods' currencies.
  No conflict. Soft-detect known gambling mods at DataLoaded and log (none in the current target
  list, Mantras of Mara).
- **OWL / DiabloRim:** gambling items are our Runefall items; their loot engines are unaffected.
  If DiabloRim is installed, gambling still works (uses our items, our wallet).
- **Hunting primacy (D-046):** gambling pool excludes unique affixes; unique items only from
  hunting/bosses/quests. Gambling is a sink for surplus runes, not a replacement for hunting.
  Hunting remains the primary v1 progression mechanism.
- **Economy balance:** gambling is an optional sink. Core economy (E-01..E-07) does not assume it.
  If enabled, it only removes runes from the economy (sink), never adds (no faucet). Safe to enable
  or disable without rebalancing core.
- **Uninstall (N-10):** removing the module leaves co-save section 11 ignored; items gained remain
  as base items (engine data remains, logic gone). Documented limitation (N-10 matrix).

### Test entry

- Unit: `Gamble` with insufficient runes → error code, no item, no deduction, no nonce increment.
  With sufficient runes → item created, runes deducted, nonce increments, event fires.
  Invalid category → error. Disabled module → error.
- Unit: pool contains no unique affixes (assert against unique_sources.csv). ilvl within tier band.
- Integration: gamble → item in inventory → salvage returns < cost (sink verified, E-GAM-2).
  Save/load round-trip: roll count + nonce persist.
- Performance: roll < 5 ms on user action (measured); MenuRender from snapshot < UI budget; no
  per-frame work (assert no gambling calls in render path).
- Migration: load a save without section 11 → defaults applied, no crash.

### Open questions

- O1: Whether gambling ships in v1 or stays reserved (recommendation: reserved, Economy pack;
  optional module if promoted). Scope decision (Q-GAM-1).
- O2: Whether a vendor NPC is ever added (recommendation: no — menu page only, for compatibility).
  Revisit if community requests a physical vendor.
- O3: Exact cost values (tunable; table above is the default shape, not final tuning).
- O4: Whether `material_pack` and `rune_pack` categories ship in v1 of the module or later
  (recommendation: ship with gambling; they are the low-stakes option for pity-averse players).
- O5: Save-scum mitigation sufficiency (documented limitation; revisit if community reports abuse).

---

## 2. Gacha (E-13) — pity pool, mechanic ID E-13

**Status:** RESERVE (Economy pack, F-17). Design only; no v1 code until register update (D-047).
Ships only if `bEnableGacha = 1` (default 0) within the Gambling module.

### L1 — Rule and numbers

- **E-13.1 Rule.** A separate "gacha" action draws one reward from a single combined pool
  (weapons + armor + accessories + materials + runes) at the current RF Tier, using published rarity
  weights and a pity system (E-13.4). Unlike gambling (E-12, player picks category), gacha is a
  single button with a combined pool. Gacha is optional and independently toggleable
  (`bEnableGacha`, default 0); gambling (E-12) can be enabled without gacha.
- **E-13.2 Pool and rates.** Combined pool of non-unique Runefall items/materials/runes at the
  current RF Tier band (ilvl clamp L-02). Rates displayed transparently in the UI (exact percent per
  rarity band). Default rates (percent, six bands): Common 35, Refined 28, Exquisite 18, Radiant 12,
  Mythic 6, Runeforged 1. (Sum = 100.) Runeforged is included but at 1% — still a chase. Tunable
  via INI `[Gacha] RarityWeight_*`. Validator enforces sum = 100. Category distribution within a
  rarity band follows the pool composition (no per-category weighting in v1; revisit if playtests
  show skew).
- **E-13.3 Cost.** Same currency (runes) and cost table as gambling (E-12.2), with an independent
  multiplier `[Gacha] CostMult` (default 1.0). A gacha roll costs the same as a gambling roll by
  default; the difference is the pool and the pity.
- **E-13.4 Pity (event-count, E-00, L-06 pattern).** Pity counters are per-character, persisted
  in co-save (section 11), and **never clock-based** (E-00). Counters increment by 1 per gacha roll:
  - `pityRefined` (threshold 10): at 10 rolls without a Refined-or-better item, the next roll is
    forced to Refined or better.
  - `pityExquisite` (threshold 40): at 40 rolls without Exquisite-or-better, next roll forced
    Exquisite or better.
  - `pityRadiant` (threshold 100): at 100 rolls without Radiant-or-better, next roll forced Radiant
    or better.
  - `pityMythic` (threshold 200): at 200 rolls without Mythic-or-better, next roll forced Mythic
    or better.
  - **Runeforged is never pity-guaranteed.** Rarity 5 stays a chase (L-06: "keeping rarity 5 a chase").
  - **Reset rule:** a drop of rarity R resets all pity counters whose threshold is ≤ R. (A Mythic
    drop resets Refined/Exquisite/Radiant/Mythic counters; a Refined drop resets only the Refined
    counter.) Mirrors L-06 ("counter resets on a qualifying drop").
  - Thresholds are tunable via INI `[Gacha] PityRefined/Exquisite/Radiant/Mythic` (default
    10/40/100/200). Validator enforces positive integers and ordering (Refined < Exquisite <
    Radiant < Mythic).
- **E-13.5 Transparency (H-GAM-1).** The UI displays: exact rates (percent per rarity band), pity
  thresholds, and current pity progress ("Guaranteed Refined+ in 10 rolls; current pity: 3/10").
  No hidden pity. No rate manipulation. No "soft pity" or undisclosed curves. The displayed rates
  equal the code constants; a validator check asserts UI strings == INI values (test entry).
- **E-13.6 No FOMO (E-00, H-GAM-1).** No daily/weekly limits. No time-gated banners. No seasonal
  pools. No "limited" items. The pool is static data; rates do not change. The player rolls whenever
  they have runes. Pity is event-count, not time-count. This is the core ethics mitigation: there is
  nothing to miss, nothing to rush, nothing expiring.
- **E-13.7 Ethics gate (H-GAM-1, H-GAM-2).** Gacha is independently toggleable and defaults OFF
  (`bEnableGacha = 0`). Enabling it is an explicit INI/FOMOD choice. Even when enabled, gacha
  rewards are materials/runes/gear only — never unique affixes or hunt/boss/quest uniques (H-GAM-2,
  D-046). Rates and pity are fully disclosed (E-13.5). The module's Nexus page must disclose the
  mechanic, rates, and pity (D-061).
- **E-13.8 Anti-abuse (C-GAM-2, E-GAM-1, L-08).** Same as E-12.7: roll applied immediately; pity
  counter and `rollNonce` persisted on save; revert discards the transaction and the pity increment.
  Save-scumming resets pity (no persistent pity gain without a save), which makes save-scumming
  pity progress impossible without saving. Documented as a known limitation (L-08).
- **E-13.9 Failure modes.** Insufficient runes → rejected, no roll, no pity increment, no consumption.
  Pool empty for current tier → rejected + log. Gacha disabled (INI) → error code. Item creation
  failure → rollback (runes refunded, pity counter not incremented), logged (E-LE-1 pattern).
  Rates not summing to 100 → module disables at DataLoaded with a log (fail-closed).

### L2 — Engine mapping

- **Owner:** same as E-12 (GamblingModule + LootEngine + RuneForge + MenuApi + PapyrusBridge).
  Gacha reuses `LootEngine.RollGacha(tier)` with gacha weights and pity logic.
- **Pity logic lives in LootEngine**, not in the UI or Papyrus: `RollGacha` reads the pity counters
  from SaveSystem, applies the forced-rarity rule, rolls, updates the counters, returns the result.
  Single code path; no duplicate pity logic (D-053).
- **Hook:** same as E-12 (MenuOpen for entry; roll on user action via task interface). No new hook.
- **Papyrus bridge:** `GachaRoll()` mutation; `GetGachaPity()` query (returns current progress per
  threshold as a compact string); `GetGachaRates()` query. Event `Runefall_OnGachaRolled(rarity, itemForm)`.
- **UI surface:** the Gambling page shows a "Gacha" tab/section (only if `bEnableGacha = 1`): rates
  table, pity progress per threshold, roll button, result panel. If disabled, the tab is hidden and
  the page shows "Gacha disabled (enable in INI)."
- **Uninstall:** same as E-12.

### L3 — Data schema

- Same as E-12: no new CSV; INI-driven rates/pity/thresholds; pool computed at roll time. Validator
  enforces rates sum to 100 and pity ordering. No new FormIDs.

### L4 — Code module

- Same as E-12. Gacha adds no new module. Config keys in `[Gacha]` (§1 L4). Co-save section 11
  holds `gachaPity` and roll counts (§5).

### Compatibility

- Same as E-12. Gacha is strictly more restrictive (default off, transparent rates, pity, no uniques),
  so it inherits all of E-12's compatibility properties. Additional note: gacha's combined pool means
  a roll can yield any category; this does not affect other mods' loot (additive, D-008).

### Test entry

- Unit: `GachaRoll` with insufficient runes → error, no pity increment. With sufficient runes → roll
  created, pity incremented, event fires. Rates sum to 100 (validator). Pity: 10 rolls without
  Refined → 11th forced Refined; counter resets on qualifying drop.
- Unit: rates displayed in UI == INI values (string match). Pity thresholds displayed == INI values.
- Integration: gacha pity persists across save/load. Migration: load save without section 11 → defaults.
- Performance: roll < 5 ms; no per-frame work.
- Ethics audit: rates table in UI matches code constants; pity thresholds match; no hidden multipliers.

### Open questions

- O6: Whether gacha ships at all (recommendation: include as opt-in default-off; if community rejects,
  gambling-only is one INI flip). (H-GAM-1.)
- O7: Whether pity thresholds (10/40/100/200) are too generous or too stingy (tunable; playtest).
- O8: Whether to add per-pool pity if multiple gacha pools are added later (recommendation: single
  combined pool + single counter for v1; per-pool counters if pools multiply).
- O9: Whether gacha should be a separate FOMOD module from gambling (recommendation: same module,
  gacha gated by INI toggle inside it, to avoid module sprawl).

---

## 3. Red-team coverage table

Maps the red-team challenges (REDTEAM-NEW-FEATURES.md §11, Gambling and gacha) and the task brief's
Q1-Q10 to resolutions in this document.

### Task brief Q1-Q10

| # | Question | Resolution | Section |
|---|---|---|---|
| Q1 | What is the currency? | Runes (R-01/R-02), rune units. No separate token, no gold. | §0.1 (decision 2), §1 L1 E-12.2 |
| Q2 | What can you gamble for? | Items (weapon/armor/accessory), material packs, rune packs. No uniques. | §1 L1 E-12.3, E-12.6 |
| Q3 | Pity system: is there one? | Yes for gacha (E-13.4), event-count based. Gambling (kadala) has no pity. | §0.1 (decision 3), §2 L1 E-13.4 |
| Q4 | Cost: how much per roll? | Per-tier table ≈ 1 reference encounter (8/15/30/65/130/260 rune units). | §1 L1 E-12.2 |
| Q5 | Pool: what's in the pool? | Tier-filtered by RF Tier, category-filtered by player choice (gambling) or combined (gacha). Non-uniques only. | §1 L1 E-12.3, E-12.5, E-12.6; §2 L1 E-13.2 |
| Q6 | UI: custom menu page or MCM? | SKSE Menu Framework page (D-022), MCM Helper fallback. | §0.1 (decision 5), §1 L2 |
| Q7 | Ethics: gacha in single-player? | No real money, no FOMO, transparent rates, pity, low stakes, opt-in default-off, disclosed. | §4 (Ethics), §2 L1 E-13.5/E-13.6/E-13.7 |
| Q8 | Save compat: pity counter in co-save? | Section 11 "gambling": pity, counts, nonce. Migration defaults. Revert discards. | §0.4, §5 |
| Q9 | Performance: roll on-demand? | Roll on user action < 5 ms; no per-frame work; no polling. | §0.6, §1 L2 (performance) |
| Q10 | Compatibility: interact with other economy mods? | Additive, no record edits, no vendor NPC, rune wallet reused, soft-detect. | §1 Compatibility, §2 Compatibility |

### Red-team challenge coverage (REDTEAM-NEW-FEATURES.md §11)

| ID | Challenge | Severity | Resolution | Section |
|---|---|---|---|---|
| C-GAM-1 | E-00: no clock-based pacing; pity must be event-count | HIGH | Pity is event-count (rolls), no time gates, no daily limits. | §2 L1 E-13.4, E-13.6 |
| C-GAM-2 | L-08 anti-abuse must apply to gambling rolls | MEDIUM | Roll applied immediately; nonce in seed; revert discards; documented limitation. | §1 L1 E-12.7, §2 L1 E-13.8 |
| E-GAM-1 | Save-scumming around gambling rolls | HIGH | Transaction reverts on death; nonce mitigates; documented as known limitation (no fake persistence). | §1 L1 E-12.7 |
| E-GAM-2 | Economy inflation: gambling must be a net sink | MEDIUM | Expected economic return ≤ 0.85 × cost (salvage gap); cost ≈ 1 encounter. | §1 L1 E-12.1, E-12.4 |
| H-GAM-1 | Ethics of gacha in a single-player mod | HIGH | No real money, no FOMO, transparent rates, pity, low stakes, opt-in default-off, disclosed. | §4, §2 L1 E-13.5-7 |
| H-GAM-2 | Gambling rewards must not undermine hunting loop | MEDIUM | No unique affixes/items in pool; hunting stays primary (D-046). | §1 L1 E-12.6, §2 L1 E-13.7 |
| M-GAM-1 | SKSE Menu Framework failure → gambling UI breaks | MEDIUM | MCM Helper fallback (D-022); page degrades gracefully. | §1 L2 (UI), §0.7 |
| Q-GAM-1 | RESERVE status (Economy pack, F-17) | MEDIUM | Remains RESERVE; register update required before promotion. | Scope note, §1 status |

### Additional red-team items inherited from sibling features

| ID | Challenge | Resolution |
|---|---|---|
| E-05 | Anti-inflation (salvage returns less than value, no passive gen) | Gambling is a sink (E-GAM-2); no passive generation; rates static. |
| E-07 | Trade-off (runes are currency + sockets) | Gambling spends runes, reducing socket budget — a real choice; hoarding vs spending preserved. |
| L-08 | Anti-abuse (container rolled once, boss drop persisted) | Gambling roll applied immediately; revert discards; documented limitation. |
| D-017 | Rarity index = enhancement count | Gambling rewards follow rarity bands (0-5) and enhancement count rules (I-02, I-04). No item exceeds rarity 5. |
| L5 | No monetization | No real money, no microtransactions, no premium currency. Runes only. |
| D-046 | Hunting primacy in v1 | Gambling pool excludes uniques; hunting remains primary progression. |
| D-047 | Scope lock (register update first) | Stays RESERVE; design only. |
| N-01 | Co-save schema version + migration | Section 11 added; migration defaults; downgrade refused. |
| N-06 | Loot tags in metadata | Gambled items tagged `source = gambling` for future filters/UI. |
| N-07 | UI data API separation | Page reads MenuApi snapshot, never CSV/INI. |
| N-08 | Diagnostics | Gambling stats (roll count, pity) exported via `/rf debug export`. |
| N-10 | Uninstall/pack-disable matrix | Section 11 ignored when module absent; items remain as base forms. |

---

## 4. Ethics section (gacha in a single-player mod)

This section exists because the red-team (H-GAM-1, HIGH) raised ethics concerns and the task brief
explicitly asks for an ethics recommendation.

### 4.1 Position

Gacha mechanics in a **free, single-player, open-source-ish mod** are not monetization and do not
extract money. However, they can still feel exploitative, addictive, or community-toxic if shipped
without safeguards. The risk is reputational (Nexus backlash, mod-page controversy) and psychological
(player frustration, regret). The benefit is a compelling optional sink that extends endgame engagement
and gives surplus runes a purpose.

**Recommendation: ship gacha, but only under all of the following conditions.** If any condition
cannot be met, ship gambling-only (E-12) and reserve gacha for a later, more considered release.

### 4.2 Conditions (all must hold)

1. **No real money, ever.** No premium currency, no microtransactions, no "supporter" gacha currency,
   no Patreon-gated rates. Runes only. (L5, D-056: no monetization.)
2. **No FOMO.** No daily/weekly limits, no time-gated banners, no seasonal pools, no "limited" items,
   no expiring content. The pool is static; rates never change; pity is event-count. A player who
   returns after a year gets the same odds. (E-00: no clock-based pacing.)
3. **Transparent odds.** The UI displays exact rates (percent per rarity band) and pity thresholds.
   No hidden pity, no "soft pity," no undisclosed curves. Displayed rates == code constants (validator
   check). (H-GAM-1 resolution.)
4. **Pity is event-count and generous enough.** Thresholds 10/40/100/200 rolls for Refined/Exquisite/
   Radiant/Mythic. No Runeforged pity (chase item stays chase, L-06). Pity never resets except on a
   qualifying drop. (E-13.4.)
5. **Low stakes.** Rewards are materials, runes, or non-unique gear only. Never unique affixes,
   hunt/boss/quest uniques, or items that gate progression. Hunting remains the primary progression
   mechanism (D-046). Gambling is a sink for surplus runes, not a replacement for hunting. (H-GAM-2.)
6. **Opt-in, default off.** Gacha (`bEnableGacha`) defaults to 0. Enabling it is an explicit INI
   or FOMOD choice. Gambling (E-12) can be enabled without gacha. A user who dislikes gacha never
   sees it. (D-047 scope lock: optional module.)
7. **Full disclosure on the Nexus page.** The mod page (D-061) must state: that gacha exists, that
   it is optional and default-off, the exact rates, the pity thresholds, that rewards are non-unique,
   and that there is no real money. No fabricated media; other visual mods disclosed (D-061).
8. **Independent disable.** A user can disable gacha at any time via INI without losing items gained.
   No lock-in, no "you spent so you must keep playing."

### 4.3 Risks and mitigations

| Risk | Mitigation |
|---|---|
| Community backlash on Nexus ("gacha in my Skyrim mod") | Default-off + full disclosure + gambling-only fallback (one INI flip). If backlash occurs, remove gacha in a patch; gambling stays. |
| Player regret / "I wasted my runes" | Confirmation dialog before each roll (cost preview + rates). Transparent expected value (sink). No undo, but no hidden costs. |
| Addictive loop (repeated rolling) | Pity thresholds are event-count and generous; cost ≈ 1 encounter per roll, so rolling is self-limiting by rune income. No animation/sound pressure (v1 ships static badges only, F-23). |
| Save-scumming to farm pity/items | Pity resets on reload without save; transaction reverts on death. Documented limitation (L-08). Not fully preventable without per-roll disk writes. |
| Rates perceived as dishonest | Validator asserts UI strings == INI values; rates never change; no "limited" pools. |

### 4.4 Fallback

If gacha is rejected by the community or by the user at promotion time, the fallback is:
gambling-only (E-12), with `bEnableGacha = 0` and the gacha UI hidden. No code removal required;
the gacha code path stays dormant and disabled. This is one INI change, not a redesign.

### 4.5 Recommendation summary

**Include gacha, but only as an opt-in, default-off, fully transparent, pity-protected, low-stakes
layer inside the optional Gambling module. Gambling (E-12) ships with or without it.** This satisfies
the ethics concern (no exploitation mechanics reach a player who doesn't opt in) while preserving
the design for players who want it. If the user prefers, the entire Gambling module can stay
reserved (Economy pack, F-17) and none of this ships in v1.

---

## 5. Co-save schema additions (v2, batched)

Reuses the v2 migration (DESIGN-ITEM-CRAFTING.md §8). Additive fields only; no new bump beyond v2.
Section 11 is new.

### Section 11 — "gambling"

| Field | Type | Default | Purpose |
|---|---|---|---|
| `gachaPity` | u32 | 0 | Current gacha pity counter (rolls since last qualifying drop) |
| `gambleRollCount` | u32 | 0 | Lifetime gambling rolls (stats) |
| `gachaRollCount` | u32 | 0 | Lifetime gacha rolls (stats) |
| `lastRollNonce` | u64 | 0 | Monotonic roll nonce mixed into RNG seed (anti-save-scum) |
| `gamblingEnabledSnapshot` | u8 | 0 | Module state at last save (diagnostics) |
| `gachaEnabledSnapshot` | u8 | 0 | Gacha state at last save (diagnostics) |

### Section 3 (item registry) — ItemMetadata

No new fields. Gambled items use the existing metadata shape (rarity, enhancements, ilvl, sockets,
tags). Tags carry `source = gambling` (N-06, already stored). No schema change needed.

### Migration (v2 → v2)

All new fields default. Old saves (without section 11) load with pity 0, counts 0, nonce 0. No crash.
Downgrades (section 11 present, loaded by a DLL without gambling support) are refused with a clear
message per D-052 (unknown sections are skipped, so a non-gambling DLL simply ignores section 11 —
this is safe because section 11 is additive and unknown sections are skipped by the TLV reader).

### Revert (death without save)

Per L4 §5 rules, revert restores the loaded state. Gambling transactions (item added, runes deducted,
pity incremented, nonce incremented) are all reverted together, consistent with all other forge
transactions. This is the same behavior as reroll/upgrade salvage; gambling is not special-cased.

---

## 6. Implementation ordering (when promoted)

1. **Phase 0 probe (optional, low risk):** verify the dynamic item creation path
   (LootEngine roll + D-023 mutation + ItemRegistry allocation + inventory add) works for a
   gambling-style mint. This is the same path as loot drops, already proven-ish; add a debug
   probe entry `GamblingProbe` to the probe harness (D-063) to confirm before relying on it.
2. **Co-save:** add section 11 to Encode/Decode (L4 §5 pattern); migration + revert handling.
   (FIX-PLAN P4.3 analogue.)
3. **LootEngine:** add `RollGambling(category, tier)` and `RollGacha(tier)` with gambling/gacha
   weights, ilvl clamp, pity logic, no-uniques filter. Unit-test rates sum to 100 and pity resets.
4. **RuneForge:** implement `Gamble(category)` and `GachaRoll()` transactions: validate balance,
   deduct runes, roll, mint item, update pity/nonce, rollback on failure (E-LE-1 pattern).
5. **GamblingModule:** optional module skeleton (fail-closed), INI gating, DataLoaded soft-detect,
   registration with HookManager only when enabled.
6. **Papyrus bridge:** queries/mutations/events (§1 L2).
7. **MenuApi + UI:** Gambling page (SKSE Menu Framework) with category selector, cost display,
   roll button, result panel, gacha rates table + pity progress, enable/disable status. MCM
   fallback.
8. **Config/INI:** `[Gambling]` and `[Gacha]` sections with defaults and caps.
9. **Validator:** rates sum to 100, pity ordering, cost positivity, pool non-empty per tier.
10. **Tests:** unit + integration + save/load + migration + performance + ethics (rates match code).
11. **Nexus disclosure:** rates, pity, optional, default-off, no real money (D-061).

---

## 7. Consolidated open questions

1. Whether gambling ships in v1 or stays reserved (recommendation: reserved, Economy pack; optional
   module if promoted). (O1, Q-GAM-1.)
2. Whether gacha ships at all (recommendation: include as opt-in default-off; gambling-only fallback
   is one INI flip). (O6, H-GAM-1.)
3. Exact cost values (tunable; table above is the default shape). (O3.)
4. Whether a vendor NPC is ever added (recommendation: no — menu page only). (O2.)
5. Whether `material_pack` and `rune_pack` categories ship with gambling or later (recommendation:
   with gambling). (O4.)
6. Save-scum mitigation sufficiency (documented limitation). (O5, L-08.)
7. Pity thresholds (10/40/100/200) generosity (tunable; playtest). (O7.)
8. Per-pool pity if multiple gacha pools are added later (recommendation: single combined pool +
   single counter for v1). (O8.)
9. Whether gacha is a separate FOMOD module from gambling (recommendation: same module, INI-gated).
   (O9.)
10. Whether the gambling module is default-on or default-off in the FOMOD if promoted
    (recommendation: default off, mirroring Durability, given the ethics posture).

---

## 8. Compliance summary

| Decision / Rule | Compliance | Evidence |
|---|---|---|
| D-016 (event-driven economy, no time pacing) | Compliant | Costs in rune units (encounter multiples); pity event-count; no daily/weekly limits; no passive gen. |
| D-006 (no vanilla AV repurposing) | Compliant | No new AVs; currency is rune wallet + MISC item; pity/counters are co-save integers. |
| D-027 (no base-record mutation) | Compliant | Items minted via dynamic channel (D-023) + co-save registry; pool computed at roll time; no vendor NPC, no cell/leveled-list edits. |
| D-028 (co-save authoritative) | Compliant | Pity, counts, nonce in section 11; rates/costs in INI (content/config, not state). |
| D-046 (hunting primacy in v1) | Compliant | Pool excludes unique affixes; uniques only from hunting/bosses/quests. |
| D-047 (scope lock) | Compliant | Stays RESERVE; register update required before promotion. |
| E-00 (no clock) | Compliant | Pity event-count; no time gates; no daily rewards. |
| E-05 (anti-inflation) | Compliant | Gambling is a sink (EV ≤ 0.85×cost); no faucet; salvage returns less than value. |
| E-07 (trade-off) | Compliant | Gambling spends runes → reduces socket budget; hoarding vs spending preserved. |
| L5 (no monetization) | Compliant | No real money, no premium currency, no microtransactions. Runes only. |
| D-017 (rarity index = enhancement count) | Compliant | Rewards follow rarity bands 0-5 and enhancement count (I-02, I-04); nothing exceeds rarity 5. |
| D-022 (SKSE Menu Framework UI) | Compliant | Gambling page via SKSE Menu Framework; MCM fallback. |
| D-056 (optional deps not required by Core) | Compliant | Gambling module optional; Core never depends on it. |
| N-01 (co-save schema + migration) | Compliant | Section 11 added; migration defaults; downgrade refused. |
| N-10 (uninstall matrix) | Compliant (design) | Section 11 ignored when absent; items remain as base forms. |

---

RUNTIME STATUS: assistant-claimed (design document, no runtime verification)
SSEEDIT/CK: not launched
UNRESOLVED: scope promotion decision (D-047 register update) for gambling/gacha; ethics community
acceptance (fallback to gambling-only); exact cost/rate/pity tuning (playtest); save-scum
mitigation sufficiency (documented limitation). Phase 0 probe for the gambling item-creation
path (low risk; same path as loot drops).