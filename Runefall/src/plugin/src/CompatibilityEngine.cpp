#include "CompatibilityEngine.h"

#include "DataStore.h"

#include "RE/T/TESDataHandler.h"

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
		bool Bool(const std::vector<std::string>& a_row, int a_column)
		{
			return Cell(a_row, a_column) == "1";
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
	}

	CompatibilityEngine& CompatibilityEngine::Get()
	{
		static CompatibilityEngine singleton;
		return singleton;
	}

	void CompatibilityEngine::Load()
	{
		profiles.clear();
		loaded = false;
		const auto* table = DataStore::Get().GetTable("compat_profiles.csv");
		if (!table) {
			logger::error("compat: compat_profiles.csv missing");
			return;
		}
		const int pId = FindColumn(*table, "profileId");
		const int pReq = FindColumn(*table, "requiredPlugins");
		const int pOpt = FindColumn(*table, "optionalPlugins");
		const int pMode = FindColumn(*table, "lootMode");
		const int pProtect = FindColumn(*table, "protectVanillaLoot");
		const int pNotes = FindColumn(*table, "notes");
		for (const auto& row : table->rows) {
			Profile profile;
			profile.profileId = Cell(row, pId);
			profile.requiredPlugins = Split(Cell(row, pReq));
			profile.optionalPlugins = Split(Cell(row, pOpt));
			profile.lootMode = Cell(row, pMode);
			profile.protectVanilla = Bool(row, pProtect);
			profile.notes = Cell(row, pNotes);
			if (!profile.profileId.empty())
				profiles.push_back(std::move(profile));
		}
		loaded = !profiles.empty();
		if (loaded)
			Detect();
	}

	void CompatibilityEngine::Detect()
	{
		std::vector<std::string> loadedFiles;
		const auto* handler = RE::TESDataHandler::GetSingleton();
		if (handler) {
			const auto* mods = handler->GetLoadedMods();
			const auto count = handler->GetLoadedModCount();
			for (std::uint8_t i = 0; mods && i < count; ++i)
				if (mods[i])
					loadedFiles.emplace_back(mods[i]->GetFilename());
		}
		const Profile* best = nullptr;
		std::size_t bestCount = 0;
		for (const auto& profile : profiles) {
			bool matches = true;
			for (const auto& required : profile.requiredPlugins) {
				if (std::find(loadedFiles.begin(), loadedFiles.end(), required) == loadedFiles.end()) {
					matches = false;
					break;
				}
			}
			if (matches && profile.requiredPlugins.size() >= bestCount) {
				best = &profile;
				bestCount = profile.requiredPlugins.size();
			}
		}
		if (!best) {
			activeProfile = "vanilla";
			lootMode = "additive";
			protectVanilla = true;
			logger::info("compat: no profile matched, using vanilla");
			return;
		}
		activeProfile = best->profileId;
		lootMode = best->lootMode;
		protectVanilla = best->protectVanilla;
		logger::info("compat: active profile {} lootMode={} protectVanilla={}", activeProfile, lootMode, protectVanilla);
	}

	SelfTestResult CompatibilityEngine::SelfTest()
	{
		auto& engine = Get();
		if (!engine.Loaded())
			return { "CompatibilityEngine.SelfTest", CheckStatus::Fail, "compat profiles not loaded" };
		if (engine.ProfileCount() < 2)
			return { "CompatibilityEngine.SelfTest", CheckStatus::Fail, "compat profile coverage too small" };
		if (engine.ActiveProfile().empty() || !engine.ShouldGenerateLoot())
			return { "CompatibilityEngine.SelfTest", CheckStatus::Fail, "active compat profile is invalid" };
		return { "CompatibilityEngine.SelfTest", CheckStatus::Pass,
			std::format("{} profiles; active {}; lootMode={}", engine.ProfileCount(), engine.ActiveProfile(), engine.ShouldGenerateLoot() ? "additive" : "disabled") };
	}
}
