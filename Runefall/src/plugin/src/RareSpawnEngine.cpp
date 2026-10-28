#include "RareSpawnEngine.h"

#include "DataStore.h"

#include <algorithm>
#include <format>

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
		int Int(const std::vector<std::string>& a_row, int a_column, int a_default = 0)
		{
			const auto text = Cell(a_row, a_column);
			return text.empty() ? a_default : static_cast<int>(std::strtol(text.c_str(), nullptr, 10));
		}
		float Float(const std::vector<std::string>& a_row, int a_column, float a_default = 0.0f)
		{
			const auto text = Cell(a_row, a_column);
			return text.empty() ? a_default : std::strtof(text.c_str(), nullptr);
		}
		bool Bool(const std::vector<std::string>& a_row, int a_column)
		{
			return Cell(a_row, a_column) == "1";
		}
		std::uint64_t Fnv(std::string_view a_text)
		{
			std::uint64_t hash = 1469598103934665603ull;
			for (const auto c : a_text) {
				hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
				hash *= 1099511628211ull;
			}
			return hash;
		}
	}

	RareSpawnEngine& RareSpawnEngine::Get()
	{
		static RareSpawnEngine singleton;
		return singleton;
	}

	void RareSpawnEngine::Load()
	{
		entries.clear();
		loaded = false;
		const auto* table = DataStore::Get().GetTable("rare_spawns.csv");
		if (!table) {
			logger::error("rare-spawn: rare_spawns.csv missing");
			return;
		}
		const int rId = FindColumn(*table, "rareId");
		const int rBand = FindColumn(*table, "bandId");
		const int rActor = FindColumn(*table, "actorEditorId");
		const int rChance = FindColumn(*table, "chancePermille");
		const int rMin = FindColumn(*table, "stageMin");
		const int rMax = FindColumn(*table, "stageMax");
		const int rCondition = FindColumn(*table, "conditionTag");
		const int rWeight = FindColumn(*table, "weight");
		const int rEnabled = FindColumn(*table, "enabled");
		for (const auto& row : table->rows) {
			Entry entry;
			entry.rareId = Cell(row, rId);
			entry.bandId = Cell(row, rBand);
			entry.actorEditorId = Cell(row, rActor);
			entry.chancePermille = (std::clamp)(Int(row, rChance), 0, 1000);
			entry.stageMin = static_cast<std::uint8_t>((std::clamp)(Int(row, rMin, 80), 1, 100));
			entry.stageMax = static_cast<std::uint8_t>((std::clamp)(Int(row, rMax, 100), 1, 100));
			entry.conditionTag = Cell(row, rCondition);
			entry.weight = Float(row, rWeight);
			entry.enabled = Bool(row, rEnabled);
			if (!entry.rareId.empty() && entry.stageMin <= entry.stageMax)
				entries.push_back(std::move(entry));
		}
		loaded = !entries.empty();
		if (loaded)
			logger::info("rare-spawn: loaded {} entries", entries.size());
	}

	bool RareSpawnEngine::TryPick(std::string_view a_bandId, std::uint64_t a_seed, RareSpawn& a_out) const
	{
		if (!loaded)
			return false;
		for (const auto& entry : entries) {
			if (!entry.enabled || entry.bandId != a_bandId)
				continue;
			const auto roll = static_cast<int>(Fnv(std::to_string(a_seed) + entry.rareId) % 1000ull);
			if (roll >= entry.chancePermille)
				continue;
			if (!entry.conditionTag.empty())
				continue; // condition hooks are content-pack data; unknown tags fail closed for now
			const auto range = static_cast<std::uint32_t>(entry.stageMax) - entry.stageMin + 1;
			const auto stage = entry.stageMin + static_cast<std::uint8_t>(range == 0 ? 0 : Fnv(std::to_string(a_seed) + entry.rareId) % range);
			a_out.rareId = entry.rareId;
			a_out.actorEditorId = entry.actorEditorId;
			a_out.stage = stage;
			a_out.conditionTag = entry.conditionTag;
			return true;
		}
		return false;
	}

	SelfTestResult RareSpawnEngine::SelfTest()
	{
		auto& engine = Get();
		if (!engine.Loaded())
			return { "RareSpawnEngine.SelfTest", CheckStatus::Fail, "rare spawn table not loaded" };
		RareSpawn first;
		if (!engine.TryPick("tamriel", 4242, first))
			return { "RareSpawnEngine.SelfTest", CheckStatus::Fail, "rare pick failed" };
		RareSpawn second;
		if (!engine.TryPick("tamriel", 4242, second) || first.rareId != second.rareId || first.stage != second.stage)
			return { "RareSpawnEngine.SelfTest", CheckStatus::Fail, "rare pick is not deterministic" };
		return { "RareSpawnEngine.SelfTest", CheckStatus::Pass,
			std::format("{} entries; seed 4242 -> {} stage {}", engine.EntryCount(), first.rareId, static_cast<int>(first.stage)) };
	}
}
