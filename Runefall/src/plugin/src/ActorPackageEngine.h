#pragma once
#include "Core.h"

namespace Runefall
{
	class ActorPackageEngine
	{
	public:
		static ActorPackageEngine& Get();

		void Load();
		[[nodiscard]] bool Loaded() const noexcept { return loaded; }
		[[nodiscard]] std::size_t PackageCount() const noexcept { return packages.size(); }
		[[nodiscard]] bool Apply(RE::Actor* a_actor, std::string_view a_roleKey, std::uint8_t a_stage, std::int32_t a_rank = 0);
		[[nodiscard]] std::string SelectPackageId(std::string_view a_roleKey, std::uint8_t a_stage) const;
		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		struct PackageDef
		{
			std::string packageId;
			std::string roleKey;
			std::uint8_t stageMin = 1;
			std::uint8_t stageMax = 100;
			std::vector<std::string> spells;
			std::vector<std::string> eliteSpells;
			std::vector<std::string> bossSpells;
			std::vector<std::string> perks;
			std::string lootTable;
			float weight = 0.0f;
			float budgetCost = 0.0f;
		};
		[[nodiscard]] const PackageDef* Select(std::string_view a_roleKey, std::uint8_t a_stage) const;

		std::vector<PackageDef> packages;
		bool loaded = false;
	};
}
