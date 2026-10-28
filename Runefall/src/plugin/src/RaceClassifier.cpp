#include "RaceClassifier.h"

#include "DataStore.h"

#include "RE/B/BGSKeyword.h"
#include "RE/T/TESForm.h"
#include "RE/T/TESRace.h"

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

		bool Bool(const std::vector<std::string>& a_row, int a_column)
		{
			return Cell(a_row, a_column) == "1";
		}

		std::int32_t Int(const std::vector<std::string>& a_row, int a_column)
		{
			const auto text = Cell(a_row, a_column);
			if (text.empty())
				return 0;
			return static_cast<std::int32_t>(std::strtol(text.c_str(), nullptr, 10));
		}

		std::string NormalizeRole(std::string_view a_role)
		{
			if (a_role.empty())
				return "rf:role/unknown";
			if (a_role.starts_with("rf:role/"))
				return std::string{ a_role };
			return "rf:role/" + std::string{ a_role };
		}
	}

	RaceClassifier& RaceClassifier::Get()
	{
		static RaceClassifier singleton;
		return singleton;
	}

	void RaceClassifier::Load()
	{
		roles.clear();
		profiles.clear();
		rules.clear();
		loaded = false;

		if (const auto* roleTable = DataStore::Get().GetTable("roles.csv")) {
			const int kKey = FindColumn(*roleTable, "roleKey");
			const int kHost = FindColumn(*roleTable, "hostPolicy");
			const int kProtection = FindColumn(*roleTable, "protectionPolicy");
			for (const auto& row : roleTable->rows) {
				RoleDef def;
				def.key = Cell(row, kKey);
				def.hostPolicy = Cell(row, kHost);
				def.protectionPolicy = Cell(row, kProtection);
				if (!def.key.empty())
					roles[def.key] = std::move(def);
			}
		}
		if (const auto* profileTable = DataStore::Get().GetTable("race_profiles.csv")) {
			const int pKey = FindColumn(*profileTable, "profileKey");
			const int pEditor = FindColumn(*profileTable, "raceEditorId");
			const int pRoles = FindColumn(*profileTable, "roles");
			const int pFamily = FindColumn(*profileTable, "family");
			for (const auto& row : profileTable->rows) {
				ProfileDef def;
				def.key = Cell(row, pKey);
				def.editorId = Cell(row, pEditor);
				def.roleKey = NormalizeRole(Cell(row, pRoles));
				def.family = Cell(row, pFamily);
				if (!def.editorId.empty())
					profiles[def.editorId] = std::move(def);
			}
		}
		if (const auto* ruleTable = DataStore::Get().GetTable("race_classification_rules.csv")) {
			const int rId = FindColumn(*ruleTable, "ruleId");
			const int rKeyword = FindColumn(*ruleTable, "keywordEditorId");
			const int rRole = FindColumn(*ruleTable, "roleKey");
			const int rFamily = FindColumn(*ruleTable, "family");
			const int rHost = FindColumn(*ruleTable, "hostEligible");
			const int rProtection = FindColumn(*ruleTable, "protectionEligible");
			const int rProvisional = FindColumn(*ruleTable, "provisional");
			const int rPriority = FindColumn(*ruleTable, "priority");
			for (const auto& row : ruleTable->rows) {
				RuleDef def;
				def.ruleId = static_cast<std::uint16_t>(Int(row, rId));
				def.keywordEditorId = Cell(row, rKeyword);
				def.roleKey = NormalizeRole(Cell(row, rRole));
				def.family = Cell(row, rFamily);
				def.hostEligible = Bool(row, rHost);
				def.protectionEligible = Bool(row, rProtection);
				def.provisional = Bool(row, rProvisional);
				def.priority = Int(row, rPriority);
				if (!def.keywordEditorId.empty())
					rules.push_back(std::move(def));
			}
			std::sort(rules.begin(), rules.end(), [](const RuleDef& a, const RuleDef& b) { return a.priority > b.priority; });
		}
		loaded = !profiles.empty() && !rules.empty() && roles.contains("rf:role/unknown");
		if (loaded)
			logger::info("race-classifier: loaded {} roles, {} race profiles, {} keyword rules", roles.size(), profiles.size(), rules.size());
		else
			logger::error("race-classifier: roles/race profiles/rules missing; classification disabled");
	}

	RaceClassification RaceClassifier::Unknown() const
	{
		RaceClassification result;
		result.profileKey = "rf:raceprofile/unknown";
		result.roleKey = "rf:role/unknown";
		result.family = "unknown";
		result.hostEligible = true;
		result.protectionEligible = true;
		result.provisional = true;
		result.source = "fallback";
		return result;
	}

	RaceClassification RaceClassifier::FromProfile(const ProfileDef& a_profile, std::string_view a_source) const
	{
		RaceClassification result;
		result.profileKey = a_profile.key;
		result.roleKey = a_profile.roleKey;
		result.family = a_profile.family.empty() ? a_profile.roleKey : a_profile.family;
		result.provisional = false;
		result.source = std::string{ a_source };
		if (const auto it = roles.find(result.roleKey); it != roles.end()) {
			result.hostEligible = it->second.hostPolicy != "exclude";
			result.protectionEligible = it->second.protectionPolicy == "host";
		}
		return result;
	}

	RaceClassification RaceClassifier::ClassifyEditorId(std::string_view a_editorId) const
	{
		if (const auto it = profiles.find(std::string{ a_editorId }); it != profiles.end())
			return FromProfile(it->second, "explicit");
		return Unknown();
	}

	RaceClassification RaceClassifier::Classify(RE::TESRace* a_race) const
	{
		if (!a_race)
			return Unknown();
		const char* editorId = a_race->GetFormEditorID();
		if (editorId && *editorId) {
			if (const auto it = profiles.find(editorId); it != profiles.end())
				return FromProfile(it->second, "explicit");
		}
		for (const auto& rule : rules) {
			const auto* keyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(rule.keywordEditorId);
			if (keyword && a_race->HasKeyword(keyword)) {
				RaceClassification result;
				result.profileKey = "rf:raceprofile/keyword/" + rule.keywordEditorId;
				result.roleKey = rule.roleKey;
				result.family = rule.family;
				result.hostEligible = rule.hostEligible;
				result.protectionEligible = rule.protectionEligible;
				result.provisional = rule.provisional;
				result.source = "keyword:" + rule.keywordEditorId;
				return result;
			}
		}
		if (a_race->armorParentRace) {
			auto parent = Classify(a_race->armorParentRace);
			parent.source = "armorParentRace";
			return parent;
		}
		if (a_race->morphRace) {
			auto parent = Classify(a_race->morphRace);
			parent.source = "morphRace";
			return parent;
		}
		return Unknown();
	}

	SelfTestResult RaceClassifier::SelfTest()
	{
		auto& classifier = Get();
		if (!classifier.Loaded())
			return { "RaceClassifier.SelfTest", CheckStatus::Fail, "classifier not loaded" };
		if (classifier.RuleCount() < 5 || classifier.ProfileCount() < 10)
			return { "RaceClassifier.SelfTest", CheckStatus::Fail, "classification data coverage too small" };
		const auto nord = classifier.ClassifyEditorId("NordRace");
		if (nord.provisional || nord.roleKey != "rf:role/humanoid" || !nord.hostEligible || !nord.protectionEligible || nord.source != "explicit")
			return { "RaceClassifier.SelfTest", CheckStatus::Fail, "known NordRace classification failed" };
		const auto unknown = classifier.ClassifyEditorId("RF_UnknownRace");
		if (!unknown.provisional || unknown.roleKey != "rf:role/unknown" || unknown.source != "fallback")
			return { "RaceClassifier.SelfTest", CheckStatus::Fail, "unknown race fallback failed" };
		return { "RaceClassifier.SelfTest", CheckStatus::Pass,
			std::format("{} roles, {} profiles, {} keyword rules; known/unknown/fallback paths OK", classifier.roles.size(), classifier.ProfileCount(), classifier.RuleCount()) };
	}
}
