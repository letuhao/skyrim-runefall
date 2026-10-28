#include "Config.h"

namespace Runefall
{
	Config& Config::Get()
	{
		static Config singleton;
		return singleton;
	}

	void Config::ApplySection(std::string_view a_section, std::string_view a_key, std::string_view a_value)
	{
		if (a_section == "General") {
			if (a_key == "bDebugSelfTest")
				debugSelfTest = a_value == "1" || a_value == "true";
			else if (a_key == "bDebugTrace")
				debugTrace = a_value == "1" || a_value == "true";
			else if (a_key == "iGameplayDelaySeconds")
				gameplayDelaySeconds = std::atoi(std::string{ a_value }.c_str());
			else if (a_key == "iRuntimeScanDelaySeconds")
				runtimeScanDelaySeconds = std::atoi(std::string{ a_value }.c_str());
			else if (a_key == "bEnableRuntimeLoot")
				enableRuntimeLoot = a_value == "1" || a_value == "true";
			else if (a_key == "bEnableContainerLoot")
				enableContainerLoot = a_value == "1" || a_value == "true";
			else if (a_key == "bEnableActorEquipment")
				enableActorEquipment = a_value == "1" || a_value == "true";
			else if (a_key == "bEnableEncounters")
				enableEncounters = a_value == "1" || a_value == "true";
			else if (a_key == "bEnableActorRanks")
				enableActorRanks = a_value == "1" || a_value == "true";
			else if (a_key == "bEnableQuestDrops")
				enableQuestDrops = a_value == "1" || a_value == "true";
			else if (a_key == "iContainerLootPerCell")
				containerLootPerCell = std::atoi(std::string{ a_value }.c_str());
			else if (a_key == "iLootAuthority")
				lootAuthority = std::atoi(std::string{ a_value }.c_str());
			else if (a_key == "bEnableMenu")
				enableMenu = a_value == "1" || a_value == "true";
			else if (a_key == "bEnableInput")
				enableInput = a_value == "1" || a_value == "true";
			else if (a_key == "bEnableLootHooks")
				enableLootHooks = a_value == "1" || a_value == "true";
			else if (a_key == "bEnablePapyrus")
				enablePapyrus = a_value == "1" || a_value == "true";
			else if (a_key == "bEnableCoSave")
				enableCoSave = a_value == "1" || a_value == "true";
			else if (a_key == "bEnableProbes")
				enableProbes = a_value == "1" || a_value == "true";
			else if (a_key == "bEnableDebugNatives")
				enableDebugNatives = a_value == "1" || a_value == "true";
		} else if (a_section == "Paths") {
			if (a_key == "sDataDir")
				dataDirOverride = a_value;
			else if (a_key == "sPackDir")
				packDirOverride = a_value;
		}
	}

	SelfTestResult Config::Load()
	{
		const auto iniPath = DataRoot() / "Runefall" / "runefall.ini";
		loadedFrom = iniPath.string();

		std::ifstream file{ iniPath };
		if (!file.is_open())
			return { "Config", CheckStatus::Warn, "runefall.ini not found; defaults used (" + loadedFrom + ")" };

		std::string section = "General";
		std::string line;
		while (std::getline(file, line)) {
			const auto trimmed = Trim(line);
			if (trimmed.empty() || trimmed.front() == ';' || trimmed.front() == '#')
				continue;
			if (trimmed.front() == '[') {
				const auto close = trimmed.find(']');
				if (close != std::string_view::npos)
					section = std::string{ trimmed.substr(1, close - 1) };
				continue;
			}
			const auto eq = trimmed.find('=');
			if (eq == std::string_view::npos)
				continue;
			ApplySection(section, Trim(trimmed.substr(0, eq)), Trim(trimmed.substr(eq + 1)));
		}

		return { "Config", CheckStatus::Pass, "loaded " + loadedFrom };
	}

	SelfTestResult Config::SelfTest() const
	{
		if (lootAuthority < 0 || lootAuthority > 2)
			return { "Config.SelfTest", CheckStatus::Fail, "iLootAuthority out of range" };
		if (gameplayDelaySeconds < 0 || gameplayDelaySeconds > 3600)
			return { "Config.SelfTest", CheckStatus::Fail, "iGameplayDelaySeconds out of range" };
		if (runtimeScanDelaySeconds < 0 || runtimeScanDelaySeconds > 3600)
			return { "Config.SelfTest", CheckStatus::Fail, "iRuntimeScanDelaySeconds out of range" };
		if (containerLootPerCell < 0 || containerLootPerCell > 64)
			return { "Config.SelfTest", CheckStatus::Fail, "iContainerLootPerCell out of range" };
		return { "Config.SelfTest", CheckStatus::Pass, "debug=" + std::string{ debugSelfTest ? "1" : "0" } + " authority=" + std::to_string(lootAuthority) };
	}
}