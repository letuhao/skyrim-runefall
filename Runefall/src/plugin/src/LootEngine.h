#pragma once
#include "Core.h"
#include "ItemMetadata.h"

namespace Runefall
{
	struct AffixDef
	{
		std::string id;
		std::string category;
		std::int32_t tier = 1;
		float        minValue = 0.0f;
		float        maxValue = 0.0f;
		std::int32_t duration = 0;
		std::int32_t area = 0;
		float        weight = 1.0f;
		std::int32_t minItemLevel = 1;
		std::string effectEditorId;
		std::string prefix;
		std::string suffix;
		std::string slots;
		std::string unit;
	};

	struct RarityDef
	{
		std::int32_t id = 0;
		std::string  name;
		std::int32_t enhancementCount = 0;
		std::int32_t socketMin = 0;
		std::int32_t socketMax = 0;
	};

	struct RarityWeightDef
	{
		std::int32_t rfTier = 0;
		std::int32_t rarityId = 0;
		float        weight = 1.0f;
	};

	struct GeneralDropDef
	{
		std::int32_t rank = 0;
		std::int32_t rfTier = 0;
		float        itemChancePct = 0.0f;
		std::int32_t itemCountMin = 1;
		std::int32_t itemCountMax = 1;
		float        rarityBias = 0.0f;
		std::int32_t runeMin = 1;
		std::int32_t runeMax = 1;
		float        uniqueChancePct = 0.0f;
	};

	class Rng
	{
	public:
		explicit Rng(std::uint64_t a_seed = 0x9E3779B97F4A7C15ull) : state(a_seed ? a_seed : 1) {}

		std::uint32_t NextU32();
		float         Float01();
		std::int32_t  Range(std::int32_t a_min, std::int32_t a_max);

	private:
		std::uint64_t state;
	};

	class LootEngine
	{
	public:
		static LootEngine& Get();

		void Reload();
		[[nodiscard]] bool HasData() const noexcept { return loaded; }
		[[nodiscard]] std::size_t AffixCount() const noexcept { return data.affixes.size(); }
		[[nodiscard]] std::size_t RarityCount() const noexcept { return data.rarities.size(); }

		bool RerollEnhancements(ItemMetadata& a_item, std::int32_t a_lockedIndex, std::string_view a_slotGroup, Rng& a_rng);

		[[nodiscard]] std::optional<ItemMetadata> Generate(
			std::int32_t  a_rank,
			std::int32_t  a_rfTier,
			std::int32_t  a_ilvl,
			std::uint16_t a_tags,
			std::string_view a_slotGroup,
			Rng&          a_rng);

		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		struct Dataset
		{
			std::vector<AffixDef>        affixes;
			std::vector<RarityDef>       rarities;
			std::vector<RarityWeightDef> weights;
			std::vector<GeneralDropDef>  generalDrops;
		};

		[[nodiscard]] std::optional<ItemMetadata> GenerateFrom(
			const Dataset& a_data,
			std::int32_t   a_rank,
			std::int32_t   a_rfTier,
			std::int32_t   a_ilvl,
			std::uint16_t  a_tags,
			std::string_view a_slotGroup,
			Rng&           a_rng) const;

		Dataset     data;
		bool        loaded = false;
	};
}