# Crafting, Economy & Quest UX Research

Reference-mod survey for Runefall. All source paths are under `D:\Works\source\skyrim-moddings\rpg-mechanism\` unless noted. This file is research only; reference mods are untouched.

## Focus and relevance

Runefall pillars touched here: material hunting / targeted crafting, forge activator flow, salvage/repair economy, quest hooks for *Call of the Hunt*, item tagging/quest-item protection, and MCM-driven configuration.

---

## 1. Fourth Era Spell-Crafting (`Fourth Era Spell-Crafting-157287-1-0-1-1763966322`)

**Path:** `Fourth Era Spell-Crafting-157287-1-0-1-1763966322\Fourth Era Spell-Crafting\`

**Architecture observed**
- `EM03SpellCrafting.esp`, `EM03SpellCrafting.dll` (SKSE plugin), Papyrus: `EM03CraftSpellScript.pex`, `EM03LearnEffectsScript.pex`, `EM03QuestScript.pex`, `EM03SKSEFunctions.pex`, `EM03SkyUIMCM.pex`, plus dialogue fragments.
- `Seq\EM03SpellCrafting.seq`, voiced lines (`MaleOldGrumpy`).
- DLL strings reveal an ImGui/SKSE fullscreen **spell crafting menu**: known spells/effects browser by school, craft spell from effects, magicka cost and gold cost calculation, tome of spellmaking inventory item, perk gating ("Must Have Perk to Add its Effect if it's From a Self Spell"), override of delivery/visuals/school from costliest effect, erase spell, name validation.
- MCM-ish toggles exposed in DLL strings: fullscreen menus, show effect cost in original spell, gold cost per magicka cost, next-effect cost multiplier, tome acquisition gold cost.

**Relevance to Runefall**
- Not item crafting, but a strong pattern for a **forge activator UI**: fullscreen crafting menu, cost preview before commit, gating by perk/knowledge, confirmation prompts ("spell has no name, are you sure?").
- Economy pattern: dual cost (magicka + gold) and tunable multiplier for chained effects; useful for targeted crafting cost models.
- MCM surface (`EM03SkyUIMCM`) shows how to expose crafting cost knobs.

---

## 2. Crafting Pulls Resource From Containers (`CraftingPullsResourceFromContainers-135613-1-1-1-1754824867`)

**Path:** `CraftingPullsResourceFromContainers-135613-1-1-1-1754824867\`

**Architecture observed**
- `SKSE\Plugins\CraftingPullFromContainers.dll` + `CraftingPullFromContainers.ini`.
- INI:
  - `Range=500`
  - `IgnoreOwnership=false`
  - `IncludeMerchantChest=false`
  - `PermaLinks=` (empty)

**Relevance to Runefall**
- Directly supports **material hunting / targeted crafting**: pulls crafting materials from nearby containers into the crafting menu.
- Config knobs to mirror: pull range, ownership respect, merchant-chest exclusion, persistent "PermaLinks" for always-pull containers.
- Good baseline for forge-adjacent resource pulling without changing container ownership.

---

## 3. Dealing With Ysolda (`Dealing With Ysolda 194285 2 2026-10-07T21-48Z Dc5uGkoFM`)

**Path:** `Dealing With Ysolda 194285 2 2026-10-07T21-48Z Dc5uGkoFM\`

**Architecture observed**
- `DealingWithYsolda.esp`, 8 Papyrus sources (PEX), SEQ, voice assets; no native SKSE DLL.
- Requires SKSE, PapyrusUtil (ActorUtil/StorageUtil), Fuz Ro D'oh optional; GPL-3.0.
- Readme: 11-chapter quest around Ysolda/Khajiit/skooma; changes Ysolda dialogue/quests/packages and touches Hulda/Arcadia/Madran/Whiterun imprisonment; save migration notes.

**Relevance to Runefall**
- Quest-authoring reference for **Call of the Hunt** hooks: multi-chapter quest, dialogue-gated progression ("become friend before asking"), faction/NPC entanglement, save-update migration.
- Shows compatibility cautions when editing vanilla NPC/quest records.

---

## 4. Container Distribution Framework (`Container Distribution Framework 120152 3.1.4 2026-10-24T04-57Z 9LMpgbfIm`)

**Path:** `Container Distribution Framework 120152 3.1.4 2026-10-24T04-57Z 9LMpgbfIm\`

**Architecture observed**
- `ContainerDistributionFramework.dll` + `ContainerDistributionFramework.ini`.
- INI: `fMaxRefLookupDistance = 25000.0` (marker/location substitution search distance).
- Cross-ref with loot research.

**Relevance to Runefall**
- Material-hunting support: distribute crafted/loot items into containers by location/marker.
- Long lookup distance matters for large worldspaces; useful for targeted crafting material placement.

---

## 5. Bounty Quests Redone (`Bounty Quests Redone-100630-2-0-2-1710161173`)

**Path:** `Bounty Quests Redone-100630-2-0-2-1710161173\`

**Architecture observed**
- `Bounty Quests Redone.esp`, native SKSE DLL, Papyrus sources: `BQR.psc`, `BQR_SCPT_Boss.psc`, `BQR_SCPT_CompleteQuests.psc`, etc.
- Native functions: `CompleteQuests`, `RewardPlayer`, `StartEveryQuest`, `StartRandomQuest`, `SetQuestCount`, `UpdateDialogueMenu`.
- Data files: `Settings.json` (concurrent quest limit, disable vanilla bounties, actor/faction patches), `Rewards.json` (reward items + quantity/base/max + type multipliers), quest JSON catalogs by region/type.
- Boss death sets quest stage 20.

**Relevance to Runefall**
- Data-driven quest catalog + native reward function pattern is ideal for **Call of the Hunt** bounties.
- Economy knobs: reward base/max, quantity, type multipliers (e.g. dragon 4.0x), concurrent quest limit, vanilla-disable toggle.
- Boss/death stage transitions can gate salvage/crafting rewards.

---

## 6. Quest Request 2.1.0 (`Quest Request 2.1.0 FOMOD 192805 2.1.0 2026-10-08T06-35Z OyYrPu7zz`)

**Path:** `Quest Request 2.1.0 FOMOD 192805 2.1.0 2026-10-08T06-35Z OyYrPu7zz\`

**Architecture observed**
- SKSE DLL + MeridianUI + Open Animation Replacer + SKSE Menu Framework; 3D paper meshes/animations; emblem PNGs.
- `QuestRequest.ini`: master `Enabled`, `Sounds`, `VolumePercent`, `PollMilliseconds`, category toggles (`Royal`, `Main`, `Guild`, `Daedric`, `War`, `DLC`, `Misc`, `Side`, `Stewards`).
- Turns eligible dialogue-accepted quests into parchment commissions: gameplay pauses, notice UI with title/journal/giver/location/destination/objectives; Enter/E dismisses.
- Stewards Quest category uses live steward alias + job membership; Jarl detection uses job membership.
- Compatibility: only dialogue-linked quests; long text may truncate; blackout issue noted.

**Relevance to Runefall**
- Strong **quest UX** reference for Call of the Hunt intake: commission notice, category toggles, MCM/INI settings, repeatable-quest handling.
- Shows how to detect quest giver role (steward/Jarl) rather than hardcoded names—useful for dynamic quest hooks.

---

## 7. Physical Quest Journal (`PhysicalQuestJournal 0.2.0 Player 194189 7 2026-10-12T15-17Z s6Og0d5QX`)

**Path:** `PhysicalQuestJournal 0.2.0 Player 194189 7 2026-10-12T15-17Z s6Og0d5QX\`

**Architecture observed**
- `PhysicalQuestJournal.dll` + `config.json`.
- Config: enabled, contentsOnOpen, bookOnly, hotkey/controllerHotkey, menuMode, timeMode/slowTime, first/third person, font/textSize, position/scale, pageSpeed, bookmarks, recent, completed, miscellaneous, mapShortcut, journalItem, highContrast, reducedMotion, closeOnCombat, debug.

**Relevance to Runefall**
- Quest tracker UX: physical journal item, hotkey, slow-time reading mode, bookmarks/recents/completed lists, accessibility options.
- Good model for a Runefall quest journal MCM.

---

## 8. Quest History (`Quest History 1.0.1 194624 1.0.1 2026-10-09T14-21Z XnIqaCgD3`)

**Path:** `Quest History 1.0.1 194624 1.0.1 2026-10-09T14-21Z XnIqaCgD3\`

**Architecture observed**
- Single `QuestHistory.dll`; no docs/config in package.

**Relevance to Runefall**
- Likely quest-history tracking; useful concept for Call of the Hunt history/audit, but implementation details unavailable from this package.

---

## 9. Hide Those Futile Quests SE (`Hide Those Futile Quests SE-76361-2-0-1664618474`)

**Path:** `Hide Those Futile Quests SE-76361-2-0-1664618474\`

**Architecture observed**
- `htfq_se` SKSE plugin + Papyrus `HTFQConfig` (SKI_ConfigBase) + `HTFQNative`.
- FormLists: `AllQuests`, `AllMiscQuests`, `HiddenQuests`, `HiddenMiscQuests`.
- MCM toggles per quest; hiding sets quest inactive.
- Native: `UpdateQuests`, `GetQuestText`, `GetFirstObjectiveText`.

**Relevance to Runefall**
- Quest clutter control: hide low-value quests from tracker; MCM per-quest toggles.
- Useful for Call of the Hunt to suppress noise while keeping target quests visible.

---

## 10. Whose Quest Is It Anyway (`Whose Quest Is It Anyway-23581-1-5-0-1682374361`)

**Path:** `Whose Quest Is It Anyway-23581-1-5-0-1682374361\`

**Architecture observed**
- `WhoseQuestIsItAnyway.dll`, `config.toml` (`printQuestFormID`, `totalMessageCount`, `useRandomMessages`), translations.
- Messages: "Quest item locked by: {}", "You remember this has something to do with {}", etc.
- Prevents dropping/selling quest items and attributes them to owning quest.

**Relevance to Runefall**
- Item tagging/protection: mark quest-critical crafted/salvaged items and block accidental loss.
- Configurable notification pool can be reused for crafting/quest item feedback.

---

## 11. Notify Events (`Notify Events 175848 1.0.0a 2026-10-16T18-01Z 6XybdaRuC`)

**Path:** `Notify Events 175848 1.0.0a 2026-10-16T18-01Z 6XybdaRuC\`

**Architecture observed**
- Single `NotifyEvents.dll`; hooks animation graph; loads custom events (`Eventsloaded: {}`).
- No config/docs in package.

**Relevance to Runefall**
- Animation-event plumbing for forge/crafting interactions; could trigger VFX/SFX on crafting, but mostly infrastructure.

---

## 12. Optional Quick Start (`Optional Quick Start - SE-63953-1-2-2-1771715657`)

**Path:** `Optional Quick Start - SE-63953-1-2-2-1771715657\`

**Architecture observed**
- `Optional Quick Start.esp`, BSA, ACHLIST; no docs/config.

**Relevance to Runefall**
- Alternate-start option candidate; could provide a Runefall-specific onboarding start with initial material/crafting loadout.

---

## 13. Alternate Start - Live Another Life (`Alternate Start - Live Another Life-272-4-2-6a`)

**Source archive:** `Alternate Start - Live Another Life-272-4-2-6a-1772996477.7z` (extracted to temp for reading)

**Architecture observed**
- `alternate start - live another life.esp`, BSAs, INI.
- Extension framework: ESMify ESP, add dialogue branch to `ARTH_LALChargenQuest`, condition to `ARTH_LALMaraStatueActivator`, attach `TopicInfo` script calling `ChargenQuest.SetAddonQuestStage(Int, Quest)`, stage fragment outfits/factions/moves player, ends with `ARTHLALRumorsOfWarQuest.RegisterForSingleUpdate(0.25)`.
- Supports custom starts without locking to a specific LAL version.

**Relevance to Runefall**
- **Call of the Hunt** onboarding: custom start option with initial faction/materials/forge access.
- Pattern for modder extensions is reusable for Runefall start scenarios.

---

## 14. Calamity 2.3.0 (`Calamity 2.3.0 (MO2 Vortex) 193193 2.3.0`)

**Source archive:** `Calamity 2.3.0 (MO2 Vortex) 193193 2.3.0 2026-10-27T07-47Z VMSnJrVTn.zip` (extracted to temp for reading)

**Architecture observed**
- `CalamityAffixes.esp`, SKSE DLL, KID/DISTR INIs, Prisma UI views, MCM Helper config (`config.json`, `keybinds.json`, `settings.ini`), Papyrus sources.
- Crafting economy: affixes are not auto-rolled on crafted/picked-up gear; currencies drop from eligible hostile corpses:
  - Runeword fragments (8%, pity after 99 misses)
  - Reforge orbs (12%)
  - Identify scrolls (20%)
  - Scouring orbs (4%)
- Currency economy is corpse-only (no containers/world drops); unique/named/boss/dragon have guaranteed rewards.
- Runeword crafting: 95 recipes, 33 runes with weights; Prisma UI panel for base selection, recipe progress, rune insertion; MCM toggles.
- MCM settings include proc multiplier, validation interval, DoT safety, language, panel hotkey, recover missing currency.
- Data-driven: `affixes.json`, `runtime_contract.json`, generator workflow.

**Relevance to Runefall**
- Very high. Demonstrates **targeted crafting** (runewords/affixes via Prisma panel), **material hunting** (currency from combat), **economy balancing** (per-corpse chances, pity, guarantees), and **MCM** patterns.
- Forge activator could mirror: select base item, show recipe progress, insert runes, confirm; currency gained from Call of the Hunt targets.

---

## 15. QOLItemTags (`QOLItemTags-142328-1-2-1742100652`)

**Path:** `QOLItemTags-142328-1-2-1742100652\`

**Architecture observed**
- `QOLItemTags.dll` + `QOLItemTags.ini`.
- INI: `iNumOfRecentItemsDisplayed=5`, `sQuestTag=(Q)`, `sRecentTag=(N)`, `iMenuRefreshMillisecondDelay=100`, log level.

**Relevance to Runefall**
- Item tagging UX: tag quest items and recent items in menus; configurable tags and refresh delay.
- Useful for highlighting salvage/crafting-relevant or quest-critical items.

---

## 16. Quick Swap (`Quick Swap 181683 1.0.1 2026-10-03T22-33Z uhRBKsPtt`)

**Path:** `Quick Swap 181683 1.0.1 2026-10-03T22-33Z uhRBKsPtt\`

**Architecture observed**
- Nemesis behavior injection (`Nemesis_Engine\mod\qckswap\...`) + BehaviorDataInjector JSON + `Language.json`.
- Animation graph changes for quick weapon swap.

**Relevance to Runefall**
- Combat/UX polish only; not core to crafting/economy/quests. Could inform animation feedback for forge interactions.

---

## 17. Equipment Durability System NG (`EquipmentDurabilitySystem NG 109899 5.1 2026-09-03T06-00Z pQ0AZ3juw`)

**Path:** `EquipmentDurabilitySystem NG 109899 5.1 2026-09-03T06-00Z pQ0AZ3juw\`

**Architecture observed**
- `EquipmentDurability.dll`, `EquipmentLists.ini` (material mapping for durability/repair), translation JSON, Papyrus native `EquipmentDurability` functions, SkyUI groups.
- Maps vanilla/mod armor/weapon materials to durability entries.

**Relevance to Runefall**
- Salvage/repair economy: durability and material lists can drive salvage yields and repair costs.
- Cross-ref with crafting: material mapping informs targeted crafting requirements.

---

## Cross-cutting takeaways for Runefall

- **Targeted crafting**: combine Fourth Era's cost preview/gating with Calamity's Prisma panel and recipe progress.
- **Resource pulling**: adopt CraftingPullFromContainers range/ownership/merchant-chest knobs.
- **Material hunting**: use CDF distribution + Calamity corpse-only currency model for Call of the Hunt rewards.
- **Quest UX**: Quest Request commission notices + Physical Journal settings + Hide Futile Quests filtering.
- **Quest hooks**: Bounty Quests Redone data-driven catalogs/rewards + Alternate Start extension framework.
- **Item tagging/protection**: QOLItemTags tags + Whose Quest quest-item locking.
- **MCM**: Calamity MCM Helper + SkyUI config patterns for all knobs.

*Generated from read-only inspection of reference mods; no reference files modified.*
