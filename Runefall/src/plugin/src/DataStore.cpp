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

	bool DataStore::ParseCsvFile(const fs::path& a_path, CsvTable& a_out)
	{
		std::ifstream file{ a_path, std::ios::binary };
		if (!file.is_open()) {
			a_out.error = "cannot open";
			return false;
		}

		std::vector<std::vector<std::string>> records;
		std::vector<std::string>              currentRecord;
		std::string                           currentField;
		bool                                  inQuotes = false;
		char                                  c;

		while (file.get(c)) {
			if (inQuotes) {
				if (c == '"') {
					if (file.peek() == '"') {
						file.get(c);
						currentField.push_back('"');
					} else {
						inQuotes = false;
					}
				} else {
					currentField.push_back(c);
				}
				continue;
			}

			switch (c) {
			case '"':
				inQuotes = true;
				break;
			case ',':
				currentRecord.emplace_back(std::move(currentField));
				currentField.clear();
				break;
			case '\r':
				break;
			case '\n':
				currentRecord.emplace_back(std::move(currentField));
				currentField.clear();
				records.emplace_back(std::move(currentRecord));
				currentRecord.clear();
				break;
			default:
				currentField.push_back(c);
				break;
			}
		}
		if (inQuotes) {
			a_out.error = "unterminated quoted field";
			return false;
		}
		if (!currentField.empty() || !currentRecord.empty()) {
			currentRecord.emplace_back(std::move(currentField));
			records.emplace_back(std::move(currentRecord));
		}

		if (records.empty()) {
			a_out.error = "empty file";
			return false;
		}

		for (auto& header : records.front())
			header = std::string{ Trim(header) };
		if (records.front().empty() || records.front().front().empty()) {
			a_out.error = "empty header";
			return false;
		}

		a_out.headers = records.front();
		for (std::size_t i = 1; i < records.size(); ++i) {
			auto& row = records[i];
			if (row.empty() || (row.size() == 1 && row.front().empty()))
				continue;
			if (row.size() != a_out.headers.size()) {
				a_out.error = std::format("row {} has {} fields, expected {}", i + 1, row.size(), a_out.headers.size());
				return false;
			}
			a_out.rows.emplace_back(std::move(row));
		}
		return true;
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
			const auto path = a_dataDir / name;
			if (!fs::exists(path)) {
				missing.emplace_back(name);
			} else if (!ParseCsvFile(path, table)) {
				malformed.emplace_back(name + ": " + table.error);
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