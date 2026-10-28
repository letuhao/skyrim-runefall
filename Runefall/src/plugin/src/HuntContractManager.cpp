#include "HuntContractManager.h"

#include "Config.h"
#include "DataStore.h"
#include "HuntEngine.h"
#include "LootService.h"

#include "RE/A/Actor.h"
#include "RE/T/TESRace.h"

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
		bool Bool(const std::vector<std::string>& a_row, int a_column)
		{
			return Cell(a_row, a_column) == "1";
		}
	}

	HuntContractManager& HuntContractManager::Get()
	{
		static HuntContractManager singleton;
		return singleton;
	}

	void HuntContractManager::Load()
	{
		contracts.clear();
		loaded = false;
		const auto* table = DataStore::Get().GetTable("hunt_contracts.csv");
		if (!table) {
			logger::error("hunt-contract: hunt_contracts.csv missing");
			return;
		}
		const int cId = FindColumn(*table, "contractId");
		const int cFamily = FindColumn(*table, "family");
		const int cMin = FindColumn(*table, "minStage");
		const int cMax = FindColumn(*table, "maxStage");
		const int cReward = FindColumn(*table, "rewardRunes");
		const int cCooldown = FindColumn(*table, "cooldownSeconds");
		const int cDuration = FindColumn(*table, "durationSeconds");
		const int cEnabled = FindColumn(*table, "enabled");
		for (const auto& row : table->rows) {
			Contract contract;
			contract.contractId = Cell(row, cId);
			contract.family = Cell(row, cFamily);
			contract.minStage = static_cast<std::uint8_t>((std::clamp)(Int(row, cMin, 1), 1, 100));
			contract.maxStage = static_cast<std::uint8_t>((std::clamp)(Int(row, cMax, 20), 1, 100));
			contract.rewardRunes = (std::max)(0, Int(row, cReward));
			contract.cooldownSeconds = (std::max)(0, Int(row, cCooldown, 60));
			contract.durationSeconds = (std::max)(1, Int(row, cDuration, 300));
			contract.enabled = Bool(row, cEnabled);
			if (!contract.contractId.empty() && contract.minStage <= contract.maxStage)
				contracts.push_back(std::move(contract));
		}
		loaded = !contracts.empty();
		if (loaded)
			logger::info("hunt-contract: loaded {} contracts", contracts.size());
	}

	bool HuntContractManager::DebugStartFirst()
	{
		if (!loaded || !Config::Get().EnableHuntContracts())
			return false;
		const auto now = std::chrono::steady_clock::now();
		if (!active.contractId.empty() && now < active.expires) {
			logger::info("hunt-contract: already active {}", active.contractId);
			return false;
		}
		for (const auto& contract : contracts) {
			if (!contract.enabled)
				continue;
			if (const auto it = cooldowns.find(contract.contractId); it != cooldowns.end() && now < it->second)
				continue;
			active.contractId = contract.contractId;
			active.family = contract.family;
			active.rewardRunes = contract.rewardRunes;
			active.expires = now + std::chrono::seconds(contract.durationSeconds);
			logger::info("hunt-contract: started {} family {} reward {} runes", active.contractId, active.family, active.rewardRunes);
			return true;
		}
		return false;
	}

	void HuntContractManager::OnActorDeath(RE::Actor* a_actor)
	{
		if (!loaded || active.contractId.empty() || !a_actor)
			return;
		const auto now = std::chrono::steady_clock::now();
		if (now >= active.expires) {
			logger::info("hunt-contract: expired {}", active.contractId);
			active = {};
			return;
		}
		const auto* race = a_actor->GetActorBase() ? a_actor->GetActorBase()->GetRace() : nullptr;
		const char* editor = race ? race->GetFormEditorID() : nullptr;
		const auto* family = editor && *editor ? HuntEngine::Get().FindFamilyForRace(editor) : nullptr;
		if (!family || family->group != active.family) {
			logger::info("hunt-contract: killed actor {:08X} is not contract family {}", a_actor->GetFormID(), active.family);
			return;
		}
		HuntEngine::Get().MarkFirstKill("contract:" + active.contractId);
		if (active.rewardRunes > 0)
			LootService::Get().DebugGiveRuneItem(active.rewardRunes);
		for (const auto& contract : contracts) {
			if (contract.contractId == active.contractId)
				cooldowns[active.contractId] = now + std::chrono::seconds(contract.cooldownSeconds);
		}
		logger::info("hunt-contract: completed {} family {} reward {} runes", active.contractId, active.family, active.rewardRunes);
		active = {};
	}

	SelfTestResult HuntContractManager::SelfTest()
	{
		auto& manager = Get();
		if (!manager.Loaded())
			return { "HuntContractManager.SelfTest", CheckStatus::Fail, "hunt contracts not loaded" };
		if (manager.ContractCount() < 1)
			return { "HuntContractManager.SelfTest", CheckStatus::Fail, "no contracts" };
		const auto& first = manager.contracts.front();
		if (first.contractId.empty() || first.family.empty())
			return { "HuntContractManager.SelfTest", CheckStatus::Fail, "contract definition invalid" };
		return { "HuntContractManager.SelfTest", CheckStatus::Pass,
			std::format("{} contracts; first {} family {}", manager.ContractCount(), first.contractId, first.family) };
	}
}
