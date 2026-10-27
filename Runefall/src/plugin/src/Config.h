#pragma once
#include "Core.h"

namespace Runefall
{
	class Config
	{
	public:
		static Config& Get();

		SelfTestResult Load();
		SelfTestResult SelfTest() const;

		[[nodiscard]] bool        DebugSelfTest() const noexcept { return debugSelfTest; }
		[[nodiscard]] bool        EnableMenu() const noexcept { return enableMenu; }
		[[nodiscard]] bool        EnableInput() const noexcept { return enableInput; }
		[[nodiscard]] bool        EnableLootHooks() const noexcept { return enableLootHooks; }
		[[nodiscard]] bool        EnablePapyrus() const noexcept { return enablePapyrus; }
		[[nodiscard]] bool        EnableCoSave() const noexcept { return enableCoSave; }
		[[nodiscard]] bool        EnableProbes() const noexcept { return enableProbes; }
		[[nodiscard]] bool        DebugTrace() const noexcept { return debugTrace; }
		[[nodiscard]] int         GameplayDelaySeconds() const noexcept { return gameplayDelaySeconds; }
		[[nodiscard]] bool        EnableRuntimeLoot() const noexcept { return enableRuntimeLoot; }
		[[nodiscard]] bool        EnableContainerLoot() const noexcept { return enableContainerLoot; }
		[[nodiscard]] bool        EnableActorRanks() const noexcept { return enableActorRanks; }
		[[nodiscard]] bool        EnableQuestDrops() const noexcept { return enableQuestDrops; }
		[[nodiscard]] int         ContainerLootPerCell() const noexcept { return containerLootPerCell; }
		[[nodiscard]] int         LootAuthority() const noexcept { return lootAuthority; }
		[[nodiscard]] std::string DataDirOverride() const { return dataDirOverride; }
		[[nodiscard]] std::string PackDirOverride() const { return packDirOverride; }

	private:
		void ApplySection(std::string_view a_section, std::string_view a_key, std::string_view a_value);

		bool        debugSelfTest = true;
		bool        debugTrace = false;
		int         gameplayDelaySeconds = 300;
		bool        enableRuntimeLoot = true;
		bool        enableContainerLoot = true;
		bool        enableActorRanks = true;
		bool        enableQuestDrops = true;
		int         containerLootPerCell = 8;
		bool        enableMenu = true;
		bool        enableInput = true;
		bool        enableLootHooks = true;
		bool        enablePapyrus = true;
		bool        enableCoSave = true;
		bool        enableProbes = true;
		int         lootAuthority = 0;
		std::string dataDirOverride;
		std::string packDirOverride;
		std::string loadedFrom;
	};
}