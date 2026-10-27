#pragma once
#include "Core.h"

namespace Runefall
{
	struct RaceGroupDef
	{
		std::string              group;
		std::vector<std::string> memberRaces;
		std::int32_t             minRfTier = 1;
		std::string              uniqueAffixId;
		std::string              materialId;
	};

	struct UniqueSourceDef
	{
		std::string uniqueId;
		std::string sourceType;
		std::string sourceEditorId;
		std::string dropRule;
		bool        firstOnly = false;
		std::string note;
	};

	struct RaceRewardDef
	{
		std::string  group;
		std::string  rewardType;
		std::string  rewardId;
		float        dropChancePct = 0.0f;
		bool         firstOnly = false;
		std::string  note;
	};

	class HuntEngine
	{
	public:
		static HuntEngine& Get();

		void Reload();
		[[nodiscard]] bool HasData() const noexcept { return loaded; }
		[[nodiscard]] std::size_t FamilyCount() const noexcept { return groups.size(); }
		[[nodiscard]] const std::vector<RaceGroupDef>& Families() const noexcept { return groups; }

		[[nodiscard]] const RaceGroupDef* FindFamilyForRace(std::string_view a_raceEditorId) const;
		[[nodiscard]] const RaceGroupDef* FindFamily(std::string_view a_group) const;
		[[nodiscard]] const RaceRewardDef* FindReward(std::string_view a_group, std::string_view a_rewardType) const;
		[[nodiscard]] const UniqueSourceDef* FindUniqueSource(std::string_view a_uniqueId) const;
		[[nodiscard]] std::size_t UniqueSourceCount() const noexcept { return uniqueSources.size(); }

		[[nodiscard]] bool HasFirstKill(std::string_view a_group) const;
		bool               MarkFirstKill(std::string_view a_group);

		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		std::vector<RaceGroupDef>  groups;
		std::vector<RaceRewardDef> rewards;
		std::vector<UniqueSourceDef> uniqueSources;
		bool                       loaded = false;
	};
}