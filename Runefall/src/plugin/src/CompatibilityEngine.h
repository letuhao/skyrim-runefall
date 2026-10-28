#pragma once
#include "Core.h"

namespace Runefall
{
	class CompatibilityEngine
	{
	public:
		static CompatibilityEngine& Get();
		void Load();
		[[nodiscard]] bool Loaded() const noexcept { return loaded; }
		[[nodiscard]] const std::string& ActiveProfile() const noexcept { return activeProfile; }
		[[nodiscard]] bool ShouldGenerateLoot() const noexcept { return lootMode != "disabled"; }
		[[nodiscard]] bool ProtectVanillaLoot() const noexcept { return protectVanilla; }
		[[nodiscard]] std::size_t ProfileCount() const noexcept { return profiles.size(); }
		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		struct Profile
		{
			std::string profileId;
			std::vector<std::string> requiredPlugins;
			std::vector<std::string> optionalPlugins;
			std::string lootMode;
			bool protectVanilla = true;
			std::string notes;
		};
		void Detect();
		std::vector<Profile> profiles;
		std::string activeProfile = "vanilla";
		std::string lootMode = "additive";
		bool protectVanilla = true;
		bool loaded = false;
	};
}
