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

		// Grants materials/runes from an item and clears its registry id; caller removes the item.
		ForgeResult Salvage(ItemMetadata& a_item);

		// Loads socket recipes (material + rune costs) from recipes.csv.
		void ReloadSocketRecipes();

		// Spends rune units from the highest rank down (R1=1, R2=5, R3=25, R4=125, R5=625).
		[[nodiscard]] static bool SpendUnits(std::int32_t a_costUnits);

		// Runtime currency grants: rank items aggregate into the rank wallet; drop tables
		// express their reward in R1-equivalent units.
		static void AddRunesToWallet(std::int32_t a_rank, std::int32_t a_amount);
		static void AddRuneUnits(std::int32_t a_units);
		[[nodiscard]] static std::int32_t WalletUnits();

		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		struct SocketCostDef
		{
			std::string materialId;
			std::int32_t materialCount = 0;
			std::int32_t runeCost = 0;
		};
		std::unordered_map<std::int32_t, SocketCostDef> socketRecipes;
		std::unordered_map<std::int32_t, SocketCostDef> convertRecipes;

	};
}