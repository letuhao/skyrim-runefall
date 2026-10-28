#pragma once
#include "Core.h"

namespace Runefall
{
	class EconomyEngine
	{
	public:
		static EconomyEngine& Get();
		void Load();
		[[nodiscard]] bool Loaded() const noexcept { return loaded; }
		[[nodiscard]] float ItemValue(std::int32_t a_ilvl, std::int32_t a_rarity) const;
		[[nodiscard]] float VendorBuy(float a_value) const;
		[[nodiscard]] float VendorSell(float a_value) const;
		[[nodiscard]] float RespecCost(std::int32_t a_level) const;
		[[nodiscard]] float RuneValue() const noexcept { return runeValue; }
		[[nodiscard]] float MaterialValue() const noexcept { return materialValue; }
		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		std::vector<float> rarityMultipliers;
		float baseItemValue = 25.0f;
		float valuePerIlvl = 5.0f;
		float vendorBuyMultiplier = 1.5f;
		float vendorSellMultiplier = 0.35f;
		float runeValue = 10.0f;
		float materialValue = 5.0f;
		float respecBaseCost = 500.0f;
		float respecCostPerLevel = 100.0f;
		float salvageValuePct = 0.25f;
		bool loaded = false;
	};
}
