#pragma once
#include "Core.h"

namespace Runefall
{
	struct CsvTable
	{
		std::string                         name;
		std::vector<std::string>            headers;
		std::vector<std::vector<std::string>> rows;
		bool                                loaded = false;
		std::string                         error;
	};

	class DataStore
	{
	public:
		static DataStore& Get();

		SelfTestResult LoadAll(const fs::path& a_dataDir);
		SelfTestResult SelfTest() const;

		[[nodiscard]] const CsvTable* GetTable(std::string_view a_name) const;
		[[nodiscard]] std::size_t    TotalRows() const;
		[[nodiscard]] fs::path       DataDirectory() const { return dataDir; }
		[[nodiscard]] int            FormatVersion() const { return formatVersion; }

		[[nodiscard]] static const std::vector<std::string>& ExpectedTables();

	private:
		static bool ParseCsvFile(const fs::path& a_path, CsvTable& a_out);

		std::unordered_map<std::string, CsvTable> tables;
		fs::path                                  dataDir;
		int                                       formatVersion = 0;
	};
}
