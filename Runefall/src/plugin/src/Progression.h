#pragma once
#include "Core.h"

namespace Runefall
{
	struct SkillProgress
	{
		std::string skill;
		std::int32_t rank = 0;
	};

	class Progression
	{
	public:
		static Progression& Get();

		void LoadFromCoSave();
		void SaveToCoSave();

		[[nodiscard]] std::int32_t Attribute(std::size_t a_index) const;
		[[nodiscard]] std::int32_t AttributePoints() const noexcept { return attributePoints; }
		[[nodiscard]] std::int32_t PointsPerLevel() const noexcept { return pointsPerLevel; }
		[[nodiscard]] std::int32_t MaxRuneLevel() const noexcept { return maxRuneLevel; }
		[[nodiscard]] std::int32_t BreakthroughBonus(std::int32_t a_tier) const;
		[[nodiscard]] std::int32_t AttributeAwardForStageAdvance(std::int32_t a_newStage) const;
		bool AwardStageAdvance(std::int32_t a_newStage);
		[[nodiscard]] std::int32_t RuneLevel() const;
		[[nodiscard]] std::uint32_t RuneXp() const;
		[[nodiscard]] std::int32_t XpForNextLevel() const;
		bool AddXp(std::int32_t a_amount);
		[[nodiscard]] std::int32_t SkillPoints() const noexcept { return skillPoints; }
		[[nodiscard]] std::int32_t RuneTier() const noexcept { return runeTier; }
		[[nodiscard]] bool         BossKillReady() const noexcept { return bossKillTier >= runeTier; }
		[[nodiscard]] std::int32_t SkillRank(std::string_view a_skill) const;
		[[nodiscard]] const std::string& ClassId() const noexcept { return characterClass; }
		void SetClassId(std::string a_class);
		bool UnlockSkill(std::string_view a_skill);
		bool AddAttribute(std::size_t a_index, std::int32_t a_amount);
		[[nodiscard]] std::size_t AttributeEffectCount() const noexcept { return attributeEffectCount; }
		[[nodiscard]] float       DerivedValue(std::string_view a_effectKey) const;
		[[nodiscard]] std::vector<std::string> DerivedKeys() const;

		bool SpendAttributePoint(std::size_t a_index);
		bool SpendSkillPoint(std::string_view a_skill);
		bool AddAttributePoint(std::int32_t a_amount = 1);
		bool AddSkillPoint(std::int32_t a_amount = 1);
		bool RespecAttributes(std::int32_t a_runeCost);
		bool RespecSkills(std::int32_t a_runeCost);

		void MarkBossKill(std::int32_t a_runeTier);

		// 0 = attuned, 1 = insufficient runes, 2 = already max tier, 3 = boss kill required.
		[[nodiscard]] std::int32_t TryAttune();

		void SetRuneTier(std::int32_t a_tier) noexcept { runeTier = std::clamp(a_tier, 0, 5); }

		[[nodiscard]] static std::int32_t SoftCappedAttribute(std::int32_t a_raw);

		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		void LoadAttributeDefinitions();
		void LoadPlayerProgression();
		void SyncGlobals();

		std::array<std::int32_t, 9>   attributes{};
		std::int32_t                  attributePoints = 0;
		std::int32_t                  skillPoints = 0;
		std::int32_t                  runeTier = 0;
		std::int32_t                  bossKillTier = -1;
		std::vector<SkillProgress>    skills;
		std::string                  characterClass;
		struct AttributeEffect
		{
			std::size_t   attributeIndex = 0;
			std::string   effectKey;
			float         perPoint = 0.0f;
		};

		std::vector<AttributeEffect>  attributeEffects;
		std::int32_t                  pointsPerLevel = 3;
		std::int32_t                  maxRuneLevel = 100;
		std::vector<std::int32_t>     breakthroughBonuses;
		float                         xpBase = 100.0f;
		float                         xpGrowth = 1.15f;
		int                           bossBreakEvery = 10;
		std::size_t                   attributeEffectCount = 0;
	};
}