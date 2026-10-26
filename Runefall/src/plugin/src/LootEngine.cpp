#include "LootEngine.h"

#include "DataStore.h"
#include "ItemRegistry.h"

#include <charconv>
#include <limits>

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

		float WeightedPick(const std::vector<float>& a_weights, Rng& a_rng)
		{
			float total = 0.0f;
			for (const auto weight : a_weights)
				total += weight > 0.0f ? weight : 0.0f;
			if (total <= 0.0f)
				return 0.0f;
			const auto roll = a_rng.Float01() * total;
			float      running = 0.0f;
			for (std::size_t i = 0; i < a_weights.size(); ++i) {
				running += a_weights[i] > 0.0f ? a_weights[i] : 0.0f;
				if (roll <= running)
					return static_cast<float>(i);
			}
			return static_cast<float>(a_weights.size() - 1);
		}

		bool SlotsMatch(std::string_view a_slots, std::string_view a_slotGroup)
		{
			if (a_slotGroup.empty() || a_slotGroup == "ANY")
				return true;
			for (const auto& token : Split(a_slots, '|')) {
				if (token == a_slotGroup)
					return true;
			}
			return false;
		}
	}

	std::uint32_t Rng::NextU32()
	{
		state ^= state << 13;
		state ^= state >> 7;
		state ^= state << 17;
		return static_cast<std::uint32_t>(state >> 16);
	}

	float Rng::Float01()
	{
		return static_cast<float>(NextU32() & 0xFFFFFFu) / static_cast<float>(0x1000000u);
	}

	std::int32_t Rng::Range(std::int32_t a_min, std::int32_t a_max)
	{
		if (a_max <= a_min)
			return a_min;
		return a_min + static_cast<std::int32_t>(NextU32() % static_cast<std::uint32_t>(a_max - a_min + 1));
	}

	LootEngine& LootEngine::Get()
	{
		static LootEngine singleton;
		return singleton;
	}

	void LootEngine::Reload()
	{
		data = Dataset{};
		loaded = false;

		const auto* affixTable = DataStore::Get().GetTable("affixes.csv");
		const auto* rarityTable = DataStore::Get().GetTable("rarity.csv");
		const auto* weightTable = DataStore::Get().GetTable("rarity_weights.csv");
		const auto* dropTable = DataStore::Get().GetTable("drops_general.csv");
		if (!affixTable || !rarityTable || !weightTable || !dropTable)
			return;

		const int aId = FindColumn(*affixTable, "affixId");
		const int aCategory = FindColumn(*affixTable, "category");
		const int aTier = FindColumn(*affixTable, "tier");
		const int aMin = FindColumn(*affixTable, "minValue");
		const int aMax = FindColumn(*affixTable, "maxValue");
		const int aDuration = FindColumn(*affixTable, "duration");
		const int aArea = FindColumn(*affixTable, "area");
		const int aWeight = FindColumn(*affixTable, "weight");
		const int aMinLevel = FindColumn(*affixTable, "minItemLevel");
		const int aEffect = FindColumn(*affixTable, "effectEditorId");
		const int aPrefix = FindColumn(*affixTable, "prefix");
		const int aSuffix = FindColumn(*affixTable, "suffix");
		const int aSlots = FindColumn(*affixTable, "slots");
		const int aUnit = FindColumn(*affixTable, "unit");
		for (const auto& row : affixTable->rows) {
			AffixDef def;
			def.id = Cell(row, aId);
			def.category = Cell(row, aCategory);
			def.tier = CellInt(row, aTier, 1);
			def.minValue = CellFloat(row, aMin);
			def.maxValue = CellFloat(row, aMax);
			def.duration = CellInt(row, aDuration);
			def.area = CellInt(row, aArea);
			def.weight = CellFloat(row, aWeight, 1.0f);
			def.minItemLevel = CellInt(row, aMinLevel, 1);
			def.effectEditorId = Cell(row, aEffect);
			def.prefix = Cell(row, aPrefix);
			def.suffix = Cell(row, aSuffix);
			def.slots = Cell(row, aSlots);
			def.unit = Cell(row, aUnit);
			if (!def.id.empty())
				data.affixes.push_back(std::move(def));
		}

		const int rId = FindColumn(*rarityTable, "rarityId");
		const int rName = FindColumn(*rarityTable, "name");
		const int rCount = FindColumn(*rarityTable, "enhancementCount");
		const int rSocketMin = FindColumn(*rarityTable, "socketMin");
		const int rSocketMax = FindColumn(*rarityTable, "socketMax");
		for (const auto& row : rarityTable->rows) {
			RarityDef def;
			def.id = CellInt(row, rId);
			def.name = Cell(row, rName);
			def.enhancementCount = CellInt(row, rCount);
			def.socketMin = CellInt(row, rSocketMin);
			def.socketMax = CellInt(row, rSocketMax);
			data.rarities.push_back(std::move(def));
		}

		const int wTier = FindColumn(*weightTable, "rfTier");
		const int wRarity = FindColumn(*weightTable, "rarityId");
		const int wWeight = FindColumn(*weightTable, "weight");
		for (const auto& row : weightTable->rows) {
			RarityWeightDef def;
			def.rfTier = CellInt(row, wTier);
			def.rarityId = CellInt(row, wRarity);
			def.weight = CellFloat(row, wWeight, 1.0f);
			data.weights.push_back(def);
		}

		const int dRank = FindColumn(*dropTable, "rank");
		const int dTier = FindColumn(*dropTable, "rfTier");
		const int dChance = FindColumn(*dropTable, "itemChancePct");
		const int dMin = FindColumn(*dropTable, "itemCountMin");
		const int dMax = FindColumn(*dropTable, "itemCountMax");
		const int dRarityBias = FindColumn(*dropTable, "rarityBias");
		const int dRuneMin = FindColumn(*dropTable, "runeMin");
		const int dRuneMax = FindColumn(*dropTable, "runeMax");
		const int dUnique = FindColumn(*dropTable, "uniqueChancePct");
		for (const auto& row : dropTable->rows) {
			GeneralDropDef def;
			def.rank = CellInt(row, dRank);
			def.rfTier = CellInt(row, dTier);
			def.itemChancePct = CellFloat(row, dChance);
			def.itemCountMin = CellInt(row, dMin, 1);
			def.itemCountMax = CellInt(row, dMax, 1);
			def.rarityBias = CellFloat(row, dRarityBias);
			def.runeMin = CellInt(row, dRuneMin, 1);
			def.runeMax = CellInt(row, dRuneMax, 1);
			def.uniqueChancePct = CellFloat(row, dUnique);
			data.generalDrops.push_back(def);
		}

		loaded = !data.affixes.empty() && data.rarities.size() == 6 && !data.weights.empty();
	}

	std::optional<ItemMetadata> LootEngine::GenerateFrom(
		const Dataset& a_data,
		std::int32_t a_rank,
		std::int32_t a_rfTier,
		std::int32_t a_ilvl,
		std::uint16_t a_tags,
		std::string_view a_slotGroup,
		Rng& a_rng) const
	{
		if (a_data.rarities.empty() || a_data.weights.empty() || a_data.affixes.empty())
			return std::nullopt;

		// Rank bias shifts weight toward higher rarities.
		float bias = 0.0f;
		for (const auto& drop : a_data.generalDrops) {
			if (drop.rank == a_rank && drop.rfTier == a_rfTier) {
				bias = drop.rarityBias;
				break;
			}
		}

		std::vector<RarityDef> candidates;
		std::vector<float>     weights;
		for (const auto& rarity : a_data.rarities) {
			float weight = 0.0f;
			for (const auto& def : a_data.weights) {
				if (def.rfTier == a_rfTier && def.rarityId == rarity.id) {
					weight = def.weight;
					break;
				}
			}
			weight *= 1.0f + (bias * 0.02f * static_cast<float>(rarity.enhancementCount));
			candidates.push_back(rarity);
			weights.push_back(weight);
		}

		const auto picked = static_cast<std::size_t>(WeightedPick(weights, a_rng));
		if (picked >= candidates.size())
			return std::nullopt;
		const auto& rarity = candidates[picked];

		ItemMetadata item;
		item.rarity = static_cast<std::uint8_t>(rarity.id);
		item.ilvl = static_cast<std::uint16_t>((std::max)(1, a_ilvl));
		item.enhancementCount = static_cast<std::uint8_t>((std::min)(5, rarity.enhancementCount));
		item.tags = a_tags;
		item.socketCount = static_cast<std::uint8_t>((std::max)(0, a_rng.Range(rarity.socketMin, rarity.socketMax)));
		for (std::uint8_t socket = 0; socket < item.socketCount && socket < 3; ++socket)
			item.socketRunes[socket] = a_rng.Range(1, 5);

		const std::int32_t maxAffixTier = (std::min)(5, 1 + (a_ilvl / 20));
		std::unordered_set<std::string> usedCategories;
		for (std::uint8_t slot = 0; slot < item.enhancementCount; ++slot) {
			std::vector<const AffixDef*> eligible;
			std::vector<float>           affixWeights;
			for (const auto& affix : a_data.affixes) {
				if (affix.tier > maxAffixTier)
					continue;
				if (affix.minItemLevel > a_ilvl)
					continue;
				if (!SlotsMatch(affix.slots, a_slotGroup))
					continue;
				if (usedCategories.contains(affix.category))
					continue;
				eligible.push_back(&affix);
				affixWeights.push_back(affix.weight);
			}
			if (eligible.empty())
				break;
			const auto index = static_cast<std::size_t>(WeightedPick(affixWeights, a_rng));
			const auto* affix = eligible[(std::min)(index, eligible.size() - 1)];
			usedCategories.insert(affix->category);

			auto& enhancement = item.enhancements[slot];
			enhancement.affixId = affix->id;
			enhancement.effectEditorId = affix->effectEditorId;
			enhancement.category = static_cast<std::int32_t>(slot);
			enhancement.tier = affix->tier;
			enhancement.magnitude = affix->minValue + (affix->maxValue - affix->minValue) * a_rng.Float01();
			enhancement.duration = affix->duration;
			enhancement.area = affix->area;
		}

		return item;
	}

	std::optional<ItemMetadata> LootEngine::Generate(
		std::int32_t a_rank,
		std::int32_t a_rfTier,
		std::int32_t a_ilvl,
		std::uint16_t a_tags,
		std::string_view a_slotGroup,
		Rng& a_rng)
	{
		if (!loaded)
			return std::nullopt;
		return GenerateFrom(data, a_rank, a_rfTier, a_ilvl, a_tags, a_slotGroup, a_rng);
	}

	SelfTestResult LootEngine::SelfTest()
	{
		Dataset synthetic;
		for (int rarity = 0; rarity < 6; ++rarity) {
			RarityDef def;
			def.id = rarity;
			def.name = "Rarity" + std::to_string(rarity);
			def.enhancementCount = rarity;
			def.socketMin = rarity / 2;
			def.socketMax = rarity / 2;
			synthetic.rarities.push_back(def);
			for (int tier = 0; tier < 6; ++tier) {
				RarityWeightDef weight;
				weight.rfTier = tier;
				weight.rarityId = rarity;
				weight.weight = 10.0f - static_cast<float>(rarity) - (tier * 0.2f) + 1.0f;
				synthetic.weights.push_back(weight);
			}
		}
		const char* categories[] = { "offense", "defense", "utility", "economy", "elemental" };
		for (int i = 0; i < 20; ++i) {
			AffixDef def;
			def.id = "test_affix_" + std::to_string(i);
			def.category = categories[i % 5];
			def.tier = 1 + (i % 5);
			def.minValue = 1.0f;
			def.maxValue = 10.0f;
			def.weight = 1.0f;
			def.minItemLevel = 1;
			def.effectEditorId = "TEST_MGEF_" + std::to_string(i);
			def.slots = "WEAPON|ARMOR|RING";
			synthetic.affixes.push_back(def);
		}
		GeneralDropDef drop;
		drop.rank = 2;
		drop.rfTier = 2;
		drop.rarityBias = 4.0f;
		synthetic.generalDrops.push_back(drop);

		LootEngine engine;
		engine.data = synthetic;

		Rng a(12345);
		Rng b(12345);
		const auto first = engine.GenerateFrom(synthetic, 2, 2, 30, static_cast<std::uint16_t>(ItemTag::Debug), "WEAPON", a);
		const auto second = engine.GenerateFrom(synthetic, 2, 2, 30, static_cast<std::uint16_t>(ItemTag::Debug), "WEAPON", b);
		if (!first || !second)
			return { "LootEngine.SelfTest", CheckStatus::Fail, "generation returned null" };
		if (first->rarity != second->rarity || first->enhancementCount != second->enhancementCount ||
			first->socketCount != second->socketCount ||
			(first->enhancementCount > 0 && first->enhancements[0].affixId != second->enhancements[0].affixId) ||
			(first->enhancementCount > 0 && first->enhancements[0].magnitude != second->enhancements[0].magnitude))
			return { "LootEngine.SelfTest", CheckStatus::Fail, "same seed produced different items" };

		for (int seed = 1; seed <= 500; ++seed) {
			Rng rng(static_cast<std::uint64_t>(seed));
			const auto item = engine.GenerateFrom(synthetic, 2, 2, 30, 0, "ANY", rng);
			if (!item)
				return { "LootEngine.SelfTest", CheckStatus::Fail, "generation failed during sweep" };
			if (item->rarity > 5 || item->enhancementCount > 5 || item->socketCount > 3)
				return { "LootEngine.SelfTest", CheckStatus::Fail, "roll produced out-of-range values" };
			if (item->enhancementCount != item->rarity)
				return { "LootEngine.SelfTest", CheckStatus::Fail, "enhancement count does not match rarity" };
			std::unordered_set<std::string> categoriesSeen;
			for (std::uint8_t e = 0; e < item->enhancementCount; ++e) {
				const auto& enhancement = item->enhancements[e];
				if (enhancement.affixId.empty() || enhancement.magnitude < 1.0f || enhancement.magnitude > 10.0f)
					return { "LootEngine.SelfTest", CheckStatus::Fail, "invalid enhancement values" };
				const auto def = std::find_if(synthetic.affixes.begin(), synthetic.affixes.end(), [&](const AffixDef& d) { return d.id == enhancement.affixId; });
				if (def == synthetic.affixes.end())
					return { "LootEngine.SelfTest", CheckStatus::Fail, "unknown affix selected" };
				if (!categoriesSeen.insert(def->category).second)
					return { "LootEngine.SelfTest", CheckStatus::Fail, "duplicate affix category on one item" };
			}
		}

		return { "LootEngine.SelfTest", CheckStatus::Pass, "deterministic, 500-roll rarity/affix/category checks" };
	}
}