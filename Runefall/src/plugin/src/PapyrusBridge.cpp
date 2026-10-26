#include "PapyrusBridge.h"
#include "DebugHarness.h"

namespace Runefall
{
	namespace
	{
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
		logger::info("Papyrus bridge: functions registered (api {})", kApiVersion);
		return true;
	}
}
