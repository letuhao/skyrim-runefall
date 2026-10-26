#pragma once
#include "Core.h"
#include "ItemMetadata.h"
#include "LootEngine.h"

namespace Runefall
{
	struct ForgeResult
	{
		bool         ok = false;
		std::int32_t error = 0;
	};

	class RuneForge
	{
	public:
		static RuneForge& Get();

		ForgeResult Reroll(ItemMetadata& a_item, std::int32_t a_lockedIndex, std::string_view a_slotGroup, Rng& a_rng);
		ForgeResult Upgrade(ItemMetadata& a_item, std::string_view a_slotGroup, Rng& a_rng);
		ForgeResult Socket(ItemMetadata& a_item, std::int32_t a_socketIndex, std::int32_t a_runeRank);
		ForgeResult Extract(ItemMetadata& a_item, std::int32_t a_socketIndex);
		ForgeResult RuneConvert(std::int32_t a_fromRank);

		[[nodiscard]] static std::int32_t RerollCost(const ItemMetadata& a_item);
		[[nodiscard]] static std::int32_t UpgradeCost(const ItemMetadata& a_item);
		[[nodiscard]] static std::int32_t SocketCost(std::int32_t a_runeRank);
		[[nodiscard]] static std::pair<std::int32_t, std::int32_t> SalvageYield(const ItemMetadata& a_item);

		[[nodiscard]] static SelfTestResult SelfTest();
	};
}