#pragma once
#include "Core.h"

namespace Runefall
{
	enum class StatKind : std::uint8_t { Health = 0, Damage, Regen, Armor };
	enum class CapacityKind : std::uint8_t {
		Stat = 0, Offense, Defense, Utility, Ability, Loot, Reward, Encounter
	};
	enum class StageSource : std::uint8_t {
		Default = 0, WorldBand = 1, Encounter = 2, Override = 3, PlayerRank = 4
	};

	struct StageRow
	{
		std::uint8_t  stage = 1;
		std::uint8_t  tier = 1;
		std::uint8_t  curve = 1;
		float         power = 1.0f;
		float         health = 1.0f;
		float         damage = 1.0f;
		float         regen = 1.0f;
		float         armor = 1.0f;
		std::uint32_t statCapacity = 100;
		std::uint32_t offenseCapacity = 100;
		std::uint32_t defenseCapacity = 100;
		std::uint32_t utilityCapacity = 100;
		std::uint32_t abilityCapacity = 100;
		std::uint32_t lootCapacity = 100;
		std::uint32_t rewardCapacity = 100;
		std::uint32_t encounterCost = 10;
	};

	struct TierDefinition
	{
		std::uint8_t  tier = 1;
		std::uint8_t  stageMin = 1;
		std::uint8_t  stageMax = 10;
		std::string   name;
		std::string   nameKey;
		std::string   keywordEditorId;
		std::string   breakpointTag;
		std::string   unlockTag;
		std::uint32_t budgetStep = 100;
	};

	struct DifficultyDefinition
	{
		std::uint8_t profileId = 0;
		std::string  name;
		std::string  nameKey;
		std::int32_t stageDelta = 0;
		float        statMult = 1.0f;
		float        damageDealtMult = 1.0f;
		float        damageTakenMult = 1.0f;
		float        xpMult = 1.0f;
		float        rewardMult = 1.0f;
		float        dropMult = 1.0f;
		std::uint8_t maxStage = 100;
		bool         enabled = true;
	};

	struct ContentBand
	{
		std::uint8_t stageMin = 1;
		std::uint8_t stageMax = 10;
		std::uint8_t tier = 1;
		std::uint8_t playerLevelMin = 1;
		std::uint8_t playerLevelMax = 10;
		std::uint8_t itemLevelMin = 1;
		std::uint8_t itemLevelMax = 10;
		std::string  gearCheckBand;
		std::string  unlockTag;
		bool         v1Populated = false;
	};

	struct AssignmentRule
	{
		std::uint16_t ruleId = 0;
		StageSource   source = StageSource::Default;
		std::uint8_t  minStage = 1;
		std::uint8_t  maxStage = 100;
		std::int8_t   offsetMin = 0;
		std::int8_t   offsetMax = 0;
		std::int8_t   jitter = 0;
		std::uint64_t seedSalt = 0;
		std::int32_t  priority = 0;
		bool          enabled = true;
	};

	struct StageRequest
	{
		StageSource   source = StageSource::Default;
		std::uint8_t  requestedStage = 0;
		std::uint8_t  band = 0;
		std::uint8_t  category = 0;
		std::uint8_t  difficultyProfile = 0;
		std::uint8_t  overrideStage = 0;
		std::uint16_t ruleId = 0;
		std::uint64_t seed = 0;
	};

	struct StageProfile
	{
		std::uint8_t  stage = 1;
		std::uint8_t  tier = 1;
		std::uint8_t  curve = 1;
		StageSource   source = StageSource::Default;
		std::uint16_t flags = 0;
	};

	class StageEngine
	{
	public:
		static StageEngine& Get();

		void Load();
		[[nodiscard]] bool Loaded() const noexcept { return loaded; }
		[[nodiscard]] std::size_t StageCount() const noexcept { return stages.size(); }
		[[nodiscard]] std::size_t TierCount() const noexcept { return tiers.size(); }

		[[nodiscard]] const StageRow* Stage(std::uint8_t a_stage) const;
		[[nodiscard]] float          Power(std::uint8_t a_stage) const;
		[[nodiscard]] float          StatMultiplier(std::uint8_t a_stage, StatKind a_kind) const;
		[[nodiscard]] std::uint8_t   TierOf(std::uint8_t a_stage) const;
		[[nodiscard]] std::uint32_t  Capacity(std::uint8_t a_stage, CapacityKind a_kind) const;
		[[nodiscard]] std::uint8_t   StageGap(std::uint8_t a_target, std::uint8_t a_attacker) const;
		[[nodiscard]] float          PowerRatio(std::uint8_t a_target, std::uint8_t a_attacker) const;
		[[nodiscard]] const TierDefinition*       Tier(std::uint8_t a_tier) const;
		[[nodiscard]] const DifficultyDefinition* Difficulty(std::uint8_t a_profile) const;
		[[nodiscard]] const ContentBand*          ContentBandFor(std::uint8_t a_stage) const;

		[[nodiscard]] StageProfile RequestStage(const StageRequest& a_request) const;

		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		[[nodiscard]] std::uint64_t MixSeed(std::uint64_t a_seed) const;
		[[nodiscard]] const AssignmentRule* FindRule(StageSource a_source) const;

		std::vector<StageRow>              stages;
		std::vector<TierDefinition>        tiers;
		std::vector<DifficultyDefinition>  difficulties;
		std::vector<ContentBand>           bands;
		std::vector<AssignmentRule>        rules;
		bool                               loaded = false;
	};
}
