#include "DebugHarness.h"
#include "CoSave.h"
#include "Config.h"
#include "DataStore.h"
#include "FormResolver.h"
#include "ItemRegistry.h"
#include "HuntEngine.h"
#include "LootEngine.h"
#include "Progression.h"
#include "DynamicEnchantment.h"
#include "RuneForge.h"
#include "TierEngine.h"
#include "PackManager.h"

#include <format>

namespace Runefall
{
	void DebugHarness::WriteReport(const std::vector<SelfTestResult>& a_results)
	{
		const auto path = ProbeLogPath();
		EnsureParentDirectory(path);
		std::ofstream out{ path, std::ios::trunc };
		if (!out.is_open()) {
			logger::error("probe: cannot open report file {}", path.string());
			return;
		}

		const auto* plugin = SKSE::PluginDeclaration::GetSingleton();
		out << "Runefall probe report\n";
		out << "plugin_version: " << (plugin ? plugin->GetVersion().string() : "unknown") << "\n";
		out << "api_version: " << kApiVersion << "\n";
		out << "save_schema: " << kSaveVersion << "\n";
		out << "runtime: " << (REL::Module::IsAE() ? "AE" : (REL::Module::IsVR() ? "VR" : "SE")) << " " << REL::Module::get().version().string() << "\n";
		out << "game_root: " << FindGameRoot().string() << "\n";
		out << "data_dir: " << DataStore::Get().DataDirectory().string() << "\n";
		out << "probe_time: " << std::time(nullptr) << "\n\n";

		for (const auto& result : a_results)
			out << "[" << result.StatusText() << "] " << result.name << ": " << result.detail << "\n";

		out << "\nSUMMARY: pass=" << LastPasses() << " warn=" << LastWarnings() << " fail=" << LastFailures() << "\n";
		out.close();
		logger::info("probe: report written to {}", path.string());
	}

	void DebugHarness::RunAll()
	{
		std::vector<SelfTestResult> results;

		results.push_back(Config::Get().SelfTest());
		results.push_back(DataStore::Get().LoadAll(RunefallDataDir()));
		results.push_back(DataStore::Get().SelfTest());
		LootEngine::Get().Reload();
		TierEngine::Get().Reload();
		HuntEngine::Get().Reload();
		results.push_back(PackManager::Get().Scan());
		results.push_back(PackManager::Get().SelfTest());
		results.push_back(FormResolver::SelfTest());
		results.push_back(ItemRegistry::SelfTest());
		results.push_back(LootEngine::SelfTest());
		results.push_back(TierEngine::SelfTest());
		results.push_back(HuntEngine::SelfTest());
		results.push_back(Progression::SelfTest());
		results.push_back(RuneForge::SelfTest());
		results.push_back(DynamicEnchantment::SelfTest());
		results.push_back(CoSave::SelfTest());

		lastPasses = 0;
		lastWarnings = 0;
		lastFailures = 0;
		for (const auto& result : results) {
			switch (result.status) {
			case CheckStatus::Pass:
				++lastPasses;
				logger::info("[probe][PASS] {}: {}", result.name, result.detail);
				break;
			case CheckStatus::Warn:
				++lastWarnings;
				logger::warn("[probe][WARN] {}: {}", result.name, result.detail);
				break;
			case CheckStatus::Fail:
				++lastFailures;
				logger::error("[probe][FAIL] {}: {}", result.name, result.detail);
				break;
			}
		}
		lastSummary = std::format("pass={} warn={} fail={}", lastPasses, lastWarnings, lastFailures);
		logger::info("probe: {}", lastSummary);

		WriteReport(results);

		if (auto* console = RE::ConsoleLog::GetSingleton())
			console->Print("[Runefall] probe %s", lastSummary.c_str());
	}
}