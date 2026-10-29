#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

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

	struct LadderOverride
	{
		std::uint64_t key = 0;
		std::uint8_t  stage = 1;
		std::uint8_t  flags = 0;
	};

	struct CoSaveState
	{
		std::array<std::int32_t, 9> attributes{};
		std::int32_t                attributePoints = 0;
		std::int32_t                skillPoints = 0;
		std::int32_t                runeTier = 0;
		std::int32_t                bossKillTier = -1;
		std::array<std::int32_t, 5> runes{};
		std::unordered_map<std::string, std::uint8_t> huntFirstKill;
		std::unordered_map<std::string, std::int32_t> materials;
		std::unordered_map<std::string, std::string>  packVersions;
		std::vector<ItemMetadata>                     items;
		std::vector<std::uint32_t>                    generatedContainers;
		std::vector<std::uint32_t>                    rewardedQuests;
		std::uint32_t                                 configHash = 0;
		std::uint8_t                                  playerRank = 1;
		std::uint8_t                                  playerStage = 1;
		std::uint32_t                                 playerXp = 0;
		std::string                                   characterClass;
		std::unordered_map<std::string, std::int32_t> skillRanks;
		bool                                          endgameComplete = false;
		std::uint8_t                                  difficultyProfile = 255;
		std::uint8_t                                  curveId = 1;
		std::vector<LadderOverride>                   ladderOverrides;
	};

	struct CoSaveRecord
	{
		std::uint32_t type = 0;
		std::uint16_t version = 0;
		std::vector<std::uint8_t> bytes;
	};

	class CoSaveCodec
	{
	public:
		static constexpr std::uint16_t kSaveVersion = 3;
		static constexpr std::uint32_t kMaxEncodedBytes = 1048576;
		static constexpr std::uint32_t kMaxRecordBytes = 2097152;
		static constexpr std::uint32_t kSerializationRecord = 'RNF2';

		static std::vector<std::uint8_t> Encode(const CoSaveState& a_state);
		static std::vector<std::uint8_t> EncodeWithBudget(const CoSaveState& a_state, std::size_t a_budget);
		static bool Decode(const std::vector<std::uint8_t>& a_bytes, CoSaveState& a_out);
		static bool LoadRecord(const std::vector<std::uint8_t>& a_bytes, std::uint32_t a_recordVersion, CoSaveState& a_out);
		static bool LoadRecords(const std::vector<CoSaveRecord>& a_records, std::uint16_t a_recordVersion, CoSaveState& a_out);
		static void MigrateLegacy(CoSaveState& a_state, std::uint16_t a_schema);
		static const std::string& LastRejectToken();

	private:
		static bool DecodeImpl(const std::vector<std::uint8_t>& a_bytes, CoSaveState& a_out);
	};
}
