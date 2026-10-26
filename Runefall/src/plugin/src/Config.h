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
		[[nodiscard]] bool        DebugTrace() const noexcept { return debugTrace; }
		[[nodiscard]] int         LootAuthority() const noexcept { return lootAuthority; }
		[[nodiscard]] std::string DataDirOverride() const { return dataDirOverride; }
		[[nodiscard]] std::string PackDirOverride() const { return packDirOverride; }

	private:
		void ApplySection(std::string_view a_section, std::string_view a_key, std::string_view a_value);

		bool        debugSelfTest = true;
		bool        debugTrace = false;
		int         lootAuthority = 0;
		std::string dataDirOverride;
		std::string packDirOverride;
		std::string loadedFrom;
	};
}
