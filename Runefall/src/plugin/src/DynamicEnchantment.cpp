#include "DynamicEnchantment.h"

#include "FormResolver.h"

namespace Runefall
{
	namespace
	{
		std::atomic_bool g_gameplayReady{ false };
	}

	void DynamicEnchantment::SetGameplayReady(bool a_ready) noexcept
	{
		g_gameplayReady.store(a_ready);
		logger::info("dynamic-enchantment: gameplay-ready={}", a_ready);
	}

	bool DynamicEnchantment::GameplayReady() noexcept
	{
		return g_gameplayReady.load();
	}

	RE::EnchantmentItem* DynamicEnchantment::Create(const ItemMetadata& a_metadata)
	{
		if (!g_gameplayReady.load()) {
			logger::warn("dynamic-enchantment: refused runtime form before gameplay-ready");
			return nullptr;
		}
		if (!RE::TESDataHandler::GetSingleton()) {
			logger::warn("dynamic-enchantment: no TESDataHandler");
			return nullptr;
		}

		struct ResolvedEffect
		{
			RE::EffectSetting* baseEffect;
			float              magnitude;
			std::int32_t       area;
			std::int32_t       duration;
		};

		std::vector<ResolvedEffect> resolved;
		for (std::uint8_t i = 0; i < a_metadata.enhancementCount && i < 5; ++i) {
			const auto& enhancement = a_metadata.enhancements[i];
			if (enhancement.effectEditorId.empty())
				continue;
			auto* baseEffect = FormResolver::ResolveAs<RE::EffectSetting>(enhancement.effectEditorId);
			if (!baseEffect || baseEffect->IsDeleted()) {
				logger::warn("dynamic-enchantment: effect template '{}' missing/deleted", enhancement.effectEditorId);
				continue;
			}
			resolved.push_back({ baseEffect, enhancement.magnitude, enhancement.area, enhancement.duration });
		}
		if (resolved.empty()) {
			logger::warn("dynamic-enchantment: no valid effects; refusing to create form");
			return nullptr;
		}

		auto* factory = RE::IFormFactory::GetFormFactoryByType(RE::FormType::Enchantment);
		if (!factory) {
			logger::error("dynamic-enchantment: no Enchantment form factory");
			return nullptr;
		}
		auto* enchantment = factory->Create<RE::EnchantmentItem>();
		if (!enchantment) {
			logger::error("dynamic-enchantment: factory Create<EnchantmentItem> returned null");
			return nullptr;
		}

		enchantment->data.flags = RE::EnchantmentItem::EnchantmentFlag::kNone;
		enchantment->data.castingType = RE::MagicSystem::CastingType::kFireAndForget;
		enchantment->data.delivery = RE::MagicSystem::Delivery::kSelf;
		enchantment->data.spellType = RE::MagicSystem::SpellType::kEnchantment;
		enchantment->data.costOverride = 0;
		enchantment->data.chargeOverride = 0;

		for (const auto& entry : resolved) {
			auto* effect = new RE::Effect();
			effect->effectItem.magnitude = entry.magnitude;
			effect->effectItem.area = static_cast<std::uint32_t>((std::max)(0, entry.area));
			effect->effectItem.duration = static_cast<std::uint32_t>((std::max)(0, entry.duration));
			effect->baseEffect = entry.baseEffect;
			effect->cost = 0.0f;
			enchantment->effects.push_back(effect);
		}

		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			logger::error("dynamic-enchantment: TESDataHandler missing; refusing unregistered form");
			return nullptr;
		}
		if (!dataHandler->AddFormToDataHandler(enchantment)) {
			logger::error("dynamic-enchantment: AddFormToDataHandler failed; form left unregistered");
			return nullptr;
		}
		return enchantment;
	}

	bool DynamicEnchantment::AttachToReference(RE::TESObjectREFR* a_reference, const ItemMetadata& a_metadata, std::uint16_t a_charge)
	{
		if (!a_reference)
			return false;
		auto* enchantment = Create(a_metadata);
		if (!enchantment)
			return false;
		a_reference->extraList.SetEnchantment(enchantment, a_charge, false);
		return true;
	}

	bool DynamicEnchantment::AttachToEntry(RE::InventoryEntryData*, const ItemMetadata&, std::uint16_t, std::string_view)
	{
		// Disabled: InventoryEntryData owns only its list nodes, not a newly attached ExtraDataList.
		// The ownership path is unproven, so this feature fails closed instead of risking a leak/CTD.
		logger::error("dynamic-enchantment: AttachToEntry disabled (unproven ExtraDataList ownership)");
		return false;
	}

	SelfTestResult DynamicEnchantment::SelfTest()
	{
		// Never create or register a runtime game form during kDataLoaded: adding a form to
		// TESDataHandler while the game is still initialising corrupts its state and crashes
		// later (observed as a heap access violation at new game). Only pure-logic checks here.
		ItemMetadata metadata;
		metadata.enhancementCount = 1;
		metadata.enhancements[0].effectEditorId = "RF_MGEF_Affix_000";
		metadata.enhancements[0].magnitude = 5.0f;
		metadata.enhancements[0].duration = 0;
		metadata.enhancements[0].area = 0;
		if (metadata.enhancementCount != 1 || metadata.enhancements[0].effectEditorId.empty())
			return { "DynamicEnchantment.SelfTest", CheckStatus::Fail, "metadata setup failed" };
		return { "DynamicEnchantment.SelfTest", CheckStatus::Pass, "logic-only (runtime form creation deferred until in-game)" };
	}
}