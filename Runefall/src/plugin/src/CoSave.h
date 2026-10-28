#pragma once
#include "Core.h"
#include "ItemMetadata.h"

namespace Runefall
{
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
		// P4.3: references already processed by the runtime loot paths. Persisted so a
		// save/load cannot roll the same container a second time.
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

	class CoSave
	{
	public:
		static CoSave& Get();

		void Register();
		[[nodiscard]] CoSaveState&       State() noexcept { return state; }
		[[nodiscard]] const CoSaveState& State() const noexcept { return state; }
		void Reset();
		void ClearLoadedState();

		static constexpr std::uint32_t kMaxEncodedBytes = 1048576;
		static constexpr std::uint32_t kMaxRecordBytes = 2097152;

		[[nodiscard]] static bool LoadRecord(const std::vector<std::uint8_t>& a_bytes, std::uint32_t a_recordVersion, CoSaveState& a_out);

		[[nodiscard]] bool HasGeneratedContainer(std::uint32_t a_formID) const;
		bool               RememberGeneratedContainer(std::uint32_t a_formID);
		void               ForgetGeneratedContainer(std::uint32_t a_formID);
		[[nodiscard]] bool HasRewardedQuest(std::uint32_t a_questFormID) const;
		bool               RememberRewardedQuest(std::uint32_t a_questFormID);

		[[nodiscard]] static SelfTestResult SelfTest();
		[[nodiscard]] static const std::string& LastRejectToken();

	private:
		static void OnSave(SKSE::SerializationInterface* a_intfc);
		static void OnLoad(SKSE::SerializationInterface* a_intfc);
		static void OnRevert(SKSE::SerializationInterface* a_intfc);

		static std::vector<std::uint8_t> Encode(const CoSaveState& a_state);
		static std::vector<std::uint8_t> EncodeWithBudget(const CoSaveState& a_state, std::size_t a_budget);
		static bool                      Decode(const std::vector<std::uint8_t>& a_bytes, CoSaveState& a_out);
		static bool                      DecodeImpl(const std::vector<std::uint8_t>& a_bytes, CoSaveState& a_out);
		struct Record { std::uint32_t type; std::uint32_t version; std::vector<std::uint8_t> bytes; };
		static bool                      LoadRecords(const std::vector<Record>& a_records, CoSaveState& a_out, std::uint32_t& a_recordVersion);
		static void                      MigrateLegacy(CoSaveState& a_state, std::uint16_t a_schema);
		static std::uint32_t             HashConfig();

		CoSaveState state;
	};
}