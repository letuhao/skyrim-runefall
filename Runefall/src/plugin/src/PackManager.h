#pragma once
#include "Core.h"

namespace Runefall
{
	struct PackInfo
	{
		std::string              plugin;
		std::string              packId;
		int                      format = 0;
		std::string              minBaseVersion;
		std::vector<std::string> contentFiles;
		std::string              state;
	};

	class PackManager
	{
	public:
		static PackManager& Get();

		SelfTestResult Scan();
		SelfTestResult SelfTest() const;

		[[nodiscard]] const std::vector<PackInfo>& Packs() const noexcept { return packs; }
		[[nodiscard]] bool                          BasePresent() const noexcept { return basePresent; }
		[[nodiscard]] const std::string&            BasePlugin() const noexcept { return basePlugin; }

	private:
		std::vector<PackInfo> packs;
		bool                  basePresent = false;
		std::string           basePlugin = "Runefall.esp";
	};
}
