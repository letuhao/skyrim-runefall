#pragma once
#include "Core.h"

namespace Runefall
{
	class AnomalyEngine
	{
	public:
		static AnomalyEngine& Get();
		void Load();
		[[nodiscard]] bool Loaded() const noexcept { return loaded; }
		[[nodiscard]] std::size_t FamilyCount() const noexcept { return families.size(); }
		[[nodiscard]] const std::string& FamilyForSeed(std::uint64_t a_seed) const;
		[[nodiscard]] std::uint8_t AdjustStage(std::uint8_t a_baseStage, std::string_view a_family) const;
		[[nodiscard]] std::string TierLabel(std::uint8_t a_stage, std::string_view a_family) const;
		[[nodiscard]] std::string CoreItem(std::string_view a_family) const;
		[[nodiscard]] std::string EncounterActor(std::string_view a_family) const;
		bool DebugSpawn();
		void OnActorDeath(RE::Actor* a_actor);
		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		struct Family
		{
			std::string family;
			std::string nameKey;
			std::uint8_t stageOffset = 0;
			std::vector<std::string> tierLabels;
			std::string coreItem;
			std::string trophyItem;
			std::string encounterActor;
			bool enabled = true;
		};
		[[nodiscard]] const Family* Find(std::string_view a_family) const;
		std::vector<Family> families;
		std::unordered_map<RE::FormID, std::string> spawnedFamilies;
		bool loaded = false;
	};
}
