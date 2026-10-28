#pragma once
#include "Core.h"

#include <chrono>

namespace Runefall
{
	class HuntContractManager
	{
	public:
		static HuntContractManager& Get();
		void Load();
		[[nodiscard]] bool Loaded() const noexcept { return loaded; }
		[[nodiscard]] std::size_t ContractCount() const noexcept { return contracts.size(); }
		bool DebugStartFirst();
		void OnActorDeath(RE::Actor* a_actor);
		[[nodiscard]] std::string ActiveContractId() const { return active.contractId; }
		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		struct Contract
		{
			std::string contractId;
			std::string family;
			std::uint8_t minStage = 1;
			std::uint8_t maxStage = 20;
			std::int32_t rewardRunes = 0;
			std::int32_t cooldownSeconds = 60;
			std::int32_t durationSeconds = 300;
			bool enabled = true;
		};
		struct Active
		{
			std::string contractId;
			std::string family;
			std::int32_t rewardRunes = 0;
			std::chrono::steady_clock::time_point expires{};
		};

		std::vector<Contract> contracts;
		Active active;
		std::unordered_map<std::string, std::chrono::steady_clock::time_point> cooldowns;
		bool loaded = false;
	};
}
