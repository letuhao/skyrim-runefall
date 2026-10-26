#include "PapyrusBridge.h"

#include "CoSave.h"
#include "DebugHarness.h"
#include "HuntEngine.h"
#include "MenuApi.h"
#include "Progression.h"

#include <map>

namespace Runefall
{
	namespace
	{
		const std::map<std::string, std::size_t, std::less<>>& AttributeIndices()
		{
			static const std::map<std::string, std::size_t, std::less<>> kIndices = {
				{ "Might", 0 }, { "Endurance", 1 }, { "Precision", 2 },
				{ "Agility", 3 }, { "Guile", 4 }, { "Cunning", 5 },
				{ "Intellect", 6 }, { "Wisdom", 7 }, { "Willpower", 8 }
			};
			return kIndices;
		}

		std::int32_t GetApiVersion(RE::StaticFunctionTag*)
		{
			return static_cast<std::int32_t>(kApiVersion);
		}

		std::int32_t GetPluginVersion(RE::StaticFunctionTag*)
		{
			const auto* plugin = SKSE::PluginDeclaration::GetSingleton();
			const auto  version = plugin ? plugin->GetVersion() : REL::Version{};
			return static_cast<std::int32_t>((version[0] << 16) | (version[1] << 8) | version[2]);
		}

		std::int32_t GetProbeFailures(RE::StaticFunctionTag*)
		{
			return DebugHarness::LastFailures();
		}

		std::string GetProbeSummary(RE::StaticFunctionTag*)
		{
			return DebugHarness::LastSummary();
		}

		std::int32_t GetAttribute(RE::StaticFunctionTag*, std::string a_attribute)
		{
			const auto& indices = AttributeIndices();
			const auto  it = indices.find(a_attribute);
			return it == indices.end() ? 0 : Progression::Get().Attribute(it->second);
		}

		std::int32_t GetAttributePoints(RE::StaticFunctionTag*)
		{
			return Progression::Get().AttributePoints();
		}

		std::int32_t GetSkillPoints(RE::StaticFunctionTag*)
		{
			return Progression::Get().SkillPoints();
		}

		std::int32_t GetRuneTier(RE::StaticFunctionTag*)
		{
			return Progression::Get().RuneTier();
		}

		std::int32_t GetSkillRank(RE::StaticFunctionTag*, std::string a_skill)
		{
			return Progression::Get().SkillRank(a_skill);
		}

		std::int32_t GetRuneCount(RE::StaticFunctionTag*, std::int32_t a_rank)
		{
			if (a_rank < 1 || a_rank > 5)
				return 0;
			return CoSave::Get().State().runes[static_cast<std::size_t>(a_rank - 1)];
		}

		std::int32_t GetHuntFirstKill(RE::StaticFunctionTag*, std::string a_group)
		{
			return HuntEngine::Get().HasFirstKill(a_group) ? 1 : 0;
		}

		std::int32_t SpendAttributePoint(RE::StaticFunctionTag*, std::string a_attribute)
		{
			// Marshal state mutation onto the main thread; the VM caller gets an accepted result.
			auto* tasks = SKSE::GetTaskInterface();
			if (!tasks)
				return 1;
			tasks->AddTask([a_attribute]() {
				const auto& indices = AttributeIndices();
				if (const auto it = indices.find(a_attribute); it != indices.end())
					Progression::Get().SpendAttributePoint(it->second);
			});
			return 0;
		}

		std::int32_t SpendSkillPoint(RE::StaticFunctionTag*, std::string a_skill)
		{
			auto* tasks = SKSE::GetTaskInterface();
			if (!tasks)
				return 1;
			tasks->AddTask([a_skill]() { Progression::Get().SpendSkillPoint(a_skill); });
			return 0;
		}

		std::int32_t AttuneRuneTier(RE::StaticFunctionTag*)
		{
			auto* tasks = SKSE::GetTaskInterface();
			if (!tasks)
				return 1;
			tasks->AddTask([]() {
				const auto result = Progression::Get().TryAttune();
				logger::info("papyrus: AttuneRuneTier result={}", result);
			});
			return 0;
		}

		std::string GetMenuSnapshot(RE::StaticFunctionTag*)
		{
			return MenuApi::BuildSnapshotJson();
		}
	}

	bool PapyrusBridge::Register(RE::BSScript::IVirtualMachine* a_vm)
	{
		if (!a_vm) {
			logger::error("Papyrus bridge: null VM");
			return false;
		}
		a_vm->RegisterFunction("GetApiVersion", "RunefallNative", GetApiVersion);
		a_vm->RegisterFunction("GetPluginVersion", "RunefallNative", GetPluginVersion);
		a_vm->RegisterFunction("GetProbeFailures", "RunefallNative", GetProbeFailures);
		a_vm->RegisterFunction("GetProbeSummary", "RunefallNative", GetProbeSummary);
		a_vm->RegisterFunction("GetAttribute", "RunefallNative", GetAttribute);
		a_vm->RegisterFunction("GetAttributePoints", "RunefallNative", GetAttributePoints);
		a_vm->RegisterFunction("GetSkillPoints", "RunefallNative", GetSkillPoints);
		a_vm->RegisterFunction("GetRuneTier", "RunefallNative", GetRuneTier);
		a_vm->RegisterFunction("GetSkillRank", "RunefallNative", GetSkillRank);
		a_vm->RegisterFunction("GetRuneCount", "RunefallNative", GetRuneCount);
		a_vm->RegisterFunction("GetHuntFirstKill", "RunefallNative", GetHuntFirstKill);
		a_vm->RegisterFunction("SpendAttributePoint", "RunefallNative", SpendAttributePoint);
		a_vm->RegisterFunction("SpendSkillPoint", "RunefallNative", SpendSkillPoint);
		a_vm->RegisterFunction("GetMenuSnapshot", "RunefallNative", GetMenuSnapshot);
		a_vm->RegisterFunction("AttuneRuneTier", "RunefallNative", AttuneRuneTier);
		// RegisterFunction returns void in this CommonLibSSE-NG version; failures surface via the VM log.
		logger::info("Papyrus bridge: functions registered (api {})", kApiVersion);
		return true;
	}
}