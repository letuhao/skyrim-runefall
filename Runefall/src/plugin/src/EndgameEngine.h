#pragma once
#include "Core.h"

namespace Runefall
{
	class EndgameEngine
	{
	public:
		static EndgameEngine& Get();
		void Load();
		[[nodiscard]] bool Loaded() const noexcept { return loaded; }
		[[nodiscard]] bool CanChallenge() const;
		[[nodiscard]] bool Completed() const;
		[[nodiscard]] const std::string& FinalBossId() const noexcept { return finalBossId; }
		[[nodiscard]] std::int32_t RequiredRank() const noexcept { return requiredRank; }
		bool Complete();
		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		std::string finalBossId;
		std::int32_t requiredRank = 90;
		std::int32_t requiredBossTier = 5;
		std::int32_t rewardRunes = 50;
		std::string completionTitleKey;
		bool loaded = false;
	};
}
