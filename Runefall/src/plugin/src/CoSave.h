#pragma once
#include "Core.h"
#include "CoSaveCodec.h"

namespace Runefall
{
	class CoSave
	{
	public:
		static CoSave& Get();

		void Register();
		[[nodiscard]] CoSaveState&       State() noexcept { return state; }
		[[nodiscard]] const CoSaveState& State() const noexcept { return state; }
		void Reset();
		void ClearLoadedState();

		static constexpr std::uint32_t kMaxEncodedBytes = CoSaveCodec::kMaxEncodedBytes;
		static constexpr std::uint32_t kMaxRecordBytes = CoSaveCodec::kMaxRecordBytes;

		[[nodiscard]] static bool LoadRecord(const std::vector<std::uint8_t>& a_bytes, std::uint32_t a_recordVersion, CoSaveState& a_out);
		[[nodiscard]] static bool LoadRecords(const std::vector<CoSaveRecord>& a_records, CoSaveState& a_out, std::uint32_t& a_recordVersion);

		[[nodiscard]] bool HasGeneratedContainer(std::uint32_t a_formID) const;
		bool               RememberGeneratedContainer(std::uint32_t a_formID);
		void               ForgetGeneratedContainer(std::uint32_t a_formID);
		[[nodiscard]] bool HasRewardedQuest(std::uint32_t a_questFormID) const;
		bool               RememberRewardedQuest(std::uint32_t a_questFormID);

		[[nodiscard]] static SelfTestResult SelfTest();
		[[nodiscard]] static const std::string& LastRejectToken();

		static std::vector<std::uint8_t> Encode(const CoSaveState& a_state);
		static std::vector<std::uint8_t> EncodeWithBudget(const CoSaveState& a_state, std::size_t a_budget);
		static bool                      Decode(const std::vector<std::uint8_t>& a_bytes, CoSaveState& a_out);

	private:
		static void OnSave(SKSE::SerializationInterface* a_intfc);
		static void OnLoad(SKSE::SerializationInterface* a_intfc);
		static void OnRevert(SKSE::SerializationInterface* a_intfc);

		static std::uint32_t HashConfig();

		CoSaveState state;
	};
}
