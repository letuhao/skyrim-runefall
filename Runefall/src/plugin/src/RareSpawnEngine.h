#pragma once
#include "Core.h"

namespace Runefall
{
	struct RareSpawn
	{
		std::string rareId;
		std::string actorEditorId;
		std::uint8_t stage = 1;
		std::string conditionTag;
	};

	class RareSpawnEngine
	{
	public:
		static RareSpawnEngine& Get();
		void Load();
		[[nodiscard]] bool Loaded() const noexcept { return loaded; }
		[[nodiscard]] bool TryPick(std::string_view a_bandId, std::uint64_t a_seed, RareSpawn& a_out) const;
		[[nodiscard]] std::size_t EntryCount() const noexcept { return entries.size(); }
		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		struct Entry
		{
			std::string rareId;
			std::string bandId;
			std::string actorEditorId;
			int chancePermille = 0;
			std::uint8_t stageMin = 80;
			std::uint8_t stageMax = 100;
			std::string conditionTag;
			float weight = 0.0f;
			bool enabled = true;
		};
		std::vector<Entry> entries;
		bool loaded = false;
	};
}
