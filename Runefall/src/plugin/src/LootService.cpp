#include "LootService.h"

#include "CoSave.h"
#include "DynamicEnchantment.h"
#include "FormResolver.h"
#include "HuntEngine.h"
#include "ItemRegistry.h"
#include "LootEngine.h"
#include "Progression.h"
#include "TierEngine.h"

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
				if (!a_event || !a_event->dead || !a_event->actorDying)
					return RE::BSEventNotifyControl::kContinue;
				auto* actor = a_event->actorDying->As<RE::Actor>();
				if (actor) {
					LootService::Get().OnActorDeath(actor);
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

	RE::TESBoundObject* LootService::PickBaseObject(RE::Actor* a_actor, Rng& a_rng)
	{
		// v1 uses a small vanilla pool; the final drop table selects by actor type/slot.
		static const std::array<const char*, 8> kPool = {
			"IronSword", "IronDagger", "IronWarAxe", "IronArmor",
			"IronHelmet", "IronShield", "HuntingBow", "IronMace"
		};
		for (int attempt = 0; attempt < 4; ++attempt) {
			const auto index = static_cast<std::size_t>(a_rng.Range(0, static_cast<std::int32_t>(kPool.size()) - 1));
			if (auto* object = FormResolver::ResolveAs<RE::TESBoundObject>(kPool[index]))
				return object;
		}
		return nullptr;
	}

	bool LootService::SpawnForActor(RE::Actor* a_actor)
	{
		if (!a_actor || LootEngine::Get().HasData() == false)
			return false;

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

		auto metadata = LootEngine::Get().Generate(rank, rfTier, 10 + rfTier * 10, tags, "WEAPON", rng);
		if (!metadata)
			return false;
		metadata->baseFormId = base->GetFormID();
		metadata->registryId = ItemRegistry::Get().Add(*metadata);

		auto* extraList = new RE::ExtraDataList();
		auto* enchantment = DynamicEnchantment::Create(*metadata);
		if (!enchantment) {
			delete extraList;
			return false;
		}
		extraList->SetEnchantment(enchantment, 100, false);
		const std::string baseName{ base->GetName() };
		const auto* rarityName = rarityNameFor(static_cast<int>(metadata->rarity));
		extraList->SetOverrideName(BuildDisplayName(*metadata, baseName, rarityName).c_str());

		a_actor->AddObjectToContainer(base, extraList, 1, nullptr);
		logger::info("loot: spawned {} (rarity {}, rank {}, tier {}, family {}) on actor {:08X}",
			baseName, metadata->rarity, rank, rfTier, family ? family->group : "none", a_actor->GetFormID());
		return true;
	}

	void LootService::OnActorDeath(RE::Actor* a_actor)
	{
		if (!a_actor || a_actor->IsPlayerRef() || a_actor->IsDead(false))
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
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return false;
		auto* base = FormResolver::ResolveAs<RE::TESBoundObject>("IronSword");
		if (!base)
			return false;
		if (!LootEngine::Get().HasData())
			return false;

		Rng rng(0xC0FFEE);
		auto metadata = LootEngine::Get().Generate(2, 2, 30, static_cast<std::uint16_t>(ItemTag::Debug), "WEAPON", rng);
		if (!metadata)
			return false;
		metadata->baseFormId = base->GetFormID();
		metadata->registryId = ItemRegistry::Get().Add(*metadata);

		auto* extraList = new RE::ExtraDataList();
		auto* enchantment = DynamicEnchantment::Create(*metadata);
		if (!enchantment) {
			delete extraList;
			return false;
		}
		extraList->SetEnchantment(enchantment, 100, false);
		const std::string baseName{ base->GetName() };
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
		auto* base = FormResolver::ResolveAs<RE::TESBoundObject>("IronSword");
		if (!base)
			return { "LootService.SelfTest", CheckStatus::Fail, "vanilla base item IronSword unresolved" };
		return { "LootService.SelfTest", CheckStatus::Pass, "base item pool resolves" };
	}
}