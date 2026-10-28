#pragma once
#include "Core.h"

namespace Runefall
{
	class ActorEquipmentDistributor
	{
	public:
		static ActorEquipmentDistributor& Get();

		void Load();
		[[nodiscard]] bool Loaded() const noexcept { return loaded; }
		[[nodiscard]] bool Apply(RE::Actor* a_actor, std::string_view a_roleKey, std::uint8_t a_stage);
		[[nodiscard]] std::string SelectSetId(std::string_view a_roleKey, std::uint8_t a_stage) const;
		[[nodiscard]] std::size_t SetCount() const noexcept { return sets.size(); }
		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		struct SetDef
		{
			std::string setId;
			std::string roleKey;
			std::uint8_t stageMin = 1;
			std::uint8_t stageMax = 100;
			std::vector<std::string> items;
			std::vector<std::string> perks;
			float weight = 0.0f;
			float applyChance = 100.0f;
		};
		[[nodiscard]] const SetDef* Select(std::string_view a_roleKey, std::uint8_t a_stage) const;

		std::vector<SetDef> sets;
		bool loaded = false;
	};
}
