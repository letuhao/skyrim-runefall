#pragma once
#include "Core.h"

#include <chrono>

namespace Runefall
{
	struct RaceDescriptor
	{
		std::string plugin;
		std::uint32_t localFormId = 0;
		std::string editorId;
	};

	struct RuntimeRaceProfile
	{
		std::string  cacheKey;
		std::string  roleKey;
		std::string  family;
		std::uint8_t stageMin = 1;
		std::uint8_t stageMax = 10;
		bool         provisional = false;
		std::vector<std::string> materialRefs;
		std::string  lootTable;
		std::string  dataVersion;
	};

	class RuntimeContentResolver
	{
	public:
		static RuntimeContentResolver& Get();

		void Load();
		[[nodiscard]] bool Loaded() const noexcept { return loaded; }
		[[nodiscard]] RuntimeRaceProfile ResolveRace(const RaceDescriptor& a_race);
		[[nodiscard]] std::size_t RoleCount() const noexcept { return roles.size(); }
		[[nodiscard]] std::size_t RaceProfileCount() const noexcept { return raceProfiles.size(); }
		[[nodiscard]] std::size_t CachedProfileCount() const noexcept { return cache.size(); }
		void Rescan();
		void MaybeStartupScan();
		[[nodiscard]] std::string_view DataVersion() const noexcept { return dataVersion; }
		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		struct RoleDef
		{
			std::string key;
			std::string parentKey;
			std::string tags;
			std::string hostPolicy;
			std::string protectionPolicy;
			std::string family;
		};
		struct RaceDef
		{
			std::string key;
			std::string raceEditorId;
			std::string roles;
			std::string family;
		};

		[[nodiscard]] const RoleDef* FindRole(std::string_view a_key) const;
		[[nodiscard]] const RaceDef* FindRace(std::string_view a_editorId) const;
		[[nodiscard]] std::string CachePath(std::string_view a_key) const;
		[[nodiscard]] static std::string CacheKey(const RaceDescriptor& a_race);
		bool WriteCache(const RuntimeRaceProfile& a_profile);
		bool WriteManifest();
		[[nodiscard]] std::string ManifestPath() const;
		void RescanIfNeeded();
		[[nodiscard]] std::string ComputeDataVersion() const;
		[[nodiscard]] std::string ReadCache(std::string_view a_path) const;

		std::unordered_map<std::string, RoleDef> roles;
		std::unordered_map<std::string, RaceDef> raceProfiles;
		std::unordered_map<std::string, RuntimeRaceProfile> cache;
		bool loaded = false;
		std::string dataVersion;
		bool startupScanDone = false;
		std::chrono::steady_clock::time_point startupStart = std::chrono::steady_clock::now();
	};
}
