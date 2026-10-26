#pragma once
#include "Core.h"
#include "LootEngine.h"

namespace Runefall
{
	struct RankDef
	{
		std::int32_t rank = 0;
		std::string  name;
		float        hpPct = 0.0f;
		float        dmgPct = 0.0f;
		float        armorPct = 0.0f;
		std::int32_t affixCount = 0;
		float        itemRarityBonus = 0.0f;
		std::int32_t runeRank = 1;
		std::int32_t runeMin = 1;
		std::int32_t runeMax = 1;
		std::int32_t materialTierBonus = 0;
	};

	struct SpawnWeightDef
	{
		std::int32_t rfTier = 0;
		std::int32_t rank = 0;
		float        weight = 0.0f;
	};

	class TierEngine
	{
	public:
		static TierEngine& Get();

		void Reload();
		[[nodiscard]] bool HasData() const noexcept { return loaded; }
		[[nodiscard]] std::size_t RankCount() const noexcept { return ranks.size(); }

		[[nodiscard]] std::int32_t RollRank(std::int32_t a_rfTier, Rng& a_rng) const;

		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		std::vector<RankDef>        ranks;
		std::vector<SpawnWeightDef> spawns;
		bool                        loaded = false;
	};
}
