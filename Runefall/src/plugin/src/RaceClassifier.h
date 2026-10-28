#pragma once
#include "Core.h"

namespace Runefall
{
	struct RaceClassification
	{
		std::string profileKey;
		std::string roleKey;
		std::string family;
		bool        hostEligible = false;
		bool        protectionEligible = false;
		bool        provisional = true;
		std::string source;
	};

	class RaceClassifier
	{
	public:
		static RaceClassifier& Get();

		void Load();
		[[nodiscard]] bool Loaded() const noexcept { return loaded; }
		[[nodiscard]] RaceClassification Classify(RE::TESRace* a_race) const;
		[[nodiscard]] RaceClassification ClassifyEditorId(std::string_view a_editorId) const;
		[[nodiscard]] std::size_t RuleCount() const noexcept { return rules.size(); }
		[[nodiscard]] std::size_t ProfileCount() const noexcept { return profiles.size(); }
		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		struct RoleDef
		{
			std::string key;
			std::string hostPolicy;
			std::string protectionPolicy;
		};
		struct ProfileDef
		{
			std::string key;
			std::string editorId;
			std::string roleKey;
			std::string family;
		};
		struct RuleDef
		{
			std::uint16_t ruleId = 0;
			std::string   keywordEditorId;
			std::string   roleKey;
			std::string   family;
			bool          hostEligible = false;
			bool          protectionEligible = false;
			bool          provisional = false;
			std::int32_t  priority = 0;
		};

		[[nodiscard]] RaceClassification FromProfile(const ProfileDef& a_profile, std::string_view a_source) const;
		[[nodiscard]] RaceClassification Unknown() const;

		std::unordered_map<std::string, RoleDef> roles;
		std::unordered_map<std::string, ProfileDef> profiles;
		std::vector<RuleDef> rules;
		bool loaded = false;
	};
}
