#include "DataStore.h"

#include <format>
#include <nlohmann/json.hpp>

namespace Runefall
{
	namespace
	{
		const std::vector<std::string> kExpectedTables = {
			"affixes.csv",
			"rarity.csv",
			"rarity_weights.csv",
			"enemy_ranks.csv",
			"spawn_weights.csv",
			"drops_general.csv",
			"drops_containers.csv",
			"drops_quests.csv",
			"drops_bosses.csv",
			"race_groups.csv",
			"drops_races.csv",
			"materials.csv",
			"recipes.csv",
			"skills.csv",
			"attributes.csv",
	"attribute_effects.csv",
			"unique_affixes.csv",
			"unique_sources.csv",
			"strings.csv",
			"form_ids.csv",
			"stage_ladder.csv",
			"tier_definitions.csv",
			"curve_profiles.csv",
			"difficulty_profiles.csv",
			"stage_assignment_rules.csv",
			"stage_content_bands.csv",
			"legacy_rf_tier_map.csv",
			"roles.csv",
			"race_profiles.csv",
			"race_classification_rules.csv",
			"threat_protection.csv",
			"enemy_packages.csv",
			"equipment_sets.csv",
			"player_progression.csv",
			"rating_profiles.csv",
			"combat_pace_profiles.csv",
			"rating_display_profiles.csv",
			"penetration_profiles.csv",
			"world_bands.csv",
		};
	}

	DataStore& DataStore::Get()
	{
		static DataStore singleton;
		return singleton;
	}

	const std::vector<std::string>& DataStore::ExpectedTables()
	{
		return kExpectedTables;
	}

	bool DataStore::ParseJsonFile(const fs::path& a_path, CsvTable& a_out)
	{
		try {
			std::ifstream file{ a_path };
			if (!file)
				return false;
			auto json = nlohmann::json::parse(file);
			if (!json.is_object())
				return false;
			if (json.value("schemaVersion", 0) != 1) {
				a_out.error = "unsupported JSON table schemaVersion";
				return false;
			}
			const auto columns = json.find("columns");
			const auto rows = json.find("rows");
			if (columns == json.end() || !columns->is_array() || rows == json.end() || !rows->is_array())
				return false;
			for (const auto& column : *columns) {
				if (!column.is_string())
					return false;
				a_out.headers.push_back(column.get<std::string>());
			}
			for (const auto& row : *rows) {
				if (!row.is_object())
					return false;
				std::vector<std::string> values;
				values.reserve(a_out.headers.size());
				for (const auto& header : a_out.headers) {
					const auto value = row.find(header);
					if (value == row.end())
						return false;
					if (value->is_string())
						values.push_back(value->get<std::string>());
					else if (value->is_boolean())
						values.push_back(value->get<bool>() ? "1" : "0");
					else if (value->is_number())
						values.push_back(value->dump());
					else if (value->is_null())
						values.push_back({});
					else
						return false;
				}
				a_out.rows.push_back(std::move(values));
			}
			return true;
		} catch (const std::exception& e) {
			a_out.error = std::string{ "json: " } + e.what();
			return false;
		}
	}

	SelfTestResult DataStore::LoadAll(const fs::path& a_dataDir)
	{
		tables.clear();
		dataDir = a_dataDir;

		std::vector<std::string> missing;
		std::vector<std::string> malformed;

		// formats.json is optional until the content pipeline generates it.
		const auto formatsPath = a_dataDir / "formats.json";
		if (fs::exists(formatsPath)) {
			try {
				std::ifstream f{ formatsPath };
				auto          json = nlohmann::json::parse(f);
				formatVersion = json.value("format", 0);
				if (formatVersion != 2)
					malformed.emplace_back("formats.json declares format " + std::to_string(formatVersion) + ", expected 2");
			} catch (const std::exception& e) {
				malformed.emplace_back("formats.json: " + std::string{ e.what() });
			}
		}

		for (const auto& name : kExpectedTables) {
			CsvTable table;
			table.name = name;
			auto jsonPath = a_dataDir / name;
			jsonPath.replace_extension(".json");
			if (!fs::exists(jsonPath)) {
				missing.emplace_back(jsonPath.filename().string());
			} else if (!ParseJsonFile(jsonPath, table)) {
				malformed.emplace_back(jsonPath.filename().string() + ": " + table.error);
			} else {
				table.loaded = true;
			}
			tables[name] = std::move(table);
		}

		if (!malformed.empty())
			return { "DataStore.LoadAll", CheckStatus::Fail, std::format("{} malformed file(s): {}", malformed.size(), malformed.front()) };

		if (missing.size() == kExpectedTables.size())
			return { "DataStore.LoadAll", CheckStatus::Warn, "no data files yet (content pipeline pending); dir=" + a_dataDir.string() };
		if (!missing.empty())
			return { "DataStore.LoadAll", CheckStatus::Warn, std::format("loaded {} of {} tables; missing {} (first: {})", kExpectedTables.size() - missing.size(), kExpectedTables.size(), missing.size(), missing.front()) };
		return { "DataStore.LoadAll", CheckStatus::Pass, std::format("loaded {} tables, {} rows", tables.size(), TotalRows()) };
	}

	SelfTestResult DataStore::SelfTest() const
	{
		if (!tables.empty() && formatVersion == 0) {
			bool anyLoaded = false;
			for (const auto& [name, table] : tables)
				anyLoaded = anyLoaded || table.loaded;
			if (anyLoaded)
				return { "DataStore.SelfTest", CheckStatus::Warn, "data loaded but formats.json/format version is missing" };
		}
		return { "DataStore.SelfTest", CheckStatus::Pass, std::to_string(tables.size()) + " tables registered" };
	}

	const CsvTable* DataStore::GetTable(std::string_view a_name) const
	{
		const auto it = tables.find(std::string{ a_name });
		return it == tables.end() ? nullptr : &it->second;
	}

	std::string DataStore::GetString(std::string_view a_key) const
	{
		const auto* table = GetTable("strings.csv");
		if (!table)
			return {};
		for (const auto& row : table->rows) {
			if (row.size() >= 2 && row[0] == a_key)
				return row[1];
		}
		return {};
	}

	std::size_t DataStore::TotalRows() const
	{
		std::size_t total = 0;
		for (const auto& [name, table] : tables)
			total += table.rows.size();
		return total;
	}
}