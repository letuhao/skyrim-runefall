#include "LootService.h"

#include "CoSave.h"
#include "DynamicEnchantment.h"
#include "FormResolver.h"
#include "HuntEngine.h"
#include "ItemRegistry.h"
#include "LootEngine.h"
#include "Progression.h"
#include "TierEngine.h"
#include "Config.h"

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
	}

	LootService& LootService::Get()
	{
		static LootService singleton;
		return singleton;
	}

	void LootService::ResetSessionState()
	{
		processedActors.clear();
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

	namespace
	{
		struct BasePoolEntry
		{
			const char*    editorId;
			RE::FormID     formId;
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
	}

	RE::TESBoundObject* LootService::PickBaseObject(RE::Actor* a_actor, Rng& a_rng)
	{
		// v1 uses a small vanilla weapon pool; the final drop table selects by actor type/slot.
		for (int attempt = 0; attempt < 4; ++attempt) {
			const auto index = static_cast<std::size_t>(a_rng.Range(0, static_cast<std::int32_t>(kBasePool.size()) - 1));
			if (auto* object = ResolveBaseObject(kBasePool[index].editorId))
				return object;
		}
		return nullptr;
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
		const auto rank = TierEngine::Get().RollRank(rfTier, rng);
		const auto base = PickBaseObject(a_actor, rng);
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
			metadata = LootEngine::Get().Generate(rank, rfTier, 10 + rfTier * 10, tags, "WEAPON", rng);
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

	void LootService::OnActorDeath(RE::Actor* a_actor)
	{
		// Death events fire while the actor is already dead; only the player is excluded.
		if (!a_actor || a_actor->IsPlayerRef())
			return;
		const auto formId = a_actor->GetFormID();
		if (processedActors.contains(formId))
			return;
		processedActors.insert(formId);
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
		auto* base = ResolveBaseObject("IronSword");
		if (!base)
			return { "LootService.SelfTest", CheckStatus::Warn, "IronSword not loaded yet (lazy form; retry after new game)" };
		return { "LootService.SelfTest", CheckStatus::Pass, "IronSword resolves to a TESBoundObject" };
	}
}