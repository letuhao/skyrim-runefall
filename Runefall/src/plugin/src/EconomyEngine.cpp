#include "EconomyEngine.h"

#include "DataStore.h"

#include <algorithm>
#include <format>
#include <sstream>

namespace Runefall
{
	namespace
	{
		int FindColumn(const CsvTable& a_table, std::string_view a_name)
		{
			for (std::size_t i = 0; i < a_table.headers.size(); ++i)
				if (a_table.headers[i] == a_name)
					return static_cast<int>(i);
			return -1;
		}
		std::string Cell(const std::vector<std::string>& a_row, int a_column)
		{
			if (a_column < 0 || static_cast<std::size_t>(a_column) >= a_row.size())
				return {};
			return a_row[static_cast<std::size_t>(a_column)];
		}
		float Float(const std::vector<std::string>& a_row, int a_column, float a_default = 0.0f)
		{
			const auto text = Cell(a_row, a_column);
			return text.empty() ? a_default : std::strtof(text.c_str(), nullptr);
		}
	}

	EconomyEngine& EconomyEngine::Get()
	{
		static EconomyEngine singleton;
		return singleton;
	}

	void EconomyEngine::Load()
	{
		rarityMultipliers = { 1.0f, 1.5f, 2.5f, 4.0f, 6.0f, 9.0f };
		loaded = false;
		const auto* table = DataStore::Get().GetTable("economy_profiles.csv");
		if (!table || table->rows.empty()) {
			logger::error("economy: economy_profiles.csv missing");
			return;
		}
		const auto& row = table->rows.front();
		const int cBase = FindColumn(*table, "baseItemValue");
		const int cPer = FindColumn(*table, "valuePerIlvl");
		const int cRarity = FindColumn(*table, "rarityMultipliers");
		const int cBuy = FindColumn(*table, "vendorBuyMultiplier");
		const int cSell = FindColumn(*table, "vendorSellMultiplier");
		const int cRune = FindColumn(*table, "runeValue");
		const int cMat = FindColumn(*table, "materialValue");
		const int cRespec = FindColumn(*table, "respecBaseCost");
		const int cRespecPer = FindColumn(*table, "respecCostPerLevel");
		const int cSalvage = FindColumn(*table, "salvageValuePct");
		baseItemValue = Float(row, cBase, 25.0f);
		valuePerIlvl = Float(row, cPer, 5.0f);
		vendorBuyMultiplier = Float(row, cBuy, 1.5f);
		vendorSellMultiplier = Float(row, cSell, 0.35f);
		runeValue = Float(row, cRune, 10.0f);
		materialValue = Float(row, cMat, 5.0f);
		respecBaseCost = Float(row, cRespec, 500.0f);
		respecCostPerLevel = Float(row, cRespecPer, 100.0f);
		salvageValuePct = Float(row, cSalvage, 0.25f);
		rarityMultipliers.clear();
		std::stringstream stream{ Cell(row, cRarity) };
		std::string token;
		while (std::getline(stream, token, '|'))
			if (!token.empty())
				rarityMultipliers.push_back(std::strtof(token.c_str(), nullptr));
		if (rarityMultipliers.size() < 6)
			rarityMultipliers = { 1.0f, 1.5f, 2.5f, 4.0f, 6.0f, 9.0f };
		loaded = true;
		logger::info("economy: loaded base={} perIlvl={} respec={}+{}", baseItemValue, valuePerIlvl, respecBaseCost, respecCostPerLevel);
	}

	float EconomyEngine::ItemValue(std::int32_t a_ilvl, std::int32_t a_rarity) const
	{
		const auto rarity = (std::clamp)(a_rarity, 0, static_cast<int>(rarityMultipliers.size()) - 1);
		const auto multiplier = rarityMultipliers[static_cast<std::size_t>(rarity)];
		return (baseItemValue + valuePerIlvl * static_cast<float>((std::max)(0, a_ilvl))) * multiplier;
	}

	float EconomyEngine::VendorBuy(float a_value) const
	{
		return (std::max)(0.0f, a_value) * vendorBuyMultiplier;
	}

	float EconomyEngine::VendorSell(float a_value) const
	{
		return (std::max)(0.0f, a_value) * vendorSellMultiplier;
	}

	float EconomyEngine::RespecCost(std::int32_t a_level) const
	{
		return respecBaseCost + respecCostPerLevel * static_cast<float>((std::max)(0, a_level));
	}

	SelfTestResult EconomyEngine::SelfTest()
	{
		auto& engine = Get();
		if (!engine.Loaded())
			return { "EconomyEngine.SelfTest", CheckStatus::Fail, "economy profile not loaded" };
		const auto common = engine.ItemValue(10, 0);
		const auto mythic = engine.ItemValue(10, 5);
		if (common <= 0.0f || mythic <= common)
			return { "EconomyEngine.SelfTest", CheckStatus::Fail, "item value scaling is invalid" };
		if (engine.VendorBuy(common) <= engine.VendorSell(common))
			return { "EconomyEngine.SelfTest", CheckStatus::Fail, "vendor buy/sell spread is invalid" };
		if (engine.RespecCost(10) <= engine.RespecCost(1))
			return { "EconomyEngine.SelfTest", CheckStatus::Fail, "respec cost does not scale" };
		return { "EconomyEngine.SelfTest", CheckStatus::Pass,
			std::format("common {:.0f}, mythic {:.0f}, buy {:.0f}, sell {:.0f}, respec L10 {:.0f}",
				common, mythic, engine.VendorBuy(common), engine.VendorSell(common), engine.RespecCost(10)) };
	}
}
