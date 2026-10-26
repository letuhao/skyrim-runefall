#pragma once
#include "Core.h"
#include "ItemMetadata.h"

namespace Runefall
{
	class DynamicEnchantment
	{
	public:
		// Runtime game forms may only be created after kNewGame/kPostLoadGame.
		static void SetGameplayReady(bool a_ready) noexcept;
		[[nodiscard]] static bool GameplayReady() noexcept;

		// Creates a runtime EnchantmentItem using the affix MGEF templates from the loaded
		// Runefall plugins. Returned form is registered with the data handler.
		[[nodiscard]] static RE::EnchantmentItem* Create(const ItemMetadata& a_metadata);

		// Single-effect runtime enchantment (unique hunt/boss rewards).
		[[nodiscard]] static RE::EnchantmentItem* CreateFromEffect(RE::EffectSetting* a_effect, float a_magnitude, std::int32_t a_duration, std::int32_t a_area);

		// Attaches the runtime enchantment to a world reference's extra data.
		[[nodiscard]] static bool AttachToReference(RE::TESObjectREFR* a_reference, const ItemMetadata& a_metadata, std::uint16_t a_charge);

		// Attaches to an inventory entry's extra data list (first list, or a new one).
		[[nodiscard]] static bool AttachToEntry(RE::InventoryEntryData* a_entry, const ItemMetadata& a_metadata, std::uint16_t a_charge, std::string_view a_displayName);

		[[nodiscard]] static SelfTestResult SelfTest();
	};
}