#include "LootService.h"

#include "CoSave.h"
#include "Config.h"
#include "DataStore.h"
#include "DynamicEnchantment.h"
#include "FormCatalog.h"
#include "FormResolver.h"
#include "HuntEngine.h"
#include "ItemRegistry.h"
#include "LootEngine.h"
#include "Progression.h"
#include "RuneForge.h"
#include "TierEngine.h"
#include "StageEngine.h"
#include "RuntimeContentResolver.h"
#include "RaceClassifier.h"
#include "ActorPackageEngine.h"
#include "ActorEquipmentDistributor.h"
#include "WorldBandEngine.h"

#include "RE/C/ContainerMenu.h"

#include <algorithm>
#include <format>

namespace Runefall
{
	namespace
	{
		class DeathSink : public RE::BSTEventSink<RE::TESDeathEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>*) override
			{
				if (!a_event)
					return RE::BSEventNotifyControl::kContinue;
				logger::info("death: event dead={} actor={}", a_event->dead ? "true" : "false",
					a_event->actorDying ? std::to_string(a_event->actorDying->GetFormID()) : "null");
				if (!a_event->dead || !a_event->actorDying)
					return RE::BSEventNotifyControl::kContinue;
				auto dying = a_event->actorDying;  // NiPointer keeps the temporary ref alive across the task
				if (auto* tasks = SKSE::GetTaskInterface()) {
					tasks->AddTask([dying]() {
						if (!dying) {
							logger::warn("death: dying ref expired before task");
							return;
						}
						auto* actor = dying->As<RE::Actor>();
						if (!actor) {
							logger::warn("death: dying ref is not an actor");
							return;
						}
						logger::info("death: task actor={:08X} dead={}", actor->GetFormID(), actor->IsDead(false) ? "true" : "false");
						LootService::Get().OnActorDeath(actor);
					});
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		DeathSink g_deathSink;

		const char* rarityNameFor(std::int32_t a_rarity)
		{
			static const std::array<const char*, 6> kNames = { "Common", "Refined", "Exquisite", "Radiant", "Mythic", "Runeforged" };
			if (a_rarity < 0)
				a_rarity = 0;
			if (a_rarity > 5)
				a_rarity = 5;
			return kNames[static_cast<std::size_t>(a_rarity)];
		}

		std::string BuildDisplayName(const ItemMetadata& a_metadata, std::string_view a_baseName, std::string_view a_rarityName)
		{
			if (a_metadata.rarity == 0)
				return std::string{ a_baseName };
			return std::format("{} {}", a_rarityName, a_baseName);
		}

		struct BasePoolEntry
		{
			const char* editorId;
			RE::FormID  formId;
		};

		constexpr std::array<BasePoolEntry, 5> kBasePool = {
			BasePoolEntry{ "IronSword", 0x00012EB7 },
			BasePoolEntry{ "IronDagger", 0x0001397E },
			BasePoolEntry{ "IronWarAxe", 0x00013790 },
			BasePoolEntry{ "HuntingBow", 0x00013985 },
			BasePoolEntry{ "IronMace", 0x00013982 },
		};

		RE::TESBoundObject* ResolveBaseObject(std::string_view a_editorId)
		{
			for (const auto& entry : kBasePool) {
				if (a_editorId != entry.editorId)
					continue;
				if (auto* form = RE::TESForm::LookupByID(entry.formId)) {
					if (auto* weapon = form->As<RE::TESObjectWEAP>())
						return static_cast<RE::TESBoundObject*>(weapon);
				}
				if (auto* weapon = FormResolver::ResolveAs<RE::TESObjectWEAP>(a_editorId))
					return static_cast<RE::TESBoundObject*>(weapon);
			}
			return nullptr;
		}

		std::int32_t RankFromSpellsImpl(RE::Actor* a_actor)
		{
			if (!a_actor)
				return -1;
			for (int rank = 5; rank >= 1; --rank) {
				const auto name = std::format("RF_SPEL_Rank_{}", rank);
				if (auto* spell = FormResolver::ResolveAs<RE::SpellItem>(name)) {
					if (a_actor->HasSpell(spell))
						return rank;
				}
			}
			return -1;
		}
	}

	RE::TESBoundObject* LootService::PickBaseObject(Rng& a_rng)
	{
		// v1 uses a small vanilla weapon pool; the final drop table selects by actor type/slot.
		for (int attempt = 0; attempt < 4; ++attempt) {
			const auto index = static_cast<std::size_t>(a_rng.Range(0, static_cast<std::int32_t>(kBasePool.size()) - 1));
			if (auto* object = ResolveBaseObject(kBasePool[index].editorId))
				return object;
		}
		return nullptr;
	}

	LootService& LootService::Get()
	{
		static LootService singleton;
		return singleton;
	}

	void LootService::ResetSessionState()
	{
		processedActors.clear();
		actorRanks.clear();
		cellLootCounts.clear();
		logger::info("loot: session state reset");
	}

	void LootService::RegisterHooks()
	{
		if (hooksRegistered)
			return;
		auto* holder = RE::ScriptEventSourceHolder::GetSingleton();
		if (!holder) {
			logger::warn("loot: ScriptEventSourceHolder unavailable; death hook not registered");
			return;
		}
		holder->AddEventSink<RE::TESDeathEvent>(&g_deathSink);
		hooksRegistered = true;
		logger::info("loot: death event sink registered");
	}

	RE::SpellItem* LootService::RankSpell(std::int32_t a_rank)
	{
		if (a_rank < 1 || a_rank > 5)
			return nullptr;
		static std::array<RE::SpellItem*, 5> cache{};
		auto& slot = cache[static_cast<std::size_t>(a_rank - 1)];
		if (!slot)
			slot = FormCatalog::Get().LookupAs<RE::SpellItem>(std::format("RF_SPEL_Rank_{}", a_rank));
		return slot;
	}

	RE::TESObjectMISC* LootService::RuneItem(std::int32_t a_rank)
	{
		if (a_rank < 1 || a_rank > 5)
			return nullptr;
		static std::array<RE::TESObjectMISC*, 5> cache{};
		auto& slot = cache[static_cast<std::size_t>(a_rank - 1)];
		if (!slot)
			slot = FormCatalog::Get().LookupAs<RE::TESObjectMISC>(std::format("RF_MISC_Rune_{}", a_rank));
		return slot;
	}

	std::int32_t LootService::CountItem(RE::TESObjectREFR* a_owner, RE::TESBoundObject* a_base)
	{
		if (!a_owner || !a_base)
			return 0;
		std::int32_t total = 0;
		const auto counts = a_owner->GetInventoryCounts([a_base](RE::TESBoundObject& a_object) { return &a_object == a_base; });
		for (const auto& [object, count] : counts) {
			(void)object;
			total += count;
		}
		return total;
	}

	std::int32_t LootService::RankFromSpells(RE::Actor* a_actor)
	{
		return RankFromSpellsImpl(a_actor);
	}

	std::int32_t LootService::RollActorRank(RE::FormID a_formID, std::int32_t a_rfTier)
	{
		Rng rng(static_cast<std::uint64_t>(a_formID) * 0x9E3779B97F4A7C15ull
			^ (static_cast<std::uint64_t>(a_rfTier) << 32)
			^ 0x52414E4Bull);
		return TierEngine::Get().RollRank(a_rfTier, rng);
	}

	namespace
	{
		bool IsContainerReference(RE::TESObjectREFR* a_ref)
		{
			if (!a_ref || a_ref->IsPlayerRef())
				return false;
			auto* base = a_ref->GetBaseObject();
			return base && base->As<RE::TESObjectCONT>();
		}
	}

	std::int32_t LootService::ContainerTierFor(std::string_view a_editorID)
	{
		if (a_editorID.empty())
			return 2;
		std::string lower;
		lower.reserve(a_editorID.size());
		for (const auto c : a_editorID)
			lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

		if (lower.find("boss") != std::string::npos || lower.find("dragon") != std::string::npos ||
			lower.find("daedric") != std::string::npos || lower.find("blackreach") != std::string::npos)
			return 5;
		if (lower.find("strongbox") != std::string::npos || lower.find("safe") != std::string::npos ||
			lower.find("dwarven") != std::string::npos || lower.find("ebony") != std::string::npos ||
			lower.find("glass") != std::string::npos || lower.find("vault") != std::string::npos)
			return 4;
		if (lower.find("urn") != std::string::npos || lower.find("chest") != std::string::npos ||
			lower.find("coffer") != std::string::npos || lower.find("burial") != std::string::npos)
			return 3;
		if (lower.find("barrel") != std::string::npos || lower.find("crate") != std::string::npos ||
			lower.find("sack") != std::string::npos || lower.find("pot") != std::string::npos ||
			lower.find("basket") != std::string::npos || lower.find("cupboard") != std::string::npos ||
			lower.find("dresser") != std::string::npos || lower.find("wardrobe") != std::string::npos ||
			lower.find("endtable") != std::string::npos)
			return 1;
		return 2;
	}

	std::int32_t LootService::QuestBandFor(std::int32_t a_questType)
	{
		switch (a_questType) {
		case 1:  // main quest
			return 4;
		case 7:  // daedric
		case 10: // Dawnguard
		case 11: // Dragonborn
			return 3;
		case 2:
		case 3:
		case 4:
		case 5:
		case 8:
		case 9:
			return 2;
		case 6:  // miscellaneous
			return 1;
		default:
			return 2;
		}
	}

	bool LootService::GrantLootItem(
		RE::TESObjectREFR* a_owner,
		std::int32_t       a_rank,
		std::int32_t       a_rfTier,
		std::int32_t       a_ilvl,
		std::uint16_t      a_tags,
		std::string_view   a_slotGroup,
		Rng&               a_rng,
		std::int32_t       a_minRarity,
		float              a_rarityBias,
		const char*        a_context)
	{
		if (!a_owner || !DynamicEnchantment::GameplayReady() || !LootEngine::Get().HasData())
			return false;
		auto* base = PickBaseObject(a_rng);
		if (!base)
			return false;

		std::optional<ItemMetadata> metadata;
		for (int attempt = 0; attempt < 16; ++attempt) {
			metadata = LootEngine::Get().Generate(a_rank, a_rfTier, a_ilvl, a_tags, a_slotGroup, a_rng, a_rarityBias);
			if (metadata && metadata->enhancementCount > 0 && static_cast<std::int32_t>(metadata->rarity) >= a_minRarity)
				break;
		}
		if (!metadata)
			return false;
		if (metadata->enhancementCount == 0) {
			a_owner->AddObjectToContainer(base, nullptr, 1, nullptr);
			logger::warn("loot: {} rolled an unenchanted base item", a_context);
			return true;
		}
		metadata->baseFormId = base->GetFormID();
		metadata->registryId = ItemRegistry::Get().Add(*metadata);

		auto* enchantment = DynamicEnchantment::Create(*metadata);
		if (!enchantment)
			return false;
		auto* inventoryChanges = a_owner->GetInventoryChanges();
		if (!inventoryChanges) {
			logger::warn("loot: {} owner has no InventoryChanges", a_context);
			return false;
		}
		auto* extraList = inventoryChanges->EnchantObject(base, nullptr, enchantment, 100);
		if (!extraList) {
			logger::warn("loot: {} EnchantObject returned null", a_context);
			return false;
		}
		const char* baseNameRaw = base->GetName();
		const std::string baseName{ baseNameRaw && *baseNameRaw ? baseNameRaw : "item" };
		extraList->SetOverrideName(BuildDisplayName(*metadata, baseName, rarityNameFor(static_cast<int>(metadata->rarity))).c_str());
		a_owner->AddObjectToContainer(base, extraList, 1, nullptr);
		logger::info("loot: {} granted '{}' (rarity {}, rank {}, tier {}) to {:08X}",
			a_context, baseName, metadata->rarity, a_rank, a_rfTier, a_owner->GetFormID());
		return true;
	}

	void LootService::EnsureActorRank(RE::Actor* a_actor)
	{
		if (!a_actor || a_actor->IsPlayerRef() || a_actor->IsDead(false))
			return;
		const auto formID = a_actor->GetFormID();
		if (!formID)
			return;
		if (actorRanks.contains(formID))
			return;
		if (const auto* race = a_actor->GetActorBase() ? a_actor->GetActorBase()->GetRace() : nullptr) {
			const auto classification = RaceClassifier::Get().Classify(const_cast<RE::TESRace*>(race));
			logger::info("race-classifier: rank-path actor {:08X} -> role {} family {} source {} provisional {}",
				formID, classification.roleKey, classification.family, classification.source, classification.provisional);
		}
		const auto stageForPackage = ResolveActorStage(a_actor);
		if (const auto* race = a_actor->GetActorBase() ? a_actor->GetActorBase()->GetRace() : nullptr) {
			const auto classification = RaceClassifier::Get().Classify(const_cast<RE::TESRace*>(race));
			(void)ActorPackageEngine::Get().Apply(a_actor, classification.roleKey, stageForPackage, 0);
			logger::info("package-rank-path: actor {:08X} role {} stage {} package {}",
				formID, classification.roleKey, stageForPackage, ActorPackageEngine::Get().SelectPackageId(classification.roleKey, stageForPackage));
		}
		const auto existing = RankFromSpells(a_actor);
		if (existing >= 0) {
			actorRanks[formID] = existing;
			if (const auto* race = a_actor->GetActorBase() ? a_actor->GetActorBase()->GetRace() : nullptr) {
				const auto classification = RaceClassifier::Get().Classify(const_cast<RE::TESRace*>(race));
				(void)ActorPackageEngine::Get().Apply(a_actor, classification.roleKey, stageForPackage, existing);
				(void)ActorEquipmentDistributor::Get().Apply(a_actor, classification.roleKey, stageForPackage);
			}
			return;
		}
		const auto rfTier = Progression::Get().RuneTier();
		const auto rank = RollActorRank(formID, rfTier);
		actorRanks[formID] = rank;
		if (const auto* race = a_actor->GetActorBase() ? a_actor->GetActorBase()->GetRace() : nullptr) {
			const auto classification = RaceClassifier::Get().Classify(const_cast<RE::TESRace*>(race));
			(void)ActorPackageEngine::Get().Apply(a_actor, classification.roleKey, stageForPackage, rank);
			logger::info("package-rank-path: actor {:08X} role {} stage {} rank {} package {}",
				formID, classification.roleKey, stageForPackage, rank, ActorPackageEngine::Get().SelectPackageId(classification.roleKey, stageForPackage));
			(void)ActorEquipmentDistributor::Get().Apply(a_actor, classification.roleKey, stageForPackage);
		}
		if (rank <= 0)
			return;
		auto* spell = RankSpell(rank);
		if (!spell) {
			logger::warn("actor: rank {} spell unresolved; requesting rank {} on {:08X} stays cosmetic", rank, rank, formID);
			return;
		}
		if (a_actor->HasSpell(spell))
			return;
		if (!a_actor->AddSpell(spell)) {
			logger::warn("actor: AddSpell failed for rank {} on {:08X}", rank, formID);
			return;
		}
		++actorRanksAssigned;
		logger::info("actor: rank {} buff applied to {:08X}", rank, formID);
	}

	std::uint8_t LootService::StageForFormID(RE::FormID a_formID) const
	{
		const auto it = actorStages.find(a_formID);
		return it == actorStages.end() ? 0 : it->second;
	}

	std::uint8_t LootService::ResolveActorStage(RE::Actor* a_actor)
	{
		RuntimeContentResolver::Get().MaybeStartupScan();
		if (a_actor) {
			if (const auto* race = a_actor->GetActorBase() ? a_actor->GetActorBase()->GetRace() : nullptr) {
				const auto classification = RaceClassifier::Get().Classify(const_cast<RE::TESRace*>(race));
				logger::info("race-classifier: actor {:08X} -> role {} family {} source {} provisional {}",
					a_actor->GetFormID(), classification.roleKey, classification.family, classification.source, classification.provisional);
			}
		}
		if (!a_actor)
			return 1;
		const auto formID = a_actor->GetFormID();
		if (const auto it = actorStages.find(formID); it != actorStages.end())
			return it->second;
		if (auto* cell = a_actor->GetParentCell()) {
			const auto worldStage = WorldBandEngine::Get().StageForCell(cell, formID);
			if (worldStage > 0) {
				actorStages[formID] = worldStage;
				logger::info("world-band: actor {:08X} cell {:08X} -> stage {}", formID, cell->GetFormID(), worldStage);
				return worldStage;
			}
		}
		const auto rfTier = (std::clamp)(Progression::Get().RuneTier(), 0, 5);
		StageRequest request;
		request.source = StageSource::WorldBand;
		request.band = static_cast<std::uint8_t>((std::clamp)(1 + rfTier * 15, 1, 100));
		request.seed = static_cast<std::uint64_t>(formID);
		request.difficultyProfile = 255;
		const auto profile = StageEngine::Get().RequestStage(request);
		const auto stage = (profile.flags & kStageProfileRejected) ? static_cast<std::uint8_t>(1) : profile.stage;
		actorStages[formID] = stage;
		if (const auto* race = a_actor->GetActorBase() ? a_actor->GetActorBase()->GetRace() : nullptr) {
			const auto classification = RaceClassifier::Get().Classify(const_cast<RE::TESRace*>(race));
			ActorPackageEngine::Get().Apply(a_actor, classification.roleKey, stage, 0);
		}
		return stage;
	}

	std::int32_t LootService::ResolveActorRank(RE::Actor* a_actor)
	{
		if (!a_actor)
			return 0;
		const auto formID = a_actor->GetFormID();
		if (const auto it = actorRanks.find(formID); it != actorRanks.end())
			return it->second;
		auto rank = RankFromSpells(a_actor);
		if (rank < 0)
			rank = RollActorRank(formID, Progression::Get().RuneTier());
		actorRanks[formID] = rank;
		return rank;
	}

	bool LootService::EnsureContainerLoot(RE::TESObjectREFR* a_ref)
	{
		if (!Config::Get().EnableContainerLoot())
			return false;
		if (!LootEngine::Get().HasData() || !DynamicEnchantment::GameplayReady())
			return false;
		if (!IsContainerReference(a_ref))
			return false;
		if (a_ref->IsDisabled())
			return false;
		const auto formID = a_ref->GetFormID();
		if (!formID)
			return false;
		if (CoSave::Get().HasGeneratedContainer(formID)) {
			++containersSkipped;
			return false;
		}
		auto* cell = a_ref->GetParentCell();
		const auto cellID = cell ? cell->GetFormID() : 0;
		if (const auto it = cellLootCounts.find(cellID); it != cellLootCounts.end() &&
			it->second >= static_cast<std::uint32_t>((std::max)(0, Config::Get().ContainerLootPerCell()))) {
			++containersSkipped;
			logger::debug("container: cell {:08X} loot budget reached; skipped {:08X}", cellID, formID);
			return false;
		}

		auto* base = a_ref->GetBaseObject();
		auto* container = base ? base->As<RE::TESObjectCONT>() : nullptr;
		if (!container)
			return false;
		const auto tier = ContainerTierFor(container->GetFormEditorID());
		const auto* drop = LootEngine::Get().ContainerDrop(tier);
		if (!drop)
			return false;

		(void)CoSave::Get().RememberGeneratedContainer(formID);
		++cellLootCounts[cellID];
		++containersGenerated;

		Rng rng(static_cast<std::uint64_t>(formID) * 0x9E3779B97F4A7C15ull
			^ (static_cast<std::uint64_t>(cellID) << 17)
			^ 0xC07A1Eull);
		const auto rfTier = Progression::Get().RuneTier();
		std::int32_t itemCount = 0;
		if (rng.Float01() * 100.0f < drop->itemChancePct)
			itemCount = rng.Range(drop->itemCountMin, drop->itemCountMax);
		if (rng.Float01() * 100.0f < drop->extraRollChancePct)
			++itemCount;
		itemCount = (std::clamp)(itemCount, 0, 4);

		std::int32_t granted = 0;
		for (std::int32_t i = 0; i < itemCount; ++i) {
			const auto rank = (std::clamp)(tier, 0, 5);
			const auto ilvl = 10 + tier * 10 + rfTier * 5;
			if (GrantLootItem(a_ref, rank, rfTier, ilvl, static_cast<std::uint16_t>(ItemTag::ContainerLoot), "WEAPON", rng, 0,
					static_cast<float>(drop->rarityBias), "container")) {
				++granted;
				++containerItemsGranted;
			}
		}
		const auto runes = rng.Range(drop->runeMin, drop->runeMax);
		if (runes > 0)
			RuneForge::AddRuneUnits(runes);
		logger::info("container: generated {} item(s), {} rune unit(s) (tier {}, ref {:08X}, cell {:08X})",
			granted, runes, tier, formID, cellID);
		return true;
	}

	void LootService::OnObject3DLoaded(RE::FormID a_refFormID)
	{
		if (!DynamicEnchantment::GameplayReady() || !a_refFormID)
			return;
		auto* form = RE::TESForm::LookupByID(a_refFormID);
		if (!form)
			return;
		if (auto* actor = form->As<RE::Actor>()) {
			EnsureActorRank(actor);
			return;
		}
		if (auto* ref = form->As<RE::TESObjectREFR>())
			(void)EnsureContainerLoot(ref);
	}

	void LootService::OnContainerMenuOpened()
	{
		if (!DynamicEnchantment::GameplayReady())
			return;
		const auto handle = RE::ContainerMenu::GetTargetRefHandle();
		if (handle == 0)
			return;
		auto ref = RE::TESObjectREFR::LookupByHandle(handle);
		if (!ref || ref->IsPlayerRef())
			return;
		(void)EnsureContainerLoot(ref.get());
	}

	void LootService::OnInventoryMenuOpened()
	{
		if (!DynamicEnchantment::GameplayReady())
			return;
		for (int rank = 1; rank <= 5; ++rank) {
			auto* item = RuneItem(rank);
			if (!item)
				continue;
			const auto count = CountItem(RE::PlayerCharacter::GetSingleton(), item);
			if (count > 0)
				OnItemAddedToPlayer(item->GetFormID(), count, 0);
		}
	}

	void LootService::OnResetRef(RE::FormID a_refFormID)
	{
		if (!a_refFormID)
			return;
		CoSave::Get().ForgetGeneratedContainer(a_refFormID);
		actorRanks.erase(a_refFormID);
		processedActors.erase(a_refFormID);
		logger::info("reset: cleared loot state for ref {:08X}", a_refFormID);
	}

	void LootService::OnCellFullyLoaded(RE::FormID a_cellFormID)
	{
		if (cellLootCounts.size() <= 32)
			return;
		auto* player = RE::PlayerCharacter::GetSingleton();
		const auto currentCellID = player && player->GetParentCell() ? player->GetParentCell()->GetFormID() : a_cellFormID;
		std::vector<RE::FormID> stale;
		for (const auto& [cellID, count] : cellLootCounts) {
			(void)count;
			if (cellID != a_cellFormID && cellID != currentCellID)
				stale.push_back(cellID);
		}
		for (const auto cellID : stale)
			cellLootCounts.erase(cellID);
		++cellsPruned;
		logger::debug("cell: pruned {} loot-budget entries (cell {:08X})", stale.size(), a_cellFormID);
	}

	void LootService::OnItemAddedToPlayer(RE::FormID a_baseObject, std::int32_t a_count, RE::FormID a_oldContainer)
	{
		(void)a_oldContainer;
		if (!DynamicEnchantment::GameplayReady() || a_baseObject == 0 || a_count <= 0)
			return;
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return;
		auto* base = RE::TESForm::LookupByID<RE::TESBoundObject>(a_baseObject);
		if (!base)
			return;

		std::int32_t runeRank = 0;
		for (int rank = 1; rank <= 5 && !runeRank; ++rank) {
			auto* item = RuneItem(rank);
			if (item && item->GetFormID() == a_baseObject)
				runeRank = rank;
		}
		if (!runeRank)
			return;

		const auto available = CountItem(player, base);
		const auto take = (std::min)(a_count, available);
		if (take <= 0)
			return;
		player->RemoveItem(base, take, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
		RuneForge::AddRunesToWallet(runeRank, take);
		runeItemsAggregated += static_cast<std::uint32_t>(take);
		logger::info("runes: aggregated {} x R{} into the wallet", take, runeRank);
	}

	void LootService::OnQuestStage(RE::FormID a_questFormID, std::uint16_t a_stage)
	{
		(void)a_stage;
		if (!Config::Get().EnableQuestDrops() || !a_questFormID)
			return;
		auto* quest = RE::TESForm::LookupByID<RE::TESQuest>(a_questFormID);
		if (!quest || !quest->IsCompleted())
			return;
		if (CoSave::Get().HasRewardedQuest(a_questFormID))
			return;
		CoSave::Get().RememberRewardedQuest(a_questFormID);
		const auto band = QuestBandFor(static_cast<std::int32_t>(quest->data.questType.get()));
		GrantQuestReward(band, "quest");
	}

	bool LootService::GrantQuestReward(std::int32_t a_band, std::string_view a_source)
	{
		if (!Config::Get().EnableQuestDrops())
			return false;
		if (!LootEngine::Get().HasData() || !DynamicEnchantment::GameplayReady())
			return false;
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return false;
		const auto band = (std::clamp)(a_band, 0, 4);
		const auto* drop = LootEngine::Get().QuestDrop(band);
		if (!drop)
			return false;

		Rng rng(static_cast<std::uint64_t>(std::time(nullptr)) * 0x9E3779B97F4A7C15ull
			^ static_cast<std::uint64_t>(band) << 24
			^ 0x51554553ull);
		const auto rfTier = Progression::Get().RuneTier();
		const auto count = (std::clamp)(drop->itemCount, 0, 3);
		std::int32_t granted = 0;
		for (std::int32_t i = 0; i < count; ++i) {
			if (GrantLootItem(player, band, rfTier, 12 + band * 12, static_cast<std::uint16_t>(ItemTag::QuestReward), "WEAPON", rng,
					drop->minRarity, static_cast<float>(band) * 2.0f, "quest")) {
				++granted;
				++questItemsGranted;
			}
		}
		const auto runes = rng.Range(drop->runeMin, drop->runeMax);
		if (runes > 0)
			RuneForge::AddRuneUnits(runes);
		++questRewardsGranted;
		logger::info("quest: {} reward band {} granted {} item(s), {} rune unit(s)", a_source, band, granted, runes);
		return granted > 0 || count == 0;
	}

	bool LootService::DebugGiveRuneItem(std::int32_t a_rank)
	{
		if (a_rank < 1 || a_rank > 5)
			return false;
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return false;
		auto* item = RuneItem(a_rank);
		if (!item)
			return false;
		player->AddObjectToContainer(item, nullptr, 1, nullptr);
		logger::info("debug: added one R{} rune item to the player for aggregation", a_rank);
		return true;
	}

	bool LootService::SpawnForActor(RE::Actor* a_actor)
	{
		if (!Config::Get().EnableRuntimeLoot()) {
			logger::info("loot: runtime loot disabled (bEnableRuntimeLoot=0)");
			return false;
		}
		if (!a_actor) {
			logger::warn("loot: spawn rejected (null actor)");
			return false;
		}
		if (!LootEngine::Get().HasData()) {
			logger::warn("loot: spawn rejected (loot data not loaded)");
			return false;
		}
		if (!DynamicEnchantment::GameplayReady()) {
			logger::warn("loot: runtime forms not ready yet; skip spawn for actor {:08X}", a_actor ? a_actor->GetFormID() : 0);
			return false;
		}

		const auto rfTier = Progression::Get().RuneTier();
		const auto seed = static_cast<std::uint64_t>(a_actor->GetFormID()) ^ (static_cast<std::uint64_t>(std::time(nullptr)) << 32);
		Rng        rng(seed);
		const auto rank = ResolveActorRank(a_actor);
		const auto stage = ResolveActorStage(a_actor);
		if (rank >= 4)
			Progression::Get().MarkBossKill(rfTier);
		auto* base = PickBaseObject(rng);
		if (!base)
			return false;

		std::string raceEditorId;
		if (const auto* race = a_actor->GetActorBase() ? a_actor->GetActorBase()->GetRace() : nullptr) {
			if (const char* editor = race->GetFormEditorID(); editor && *editor)
				raceEditorId = editor;
		}

		const auto* family = raceEditorId.empty() ? nullptr : HuntEngine::Get().FindFamilyForRace(raceEditorId);
		std::uint16_t tags = static_cast<std::uint16_t>(ItemTag::HuntDrop);
		if (rank >= 4)
			tags |= static_cast<std::uint16_t>(ItemTag::BossDrop);

		std::optional<ItemMetadata> metadata;
		for (int attempt = 0; attempt < 16; ++attempt) {
			metadata = LootEngine::Get().Generate(rank, rfTier, stage, tags, "WEAPON", rng);
			if (metadata && metadata->enhancementCount > 0)
				break;
		}
		if (!metadata)
			return false;
		metadata->baseFormId = base->GetFormID();
		metadata->registryId = ItemRegistry::Get().Add(*metadata);

		if (metadata->enhancementCount == 0) {
			// Fail closed on the affix roll, but still leave a readable base drop.
			a_actor->AddObjectToContainer(base, nullptr, 1, nullptr);
			logger::warn("loot: actor {:08X} rolled an unenchanted drop", a_actor->GetFormID());
			return true;
		}

		auto* enchantment = DynamicEnchantment::Create(*metadata);
		if (!enchantment)
			return false;
		auto* inventoryChanges = a_actor->GetInventoryChanges();
		if (!inventoryChanges) {
			logger::error("loot: actor has no InventoryChanges");
			return false;
		}
		auto* extraList = inventoryChanges->EnchantObject(base, nullptr, enchantment, 100);
		if (!extraList) {
			logger::error("loot: EnchantObject returned null");
			return false;
		}
		const char* baseNameRaw = base->GetName();
		const std::string baseName{ baseNameRaw && *baseNameRaw ? baseNameRaw : "item" };
		const auto* rarityName = rarityNameFor(static_cast<int>(metadata->rarity));
		extraList->SetOverrideName(BuildDisplayName(*metadata, baseName, rarityName).c_str());

		a_actor->AddObjectToContainer(base, extraList, 1, nullptr);
		logger::info("loot: spawned {} (rarity {}, rank {}, tier {}, family {}) on actor {:08X}",
			baseName, metadata->rarity, rank, rfTier, family ? family->group : "none", a_actor->GetFormID());
		return true;
	}

	namespace
	{
		void GrantUniqueHuntItem(RE::Actor* a_actor, const RaceGroupDef& a_family, std::size_t a_familyIndex)
		{
			const auto* table = DataStore::Get().GetTable("unique_affixes.csv");
			if (!table) {
				logger::warn("hunt: unique_affixes.csv not loaded");
				return;
			}
			auto column = [&](std::string_view a_name) {
				for (std::size_t i = 0; i < table->headers.size(); ++i)
					if (table->headers[i] == a_name)
						return static_cast<int>(i);
				return -1;
			};
			const int cId = column("uniqueId");
			const int cName = column("name");
			const int cEffect = column("effectEditorId");
			const int cMin = column("minValue");
			const int cMax = column("maxValue");
			const int cDuration = column("duration");
			const int cArea = column("area");
			for (const auto& row : table->rows) {
				if (cId < 0 || static_cast<std::size_t>(cId) >= row.size() || row[static_cast<std::size_t>(cId)] != a_family.uniqueAffixId)
					continue;
				auto cell = [&](int a_col) -> std::string_view {
					if (a_col < 0 || static_cast<std::size_t>(a_col) >= row.size())
						return {};
					return row[static_cast<std::size_t>(a_col)];
				};
				const auto effectEditorId = cell(cEffect);
				if (effectEditorId.empty())
					return;
				RE::EffectSetting* effect = FormCatalog::Get().LookupAs<RE::EffectSetting>(effectEditorId);
				(void)a_familyIndex;
				auto* base = ResolveBaseObject("IronSword");
				if (!effect || !base) {
					logger::warn("hunt: unique '{}' resources unresolved", a_family.uniqueAffixId);
					return;
				}
				auto parseFloat = [](std::string_view a_text, float a_default) {
					if (a_text.empty())
						return a_default;
					std::string copy{ a_text };
					char* end = nullptr;
					const auto value = std::strtof(copy.c_str(), &end);
					return end == copy.c_str() ? a_default : value;
				};
				const auto minValue = parseFloat(cell(cMin), 25.0f);
				const auto maxValue = parseFloat(cell(cMax), 35.0f);
				const auto duration = static_cast<std::int32_t>(parseFloat(cell(cDuration), 0.0f));
				const auto area = static_cast<std::int32_t>(parseFloat(cell(cArea), 0.0f));
				auto* enchantment = DynamicEnchantment::CreateFromEffect(effect, (minValue + maxValue) * 0.5f, duration, area);
				if (!enchantment)
					return;
				auto* changes = a_actor->GetInventoryChanges();
				if (!changes)
					return;
				auto* extraList = changes->EnchantObject(base, nullptr, enchantment, 100);
				if (!extraList)
					return;
				const auto displayName = "Runed " + a_family.group + " " + std::string{ cell(cName) };
				extraList->SetOverrideName(displayName.c_str());
				a_actor->AddObjectToContainer(base, extraList, 1, nullptr);
				if (const auto* source = HuntEngine::Get().FindUniqueSource(a_family.uniqueAffixId))
					logger::info("hunt: unique source '{}' ({})", source->sourceEditorId, source->sourceType);
				logger::info("hunt: granted unique affix '{}' as '{}'", a_family.uniqueAffixId, displayName);
				return;
			}
		}

		void GrantHuntFirstKill(RE::Actor* a_actor)
		{
			auto* actorBase = a_actor ? a_actor->GetActorBase() : nullptr;
			auto* race = actorBase ? actorBase->GetRace() : nullptr;
			const char* raceEditorID = race ? race->GetFormEditorID() : nullptr;
			if (!raceEditorID || !*raceEditorID)
				return;
			const auto* family = HuntEngine::Get().FindFamilyForRace(raceEditorID);
			if (!family || HuntEngine::Get().HasFirstKill(family->group))
				return;
			std::size_t familyIndex = 0;
			for (const auto& candidate : HuntEngine::Get().Families()) {
				if (candidate.group == family->group)
					break;
				++familyIndex;
			}
			if (!HuntEngine::Get().MarkFirstKill(family->group))
				return;
			logger::info("hunt: first kill family '{}' on actor {:08X}", family->group, a_actor->GetFormID());
			if (family->materialId.empty())
				return;
			auto* material = FormCatalog::Get().LookupAs<RE::TESObjectMISC>(family->materialId);
			if (material) {
				a_actor->AddObjectToContainer(material, nullptr, 1, nullptr);
				logger::info("hunt: granted first-kill material '{}'", family->materialId);
			} else {
				logger::warn("hunt: material '{}' unresolved", family->materialId);
			}
			GrantUniqueHuntItem(a_actor, *family, familyIndex);
		}
	}

	void LootService::OnActorDeath(RE::Actor* a_actor)
	{
		// Death events fire while the actor is already dead; only the player is excluded.
		if (!a_actor || a_actor->IsPlayerRef())
			return;
		const auto formId = a_actor->GetFormID();
		if (processedActors.contains(formId))
			return;
		processedActors.insert(formId);
		GrantHuntFirstKill(a_actor);
		if (!SpawnForActor(a_actor))
			logger::warn("loot: death roll failed for actor {:08X}", formId);
	}

	bool LootService::InjectProbeItem()
	{
		if (!Config::Get().EnableRuntimeLoot()) {
			logger::warn("loot: probe item injection disabled (bEnableRuntimeLoot=0)");
			return false;
		}
		if (!DynamicEnchantment::GameplayReady())
			return false;
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return false;
		auto* base = ResolveBaseObject("IronSword");
		if (!base)
			return false;
		if (!LootEngine::Get().HasData())
			return false;

		Rng rng(0xC0FFEE);
		std::optional<ItemMetadata> metadata;
		for (int attempt = 0; attempt < 16; ++attempt) {
			metadata = LootEngine::Get().Generate(2, 2, 30, static_cast<std::uint16_t>(ItemTag::Debug), "WEAPON", rng);
			if (metadata && metadata->enhancementCount > 0)
				break;
		}
		if (!metadata || metadata->enhancementCount == 0) {
			logger::warn("loot: probe generation produced no enchanted item");
			return false;
		}
		metadata->baseFormId = base->GetFormID();
		metadata->registryId = ItemRegistry::Get().Add(*metadata);

		auto* enchantment = DynamicEnchantment::Create(*metadata);
		if (!enchantment)
			return false;
		auto* inventoryChanges = player->GetInventoryChanges();
		if (!inventoryChanges) {
			logger::error("loot: player has no InventoryChanges");
			return false;
		}
		auto* extraList = inventoryChanges->EnchantObject(base, nullptr, enchantment, 100);
		if (!extraList) {
			logger::error("loot: EnchantObject returned null for probe item");
			return false;
		}
		const char* baseNameRaw = base->GetName();
		const std::string baseName{ baseNameRaw && *baseNameRaw ? baseNameRaw : "item" };
		extraList->SetOverrideName(BuildDisplayName(*metadata, baseName, rarityNameFor(static_cast<int>(metadata->rarity))).c_str());
		player->AddObjectToContainer(base, extraList, 1, nullptr);
		logger::info("loot: probe item injected into player inventory (rarity {})", metadata->rarity);
		return true;
	}

	SelfTestResult LootService::SelfTest()
	{
		if (!RE::TESDataHandler::GetSingleton())
			return { "LootService.SelfTest", CheckStatus::Warn, "not run before data load" };
		if (!LootEngine::Get().HasData())
			return { "LootService.SelfTest", CheckStatus::Warn, "loot data not loaded" };

		if (ContainerTierFor("BossChest01") != 5)
			return { "LootService.SelfTest", CheckStatus::Fail, "boss container classification wrong" };
		if (ContainerTierFor("Barrel") != 1)
			return { "LootService.SelfTest", CheckStatus::Fail, "barrel container classification wrong" };
		if (ContainerTierFor("Chest") < 3)
			return { "LootService.SelfTest", CheckStatus::Fail, "chest container classification wrong" };
		if (QuestBandFor(1) != 4 || QuestBandFor(6) != 1 || QuestBandFor(8) != 2)
			return { "LootService.SelfTest", CheckStatus::Fail, "quest band mapping wrong" };

		auto* base = ResolveBaseObject("IronSword");
		if (!base)
			return { "LootService.SelfTest", CheckStatus::Warn, "IronSword not loaded yet (lazy form; retry after new game)" };
		return { "LootService.SelfTest", CheckStatus::Pass, "container tier + quest band policy; IronSword resolves" };
	}
}
