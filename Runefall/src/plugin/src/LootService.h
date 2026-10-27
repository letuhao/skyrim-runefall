#pragma once
#include "Core.h"
#include "ItemMetadata.h"
#include "LootEngine.h"

namespace Runefall
{
	class LootService
	{
	public:
		static LootService& Get();

		void RegisterHooks();
		void ResetSessionState();
		void OnActorDeath(RE::Actor* a_actor);

		// P4.2 runtime entry points. Every one of these is called from a main-thread SKSE
		// task queued by HookManager, never from inside an engine dispatch callback.
		void OnObject3DLoaded(RE::FormID a_refFormID);
		void OnContainerMenuOpened();
		void OnInventoryMenuOpened();
		void OnResetRef(RE::FormID a_refFormID);
		void OnCellFullyLoaded(RE::FormID a_cellFormID);
		void OnItemAddedToPlayer(RE::FormID a_baseObject, std::int32_t a_count, RE::FormID a_oldContainer);
		void OnQuestStage(RE::FormID a_questFormID, std::uint16_t a_stage);

		// P3.7 quest-table consumer. Also exposed to Papyrus for scripted quest rewards.
		bool GrantQuestReward(std::int32_t a_band, std::string_view a_source);

		// Test hook: adds one rank-rune MISC item to the player; the item-added sink must
		// aggregate it into the wallet and remove the physical item.
		bool DebugGiveRuneItem(std::int32_t a_rank);

		// Debug/probe path: rolls one item, attaches metadata + runtime enchantment and adds it
		// to the player inventory. Used by the batch probe; safe to call once per load.
		[[nodiscard]] bool InjectProbeItem();

		// Pure policy helpers (covered by SelfTest).
		[[nodiscard]] static std::int32_t ContainerTierFor(std::string_view a_editorID);
		[[nodiscard]] static std::int32_t QuestBandFor(std::int32_t a_questType);

		[[nodiscard]] std::uint32_t ContainersGenerated() const noexcept { return containersGenerated; }
		[[nodiscard]] std::uint32_t ContainersSkipped() const noexcept { return containersSkipped; }
		[[nodiscard]] std::uint32_t ContainerItemsGranted() const noexcept { return containerItemsGranted; }
		[[nodiscard]] std::uint32_t ActorRanksAssigned() const noexcept { return actorRanksAssigned; }
		[[nodiscard]] std::uint32_t QuestRewardsGranted() const noexcept { return questRewardsGranted; }
		[[nodiscard]] std::uint32_t QuestItemsGranted() const noexcept { return questItemsGranted; }
		[[nodiscard]] std::uint32_t RuneItemsAggregated() const noexcept { return runeItemsAggregated; }
		[[nodiscard]] std::uint32_t CellsPruned() const noexcept { return cellsPruned; }

		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		[[nodiscard]] RE::TESBoundObject* PickBaseObject(Rng& a_rng);
		[[nodiscard]] bool                SpawnForActor(RE::Actor* a_actor);
		[[nodiscard]] bool EnsureContainerLoot(RE::TESObjectREFR* a_ref);
		void               EnsureActorRank(RE::Actor* a_actor);
		[[nodiscard]] std::int32_t ResolveActorRank(RE::Actor* a_actor);
		[[nodiscard]] std::uint8_t ResolveActorStage(RE::Actor* a_actor);
		[[nodiscard]] bool GrantLootItem(
			RE::TESObjectREFR* a_owner,
			std::int32_t       a_rank,
			std::int32_t       a_rfTier,
			std::int32_t       a_ilvl,
			std::uint16_t      a_tags,
			std::string_view   a_slotGroup,
			Rng&               a_rng,
			std::int32_t       a_minRarity,
			float              a_rarityBias,
			const char*        a_context);
		[[nodiscard]] static RE::SpellItem*       RankSpell(std::int32_t a_rank);
		[[nodiscard]] static RE::TESObjectMISC*   RuneItem(std::int32_t a_rank);
		[[nodiscard]] static std::int32_t         CountItem(RE::TESObjectREFR* a_owner, RE::TESBoundObject* a_base);
		[[nodiscard]] static std::int32_t         RankFromSpells(RE::Actor* a_actor);
		[[nodiscard]] static std::int32_t         RollActorRank(RE::FormID a_formID, std::int32_t a_rfTier);

		std::unordered_set<RE::FormID>         processedActors;
		std::unordered_map<RE::FormID, std::int32_t> actorRanks;
		std::unordered_map<RE::FormID, std::uint8_t> actorStages;
		std::unordered_map<RE::FormID, std::uint32_t> cellLootCounts;
		bool                                   hooksRegistered = false;

		std::uint32_t containersGenerated = 0;
		std::uint32_t containersSkipped = 0;
		std::uint32_t containerItemsGranted = 0;
		std::uint32_t actorRanksAssigned = 0;
		std::uint32_t questRewardsGranted = 0;
		std::uint32_t questItemsGranted = 0;
		std::uint32_t runeItemsAggregated = 0;
		std::uint32_t cellsPruned = 0;
	};
}
