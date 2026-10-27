#include "FormCatalog.h"

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
	}

	FormCatalog& FormCatalog::Get()
	{
		static FormCatalog singleton;
		return singleton;
	}

	void FormCatalog::Load()
	{
		entries.clear();
		loaded = false;

		const auto* table = DataStore::Get().GetTable("form_ids.csv");
		if (!table) {
			logger::warn("form-catalog: form_ids.csv not loaded; falling back to legacy local FormIDs");
			return;
		}
		const int cEditor = FindColumn(*table, "editorId");
		const int cPlugin = FindColumn(*table, "plugin");
		const int cLocal = FindColumn(*table, "localFormId");
		const int cType = FindColumn(*table, "recordType");
		if (cEditor < 0 || cPlugin < 0 || cLocal < 0) {
			logger::error("form-catalog: form_ids.csv is missing required columns");
			return;
		}

		for (const auto& row : table->rows) {
			Entry entry;
			entry.editorID = Cell(row, cEditor);
			entry.plugin = Cell(row, cPlugin);
			entry.recordType = Cell(row, cType);
			if (entry.editorID.empty() || entry.plugin.empty())
				continue;
			char* end = nullptr;
			const auto local = std::strtoul(Cell(row, cLocal).c_str(), &end, 16);
			if (end == nullptr || *end != '\0' || local == 0 || local > 0xFFFFFF)
				continue;
			entry.localFormID = static_cast<std::uint32_t>(local);
			entries.emplace(entry.editorID, std::move(entry));
		}
		loaded = !entries.empty();
		logger::info("form-catalog: loaded {} editor-ID records", entries.size());
	}

	RE::TESForm* FormCatalog::Lookup(std::string_view a_editorID)
	{
		if (!loaded || a_editorID.empty())
			return nullptr;
		const auto it = entries.find(std::string{ a_editorID });
		if (it == entries.end())
			return nullptr;
		auto& entry = it->second;
		if (entry.resolved) {
			if (entry.resolved->IsDeleted()) {
				logger::warn("form-catalog: '{}' resolved to a deleted form", a_editorID);
				entry.resolved = nullptr;
				return nullptr;
			}
			return entry.resolved;
		}
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler)
			return nullptr;
		auto* form = dataHandler->LookupForm(entry.localFormID, entry.plugin);
		if (!form) {
			logger::warn("form-catalog: '{}' missing ({:06X} in '{}')", a_editorID, entry.localFormID, entry.plugin);
			return nullptr;
		}
		if (form->IsDeleted()) {
			logger::warn("form-catalog: '{}' is deleted", a_editorID);
			return nullptr;
		}
		entry.resolved = form;
		return form;
	}

	std::uint32_t FormCatalog::LocalFormID(std::string_view a_editorID) const
	{
		const auto it = entries.find(std::string{ a_editorID });
		return it == entries.end() ? 0 : it->second.localFormID;
	}

	std::string_view FormCatalog::PluginFor(std::string_view a_editorID) const
	{
		const auto it = entries.find(std::string{ a_editorID });
		return it == entries.end() ? std::string_view{} : std::string_view{ it->second.plugin };
	}

	std::string_view FormCatalog::RecordTypeFor(std::string_view a_editorID) const
	{
		const auto it = entries.find(std::string{ a_editorID });
		return it == entries.end() ? std::string_view{} : std::string_view{ it->second.recordType };
	}

	SelfTestResult FormCatalog::SelfTest()
	{
		auto& catalog = Get();
		if (!catalog.Loaded())
			return { "FormCatalog.SelfTest", CheckStatus::Warn, "form_ids.csv not loaded" };
		const auto* rune = catalog.Lookup("RF_MISC_Rune_1");
		if (!rune || !rune->As<RE::TESObjectMISC>())
			return { "FormCatalog.SelfTest", CheckStatus::Fail, "RF_MISC_Rune_1 did not resolve" };
		const auto* spell = catalog.Lookup("RF_SPEL_Rank_1");
		if (!spell || !spell->As<RE::SpellItem>())
			return { "FormCatalog.SelfTest", CheckStatus::Fail, "RF_SPEL_Rank_1 did not resolve" };
		return { "FormCatalog.SelfTest", CheckStatus::Pass, std::to_string(catalog.Size()) + " editor-ID records" };
	}
}
