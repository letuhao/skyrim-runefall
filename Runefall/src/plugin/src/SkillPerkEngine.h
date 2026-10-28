#pragma once
#include "Core.h"

namespace Runefall
{
	class SkillPerkEngine
	{
	public:
		static SkillPerkEngine& Get();
		void Load();
		[[nodiscard]] bool Loaded() const noexcept { return loaded; }
		[[nodiscard]] float BonusFor(std::string_view a_effectKey) const;
		[[nodiscard]] std::size_t PerkCount() const noexcept { return perks.size(); }
		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		struct Perk
		{
			std::string perkId;
			std::string skill;
			std::int32_t rankRequired = 1;
			std::string effectKey;
			float magnitude = 0.0f;
			std::string descriptionKey;
		};
		std::vector<Perk> perks;
		bool loaded = false;
	};
}
