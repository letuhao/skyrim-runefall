#pragma once
#include "Core.h"

namespace Runefall
{
	enum class ItemTag : std::uint16_t
	{
		None = 0,
		HuntDrop = 1 << 0,
		BossDrop = 1 << 1,
		QuestReward = 1 << 2,
		UniqueAffix = 1 << 3,
		ContainerLoot = 1 << 4,
		Debug = 1 << 5
	};

	[[nodiscard]] constexpr ItemTag operator|(ItemTag a_lhs, ItemTag a_rhs) noexcept
	{
		return static_cast<ItemTag>(static_cast<std::uint16_t>(a_lhs) | static_cast<std::uint16_t>(a_rhs));
	}

	struct Enhancement
	{
		std::string affixId;
		std::string effectEditorId;
		std::int32_t category = 0;
		std::int32_t tier = 0;
		float        magnitude = 0.0f;
		std::int32_t duration = 0;
		std::int32_t area = 0;
	};

	struct ItemMetadata
	{
		std::uint32_t registryId = 0;
		std::uint32_t baseFormId = 0;
		std::uint8_t  rarity = 0;
		std::uint16_t ilvl = 0;
		std::uint8_t  socketCount = 0;
		std::array<std::int32_t, 3> socketRunes{};
		std::uint8_t  enhancementCount = 0;
		std::array<Enhancement, 5> enhancements{};
		std::uint16_t tags = 0;
	};
}
