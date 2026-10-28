#include "EndgameEngine.h"

#include "CoSave.h"
#include "DataStore.h"
#include "Progression.h"

#include <algorithm>
#include <format>

namespace Runefall
{
	namespace
	{
		int FindColumn(const CsvTable& a_table, std::string_view a_name)
		{
			for (std::size_t i = 0; i < a_table.headers.size(); ++i)
				if (a_table.headers[i] == a_name)
					return static_cast<int>(i);
			return -1;
		}
		std::string Cell(const std::vector<std::string>& a_row, int a_column)
		{
			if (a_column < 0 || static_cast<std::size_t>(a_column) >= a_row.size())
				return {};
			return a_row[static_cast<std::size_t>(a_column)];
		}
		int Int(const std::vector<std::string>& a_row, int a_column, int a_default = 0)
		{
			const auto text = Cell(a_row, a_column);
			return text.empty() ? a_default : static_cast<int>(std::strtol(text.c_str(), nullptr, 10));
		}
	}

	EndgameEngine& EndgameEngine::Get()
	{
		static EndgameEngine singleton;
		return singleton;
	}

	void EndgameEngine::Load()
	{
		loaded = false;
		const auto* table = DataStore::Get().GetTable("endgame_profiles.csv");
		if (!table || table->rows.empty()) {
			logger::error("endgame: endgame_profiles.csv missing");
			return;
		}
		const auto& row = table->rows.front();
		finalBossId = Cell(row, FindColumn(*table, "finalBossId"));
		requiredRank = (std::clamp)(Int(row, FindColumn(*table, "requiredRank"), 90), 1, 100);
		requiredBossTier = (std::clamp)(Int(row, FindColumn(*table, "requiredBossTier"), 5), 0, 5);
		rewardRunes = (std::max)(0, Int(row, FindColumn(*table, "rewardRunes"), 50));
		completionTitleKey = Cell(row, FindColumn(*table, "completionTitleKey"));
		loaded = !finalBossId.empty() && !completionTitleKey.empty();
		if (loaded)
			logger::info("endgame: final boss {} required rank {} bossTier {}", finalBossId, requiredRank, requiredBossTier);
	}

	bool EndgameEngine::CanChallenge() const
	{
		return loaded && Progression::Get().RuneLevel() >= requiredRank && CoSave::Get().State().bossKillTier >= requiredBossTier;
	}

	bool EndgameEngine::Completed() const
	{
		return CoSave::Get().State().endgameComplete;
	}

	bool EndgameEngine::Complete()
	{
		if (!loaded || !CanChallenge())
			return false;
		auto& state = CoSave::Get().State();
		if (state.endgameComplete)
			return false;
		state.endgameComplete = true;
		for (std::size_t i = 0; i < state.runes.size(); ++i)
			state.runes[i] += rewardRunes / static_cast<std::int32_t>(state.runes.size());
		logger::info("endgame: completed final boss {} reward {} runes", finalBossId, rewardRunes);
		return true;
	}

	SelfTestResult EndgameEngine::SelfTest()
	{
		auto& engine = Get();
		if (!engine.Loaded())
			return { "EndgameEngine.SelfTest", CheckStatus::Fail, "endgame profile not loaded" };
		if (engine.FinalBossId().empty() || engine.RequiredRank() < 1)
			return { "EndgameEngine.SelfTest", CheckStatus::Fail, "endgame profile invalid" };
		return { "EndgameEngine.SelfTest", CheckStatus::Pass,
			std::format("final boss {}, rank {}, completion={}", engine.FinalBossId(), engine.RequiredRank(), engine.Completed()) };
	}
}
