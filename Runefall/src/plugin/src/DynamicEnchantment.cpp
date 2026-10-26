#include "DynamicEnchantment.h"

#include "FormResolver.h"

namespace Runefall
{
	RE::EnchantmentItem* DynamicEnchantment::Create(const ItemMetadata& a_metadata)
	{
		auto* factory = RE::IFormFactory::GetFormFactoryByType(RE::FormType::Enchantment);
		if (!factory)
			return nullptr;
		auto* enchantment = factory->Create<RE::EnchantmentItem>();
		if (!enchantment)
			return nullptr;

		enchantment->data.flags = RE::EnchantmentItem::EnchantmentFlag::kNone;
		enchantment->data.castingType = RE::MagicSystem::CastingType::kFireAndForget;
		enchantment->data.delivery = RE::MagicSystem::Delivery::kSelf;
		enchantment->data.spellType = RE::MagicSystem::SpellType::kEnchantment;
		enchantment->data.costOverride = 0;
		enchantment->data.chargeOverride = 0;

		std::uint8_t created = 0;
		for (std::uint8_t i = 0; i < a_metadata.enhancementCount && i < 5; ++i) {
			const auto& enhancement = a_metadata.enhancements[i];
			if (enhancement.effectEditorId.empty())
				continue;
			auto* effect = new RE::Effect();
			effect->effectItem.magnitude = enhancement.magnitude;
			effect->effectItem.area = static_cast<std::uint32_t>((std::max)(0, enhancement.area));
			effect->effectItem.duration = static_cast<std::uint32_t>((std::max)(0, enhancement.duration));
			effect->baseEffect = FormResolver::ResolveAs<RE::EffectSetting>(enhancement.effectEditorId);
			effect->cost = 0.0f;
			enchantment->effects.push_back(effect);
			++created;
		}

		if (created == 0) {
			// Never delete a form created by IFormFactory: the game owns it.
			return nullptr;
		}

		if (auto* dataHandler = RE::TESDataHandler::GetSingleton())
			dataHandler->AddFormToDataHandler(enchantment);

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

	bool DynamicEnchantment::AttachToEntry(RE::InventoryEntryData* a_entry, const ItemMetadata& a_metadata, std::uint16_t a_charge, std::string_view a_displayName)
	{
		if (!a_entry)
			return false;
		auto* enchantment = Create(a_metadata);
		if (!enchantment)
			return false;

		RE::ExtraDataList* list = nullptr;
		if (a_entry->extraLists) {
			for (auto& existing : *a_entry->extraLists) {
				if (existing) {
					list = existing;
					break;
				}
			}
		}
		if (!list)
			list = new RE::ExtraDataList();

		list->SetEnchantment(enchantment, a_charge, false);
		if (!a_displayName.empty())
			list->SetOverrideName(std::string{ a_displayName }.c_str());
		if (!a_entry->extraLists)
			a_entry->AddExtraList(list);
		return true;
	}

	SelfTestResult DynamicEnchantment::SelfTest()
	{
		ItemMetadata metadata;
		metadata.enhancementCount = 1;
		metadata.enhancements[0].effectEditorId = "RF_MGEF_Affix_000";
		metadata.enhancements[0].magnitude = 5.0f;
		metadata.enhancements[0].duration = 0;
		metadata.enhancements[0].area = 0;

		if (!RE::TESDataHandler::GetSingleton())
			return { "DynamicEnchantment.SelfTest", CheckStatus::Warn, "not run before data load" };

		auto* enchantment = Create(metadata);
		if (!enchantment)
			return { "DynamicEnchantment.SelfTest", CheckStatus::Fail, "runtime enchantment creation failed (MGEF unresolved or factory unavailable)" };
		if (enchantment->effects.size() != 1)
			return { "DynamicEnchantment.SelfTest", CheckStatus::Fail, "unexpected effect count" };
		return { "DynamicEnchantment.SelfTest", CheckStatus::Pass, "created runtime enchantment with 1 effect" };
	}
}