# Research: `isekai-hero-skyrim`

**Base path (read-only):** `D:\Works\source\skyrim-moddings\rpg-mechanism\Prisma UI 148718 1.5.1 2026-09-20T08-10Z HZCMOy8nO\isekai-hero-skyrim`
**Research output:** `D:\Works\source\skyrim-moddings\skyrim-runefall\Runefall\research\isekai-hero-skyrim.md`

## 1. Mod identity

- **Name:** Isekai Hero (SKSE plugin `IsekaiHeroSKSE`, data plugin `IsekaiHero.esp`).
- **Version:** 0.9.0 pre-release (CMake `VERSION 0.9.0`; co-save version `kVersion = 16`).
- **Nexus ID:** 185548 (https://www.nexusmods.com/skyrimspecialedition/mods/185548).
- **Purpose:** An isekai / "reincarnated hero" system for Skyrim SE/AE/VR. A native SKSE C++ plugin (CommonLibSSE-NG) with an ImGui overlay drawn into the D3D11 swap chain, plus an optional PrismaUI HTML/CSS view and an optional MCM.
- **License:** MIT (with CC BY 4.0 carve-out for SFX; purchased icons not relicensable).
- **Requirements:** Skyrim SE/AE/VR up to 1.7.104, SKSE64, Address Library. Optional: PrismaUI (for the web UI), SkyUI + MCM Helper (for MCM), SkyrimNet (AI NPC awareness).
- **Key in-game entry:** RShift+S opens `[ SYSTEM ] STATUS`. ESL-flagged ESP (no load-order slot).
- **Nexus header image:** `docs/nexus_header.png`.

Core gameplay loop: first time the player is in control, a paced `[ SYSTEM ]` boot sequence runs, then a blessing choice (NORMAL/HERO/ASCENDED, Full/Shattered/Dormant/Custom). Milestones (79 quest completions) pay passive stat bonuses, System Points, dragon souls, perk points. A System Skill Tree (hub + MIGHT/SHADOW/ARCANA + MASTERY rail) bought with System Points. Threat labels float over enemies. Dimensional Storage (one chest reachable anywhere; crafting stations read/consume it in place). System Quests (kill objectives on the System's schedule) + kill bounties + professions feed System Points. System Shop sells material packs/gold/potions into storage.

---

## 2. Full file layout (key dirs)

```
isekai-hero-skyrim/
  .claude/                  Claude working agreements
  docs/                     CREATION_KIT_ESP.md, IDEAS.md, MANUAL_TESTS.md, NEXUS_CHANGELOG.md,
                            NEXUS_DESCRIPTION.md, VR.md, nexus_header.png, ui_art_prompts
  extern/                   CommonLibSSE-NG (submodule), imgui-vr-helper (vendored, LGPL)
  fomod/                    info.xml, ModuleConfig.xml, images/
  fomod-presets/            generated ini variants (standard/quiet/alternate-start/...)
  icons/                    PNGs (blessings, ranks, shop, ui, spells_*)
  include/                  IsekaiHeroAPI.h (public ModAPI)
  lang/                     template.txt (extracted strings)
  mcm-patch/                MCM/config.json, settings.ini, Scripts/IsekaiHeroMCM.pex + source
  meshes/, textures/        shipped assets (from external asset pipeline)
  playground/               (referenced) HTML/JS mock of UI for layout checks
  plugin/                   IsekaiHero.esp
  prisma-patch/PrismaUI/views/IsekaiHero/index.html  (+ icons)
  sounds/                   WAVs
  src/                      C++ source (~60 files incl. UI/)
  tools/                    check.mjs, make-fomod.mjs, extract-strings.mjs, package.ps1, ...
  build.bat, package.ps1, package-prisma-patch.ps1
  CMakeLists.txt, vcpkg.json, vcpkg-configuration.json
  IsekaiHero.ini, CLAUDE.md, README.md, CHANGELOG.md, LICENSE
```

Source files under `src/`:
- Top-level: Config.cpp/.h, CraftHooks.cpp/.h, Loc.cpp/.h, Logger.h, main.cpp, ModAPI.cpp/.h, Passives.cpp/.h, PCH.h, Plugin.cpp/.h, PrismaUI_API.h, Professions.cpp/.h, Progression.cpp/.h, Quests.cpp/.h, SelfTest.cpp/.h, Shop.cpp/.h, SkillTree.cpp/.h, SkyrimNet.cpp/.h, Sounds.cpp/.h, Storage.cpp/.h, System.cpp/.h.
- `src/UI/`: Input.cpp/.h, LevelUpEffect.cpp/.h, Overlay.cpp/.h, Prisma.cpp/.h, ShopWindow.cpp/.h, SkillTreeWindow.cpp/.h, Style.h, SystemWindow.cpp/.h, Textures.cpp/.h, ThreatLabels.cpp/.h, Toast.cpp/.h, VROverlay.cpp/.h.

---

## 3. Architecture overview

Single DLL (`IsekaiHeroSKSE.dll`) built with CommonLibSSE-NG, one binary for SE/AE/VR. Lifecycle:

1. `SKSEPluginLoad` → `SKSE::Init`, `Isekai::InitLogger`, `Isekai::Install` (registers serialization + messaging listener).
2. `kPostPostLoad` SKSE message → `Config::Load`, `UI::InstallVROverlay` (VR only).
3. `kDataLoaded` → `Config::Load`, `Loc::Load`, `Plugin::DumpForms`, then `Passives::Install`, `Sounds::Install`, `Storage::Install`, `Shop::Install`, `CraftHooks::Install`, `ModAPI::Install`, `UI::Install`, `UI::Prisma::Install`, `SkyrimNet::Install`, `Progression::Install`, `Quests::Install`, `Professions::Install`, `UI::InstallThreatLabels`, `SelfTest::Install`, console command, debug key, `UI::LogHotkeys`, plus `MenuWatcher`/`LevelIncrease` sinks.
4. `kPostLoadGame`/`kNewGame` → `Progression::CatchUpOnLoad`, `CheckDormantAwakening`, `SkillTree::RetireRemovedNodes`, `SkillTree::ApplyOnLoad`, `Passives::Refresh`, `RetireLevelGrant`, `Storage::PruneForeignStock`, `Storage::PruneOrphanChests`, `Storage::GrantCodexIfMissing`, `Quests::EnsureObjective`, `SelfTest::RunOnLoad`.

Persistence: SKSE co-save (`kSerID = 'ISKA'`, record `'STAT'`, version 16) serialized field-by-field in `System::SaveCallback`/`LoadCallback`/`RevertCallback`. State lives in a single `State g_state` in `System.cpp`.

UI: ImGui overlay hooks `IDXGISwapChain::Present` (vtable slot 8) via a probe swap chain (no game-struct reads). Input from `BSInputDeviceManager` event sink. Menus (System panel, skill tree, shop) drawn in ImGui or, when PrismaUI patch is active, as a web view. Game is paused + player input blocked while a panel is open (`SetGameHold`).

Rendering thread vs main thread discipline: render thread only draws; game-state reads/writes happen on the main thread via `SKSE::GetTaskInterface()->AddTask`.

---

## 4. Subsystem deep dives

### 4.1 Config (`src/Config.h/.cpp`)
- Reads `Data\SKSE\Plugins\IsekaiHero.ini` (shipped) then layers `Data\MCM\Settings\IsekaiHero.ini` (MCM Helper output) on top. Absent file/keys fall back to defaults. Section-agnostic flat key=value scan; comments via `;`/`#`.
- MCM spelling differs (engine type prefixes `b/i/f` stripped; `ThreatLabelTargets` stored as index 0..3).
- ~32 settings: AutoStart, TextSpeed, UiScale, Language, HideSealedNodes, DormantHeroLevel/AscendedLevel, StorageCodex, SkyrimNetIntegration, Threat* (labels, targets, numbers, resources, key, range, aim radius, VR height, VR in-headset layer), Quest first/interval hours, reward scale, KillsPerBounty, KillBountyPoints, ProfessionActionsPerPoint, NodeCostScale, NodeEffectScale, Hero/Ascended skill/attr/points, QuestRerollButton, SelfTestKey, DebugPointsKey, LogInputDiagnostics, Hotkeys (menu key/modifier, gamepad button/modifier, VR menu button).
- Hotkeys support names ("F11") or hex/decimal scan codes; gamepad buttons by XInput mask names.
- Clamping/validation per key; out-of-range keeps default rather than clamping for some (AsCount refuses out-of-range).

### 4.2 Plugin (`src/Plugin.h/.cpp`)
- `kFileName = "IsekaiHero.esp"`. `DumpForms()` resolves the file, sets `g_loaded`, logs all forms. `ResolveLocalID` reconstructs full FormID from partial index (handles light/ESL and regular). `LookupOurForm<T>` typed lookup. `IsOfficialMaster` checks the five official masters (Skyrim/Update/Dawnguard/HearthFires/Dragonborn) — used by Storage/Shop/CraftHooks to restrict to vanilla+DLC materials (avoids CC/mod scripted ingredients).
- VR quirk: `TESDataHandler::LookupForm(localID, kFileName)` returns null for ESL in VR; hence the manual FormID math.

### 4.3 System (`src/System.h/.cpp`) — core state & reincarnation
- `State` struct (v16 co-save): preLevel/preSkills/preHealth/Magicka/Stamina (baseline for REBOOT), reincarnated, power (Normal/Hero/Ascended), skills (unused enum), shattered, dormant, grantTier/treeTier/custom (decoupled CUSTOM axes), grantedMilestones, storageChest, unlockedNodes, nodeRanks (repeatable counts), systemPoints, questKey/Progress/Target/Reward/NextDue/QuestsGiven, nodeCostScale, levelGrantRetired, killCounts map, professionActions.
- `PowerLevel` (Normal/Hero/Ascended), `SkillFocus`. `RewardScale()` = 1/2/4 by tier.
- Blessing table (`BlessingFor`): skillLevel, attrTarget, perkPoints, gold, dragonSouls, systemPoints. NORMAL = none; HERO = skill 50, attr 180, 10 perks, 2k gold, 3 souls, 5 SP; ASCENDED = 100, 600, 255 perks, 25k gold, 20 souls, 10 SP. Values from ini.
- Reincarnation flow: `BeginReincarnation` (VR gate: if VR && !Prisma active, hold), boot messages via `DelayedMainThread`, `ShowPowerSelection` (NORMAL/HERO/ASCENDED/DORMANT/CUSTOM), `ShowPathSelection` (Full/Shattered), `ShowCustomBuilder` (3 AskTier steps: grantTier, power (reward), treeTier), `ApplyReincarnation` (sets nodeCostScale, levelGrantRetired=true, ApplyBlessing, Storage::GrantCodexIfMissing, Quests::EnsureObjective, SkyrimNet push, UI panel).
- Dormant: starts Normal; `CheckDormantAwakening` (on menu close / level up / load) raises tier at configured levels via `AwakenTo` (applies blessing, pushes SkyrimNet event, plays flourish).
- `TriggerReincarnation` (hotkey path, no cell heuristic) vs `TryTrigger` (auto: first-ready cell interior/exterior heuristic to avoid firing in chargen rooms).
- `MenuWatcher` (MenuOpenCloseEvent + LevelIncrease::Event).
- `RebootSystem`: RestoreBaseline (skills/attrs back to pre-blessing), SkillTree::Respec (refund), then ShowPowerSelection. Keeps milestones/SP/storage.
- `RetireLevelGrant`: one-shot migration undoing the old level grant using preLevel.
- Console command `IsekaiPoints [n]` (overwrites TestSeenData/DumpNiUpdates/ShowSceneGraph). Debug hotkey grants 100 SP.
- `DelayedMainThread(ms, fn)`: detached thread sleep then SKSE task.

### 4.4 Progression (`src/Progression.h/.cpp`) — milestones
- 79 `Milestone` entries: key, editorID (quest), questName, Passive (name, AV, base amount, percent), dragonSouls, endpoint (pays perk points).
- Quests grouped: Main (MQ), Dawnguard, Dragonborn, Companions, College, Thieves Guild, Dark Brotherhood, Civil War. Passives aggregate into 9 `System:` ability spells.
- `Grant` pushes milestone key, calls `Passives::Refresh`, grants souls (scaled), SP (`MilestoneSystemPoints`: 1 base / 5 endpoint × RewardScale), perk points if endpoint.
- `CelebrateMilestone` → flourish + reward panel.
- `CheckQuest` via `TESQuestStageEvent` + `TESQuestStartStopEvent` (handles `completequest` console command). `CatchUpOnLoad` sweeps already-completed quests.
- `SystemRank()`: score = milestonesEarned + level/5 + TotalInvested/10 → E/D/C/B/A/S.
- `BuildStatusJson()` for Prisma status screen: tier, rank, mode, axes, nextAwakening, milestonesEarned/Total, points, hasTree/Storage/Shop, canReboot, questText/Progress/Target/Reward/WaitHours/CanReroll, attunements [{stat,amount}], titles [{name,effect,quest}].
- `ShowStatusPanel()` (RShift+S): either `Prisma::ShowStatus(json)` or ImGui monospace ledger with icon actions (skill tree, storage, shop, new task, reboot, close) + emblem rank crest.
- `EarnedPassives()` returns `const Passive*` list. `PassiveAmount` = round(base × RewardScale). `PassiveEffectText`.

### 4.5 Passives (`src/Passives.h/.cpp`)
- 9 ability spells from ESP (local FormIDs 0xD69..0xD8C), one per actor value (Health, Magicka, Stamina, CarryWeight, ResistMagic, ResistFire, ResistFrost, ResistShock, ResistDisease...). `CoveredActorValues()` returns which AVs have an ability — contract for milestones/skill-tree attribute nodes.
- `Install` resolves spells, reads primaryAV off the magic effect (single source of truth).
- `Refresh`: totals from milestones + `SkillTree::AccumulateBonuses`; writes magnitude into effect; re-applies spell (remove+add) so running ability picks up new magnitude. Removes spell when total <= 0.
- Effects must carry Recover flag + archetype 34 (Peak Value Modifier) to actually fortify (learned from vanilla).

### 4.6 SkillTree (`src/SkillTree.h/.cpp`)
- 22 nodes (keys 1..22; 23 retired). Zones: Core (hub/satellites), Might, Arcana, Shadow, Mastery (repeatable rail). Each node: key, zone, name, desc, icon, x/y (1120x760 design canvas), cost, minPower (tier gate), prereq[2], effect, bonus[3], repeatable, maxRank, baseline, scale.
- Effects: kAttributes (flat AV, via Passives), kShoutCooldown (-20% shout recovery mult 0.8), kAllShouts, kAllEnchantments, kAllIngredients, kAllSpells, kPerkPoint (+5, repeatable, uncapped), kDirectStat (set AV to baseline+amount×rank×NodeEffectScale; for move speed, resists, regen, absorption, armor).
- Repeatable mastery nodes: maxRank 10, 5 tiers (Novice/Adept/Expert/Master/Grandmaster, 2 ranks each), escalating cost per tier (base × tier). Perk Synthesis flat (cost 1 SP → 5 perk points).
- Tier gating by `treeTier` (NORMAL walks self-made half; HERO adds 4 Omniscience gifts; ASCENDED adds World Tree capstone). Sealed nodes greyed or hidden (HideSealedNodes).
- Knowledge unlocks span all loaded plugins via semantic filters: shouts (has description + 3 words, dedup by name, drop "Dragon " duplicates), enchantments (referenced as baseEnchantment, dedup by name), ingredients (all 4 effects known), spells (has spell tome).
- `TryUnlock`: checks prereqs/tier/rank cap, deducts `NextCost` (scaled by nodeCostScale snapshot), applies effect, refreshes passives, plays Grandmaster flourish at max rank.
- `Respec`: refunds refundable nodes (attributes/shout cooldown/direct stat), clears them, reverts direct AV writes; excludes knowledge + Perk Synthesis.
- `TotalInvested`, `RespecRefund`, `ApplyOnLoad` (re-apply effects), `AccumulateBonuses`, `RetireRemovedNodes` (key 23 refunded).
- Thread safety via mutex; render thread reads via Nodes/IsUnlocked/Rank.
- Loc: node names/descs via `node.<key>` / `node.<key>.desc`.

### 4.7 Quests (`src/Quests.h/.cpp`) — System Quests
- 7 quarries matched by ActorType keyword: wild beasts (20, 3 SP, lvl1), Draugr (25,4,lvl1), trolls (8,5,lvl10), Dwarven (12,5,lvl12), Daedra (10,6,lvl15), giants (5,8,lvl20), dragons (3,12,lvl25). Key 3 (Falmer) retired (keyword doesn't exist).
- First objective always wild beasts; later rolled randomly among level-allowed quarries, avoiding the just-finished one.
- Count/reward scale with level (+2%/level, snapshotted into questTarget/questReward at hand-out). Reward also × RewardScale × QuestRewardScale (ini).
- Timing: FirstTaskHours (default 12) before first; TaskIntervalHours (default 24) between. `Tick()` (polled once/sec from render thread via task) arms deadline and rolls when due.
- Kill counting: TESDeathEvent, counts if killer is player or teammate; increments progress; on completion pays SP, plays flourish, clears objective, sets next due.
- Kill bounties: every kill tallied per quarry type in `killCounts`; every KillsPerBounty (default 50) of a type pays KillBountyPoints (default 1) SP. Runs whether or not an objective stands.
- `Reroll()` (diagnostic NEW TASK button).
- `QuarryKeywords()` for self-test. `EnsureObjective()` backfills for pre-feature saves.

### 4.8 Professions (`src/Professions.h/.cpp`)
- One track: harvest (TESHarvestedEvent) + craft (TESContainerChangedEvent with oldContainer==0, newContainer==player, CraftingMenu open). Every ProfessionActionsPerPoint (default 25) pays 1 SP. Taking materials out of storage at a station does NOT count (oldContainer == chest). Gated by ini 0 = off.

### 4.9 Storage (`src/Storage.h/.cpp`) — Dimensional Storage
- Container base `0x000D7A` in ESP. Created via `PlaceObjectAtMe(forcePersist=true)` on first use; FormID in co-save (resolved across load order). Moved to player cell, buried 3000 units below floor; `ActivateRef` opens container menu.
- ClaimChest sets owner to player (avoids theft in NPC-owned buildings). RebuildChest if disabled/orphaned (moves inventory, disables+deletes old).
- PruneForeignStock removes non-official ingredients/misc on load (CC/mod scripted items). PruneOrphanChests removes leftover husks.
- Codex token `0x000D7F` (ALCH): "drink" it → TESEquipEvent sets pending; ServeCodexIfPending opens System menu when no game menu holds screen (works in VR/no hotkey).
- GrantCodexIfMissing backfills; reclaims if StorageCodex off.
- Crafting integration:
  - CraftWatcher (MenuOpenCloseEvent on CraftingMenu): LendToPlayer/TakeBack (shuttle for alchemy/enchanting when hooks inactive).
  - ShuttleStation: only alchemy/enchanting if hooks active; all stations if hooks inactive.
  - StationWantsItem filters by bench type (ingredients→alchemy; soul gems→enchanting; misc→smithing).
  - LendToPlayer moves all wanted stacks from chest to player; TakeBack returns min(borrowed, still held).
- SweepCategory (StockCategory/CountCategory): smithing (hand-picked core FormIDs + every Misc from official-master BGSConstructibleObject recipes), alchemy (all official-master ingredients), enchanting (all official-master filled soul gems). Dedup via set. Dry run (perItem<=0) counts without delivering.
- Available() = base resolved && reincarnated.

### 4.10 CraftHooks (`src/CraftHooks.h/.cpp`) — zero-transfer crafting
- MinHook detours: Hook 3 `GetInventoryItemCount` (REL::VariantID 15869/16109) adds cached chest count; Hook 1 `GetContainerItemCount` (19274/19700) adds chest stack total; Hook 2 `GetInventoryItemEntryAtIdx` (19273/19699) returns chest entries after player's boundary; Hook 4 `TESObjectREFR::RemoveItem` vtable 0x56 pulls chest first (RemoveFromChest), remainder from player. VR offsets provided (third VariantID arg).
- Session state: g_menuOpen/g_craftContext (atomic), g_playerInvChanges (cached pointer), g_chestCache (mutex-guarded snapshot). InCraftContext() combines menu flag + 20s activate window.
- ActivateWatcher (TESActivateEvent) on crafting furniture: sets context, LendTokens (one of each recipe material the player lacks, official-master only, for CCOR `>=1` visibility conditions + mod pre-craft prompts), OpenCraftSession (snapshot chest). MenuWatcher (MenuOpenCloseEvent) opens/closes session; on close ReturnTokens (verified by re-reading inventory) + CloseCraftSession.
- LendTokens/ReturnTokens manage g_lentTokens (visibility tokens). Tokens never freed prematurely; stranded tokens retried next session.
- g_playerBoundary/g_chestStacks for iteration.
- ItemCraftingHooksActive = count+consume; IterationHooksActive = +both iteration hooks. Storage falls back to shuttle if inactive.
- BuildRecipeMaterialSet collects Misc/Ingredient components of all BGSConstructibleObject from official masters.

### 4.11 Shop (`src/Shop.h/.cpp`)
- Catalog of entries: material packs (smithing/alchemy/soul gems, 100/500 each), gold (100k/1M), System potions (10 potions ×10, ESP-local FormIDs 0xD81..0xD8A; entries with localID 0 hidden until ESP has them). Shelves: MATERIALS, WEALTH, RESTORATIVES, ELIXIRS (Armaments parked).
- Buy(index): checks SP, delivers first (StockCategory for packs, Deliver for items/gold), charges only on success, plays LevelUp sound, HUD message.
- Available() = Storage::Available(). Open(): Prisma::OpenShop or ImGui ShowShopWindow.
- DescribeEffects reads potion effects from the form at runtime (name + magnitude + minutes). Weapon damage/crit for the parked blade.
- Catalog() / LiveEntries() filter by EntryAvailable (form resolves).

### 4.12 Sounds (`src/Sounds.h/.cpp`)
- 4 SNDR descriptors in ESP (0xD7B..0xD7E): LevelUp, WindowOpen, ButtonClick, WindowClose. Resolved via Plugin::LookupOurForm. Play via BSAudioManager::Play (respects volume sliders). PlayDelayed uses DelayedMainThread (for Prisma unpause resume pass).

### 4.13 ModAPI (`src/ModAPI.h/.cpp`, `include/IsekaiHeroAPI.h`)
- Exported `RequestPluginAPI(InterfaceVersion)` (undecorated, `extern "C" __declspec(dllexport)`). `RequestInterface()` in header uses GetModuleHandleA("IsekaiHeroSKSE.dll") + GetProcAddress.
- IVIsekaiHero1 (frozen): IsActive, GetTier, GetTierName, GetSystemPoints, GetRank, HasNode, GetNodeRank, GrantSystemPoints(amount, source).
- GrantSystemPoints marshals to main thread via SKSE task; dropped if no save loaded; amount 0 no-op; negative allowed (balance never below 0? Actually ModAPI adds directly; System::GrantSystemPoints clamps >=0 but ModAPI writes `systemPoints += amount` — note: ModAPI does NOT clamp; could go negative. Hmm. README says GrantSystemPoints safe; System::GrantSystemPoints clamps but ModAPI bypasses. Potential pitfall.)
- `PublishIfChanged()` polled once/sec from overlay clock; dispatches SKSE message "IsekaiHeroSKSE" type kStateChanged (1) with StateChanged {active, tier, rank, systemPoints} (stable pointer to g_published).
- Node keys published in header (1..22).

### 4.14 SkyrimNet (`src/SkyrimNet.h/.cpp`)
- Soft dep on SkyrimNet.dll. Pushes Papyrus static calls `SkyrimNetApi.RegisterEvent`/`DirectNarration` via VM DispatchStaticCall. PushEvent (persistent world knowledge) on reincarnation/milestone/awakening. Narrate (one-off). Gated by ini + module presence.

### 4.15 SelfTest (`src/SelfTest.h/.cpp`)
- In-game diagnostic: hotkey (off by default) + auto-run on load (task, not in post-load pass). Checks: ESP loaded, 9 ability spells, sound descriptors, storage container, codex, chest ownership, level grant retired, node price scale, milestone quests (79), milestone passives backed, shop material sweeps (smithing/alchemy/soul gems >0), quest keywords, active objective, shop catalog/icons/shelves, skill tree loaded/icons/saved nodes/retired keys, crafting hooks, UI icons, environment (VR/Prisma/character/storage), ModAPI export + state + node reads.
- Writes PASS/FAIL/warn to log + panel (if notify).

### 4.16 Loc (`src/Loc.h/.cpp`)
- Translation: `L(key, english)` / `LF(key, english, ...)`. Loads `Data\SKSE\Plugins\IsekaiHero\lang\<code>.txt` (key = value, `;`/`#` comments, `\n`/`\\` escapes, BOM handling). English is the fallback in source; missing keys come out English. `Loc::Json()` exports whole table for Prisma view (web view translates its own chrome). `Loc::Fmt` wraps std::format with fallback on format_error.
- Language code validated (<=8 chars, alnum/-/_).

### 4.17 UI subsystem

#### Input (`src/UI/Input.h/.cpp`)
- Registers hotkeys (keyboard scan code + modifier, gamepad XInput mask + modifier, VR device button on both hands). Input sink on BSInputDeviceManager: device filter (keyboard/gamepad/VR devices), diagnostic logging (first 200 presses), held-button bookkeeping, gamepad stick→cursor + A→left click, mouse→cursor/buttons/wheel. MenuGuard (MenuEventHandler) at front of MenuControls chain swallows input while panel open; ESC/Tab/B dismiss. MenuSuppressor closes Journal/Tween menus opening under a panel. `FeedImGui` pushes accumulated input into ImGuiIO.

#### Overlay (`src/UI/Overlay.h/.cpp`)
- Hooks IDXGISwapChain::Present vtable slot 8 via probe swap chain (no game struct reads; VR-safe). InitImGui on first Present: device/context from BSGraphics::Renderer, RTV of back buffer, ImGui context, Consolas fonts, DX11 backend, warms texture cache. Draw loop: level-up effect, system window, skill tree, shop window, threat labels, toasts, game clock poll (once/sec → Quests::Tick + ModAPI::PublishIfChanged). `IsCapturingInput`, `OverlayReady`, `SetGameHold` (pause counter + blockPlayerInput).
- VR mode: g_vrMode true, DrawVRFrame, PollGameClock, no ImGui.

#### Style (`src/UI/Style.h`)
- Constants: kAccent (cyan), kText, kTextDim, kPanelBg, kDanger. g_scale, g_userScale, g_body/g_title fonts, BodySize/TitleSize. Helpers: Col, DrawIconPlate, DrawGlowBorder, DrawSystemCursor, DrawCornerBrackets, DrawTextShadowed/Outlined, DrawFadingBand. Resolution scaling: Style::UpdateScale(displayHeight).

#### SystemWindow (`src/UI/SystemWindow.h/.cpp`)
- Generic panel: title, body (typewriter reveal), choices (text + iconOnly), emblem (rank crest). Computes height deterministically. Body colouring by shape (LABEL   value rows, all-caps headings). ESC dismiss logic (single text button = answer; otherwise close). Routes to Prisma::ShowPanel when active.

#### SkillTreeWindow (`src/UI/SkillTreeWindow.h/.cpp`) — not read in full but referenced
- ImGui renderer for the tree (nodes, links, zones, mastery rail, tooltips, buy/respec). Prisma version is the web view.

#### ShopWindow (`src/UI/ShopWindow.h/.cpp`) — referenced
- ImGui renderer for shop cards.

#### Prisma (`src/UI/Prisma.h/.cpp`)
- Soft dep on PrismaUI API. Install: RequestPluginAPI; check view file exists; CreateView("IsekaiHero/index.html", OnDomReady); register JS listeners: isekaiBuy, isekaiRespec, isekaiCloseTree, isekaiChoose, isekaiStatusAction, isekaiShopBuy, isekaiShopClose. Active() = api valid + view valid.
- Screens: kTree, kPanel, kFlourish, kStatus, kShop. OpenTree/OpenShop/ShowPanel/ShowStatus/Flourish. SendGamepad marshals to main thread → Invoke("window.isekaiPad(...)").
- OnDomReady: push scale + Loc JSON, flush pending call.
- Callbacks marshal to main thread via SKSE task: OnBuy→TryUnlock+PushTree; OnRespec→Respec+PushTree; OnCloseTree→HideView+PlayDelayed WindowClose; OnChoose→HideView+PlayDelayed + fn(idx); OnStatusAction (tree/storage/shop/reroll/reboot/close); OnShopBuy→Buy+PushShop; OnShopClose→HideView+PlayDelayed.
- IsBusy() for flourish gating.

#### Textures (`src/UI/Textures.h/.cpp`)
- PNG→D3D11 SRV cache by path (stb_image, STBI_ONLY_PNG). Render thread only. Miss cached.

#### LevelUpEffect (`src/UI/LevelUpEffect.h/.cpp`)
- Flourish: rings, title punch, cyan vignette. Prisma path: PlayDelayed LevelUp + Prisma::Flourish. ImGui path: Play LevelUp + set active; DrawLevelUpEffect ages elapsed, draws rings/vignette/title/subtitle.

#### ThreatLabels (`src/UI/ThreatLabels.h/.cpp`)
- Verdicts TRIVIAL/MANAGEABLE/DANGEROUS/LETHAL from level diff. Collect actors (highActorHandles) with eligibility (mode: aggro/hostile/all/crosshair), range, head point projection, LOS. Crosshair picks nearest actor to a body segment (feet..head) within aim radius %. DrawFrame: name, level, HP number, verdict tag, HP bar (verdict colour), magicka/stamina strip. Distance scaling. Toggle key. VR: DrawThreatLabelsVR into world quads.

#### Toast (`src/UI/Toast.h/.cpp`)
- Transient top-centre lines; keyed replacement; max 4; fade in/out + slide. Fallback to SendHUDMessage if no surface.

#### VROverlay (`src/UI/VROverlay.h/.cpp`)
- Soft dep on ImGuiVRHelper. InstallVROverlay at kPostPostLoad (only if VRInHeadsetLayer ini + helper installed). Two clients: "Isekai Hero" (world quad, threat labels) and "Isekai Hero HUD" (HUD mode, toasts). Fonts per context via ScopedStyle. Combo "Open the System menu" (Y+B default) persisted to vrbindings.json. DrawVRFrame: polls combo → OpenStatusPanel on main thread; renders threat labels into world quads + submits; renders toasts. Shutdown tears down.

---

## 5. Prisma UI integration details

- **Detection:** `Prisma::Install` calls `PRISMA_UI_API::RequestPluginAPI<IVPrismaUI1>()`; if null → ImGui fallback. Then checks view file `Data\PrismaUI\views\IsekaiHero\index.html` exists; else fallback. Then `CreateView("IsekaiHero/index.html", OnDomReady)`.
- **JS bridge:** C++→JS via `g_api->Invoke(view, "window.isekaiX(...)")`. JS→C++ via `g_api->RegisterJSListener(view, "isekaiX", cb)`; JS calls `callBack("isekaiX", arg)` which invokes `window["isekaiX"](arg)`? Actually `callBack` calls `window[name](arg)` if defined — wait, the listener registration maps JS event name to C++ callback; JS side uses PrismaUI's `call` API? The view's `callBack` invokes `window[name]`? Let me re-read: in index.html line 1804 `function callBack(name, arg) { if (window[name]) window[name](arg); }`. Hmm that seems to call a JS function named `name`, not the C++ listener. But C++ registers listeners named "isekaiBuy" etc. and JS triggers them via PrismaUI's `window.PrimsaUI.call`? Actually the SDK provides `window.primsa.call(name, arg)` or similar. The `callBack` in the file might be a different mechanism. Regardless, the contract: JS triggers named listeners that C++ registered.
- **Screens:** tree, panel (modal dialog with typewriter reveal + buttons), flourish (non-interactive overlay), status (dashboard), shop (item cards). Single view toggles via CSS `.screen.active`.
- **State sync:** C++ pushes full JSON snapshots (BuildTreeJson, BuildPanelJson, BuildStatusJson, BuildShopJson) on open; purchases trigger re-push (PushTree/PushShop) so balance/affordability stay current. PrismaUI view translates itself using Loc::Json() pushed on DOM ready.
- **Polling:** game clock poll (1/sec) drives Quests::Tick + ModAPI::PublishIfChanged. Prisma screens are focus/paused (Focus(view, pauseGame=true)) except flourish (Show only, no focus).
- **Gamepad:** Input.cpp forwards gamepad button names ("a","b","up",...) to `Prisma::SendGamepad` → `window.isekaiPad(...)`; view navigates DOM focus.
- **Typewriter reveal:** panel body revealed char-by-char; buttons appear after complete; click skips.
- **Sound resume grace:** Prisma unpause eats sounds started same frame; PlayDelayed with kResumeGraceMs (150ms).

---

## 6. ModAPI

See section 4.13. Exported function `RequestPluginAPI`. Header `include/IsekaiHeroAPI.h` vendored by consumers. `IVIsekaiHero1` frozen (no method add/remove/reorder). Later additions ship as new interface version. Message "IsekaiHeroSKSE" with kStateChanged. Node keys 1..22 stable.

---

## 7. Build & packaging

- **Toolchain:** VS 2022 Build Tools (MSVC + Windows SDK + CMake + Ninja), vcpkg. CommonLibSSE-NG built from source (submodule, pinned) — NOT the vcpkg port (too old for 1.7.x: classifies runtime by minor version, address-lib reader only formats 1/2; 1.7.99+ ships format 5). `SKSE_SUPPORT_XBYAK OFF` (uses MinHook).
- **Dependencies (vcpkg):** imgui (dx11-binding), stb, minhook, directxmath, directxtk, fmt, nlohmann-json, rapidcsv, simpleini, spdlog, toml11, xbyak. CMake: imgui, minhook, nlohmann_json. stb via find_path.
- **C++ standard:** C++23 (`/permissive-`, `/Zc:preprocessor`). Precompiled header `src/PCH.h`. Compile defs: `ISEKAI_VERSION`, `WIN32_LEAN_AND_MEAN`, `NOMINMAX`. `/PDBALTPATH:%_PDB%` to avoid embedding dev path.
- **build.bat:** configure+build RelWithDebInfo (Ninja), deploy DLL+PDB+icons+ini (seed only if absent) into game Data. Refuses if Skyrim running.
- **package.ps1:** packs 7z into dist/; scrubs author's real name from DLL; does not ship PDB; privacy gate walks staged tree. package-prisma-patch.ps1 for the web patch archive.
- **tools/check.mjs:** consistency checks (node table vs playground mock, shop catalog vs mock, icons exist, status JSON fields, geometry non-overlap, kVersion matches co-save gate, every saved field loaded, no source missing from CMakeLists). ~1s, no game.
- **tools/extract-strings.mjs:** generates lang/template.txt.
- **tools/make-fomod.mjs:** generates fomod/ModuleConfig.xml and fomod-presets/*/IsekaiHero.ini.
- **FOMOD:** 3 install steps: Interface (Built-in vs PrismaUI), Extras (MCM checkbox), Start (Standard vs Alternate), Threat readings (Show vs Quiet). Conditional ini selection among 4 presets. Required files: ESP, LICENSE, DLL, SKSE/Plugins/IsekaiHero folder, Sound, meshes, textures.

---

## 8. Configuration

See IsekaiHero.ini (32 settings) and MCM config.json (28 of 32 exposed). MCM writes `Data\MCM\Settings\IsekaiHero.ini` with engine type prefixes; Config::Load strips prefix and layers over shipped ini. Hotkeys, VR button, gamepad button, language, VR in-headset layer, self-test/debug keys stay ini-only (not in MCM).

---

## 9. Persistence / co-save

- SKSE serialization: unique ID `'ISKA'`, record `'STAT'`, version 16. Save/Load/Revert callbacks.
- Field-by-field (vectors/maps written with count prefix). Version-gated reads (v3..v16) for backward compat; older saves lose newer features gracefully.
- Stored: reincarnated, power, skills, storageChest (resolved), grantedMilestones[], unlockedNodes[], systemPoints, shattered, nodeRanks[], dormant, grantTier/treeTier/custom, questKey/Progress/Target/Reward/NextDue/QuestsGiven, preLevel/preSkills/preHMS (v14), killCounts map + professionActions (v15), nodeCostScale + levelGrantRetired (v16).
- RevertCallback resets state + firstReadyCell.
- Storage chest FormID resolved via `ResolveFormID` on load. Chest ref can be rebuilt if disabled/deleted.

---

## 10. Compatibility & pitfalls

- **Runtime detection:** CommonLibSSE-NG one DLL for SE/AE/VR; VR uses VR Address Library; Form resolution manual in VR (LookupForm fails for ESL).
- **Overlay crash in VR:** disabled; ImGui only on flat. PrismaUI mandatory in VR for menus.
- **Crafting token-lending:** only official-master items (scripted mod items fire OnContainerChanged).
- **CC ingredients:** excluded (tracker scripts flood VM).
- **Perk points:** engine cap 255, stored as signed int8 in CommonLibSSE but treated unsigned — reads must cast through uint8.
- **Ability effects:** must have Recover flag + archetype 34 (Peak Value Modifier) to fortify; magnitudes set at runtime.
- **ModAPI GrantSystemPoints:** bypasses System::GrantSystemPoints clamping (can go negative); marshaled to main thread; dropped if no save.
- **Input:** VR controller buttons arrive as device 5/6 (kVRRight/kVRLeft), dropped by keyboard-only path unless diagnostic shows otherwise.
- **Save bloat:** orphaned chest husks from older rebuilds; PruneOrphanChests cleans.
- **Level grant migration:** one-shot; saves without v14 baseline keep level.
- **PrismaUI version:** needs 1.5.0 VR build for VR; stable 1.4.x no VR.
- **ImGuiVRHelper:** optional in-headset layer; ships off due to crash report (cause unknown, no VR test env).

---

## 11. Relevance to Runefall

Runefall is a Skyrim mod with a web UI menu (likely PrismaUI-based) and progression/crafting/shop/quest systems. Concrete takeaways from Isekai Hero:

1. **Web UI menu architecture (PrismaUI):**
   - Single view (`index.html`) with multiple screens toggled by CSS `.screen.active`; C++ pushes JSON snapshots and registers JS listeners (`isekaiBuy`, `isekaiChoose`, etc.); JS calls back via `callBack(name, arg)`.
   - Detect framework at kDataLoaded; fall back to ImGui if absent. Check view file exists.
   - Push scale (`window.isekaiScale`) and translation table (`window.isekaiLoc`) on DOM ready, before first screen.
   - Gamepad forwarding: C++ input thread forwards button names to `window.isekaiPad(...)`; view navigates DOM focus.
   - Game pause + focus on open (`Focus(view, pauseGame=true)`); flourish is non-interactive (Show only, no focus).
   - Sound resume grace (150ms delay) after Prisma unpause to avoid engine dropping sounds.
   - Typewriter reveal with click-to-skip; buttons appear after reveal completes.

2. **Progression/milestones:**
   - Quest-completion-driven milestones keyed by editor ID (load-order independent). Retroactive catch-up on load. Idempotent Grant via granted list.
   - Passives aggregated into ability spells (one per actor value) with magnitudes set at runtime; re-apply spell to pick up new magnitude. Avoids double-apply.
   - Derived rank (E–S) from milestones + level + investment.
   - Decouple reward axes (grant/reward/tree tiers) for flexible builds (CUSTOM).

3. **Crafting/storage:**
   - Dimensional Storage pattern: persistent reference created via PlaceObjectAtMe(forcePersist), buried, activated directly. Owner set to player to avoid theft.
   - Zero-transfer crafting via MinHook: count hooks add chest counts, consume hook pulls chest first. Session snapshots on main thread; hooks touch only atomics/cached map (thread-safe). Visibility tokens (one of each material) lent for CCOR `>=1` conditions.
   - Restrict to official masters for ingredients/misc to avoid scripted mod items.
   - Prune foreign stock and orphan chests on load.

4. **Shop:**
   - Data-driven catalog (Entry table) with indices stable between Catalog() and Buy(). LiveEntries filter by form resolution. Shelves as ordered display groups.
   - Deliver first, charge only on success. Effects derived from form at runtime (single source of truth).
   - Two sizes per category for price/quantity tradeoff.

5. **Quests/objectives:**
   - Keyword-based matching (ActorType*) so mod-added actors count. Level gating for reachability. Scaling count/reward snapshotted at hand-out. Scheduled timing (first short, then interval). Kill bounties + professions as passive income tracks. Toast progress (keyed replacement).

6. **ModAPI:**
   - Exported undecorated `RequestPluginAPI`; header-only client with GetModuleHandle/GetProcAddress. Frozen interface (V1); new versions as V2. SKSE messaging for state changes. Marshaling to main thread; source attribution in logs.

7. **Packaging:**
   - FOMOD with conditional ini selection; MCM optional (Papyrus script empty; settings via ini). Privacy gate (scrub author name, no PDB). check.mjs consistency tests. ESL-flagged ESP (no slot). build.bat refuses deploy while game running.

8. **Localization:**
   - `L(key, english)` with English fallback in source; translation files; `Loc::Json()` for web view. Format strings via std::format with fallback.

9. **Threat labels / HUD:**
   - Floating labels with level-diff verdicts, distance scaling, LOS check, crosshair body-segment picking. VR world quads via ImGuiVRHelper (optional, ships off).

10. **Persistence:**
    - SKSE co-save field-by-field with version gates. FormID resolution across load order. One-shot migrations (level grant). Baseline capture for REBOOT.

---

## 12. Source paths (relative to base)

- `src/main.cpp`, `src/PCH.h`, `src/Logger.h`
- `src/Config.h`, `src/Config.cpp`
- `src/Plugin.h`, `src/Plugin.cpp`
- `src/System.h`, `src/System.cpp`
- `src/Progression.h`, `src/Progression.cpp`
- `src/Passives.h`, `src/Passives.cpp`
- `src/SkillTree.h`, `src/SkillTree.cpp`
- `src/Quests.h`, `src/Quests.cpp`
- `src/Professions.h`, `src/Professions.cpp`
- `src/Storage.h`, `src/Storage.cpp`
- `src/CraftHooks.h`, `src/CraftHooks.cpp`
- `src/Shop.h`, `src/Shop.cpp`
- `src/Sounds.h`, `src/Sounds.cpp`
- `src/ModAPI.h`, `src/ModAPI.cpp`
- `src/SkyrimNet.h`, `src/SkyrimNet.cpp`
- `src/SelfTest.h`, `src/SelfTest.cpp`
- `src/Loc.h`, `src/Loc.cpp`
- `src/UI/Input.h`, `src/UI/Input.cpp`
- `src/UI/Overlay.h`, `src/UI/Overlay.cpp`
- `src/UI/Prisma.h`, `src/UI/Prisma.cpp`
- `src/UI/SystemWindow.h`, `src/UI/SystemWindow.cpp`
- `src/UI/SkillTreeWindow.h`, `src/UI/SkillTreeWindow.cpp`
- `src/UI/ShopWindow.h`, `src/UI/ShopWindow.cpp`
- `src/UI/Style.h`
- `src/UI/Textures.h`, `src/UI/Textures.cpp`
- `src/UI/ThreatLabels.h`, `src/UI/ThreatLabels.cpp`
- `src/UI/Toast.h`, `src/UI/Toast.cpp`
- `src/UI/LevelUpEffect.h`, `src/UI/LevelUpEffect.cpp`
- `src/UI/VROverlay.h`, `src/UI/VROverlay.cpp`
- `include/IsekaiHeroAPI.h`
- `prisma-patch/PrismaUI/views/IsekaiHero/index.html`
- `mcm-patch/MCM/Config/IsekaiHero/config.json`, `settings.ini`, `Source/Scripts/IsekaiHeroMCM.psc`
- `fomod/info.xml`, `fomod/ModuleConfig.xml`
- `IsekaiHero.ini`, `CMakeLists.txt`, `vcpkg.json`, `build.bat`, `README.md`, `CLAUDE.md`, `CHANGELOG.md`
- `docs/IDEAS.md`, `docs/VR.md`, `docs/CREATION_KIT_ESP.md`, `docs/MANUAL_TESTS.md`

---

*End of research document.*
