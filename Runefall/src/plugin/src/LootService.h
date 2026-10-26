#pragma once
#include "Core.h"
#include "ItemMetadata.h"
#include "LootEngine.h"

namespace Runefall
{
	class LootService
	{
	public:
		static LootService& Get();

		void RegisterHooks();
		void OnActorDeath(RE::Actor* a_actor);

		// Debug/probe path: rolls one item, attaches metadata + runtime enchantment and adds it
		// to the player inventory. Used by the batch probe; safe to call once per load.
		[[nodiscard]] bool InjectProbeItem();

		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		[[nodiscard]] RE::TESBoundObject* PickBaseObject(RE::Actor* a_actor, Rng& a_rng);
		[[nodiscard]] bool                SpawnForActor(RE::Actor* a_actor);

		std::unordered_set<RE::FormID> processedActors;
		bool                           hooksRegistered = false;
	};
}