#pragma once
#include "Core.h"

#include <chrono>

namespace Runefall
{
	class EncounterManager
	{
	public:
		static EncounterManager& Get();

		void Load();
		[[nodiscard]] bool Loaded() const noexcept { return loaded; }
		[[nodiscard]] std::size_t TableCount() const noexcept { return entries.size(); }
		bool DebugSpawn();
		[[nodiscard]] std::string SelectEncounterId(std::string_view a_bandId, std::uint8_t a_stage, std::uint64_t a_seed) const;
		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		struct Entry
		{
			std::string encounterId;
			std::string bandId;
			std::string className;
			std::string actorEditorId;
			std::uint8_t stageMin = 1;
			std::uint8_t stageMax = 20;
			float weight = 0.0f;
			std::int32_t maxPerCell = 1;
			std::int32_t cooldownSeconds = 60;
			bool enabled = true;
		};
		[[nodiscard]] const Entry* Select(std::string_view a_bandId, std::uint8_t a_stage, std::uint64_t a_seed) const;

		std::vector<Entry> entries;
		std::unordered_map<RE::FormID, std::int32_t> cellCounts;
		std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point> cooldowns;
		bool loaded = false;
	};
}
