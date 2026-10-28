#include "WorldBandEngine.h"

#include "DataStore.h"

#include "RE/T/TESObjectCELL.h"
#include "RE/T/TESWorldSpace.h"

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

		std::vector<std::string> Split(const std::string& a_text)
		{
			std::vector<std::string> values;
			std::stringstream stream{ a_text };
			std::string value;
			while (std::getline(stream, value, '|'))
				if (!value.empty())
					values.push_back(value);
			return values;
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

	WorldBandEngine& WorldBandEngine::Get()
	{
		static WorldBandEngine singleton;
		return singleton;
	}

	void WorldBandEngine::Load()
	{
		bands.clear();
		loaded = false;
		const auto* table = DataStore::Get().GetTable("world_bands.csv");
		if (!table) {
			logger::error("world-band: world_bands.csv missing");
			return;
		}
		const int bId = FindColumn(*table, "bandId");
		const int bType = FindColumn(*table, "matchType");
		const int bValue = FindColumn(*table, "matchValue");
		const int bMin = FindColumn(*table, "stageMin");
		const int bMax = FindColumn(*table, "stageMax");
		const int bRate = FindColumn(*table, "rate");
		const int bExclusions = FindColumn(*table, "exclusions");
		for (const auto& row : table->rows) {
			BandDef def;
			def.bandId = Cell(row, bId);
			def.matchType = Cell(row, bType);
			def.matchValue = Cell(row, bValue);
			def.stageMin = static_cast<std::uint8_t>((std::clamp)(static_cast<int>(std::strtol(Cell(row, bMin).c_str(), nullptr, 10)), 1, 100));
			def.stageMax = static_cast<std::uint8_t>((std::clamp)(static_cast<int>(std::strtol(Cell(row, bMax).c_str(), nullptr, 10)), 1, 100));
			def.rate = std::strtof(Cell(row, bRate).c_str(), nullptr);
			def.exclusions = Split(Cell(row, bExclusions));
			if (!def.bandId.empty() && def.stageMin <= def.stageMax)
				bands.push_back(std::move(def));
		}
		loaded = !bands.empty();
		if (loaded)
			logger::info("world-band: loaded {} bands", bands.size());
		else
			logger::error("world-band: no valid bands");
	}

	bool WorldBandEngine::Excluded(const BandDef& a_band, std::string_view a_zone, std::string_view a_cell, std::string_view a_worldspace)
	{
		for (const auto& exclusion : a_band.exclusions) {
			if ((!a_zone.empty() && exclusion == a_zone) || (!a_cell.empty() && exclusion == a_cell) || (!a_worldspace.empty() && exclusion == a_worldspace))
				return true;
		}
		return false;
	}

	std::uint8_t WorldBandEngine::PickStage(const BandDef& a_band, std::uint64_t a_seed) const
	{
		const auto range = static_cast<std::uint32_t>(a_band.stageMax) - a_band.stageMin + 1;
		if (range == 0)
			return a_band.stageMin;
		const auto roll = static_cast<std::uint32_t>(Fnv(std::to_string(a_seed) + a_band.bandId) % range);
		return static_cast<std::uint8_t>(a_band.stageMin + roll);
	}

	const WorldBandEngine::BandDef* WorldBandEngine::Match(const BandDef& a_band, std::string_view a_zone, std::string_view a_cell, std::string_view a_worldspace) const
	{
		if (a_band.matchType == "encounterZone" && !a_zone.empty() && a_zone == a_band.matchValue)
			return &a_band;
		if (a_band.matchType == "cell" && !a_cell.empty() && a_cell == a_band.matchValue)
			return &a_band;
		if (a_band.matchType == "worldspace" && !a_worldspace.empty() && a_worldspace == a_band.matchValue)
			return &a_band;
		if (a_band.matchType == "location" && !a_cell.empty() && a_cell == a_band.matchValue)
			return &a_band;
		return nullptr;
	}

	std::uint8_t WorldBandEngine::StageForCell(RE::TESObjectCELL* a_cell, std::uint64_t a_seed) const
	{
		if (!loaded)
			return 0;
		std::string zone;
		std::string cell;
		std::string worldspace;
		if (a_cell) {
			if (const auto* editor = a_cell->GetFormEditorID(); editor && *editor)
				cell = editor;
			const auto& runtime = a_cell->GetRuntimeData();
			if (runtime.loadedData && runtime.loadedData->encounterZone) {
				if (const auto* editor = runtime.loadedData->encounterZone->GetFormEditorID(); editor && *editor)
					zone = editor;
			}
			if (const auto* ws = runtime.worldSpace) {
				if (const auto* editor = ws->GetFormEditorID(); editor && *editor)
					worldspace = editor;
			}
		}
		for (const auto& band : bands) {
			if (Match(band, zone, cell, worldspace) && !Excluded(band, zone, cell, worldspace))
				return PickStage(band, a_seed);
		}
		for (const auto& band : bands) {
			if (band.matchType == "default" && !Excluded(band, zone, cell, worldspace))
				return PickStage(band, a_seed);
		}
		return 0;
	}

	std::uint8_t WorldBandEngine::StageDefault(std::uint64_t a_seed) const
	{
		for (const auto& band : bands)
			if (band.matchType == "default")
				return PickStage(band, a_seed);
		return 0;
	}

	SelfTestResult WorldBandEngine::SelfTest()
	{
		auto& engine = Get();
		if (!engine.Loaded())
			return { "WorldBandEngine.SelfTest", CheckStatus::Fail, "world bands not loaded" };
		if (engine.BandCount() < 2)
			return { "WorldBandEngine.SelfTest", CheckStatus::Fail, "world band coverage too small" };
		const auto first = engine.StageDefault(1234);
		const auto second = engine.StageDefault(1234);
		if (first < 1 || first > 100 || first != second)
			return { "WorldBandEngine.SelfTest", CheckStatus::Fail, "default world band is not deterministic" };
		return { "WorldBandEngine.SelfTest", CheckStatus::Pass,
			std::format("{} bands; default seed 1234 -> stage {}", engine.BandCount(), first) };
	}
}
