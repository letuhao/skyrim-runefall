#include "TierEngine.h"

#include "DataStore.h"

namespace Runefall
{
	namespace
	{
		int FindColumn(const CsvTable& a_table, std::string_view a_name)
		{
			for (std::size_t i = 0; i < a_table.headers.size(); ++i) {
				if (a_table.headers[i] == a_name)
					return static_cast<int>(i);
			}
			return -1;
		}

		std::string Cell(const std::vector<std::string>& a_row, int a_column)
		{
			if (a_column < 0 || static_cast<std::size_t>(a_column) >= a_row.size())
				return {};
			return a_row[static_cast<std::size_t>(a_column)];
		}

		float CellFloat(const std::vector<std::string>& a_row, int a_column, float a_default = 0.0f)
		{
			const auto text = Cell(a_row, a_column);
			if (text.empty())
				return a_default;
			char* end = nullptr;
			const auto value = std::strtof(text.c_str(), &end);
			return end == text.c_str() ? a_default : value;
		}

		std::int32_t CellInt(const std::vector<std::string>& a_row, int a_column, std::int32_t a_default = 0)
		{
			const auto text = Cell(a_row, a_column);
			if (text.empty())
				return a_default;
			char* end = nullptr;
			const auto value = std::strtol(text.c_str(), &end, 10);
			return end == text.c_str() ? a_default : static_cast<std::int32_t>(value);
		}
	}

	TierEngine& TierEngine::Get()
	{
		static TierEngine singleton;
		return singleton;
	}

	void TierEngine::Reload()
	{
		ranks.clear();
		spawns.clear();
		loaded = false;

		const auto* rankTable = DataStore::Get().GetTable("enemy_ranks.csv");
		const auto* spawnTable = DataStore::Get().GetTable("spawn_weights.csv");
		if (!rankTable || !spawnTable)
			return;

		const int rRank = FindColumn(*rankTable, "rank");
		const int rName = FindColumn(*rankTable, "name");
		const int rHp = FindColumn(*rankTable, "hpPct");
		const int rDmg = FindColumn(*rankTable, "dmgPct");
		const int rArmor = FindColumn(*rankTable, "armorPct");
		const int rAffix = FindColumn(*rankTable, "affixCount");
		const int rRarity = FindColumn(*rankTable, "itemRarityBonus");
		const int rRuneRank = FindColumn(*rankTable, "runeRank");
		const int rRuneMin = FindColumn(*rankTable, "runeMin");
		const int rRuneMax = FindColumn(*rankTable, "runeMax");
		const int rMat = FindColumn(*rankTable, "materialTierBonus");
		for (const auto& row : rankTable->rows) {
			RankDef def;
			def.rank = CellInt(row, rRank);
			def.name = Cell(row, rName);
			def.hpPct = CellFloat(row, rHp);
			def.dmgPct = CellFloat(row, rDmg);
			def.armorPct = CellFloat(row, rArmor);
			def.affixCount = CellInt(row, rAffix);
			def.itemRarityBonus = CellFloat(row, rRarity);
			def.runeRank = CellInt(row, rRuneRank, 1);
			def.runeMin = CellInt(row, rRuneMin, 1);
			def.runeMax = CellInt(row, rRuneMax, 1);
			def.materialTierBonus = CellInt(row, rMat);
			ranks.push_back(std::move(def));
		}

		const int sTier = FindColumn(*spawnTable, "rfTier");
		const int sRank = FindColumn(*spawnTable, "rank");
		const int sWeight = FindColumn(*spawnTable, "weight");
		for (const auto& row : spawnTable->rows) {
			SpawnWeightDef def;
			def.rfTier = CellInt(row, sTier);
			def.rank = CellInt(row, sRank);
			def.weight = CellFloat(row, sWeight);
			spawns.push_back(def);
		}

		loaded = ranks.size() == 6 && !spawns.empty();
	}

	std::int32_t TierEngine::RollRank(std::int32_t a_rfTier, Rng& a_rng) const
	{
		std::vector<std::int32_t> candidates;
		std::vector<float>        weights;
		for (const auto& spawn : spawns) {
			if (spawn.rfTier != a_rfTier || spawn.weight <= 0.0f)
				continue;
			candidates.push_back(spawn.rank);
			weights.push_back(spawn.weight);
		}
		if (candidates.empty())
			return 0;

		float total = 0.0f;
		for (const auto weight : weights)
			total += weight;
		const auto roll = a_rng.Float01() * total;
		float      running = 0.0f;
		for (std::size_t i = 0; i < weights.size(); ++i) {
			running += weights[i];
			if (roll <= running)
				return candidates[i];
		}
		return candidates.back();
	}

	SelfTestResult TierEngine::SelfTest()
	{
		TierEngine engine;
		for (int rank = 0; rank < 6; ++rank) {
			RankDef def;
			def.rank = rank;
			def.name = "Rank" + std::to_string(rank);
			engine.ranks.push_back(def);
		}
		for (int tier = 0; tier < 6; ++tier) {
			for (int rank = 1; rank <= 5; ++rank) {
				SpawnWeightDef def;
				def.rfTier = tier;
				def.rank = rank;
				def.weight = static_cast<float>(6 - rank);
				engine.spawns.push_back(def);
			}
		}
		engine.loaded = true;

		Rng a(777);
		Rng b(777);
		const auto first = engine.RollRank(2, a);
		const auto second = engine.RollRank(2, b);
		if (first != second)
			return { "TierEngine.SelfTest", CheckStatus::Fail, "rank roll is not deterministic" };

		for (int seed = 1; seed <= 500; ++seed) {
			Rng rng(static_cast<std::uint64_t>(seed));
			for (int tier = 0; tier < 6; ++tier) {
				const auto rank = engine.RollRank(tier, rng);
				if (rank < 0 || rank > 5)
					return { "TierEngine.SelfTest", CheckStatus::Fail, "rank outside 0-5" };
			}
		}

		return { "TierEngine.SelfTest", CheckStatus::Pass, "deterministic rank rolls across 500 seeds x 6 tiers" };
	}
}
