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
		[[nodiscard]] std::int32_t SkillRank(std::string_view a_skill) const;

		bool SpendAttributePoint(std::size_t a_index);
		bool SpendSkillPoint(std::string_view a_skill);
		bool AddAttributePoint(std::int32_t a_amount = 1);
		bool AddSkillPoint(std::int32_t a_amount = 1);
		bool RespecAttributes(std::int32_t a_runeCost);
		bool RespecSkills(std::int32_t a_runeCost);

		void SetRuneTier(std::int32_t a_tier) noexcept { runeTier = std::clamp(a_tier, 0, 5); }

		[[nodiscard]] static std::int32_t SoftCappedAttribute(std::int32_t a_raw);

		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		std::array<std::int32_t, 9>   attributes{};
		std::int32_t                  attributePoints = 0;
		std::int32_t                  skillPoints = 0;
		std::int32_t                  runeTier = 0;
		std::vector<SkillProgress>    skills;
	};
}
