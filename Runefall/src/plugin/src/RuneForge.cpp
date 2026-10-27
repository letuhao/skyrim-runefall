#include "RuneForge.h"

#include "CoSave.h"
#include "LootEngine.h"
#include "MaterialRegistry.h"
#include "ItemRegistry.h"

namespace Runefall
{
	namespace
	{
		struct WalletRestore
		{
			std::array<std::int32_t, 5>* wallet = nullptr;
			std::array<std::int32_t, 5>  saved{};
			~WalletRestore() { if (wallet) *wallet = saved; }
		};
	}

	namespace
	{
		std::array<std::int32_t, 5>& Wallet()
		{
			return CoSave::Get().State().runes;
		}

		constexpr std::array<std::int32_t, 5> kRuneUnits{ 1, 5, 25, 125, 625 };

		std::int32_t WalletTotalUnits()
		{
			std::int32_t total = 0;
			for (std::size_t i = 0; i < Wallet().size(); ++i)
				total += Wallet()[i] * kRuneUnits[i];
			return total;
		}

		bool SpendRunes(std::int32_t a_costUnits)
		{
			if (a_costUnits <= 0)
				return true;
			if (WalletTotalUnits() < a_costUnits)
				return false;
			// Spend from the highest rank down: one R5 is worth 625 R1 units.
			for (int rank = 4; rank >= 0; --rank) {
				auto& count = Wallet()[static_cast<std::size_t>(rank)];
				const auto unit = kRuneUnits[static_cast<std::size_t>(rank)];
				const auto use = (std::min)(count, a_costUnits / unit);
				count -= use;
				a_costUnits -= use * unit;
				if (a_costUnits == 0)
					return true;
			}
			return false;
		}

		void AddRunes(std::int32_t a_rank, std::int32_t a_amount)
		{
			if (a_rank < 1 || a_rank > 5 || a_amount <= 0)
				return;
			Wallet()[static_cast<std::size_t>(a_rank - 1)] += a_amount;
		}
	}

	bool RuneForge::SpendUnits(std::int32_t a_costUnits)
	{
		return SpendRunes(a_costUnits);
	}

	RuneForge& RuneForge::Get()
	{
		static RuneForge singleton;
		return singleton;
	}

	std::int32_t RuneForge::RerollCost(const ItemMetadata& a_item)
	{
		return 10 + static_cast<std::int32_t>(a_item.rarity) * 5;
	}

	std::int32_t RuneForge::UpgradeCost(const ItemMetadata& a_item)
	{
		return 25 + static_cast<std::int32_t>(a_item.rarity) * 10;
	}

	std::int32_t RuneForge::SocketCost(std::int32_t a_runeRank)
	{
		return (std::max)(1, a_runeRank) * 10;
	}

	std::pair<std::int32_t, std::int32_t> RuneForge::SalvageYield(const ItemMetadata& a_item)
	{
		const std::int32_t materials = 1 + a_item.rarity / 2 + a_item.ilvl / 20;
		const std::int32_t runes = 10 + a_item.rarity * 5;
		return { materials, runes };
	}

	ForgeResult RuneForge::Salvage(ItemMetadata& a_item)
	{
		const auto [materials, runes] = SalvageYield(a_item);
		if (materials <= 0 || runes <= 0)
			return { false, 7 };
		MaterialRegistry::Get().AddDefaultForTier(a_item.rarity, materials);
		AddRunes(1, runes);
		a_item.registryId = 0;
		logger::info("rune-forge: salvaged item for {} materials / {} runes", materials, runes);
		return { true, 0 };
	}

	ForgeResult RuneForge::Reroll(ItemMetadata& a_item, std::int32_t a_lockedIndex, std::string_view a_slotGroup, Rng& a_rng)
	{
		const auto cost = RerollCost(a_item);
		if (!SpendRunes(cost))
			return { false, 1 };
		a_item.registryId = 0; // registry keeps the old entry until the caller re-adds the updated one
		if (!LootEngine::Get().RerollEnhancements(a_item, a_lockedIndex, a_slotGroup, a_rng))
			return { false, 2 };
		return { true, 0 };
	}

	ForgeResult RuneForge::Upgrade(ItemMetadata& a_item, std::string_view a_slotGroup, Rng& a_rng)
	{
		if (a_item.rarity >= 5)
			return { false, 3 };
		const auto cost = UpgradeCost(a_item);
		if (!SpendRunes(cost))
			return { false, 1 };
		if (a_rng.Float01() > 0.60f)
			return { false, 4 };
		++a_item.rarity;
		a_item.enhancementCount = (std::min)(5, a_item.enhancementCount + 1);
		if (!LootEngine::Get().RerollEnhancements(a_item, -1, a_slotGroup, a_rng))
			return { false, 2 };
		return { true, 0 };
	}

	ForgeResult RuneForge::Socket(ItemMetadata& a_item, std::int32_t a_socketIndex, std::int32_t a_runeRank)
	{
		if (a_socketIndex < 0 || a_socketIndex >= a_item.socketCount || a_socketIndex >= 3)
			return { false, 5 };
		if (!SpendRunes(SocketCost(a_runeRank)))
			return { false, 1 };
		a_item.socketRunes[static_cast<std::size_t>(a_socketIndex)] = a_runeRank;
		return { true, 0 };
	}

	ForgeResult RuneForge::Extract(ItemMetadata& a_item, std::int32_t a_socketIndex)
	{
		if (a_socketIndex < 0 || a_socketIndex >= a_item.socketCount || a_socketIndex >= 3)
			return { false, 5 };
		const auto rank = a_item.socketRunes[static_cast<std::size_t>(a_socketIndex)];
		if (rank < 1 || rank > 5)
			return { false, 6 };
		a_item.socketRunes[static_cast<std::size_t>(a_socketIndex)] = 0;
		AddRunes(rank, 1);
		return { true, 0 };
	}

	ForgeResult RuneForge::RuneConvert(std::int32_t a_fromRank)
	{
		if (a_fromRank < 1 || a_fromRank >= 5)
			return { false, 5 };
		auto& wallet = Wallet();
		auto& from = wallet[static_cast<std::size_t>(a_fromRank - 1)];
		if (from < 5)
			return { false, 1 };
		from -= 5;
		wallet[static_cast<std::size_t>(a_fromRank)] += 1;
		return { true, 0 };
	}

	SelfTestResult RuneForge::SelfTest()
	{
		auto& wallet = Wallet();
		WalletRestore restore{ &wallet, wallet };
		wallet = { 100, 100, 100, 100, 100 };

		ItemMetadata item;
		item.rarity = 1;
		item.ilvl = 20;
		item.enhancementCount = 1;
		item.socketCount = 1;
		item.socketRunes[0] = 0;

		const auto beforeReroll = WalletTotalUnits();
		Rng rng(4242);
		const auto reroll = RuneForge::Get().Reroll(item, -1, "WEAPON", rng);
		if (!reroll.ok || WalletTotalUnits() >= beforeReroll)
			return { "RuneForge.SelfTest", CheckStatus::Fail, "reroll did not spend runes" };
		if (item.enhancementCount != 1 || item.enhancements[0].affixId.empty())
			return { "RuneForge.SelfTest", CheckStatus::Fail, "reroll did not fill enhancement" };

		const auto socketResult = RuneForge::Get().Socket(item, 0, 2);
		if (!socketResult.ok || item.socketRunes[0] != 2)
			return { "RuneForge.SelfTest", CheckStatus::Fail, "socket failed" };
		const auto unitsBeforeExtract = WalletTotalUnits();
		const auto extractResult = RuneForge::Get().Extract(item, 0);
		if (!extractResult.ok || item.socketRunes[0] != 0 || WalletTotalUnits() != unitsBeforeExtract + 5)
			return { "RuneForge.SelfTest", CheckStatus::Fail, "extract failed" };

		const auto beforeUpgrade = item.rarity;
		bool upgraded = false;
		for (int attempt = 0; attempt < 10 && !upgraded; ++attempt) {
			wallet = { 1000, 1000, 1000, 1000, 1000 };
			const auto result = RuneForge::Get().Upgrade(item, "ANY", rng);
			upgraded = result.ok;
		}
		if (!upgraded || item.rarity <= beforeUpgrade || item.enhancementCount != item.rarity)
			return { "RuneForge.SelfTest", CheckStatus::Fail, "upgrade failed" };

		wallet = { 5, 0, 0, 0, 0 };
		const auto convert = RuneForge::Get().RuneConvert(1);
		if (!convert.ok || wallet[0] != 0 || wallet[1] != 1)
			return { "RuneForge.SelfTest", CheckStatus::Fail, "rune conversion failed" };

		const auto salvage = SalvageYield(item);
		if (salvage.first <= 0 || salvage.second <= 0)
			return { "RuneForge.SelfTest", CheckStatus::Fail, "salvage yield invalid" };

		return { "RuneForge.SelfTest", CheckStatus::Pass, "reroll/upgrade/socket/extract/convert/salvage" };
	}
}