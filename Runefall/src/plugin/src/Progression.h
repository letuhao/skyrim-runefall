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
		[[nodiscard]] std::int32_t SkillPoints() const noexcept { return skillPoints; }
		[[nodiscard]] std::int32_t RuneTier() const noexcept { return runeTier; }
		[[nodiscard]] bool         BossKillReady() const noexcept { return bossKillTier >= runeTier; }
		[[nodiscard]] std::int32_t SkillRank(std::string_view a_skill) const;
		[[nodiscard]] std::size_t AttributeEffectCount() const noexcept { return attributeEffectCount; }

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

		std::array<std::int32_t, 9>   attributes{};
		std::int32_t                  attributePoints = 0;
		std::int32_t                  skillPoints = 0;
		std::int32_t                  runeTier = 0;
		std::int32_t                  bossKillTier = -1;
		std::vector<SkillProgress>    skills;
		std::size_t                   attributeEffectCount = 0;
	};
}