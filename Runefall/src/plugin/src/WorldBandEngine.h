#pragma once
#include "Core.h"

namespace Runefall
{
	class WorldBandEngine
	{
	public:
		static WorldBandEngine& Get();

		void Load();
		[[nodiscard]] bool Loaded() const noexcept { return loaded; }
		[[nodiscard]] std::uint8_t StageForCell(RE::TESObjectCELL* a_cell, std::uint64_t a_seed) const;
		[[nodiscard]] std::uint8_t StageDefault(std::uint64_t a_seed) const;
		[[nodiscard]] std::size_t BandCount() const noexcept { return bands.size(); }
		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		struct BandDef
		{
			std::string bandId;
			std::string matchType;
			std::string matchValue;
			std::uint8_t stageMin = 1;
			std::uint8_t stageMax = 20;
			float rate = 0.0f;
			std::vector<std::string> exclusions;
		};
		[[nodiscard]] const BandDef* Match(const BandDef& a_band, std::string_view a_zone, std::string_view a_cell, std::string_view a_worldspace) const;
		[[nodiscard]] std::uint8_t PickStage(const BandDef& a_band, std::uint64_t a_seed) const;
		[[nodiscard]] static bool Excluded(const BandDef& a_band, std::string_view a_zone, std::string_view a_cell, std::string_view a_worldspace);

		std::vector<BandDef> bands;
		bool loaded = false;
	};
}
