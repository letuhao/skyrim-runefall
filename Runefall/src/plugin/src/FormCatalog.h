#pragma once
#include "Core.h"

namespace Runefall
{
	// Single source of truth for Runefall record identity. The content generator writes
	// Data\Runefall\data\form_ids.csv (editorId, plugin, localFormId, recordType); runtime code
	// resolves through this catalog instead of scattering local FormID windows through modules.
	class FormCatalog
	{
	public:
		static FormCatalog& Get();

		// Loads form_ids.csv from DataStore. Safe to call more than once (last load wins).
		void Load();

		[[nodiscard]] bool        Loaded() const noexcept { return loaded; }
		[[nodiscard]] std::size_t Size() const noexcept { return entries.size(); }

		// Resolves an EditorID through the generated catalog; returns nullptr when the ID is not
		// catalogued or the plugin/form is missing. The result is cached only when non-null.
		[[nodiscard]] RE::TESForm* Lookup(std::string_view a_editorID);

		template <class T>
		[[nodiscard]] T* LookupAs(std::string_view a_editorID)
		{
			auto* form = Lookup(a_editorID);
			return form ? form->As<T>() : nullptr;
		}

		// Metadata lookup for diagnostics/tests. Returns 0 when unknown.
		[[nodiscard]] std::uint32_t LocalFormID(std::string_view a_editorID) const;
		[[nodiscard]] std::string_view PluginFor(std::string_view a_editorID) const;
		[[nodiscard]] std::string_view RecordTypeFor(std::string_view a_editorID) const;

		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		struct Entry
		{
			std::string   editorID;
			std::string   plugin;
			std::uint32_t localFormID = 0;
			std::string   recordType;
			RE::TESForm*  resolved = nullptr;
		};

		std::unordered_map<std::string, Entry> entries;
		bool                                   loaded = false;
	};
}
