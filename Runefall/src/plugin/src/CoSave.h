#pragma once
#include "Core.h"
#include "ItemMetadata.h"

namespace Runefall
{
	struct CoSaveState
	{
		std::array<std::int32_t, 9> attributes{};
		std::int32_t                attributePoints = 0;
		std::int32_t                skillPoints = 0;
		std::int32_t                runeTier = 0;
		std::int32_t                bossKillTier = -1;
		std::array<std::int32_t, 5> runes{};
		std::unordered_map<std::string, std::uint8_t> huntFirstKill;
		std::unordered_map<std::string, std::string>  packVersions;
		std::vector<ItemMetadata>                     items;
		std::uint32_t                                 configHash = 0;
	};

	class CoSave
	{
	public:
		static CoSave& Get();

		void Register();
		[[nodiscard]] CoSaveState&       State() noexcept { return state; }
		[[nodiscard]] const CoSaveState& State() const noexcept { return state; }
		void Reset();

		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		static void OnSave(SKSE::SerializationInterface* a_intfc);
		static void OnLoad(SKSE::SerializationInterface* a_intfc);
		static void OnRevert(SKSE::SerializationInterface* a_intfc);

		static std::vector<std::uint8_t> Encode(const CoSaveState& a_state);
		static bool                      Decode(const std::vector<std::uint8_t>& a_bytes, CoSaveState& a_out);
		static std::uint32_t             HashConfig();

		CoSaveState state;
	};
}