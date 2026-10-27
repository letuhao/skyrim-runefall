# Calamity - Reactive Loot & Affixes: Deep Research

Base (READ-ONLY): `D:\Works\source\skyrim-moddings\rpg-mechanism\Prisma UI 148718 1.5.1 2026-09-20T08-10Z HZCMOy8nO\calamity-reactive-loot-affixes`
Evidence date: 2026-10-28 (workspace snapshot v2.3.0).

---

## 1. Mod identity

- **Name**: Calamity - Reactive Loot & Affixes (Korean: 칼라미티)
- **Author / repo**: `servaltullius/calamity-reactive-loot-affixes`
- **Current release**: v2.3.0 (2026-10-27); Nexus page mods/193193 (new page; old mods/172116 frozen at 1.2.21)
- **Type**: SKSE-based Skyrim SE/AE equipment-build mod. Diablo/PoE-style affixes + runewords + currency progression loop on top of vanilla combat/loot. No new dungeons/quests.
- **Scope**: player gear + player-owned summons/proxies only. Followers/NPC gear is NOT tracked.
- **Runtime targets**: SE 1.5.97 and AE 1.6.x/1.7.x via a single CommonLibSSE-NG build + Address Library.
- **Hard deps**: SKSE64, Address Library, Prisma UI. Soft: SkyUI, powerofthree's Tweaks (optional since 2.2.8), KID, MCM Helper, I4.
- **Language**: bilingual Korean/English throughout (panel + MCM + docs).
- **Core loop**: kill eligible hostile → currency only in that corpse's inventory → identify/reforge/scour/runeword craft in Prisma panel → build.

Key concept: gear is tracked **per instance** (owner FormID + ExtraUniqueID UID), not per base record. Affixes are assigned manually via currencies, never auto-rolled on pickup (legacy auto-assign field `loot.chancePercent` is now a no-op).

---

## 2. Top-level file layout

```
calamity-reactive-loot-affixes/
  README.md, AGENTS.md, CHANGELOG.md, THIRD_PARTY_NOTICES.txt
  .claude/skills/calamity-ui-style/        # UI style skill
  .github/workflows/ci-verify.yml, release.yml
  affixes/
    affixes.json                            # composed spec (SSOT for generator)
    affixes.modules.json                    # module manifest
    affixes.schema.json                     # JSON schema
    runeword.contract.json                   # 95 runewords + 33 rune weights
    modules/*.json                          # tags, core/runeword/suffix/status affixes, kidRules, spidRules, tags, spec.root
  Data/                                     # staging tree installed into game Data/
    CalamityAffixes.esp                      # generated ESP (keywords, spells/MGEFs, MCM quest)
    CalamityAffixes_KID.ini                  # KID rules (mostly empty; only CAFF_TAG_DOT rule chance 0)
    CalamityAffixes_DISTR.ini                # empty SPID compat artifact
    MCM/Config/CalamityAffixes/{config.json,keybinds.json,settings.ini}
    Meshes/CalamityAffixes/...               # ShadowEchoHit, TrapMarkers
    PrismaUI/views/CalamityAffixes/          # index.html + styles/*.css + scripts/*.js
    Scripts/Source/*.psc                     # Papyrus source (7 files)
    Scripts/*.pex                            # compiled (3 active)
    SKSE/Plugins/CalamityAffixes/{affixes.json,runtime_contract.json}
    SKSE/Plugins/InventoryInjector/CalamityAffixes.json  # placeholder rules:[]
    Sound/FX/CalamityAffixes/...
    Textures/...
  docs/                                     # INDEX, AFFIX_CATALOG, EFFECTS_EN, PREFIX/SUFFIX/RUNEWORD_EFFECTS, VFX_MATRIX
    adr/{0001..0003}.md
    design/{개발명세서, CK_MVP, 데이터주도..., 아키텍처...}.md
    plans/*.md (many)
    references/{event-signatures, architecture snapshot, corpse policies, LoreBox}
    releases/*.md
  skse/CalamityAffixes/                     # CMake/vcpkg CommonLibSSE-NG plugin
    CMakeLists.txt, CMakePresets.json, vcpkg.json
    include/CalamityAffixes/                 # ~90 policy headers
    src/*.cpp                                # EventBridge split + Hooks, main, Papyrus, etc.
    tests/                                   # unit + runtime gate store checks
    extern/                                  # vendored CommonLibSSE-NG, rapidcsv, patches, vendor
  tools/                                    # compose_affixes.py, generator (.NET 8), lint, verify_*, tests
```

---

## 3. Architecture (layers)

Four layers (from architecture snapshot v1.2.14, still valid shape):

| Layer | Tech | Role | Scale |
|---|---|---|---|
| Native SKSE plugin | C++23, CommonLibSSE-NG | core runtime: loot, triggers, actions, serialization, Prisma bridge | ~11k LOC `src/*.cpp`, ~90 headers |
| C# generator | .NET 8 + Mutagen | ESP/INI/runtime JSON generation | ~1k LOC |
| Prisma UI | HTML/CSS/JS SPA (Ultralight) | in-game tooltip + control panel | 14 files, ~2.7k lines |
| Papyrus | Skyrim Papyrus | MCM + mod-event bridge | 7 source files; only 3 compiled (`ModeControl`, `ModEventEmitter`, `MCMConfig`) |

**Native core**: `EventBridge` singleton implements 7 SKSE event sinks:
`TESHitEvent`, `TESDeathEvent`, `TESEquipEvent`, `TESMagicEffectApplyEvent`, `TESContainerChangedEvent`, `TESUniqueIDChangeEvent`, `SKSE::ModCallbackEvent`.
Defined in `include/CalamityAffixes/EventBridge.h`. Dispatches by `ActionType` (Strategy). Affix definitions are shared flyweights; instances store only token slots.

**Key states**:
- Instance key = `(ownerFormID << 16) | uniqueID` (`MakeInstanceKey`).
- `_instanceAffixes: unordered_map<uint64_t, InstanceAffixSlots>` (max 3 tokens + count).
- Serialization: SKSE co-save record `IAXF` (v7 = 64-bit FNV-1a tokens; v1-v6 strings migrated on load).
- UniqueID change events move all related state together; conflicts (both sides non-empty) are fail-closed discarded.

**Subsystems (LOC, v1.2.14 snapshot)**: Config 18.6%, Loot 28.1%, UI/Tooltip 16.4%, Actions 14.0%, Triggers 11.6%, Serialization 6.3%, Core 3.2%, Traps 1.8%.

---

## 4. Affix system

### 4.1 Data model

Source of truth: `affixes/affixes.modules.json` → composes `affixes/affixes.json` (247 affixes: 181 prefix incl. 95 runeword finals + 86 regular prefixes? Actually 181 prefix total incl. runeword finals `*_final`, 66 suffix). Plus `affixes/runeword.contract.json` (95 runewords, 33 runes with weights).

Module files:
- `modules/keywords.tags.json` (2 tags)
- `modules/keywords.affixes.core.json`, `.runewords.json`, `.suffixes.json`, `.status.json`
- `modules/keywords.kidRules.json` (1 rule: `CAFF_TAG_DOT` on `*Alch*|H`, chance 0, disabled)
- `modules/keywords.spidRules.json` (0 rules)
- `modules/keywords.tags.json`
- `modules/spec.root.json` (loot config, appended records: magic effects, spells, art objects, movable statics, sound descriptors, keywords, misc items; ~113 appended records)

**Affix schema** (`affixes/affixes.schema.json`): each affix has `id, editorId, name (+nameEn/nameKo), slot (prefix|suffix), family, weaponSubtypes, records (magicEffect/spell), kid (KID distribution rule), runtime (trigger, chances, ICD, conditions, action)`.

Runtime triggers: `Hit`, `IncomingHit`, `DotApply`, `Kill`, `LowHealth`.
Action types (13): `DebugNotify, CastSpell, CastSpellAdaptiveElement, CastOnCrit, ConvertDamage, MindOverMatter, Archmage, CorpseExplosion, SummonCorpseExplosion, SpawnTrap, EchoStrike, DoomMark, SpreadStatus`.

Status tags (shared status system, v2.3): `Exposed, Doom, Burning, Freeze, Bleed, Shock`. Status amounts accumulate; one item applies, another consumes (e.g. Doom Brand, Exploit Weakness, Contagion prefixes).

### 4.2 Prefix/suffix/runeword layout (v2.3)

- 3 regular affix slots: **slot 0 = prefix OR runeword** (mutually exclusive), slots 1-2 = suffixes.
- Runeword occupying slot 0 removes the prefix (trade-off). Pre-2.3 items keeping both are preserved.
- Normal prefixes: 73 (3 excluded from loot root pool). Suffixes: 66 across 22 families.
- Runewords: 95 (D2/D2R-derived), 33 runes with 4-tier weights (El-Amn=4, Sol-Um=3, Mal-Lo=2, Sur-Zod=1; ratio 4:1).

### 4.3 Generation pipeline

1. `python3 tools/compose_affixes.py` merges module JSONs into `affixes/affixes.json`.
2. `dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data` generates:
   - `Data/SKSE/Plugins/CalamityAffixes/affixes.json` (runtime catalog)
   - `Data/SKSE/Plugins/CalamityAffixes/runtime_contract.json` (trigger/action + runeword catalog + rune weights snapshot)
   - `Data/SKSE/Plugins/InventoryInjector/CalamityAffixes.json` (placeholder `rules: []`)
   - `Data/CalamityAffixes.esp` (keywords + optional Spell/MGEF records + MCM quest + `CalamityAffixes_MCMConfig` binding)
   - `Data/CalamityAffixes_KID.ini`, `Data/CalamityAffixes_DISTR.ini`
3. Papyrus compile → `Data/Scripts/*.pex` (3 active targets).
4. CMake/vcpkg builds `CalamityAffixes.dll` → staged at `Data/SKSE/Plugins/CalamityAffixes.dll`.
5. `tools/build_mo2_zip.sh` automates generate + compile + zip.

CI (`ci-verify.yml`): compose check, affix lint, version consistency, doc drift (`update_docs.py --check`), CommonLib pin, Papyrus pin, Prisma view structure, generator tests, tools tests, runtime contract sync, SKSE build + runtime gate tests.

### 4.4 Crafting currencies & economy

Four currencies, all obtained only from eligible hostile corpses (never containers/world):
- **Identify Scroll** (default 20%): grants 1-3 regular affixes to an un-identified item (60/30/10 for 1/2/3 slots; runeword items get 1-2 suffixes 75/25).
- **Reforge Orb** (default 12%): replace one regular affix (cost 2) or expand slots (1→2 = 2 orbs, 2→3 = 4 orbs).
- **Scouring Orb** (default 4%): reroll all regular affixes, keep slot count + runeword, restore 6 reforges.
- **Runeword fragments** (default 8%): one of 33 runes (4-tier weights), pity after 99 failures.

Crafting actions in panel:
- Identify: 1 scroll → 1-3 affixes.
- Reforge Selected: 2 orbs → replace chosen regular affix (per-item limit 6 reforges; scouring resets to 6).
- Expand slot: 2/4 orbs.
- Scour: 1 orb → reroll all regular affixes.
- Remove runeword: 1 orb → remove runeword, roll new prefix; suffixes & reforges kept.
- Exchange: 3 scrolls → 1 reforge orb; 10 scrolls → 1 scouring orb (one-way).

Per-item reforge limit (6) stored in co-save. Boss/dragon/unique enemies give guaranteed drops (unique: 40% fragment / 60% orb single; boss/dragon: one of each). Pity counters + per-corpse category ledger in `CCRT v1` co-save record.

### 4.5 Runewords

95 recipes defined in `runeword.contract.json`. Each has `id, name, runes[], resultAffixId, recommendedBase` (Weapon/Armor/null). Crafting requires inserting runes in order onto a selected equipped base; completed runeword becomes the prefix-slot token. The panel shows search + counts + EN/KO details + hover. Fragment inventory tracked; material filters (ready / missing 1) in panel.

### 4.6 Shared status effects (v2.3)

Status axes: Freeze (build-up + slow + weaken), Bleed (burst), Shock (discharge), Burning (DoT), Exposed, Doom (mark → burst). Statuses applied by one affix and consumed by another (e.g. Doom Brand, Exploit Weakness, Contagion prefixes). Shared state persists per target.

---

## 5. Trigger pipeline

ADR-0003 design. Three paths:

1. **Primary hit pipeline**: `Actor::HandleHealthDamage` vfunc hook. Handles `EvaluateConversion` (damage conversion) and `EvaluateCastOnCrit` (CoC) in-hook, then calls `EventBridge::OnHealthDamage(...)` → routes `Hit`/`IncomingHit` procs.
2. **Fallback hit pipeline**: `TESHitEvent` sink. Used only when the hook is observed not firing (hooked over). Routes procs with lower fidelity.
3. **DoT trigger**: `TESMagicEffectApplyEvent` + `CAFF_TAG_DOT` keyword filter. Apply/refresh only (never ticks). Per target+MGEF ICD default 1.5s. Rate-limited, then emitted to Papyrus as `CalamityAffixes_DotApply`.

Events emitted to Papyrus (ModCallbackEvent):
- `CalamityAffixes_Hit` (sender = target Actor) → `CalamityAffixes_SkseBridge.OnCalamityHit` → `AffixManager.ProcessOutgoingHit(target)`.
- `CalamityAffixes_DotApply` (sender = target Actor) → same, treated as outgoing hit.
- `CalamityAffixes_Kill` (sender = dead Actor) — C++ only, not wired to Papyrus currently.
- MCM callbacks: `CalamityAffixes_MCM_SetEnabled`, `_SetDebugNotifications`, `_SetValidationInterval`, `_SetProcChanceMult`, `_SpawnTestItem`, UI panel open/toggle, mode cycle next/prev, runeword base/recipe cycle, insert rune, status, grant fragments/sets.

Guards: hostile check (player vs target), duplicate-hit suppression window 100ms, ICD per affix, per-target ICD, proc depth/budget caps (`triggerProcBudgetPerWindow=12` per 100ms), low-health rearm policy, non-hostile first-hit gate (default off).

DoT policy (PoE-style): DoT is NOT a hit; no per-tick procs; only apply/refresh triggers. `CAFF_TAG_DOT` distributed narrowly (`*Alch*|H` vanilla poisons) with chance 0 by default to avoid dispel/purge side effects. Enabling sets chance 100 in KID INI.

---

## 6. Instance storage & serialization

ADR-0001. Key design decisions:

- Instance identity = `RE::ExtraUniqueID` (owner FormID + UID).
- Internal map key = `ownerFormID << 16 | uniqueID` (`MakeInstanceKey`).
- Value = up to 3 affix tokens (64-bit FNV-1a of affix id) + count (`InstanceAffixSlots`).
- Persisted via SKSE Serialization co-save (`IAXF` record):
  - v1-v6: strings (migrated on load)
  - v7 (current): 64-bit tokens only (size/memory optimization)
- `TESUniqueIDChangeEvent` moves all related state (tokens, slots, runeword progress, reforge counts) from old key to new key. If destination already has real state, both are discarded (fail-closed, no silent merge) — prevents cross-item leakage.
- v1.2.22 migration: synthetic `(itemFormID, UID)` keys from old builds are corrected to `(playerFormID, UID)` only when current player inventory + UID uniqueness proves it. Ambiguous existing state is discarded first.
- Safe pruning: when a world-dropped reference is deleted (SKSE form delete callback), its instance entry is pruned. Inventory/container items are never pruned (avoids losing affixes on stored gear).
- `CCRT v1` record: pity counters (fragment fail streak, orb fail streak) + per-corpse ledger (FormID, game date stamp, category processed mask).

Legacy Papyrus `AffixManager`/`PlayerAlias`/`SkseBridge` paths are reference-only; active build compiles only `ModeControl`, `ModEventEmitter`, `MCMConfig`.

---

## 7. Prisma UI panel architecture

Path: `Data/PrismaUI/views/CalamityAffixes/`. Ultralight (WebKit-ish), NOT CEF.

Files:
- `index.html` (markup only; CSS/JS via ordered `<link>`/`<script src>`; classic scripts only, never modules).
- `styles/`: base.css, tooltip.css, panel.css, build-summary.css, runeword.css, resource-dashboard.css, responsive.css.
- `scripts/`: dom.js, tooltip.js, state.js, i18n.js, ui-text.js, panel-layout.js, recipe-text.js, recipe-list.js, runeword.js, build-summary.js, resource-dashboard.js, interop.js, commands.js, bootstrap.js, scroll.js.

Architecture:
- **Single global lexical environment** (classic scripts) so cross-file refs work.
- **Load order = tag order in index.html** (cascade/execution order).
- Interop: C++ pushes data via `PrismaUI_Interop(method, callback)` for: tooltip, controlPanel, actionFeedback, panelHotkeyText, uiLanguage, selectedItemName/Source, inventoryItems, recipeItems, runewordPanelState, runewordAffixPreview, panelLayout, tooltipLayout. Fallback window globals registered for debugging.
- Command bridge: panel buttons → `window.calamityCommand(command)` → C++ handles `ui.*`, `mode.*`, `runeword.*`, `affix.*`, `currency.*`, `spawn.*`.
- Three main tabs: Item Affixes, Runeword, Advanced. Runeword tab has resource dashboard (rune fragments + pity), recipe explorer (search, base/material filters, listbox with wheel/keyboard/ARIA), action inspector (transmute/remove, rune grid). Affixes tab has slot progress (prefix + 2 suffix), reforge lock list, identify/reforge/expand/scour/remove buttons, orb dashboard (identify/reforge/scouring counts + exchange), equipped build summary (offense/defense/kill/passive groups). Advanced tab: tooltip UI controls (font size, nudge, reset), manual mode cycle, recovery & reset (6s confirm), debug tools.
- Tooltip panel (click-through by default) auto-hides unless selected item is an instance affix item; moves into panel when panel open.
- Layout persistence: panel position/size/scale saved via C++ (`PrismaLayoutPersistence`), tooltip layout separately. UI scale 0.9-1.75 proportional to panel size.
- Wheel scrolling: `data-wheel-scroll-mode="smooth"` opts into `scroll.js` frame-based controller (tooltip + recipe list); avoids Ultralight small deltas.
- Palette: Nord-bone-gold (v2.3): base rgb 23,20,18; text 236,229,214; accent 201,184,140; danger/warn/magic variants. All colors via `rgba(var(--<role>-rgb), alpha)` tokens in `base.css :root`.

---

## 8. MCM

Path: `Data/MCM/Config/CalamityAffixes/{config.json,keybinds.json,settings.ini}`.

- MCM Helper config generated by `tools/gen_mcm_config.py`. Pages: Runtime, Prisma UI, Hotkey & Debug.
- Runtime settings: enable mod, validation interval (0-30s), proc multiplier (0.1-3.0), fragment chance (0-100), reforge chance, identify chance, scouring chance, DoT safety auto-disable, neutral first-hit procs.
- Prisma page: open/close panel button, panel language (EN/KO/Both), crafting guide, status/recovery.
- Hotkey page: `prisma_panel_toggle` keymap (default F11 via fallback), debug notifications toggle.
- settings.ini defaults mirror above; overrides via `Data/MCM/Settings/CalamityAffixes.ini`.
- Runtime values also persisted to `Data/SKSE/Plugins/CalamityAffixes/user_settings.json` (debounced ~250ms, atomic temp-file replace, never clobbered on parse failure).
- MCM script `CalamityAffixes_MCMConfig.psc` extends `MCM_ConfigBase`; canonical-quest leader election ensures only one quest instance participates (stale-save safety); language groups (hidden toggles) drive bilingual display.

---

## 9. KID/SPID distribution

- `Data/CalamityAffixes_KID.ini`: auto-generated; instance mode means affix keywords are NOT distributed via KID. Only rule: `CAFF_TAG_DOT|Magic Effect|*Alch*|H|0` (disabled; set chance 100 to enable DoT triggers on vanilla poisons).
- `Data/CalamityAffixes_DISTR.ini`: empty compat artifact (no DeathItem/Item/Perk rules). Currency drops are runtime death-event only; SPID is optional and does not expand scope.
- `Data/SKSE/Plugins/InventoryInjector/CalamityAffixes.json`: placeholder `rules: []`.

---

## 10. VFX & audio

- Custom meshes: `Data/Meshes/CalamityAffixes/ShadowEchoHit.nif`, `TrapMarkers/*.nif` (bear, rune plate, spider sack, oil puddle, spider eggs, pressure plates).
- Custom textures: `Textures/CalamityAffixes/ShadowFistSmoke.dds`.
- Custom sounds: `Sound/FX/CalamityAffixes/ShadowPunch01-03.wav`.
- Appended art objects (from spec.root): ~30 `ArtObject` records (VFX hit effects for voice of power, spirit absorb, traps, corpse explosions, proc feedbacks).
- Shadow Boxer runeword uses `ShadowEchoHit.nif` + shadow punch sounds (built via `tools/vfx/`).
- VFX matrix doc: `docs/VFX_MATRIX.md`. Proc feedback system: `actionFeedback` (artObject/sound, target, duration, playOn Proc/PassiveAdd), trap feedback (marker art/world object, scales, placed/armed/triggered/expired cues).
- Hit shaders/forms referenced from Skyrim.esm (fire/frost/shock dynamic hit FX).

---

## 11. Compatibility / corpse policies

- Corpse-only currency: death event from player or player-owned summon/proxy killing an eligible hostile. Excludes followers/teammates, summoned/commanded, children, player-owned, non-hostile, environmental kills, independent NPC kills.
- Boss (`LocRefTypeBoss`) and dragons (`ActorTypeDragon`) → one fragment + one orb guaranteed; unique actors → one guaranteed (40/60 split). Special guarantees don't touch normal rolls or pity.
- Legacy SPID pre-distribution on transition corpses detected per category (fragment/orb) and skips that category's runtime roll.
- Name mutation: item names get `*`/`**`/`***` suffix (trailing by default) for affix count; ASCII markers for save stability.
- Removal safety: disable mod in MCM + wait 10s + save before uninstall; ESP+DLL must be removed together; passive stats persist otherwise.
- Downgrade path: disable first, then install old version; user_settings.json keeps disabled state for old DLL.

---

## 12. Relevance to Runefall

Runefall loot pillar (per `Runefall/research/loot-systems.md`): six rarity bands 0-5 (rarity = enhancement count), affix CSV schema (`affixId,name,category,tier,points,minValue,maxValue,unit,duration,area,slots,weight,minItemLevel,mgef,npcExclude,prefix,suffix`), additive runtime injection (no leveled-list edits), salvage, magic find.

Calamity is a **different paradigm** (instance-tracked affixes via SKSE + manual crafting currencies) but offers many reusable patterns:

1. **Affix CSV schema**: Calamity's affix JSON schema is richer (trigger/action/icd/status) but the generator→runtime contract pattern (JSON → ESP/INI/runtime JSON) is directly applicable. Runefall's CSV can be extended with trigger/action columns later. The "compose modules → lint → generate → verify sync" pipeline is a model for Runefall's data pipeline.

2. **Runtime injection (no leveled-list edits)**: Calamity's corpse-only currency + instance key approach avoids leveled-list edits entirely. For Runefall's additive loot, the CID NG hook set (`Load3D`/`Load3DCharacter`/`ResetInventory`/`SaveGame` + load-game clear + save-stripping) from loot-systems.md research is the template; Calamity adds the `TESContainerChangedEvent` + instance UID tracking layer on top. Runefall can adopt instance UID tracking for per-item affix persistence instead of enchantment records.

3. **Instance metadata persistence**: Calamity's `ExtraUniqueID` + 64-bit token co-save (`IAXF v7`) with migration and conflict fail-closed is the strongest reference for Runefall's "per-item affix state survives save/load and container moves". The `TESUniqueIDChangeEvent` move logic and synthetic-key correction are directly reusable. Tokens (FNV-1a) keep save size small and make affix IDs rename-forbidden (version discipline).

4. **Prisma UI panel**: Calamity's panel is the most sophisticated reference UI in the mechanism folder — Ultralight constraints, click-through tooltip, tabs, resource dashboards, recipe search, confirmation arming, layout persistence, bilingual. Runefall's web-menu research can reuse: classic-script global env pattern, interop push model, command bridge, wheel controller, palette tokens, confirmation windows (6s arm). The `.claude/skills/calamity-ui-style` skill documents Ultralight pitfalls (grid/flex hidden override, integer scrollTop, backdrop-filter removal).

5. **KID/SPID vs native**: Calamity demonstrates the modern shift: KID/SPID are now compat artifacts; the real logic is native. Runefall's "no leveled-list edits" can follow the same: keep SPID/KID only for optional keyword tagging (e.g. DoT tag), do distribution in DLL. This matches Runefall's additive-injection preference and avoids SPID dependency for core loot.

6. **Damage hook / trigger pipeline**: Calamity's `HandleHealthDamage` primary + `TESHitEvent` fallback + DoT apply-only design is the reference for Runefall's combat-event-driven affix procs. The guards (duplicate suppression, ICD, per-target cooldown, proc budget, hostile checks, non-hostile first-hit gate) are reusable safety patterns. DoT-as-apply-only (PoE-style) avoids tick spam.

7. **Runewords vs rune families**: Calamity's 95-recipe runeword system (ordered rune insertion, fragment economy with pity, per-corpse ledger) is a model for Runefall's "rune families" if Runefall wants socketed/runeword crafting. The fragment economy (8% base, 99-fail pity, 4-tier weights, boss guarantees) and crafting actions (identify/reforge/expand/scour/remove) map to Runefall's salvage/crafting loop. Runeword occupying the prefix slot (mutually exclusive with prefix) is a clean trade-off pattern.

8. **Other reusable pieces**: MCM bilingual config with hidden language-group toggles and canonical-quest leader election; user_settings.json atomic persistence with debounce; name mutation markers for affix count; recovery/reset with 6s confirmation; VFX feedback contract (artObject + sound + target + duration); trap system with world markers and global cap.

Caveats for Runefall: Calamity is instance-tracked (not enchantment-record based), manual crafting (not auto-roll on loot), and player-only scope. Runefall's auto-roll-on-container-open model (DiabloRim/CID) differs in timing; but Calamity's instance persistence layer can be combined with DiabloRim's roll timing to get "auto-rolled but instance-persistent" affixes — arguably the best of both.

---

## 13. Source paths (key files)

- `README.md`, `AGENTS.md`, `CHANGELOG.md`, `THIRD_PARTY_NOTICES.txt`
- `affixes/affixes.json`, `affixes.modules.json`, `affixes.schema.json`, `affixes/runeword.contract.json`
- `affixes/modules/*.json`
- `docs/INDEX.md`, `docs/adr/{0001,0002,0003}.md`, `docs/design/개발명세서.md`, `docs/references/2026-02-15-calamityaffixes-architecture-snapshot-v1.2.14.md`, `docs/references/2026-02-09-event-signatures.md`, `docs/references/2026-02-22-corpse-drop-compat-policy.md`, `docs/references/2026-07-14-corpse-runtime-currency-policy.md`
- `docs/PREFIX_EFFECTS.md`, `SUFFIX_EFFECTS.md`, `RUNEWORD_EFFECTS.md`, `EFFECTS_EN.md`, `VFX_MATRIX.md`, `AFFIX_CATALOG.md`
- `Data/Scripts/Source/CalamityAffixes_{AffixEffectBase,AffixManager,MCMConfig,ModeControl,ModEventEmitter,PlayerAlias,SkseBridge}.psc`
- `Data/MCM/Config/CalamityAffixes/{config.json,keybinds.json,settings.ini}`
- `Data/CalamityAffixes_{KID,DISTR}.ini`
- `Data/SKSE/Plugins/CalamityAffixes/{affixes.json,runtime_contract.json}`
- `Data/SKSE/Plugins/InventoryInjector/CalamityAffixes.json`
- `Data/PrismaUI/views/CalamityAffixes/{index.html,styles/*.css,scripts/*.js}`
- `skse/CalamityAffixes/{CMakeLists.txt,CMakePresets.json,vcpkg.json,include/CalamityAffixes/*.h,src/*.cpp,tests/*}`
- `.claude/skills/calamity-ui-style/SKILL.md`
- `.github/workflows/{ci-verify,release}.yml`
- `tools/{compose_affixes.py,lint_affixes.py,gen_*_doc.py,verify_*.py,CalamityAffixes.Generator/*}`

---

## 14. Notes

- Read-only analysis; no files in rpg-mechanism modified.
- Extern subtree (`skse/.../extern/CommonLibSSE-NG`) is vendored dependency, not analyzed.
- Release history spans v0.1.0-beta through v2.3.0; CHANGELOG and `docs/releases/*` provide per-version detail.
- The mod's actual in-game name is "칼라미티" (not 칼래미티); mod name "Calamity".
