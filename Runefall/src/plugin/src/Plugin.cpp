#include "PCH.h"

#include "CoSave.h"
#include "Config.h"
#include "DebugHarness.h"
#include "InputManager.h"
#include "MenuUI.h"
#include "LootService.h"
#include "DynamicEnchantment.h"
#include "PapyrusBridge.h"
#include "DataStore.h"
#include "FormCatalog.h"
#include "StageEngine.h"
#include "RuntimeContentResolver.h"
#include "RaceClassifier.h"
#include "DamagePipeline.h"
#include "HookManager.h"
#include "MaterialRegistry.h"
#include "RuneForge.h"
#include "LootEngine.h"
#include "TierEngine.h"
#include "HuntEngine.h"

#include <atomic>
#include <chrono>
#include <thread>

namespace
{
	std::atomic_bool   g_activationScheduled{ false };
	std::atomic_uint64_t g_activationGeneration{ 0 };
	std::jthread       g_activationThread;

	void SetupLog()
	{
		auto logsFolder = logger::log_directory();
		if (!logsFolder) {
			util::report_and_fail("SKSE log_directory not provided, logs can't be written");
		}

		const auto* plugin = SKSE::PluginDeclaration::GetSingleton();
		const auto  logName = plugin ? std::string{ plugin->GetName() } + ".log" : "Runefall.log";
		auto        logPath = *logsFolder / logName;

		auto                          fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
		std::vector<spdlog::sink_ptr> sinks{ fileSink };
		if (IsDebuggerPresent()) {
			sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
		}

		auto spdlogger = std::make_shared<spdlog::logger>("global", sinks.begin(), sinks.end());
		spdlog::set_default_logger(std::move(spdlogger));
		spdlog::set_pattern("[%H:%M:%S.%e] [%l] [%s:%#] %v");
#ifdef NDEBUG
		spdlog::set_level(spdlog::level::info);
#else
		spdlog::set_level(spdlog::level::trace);
#endif
		spdlog::flush_on(spdlog::level::info);
	}

	void ActivateGameplayAfterDelay()
	{
		if (g_activationScheduled.exchange(true))
			return;
		const auto delaySeconds = (std::max)(5, Runefall::Config::Get().GameplayDelaySeconds());
		const auto generation = ++g_activationGeneration;
		Runefall::DynamicEnchantment::SetGameplayReady(false);
		logger::info("gameplay activation scheduled in {} seconds (generation {})", delaySeconds, generation);
		g_activationThread = std::jthread([delaySeconds, generation](std::stop_token a_stop) {
			const auto until = std::chrono::steady_clock::now() + std::chrono::seconds{ delaySeconds };
			while (!a_stop.stop_requested() && std::chrono::steady_clock::now() < until)
				std::this_thread::sleep_for(std::chrono::milliseconds{ 500 });
			if (a_stop.stop_requested() || generation != g_activationGeneration.load())
				return;
			if (auto* tasks = SKSE::GetTaskInterface()) {
				tasks->AddTask([generation]() {
					if (generation != g_activationGeneration.load()) {
						g_activationScheduled.store(false);
						return;
					}
					auto* player = RE::PlayerCharacter::GetSingleton();
					if (!player) {
						logger::warn("gameplay activation: player still unavailable; delaying activation");
						g_activationScheduled.store(false);
						return;
					}
					Runefall::DynamicEnchantment::SetGameplayReady(true);
					if (Runefall::Config::Get().EnableProbes())
						Runefall::DebugHarness::RunGameplayProbes();
					g_activationScheduled.store(false);
				});
			} else {
				logger::error("gameplay activation: task interface unavailable");
				g_activationScheduled.store(false);
			}
		});
	}

	void OnDataLoaded()
	{
		logger::info("Runefall data loaded: bootstrap complete, API {}", Runefall::kApiVersion);
		Runefall::DynamicEnchantment::SetGameplayReady(false);
		Runefall::HookManager::Get().Register();
		if (Runefall::Config::Get().EnableLootHooks())
			Runefall::LootService::Get().RegisterHooks();
		if (Runefall::Config::Get().EnableInput())
			Runefall::InputManager::Get().Register();
		if (Runefall::Config::Get().EnableMenu())
			Runefall::MenuUI::Register();
		// Data/engine tables are required by loot/hunt/tier even when probes are off.
		Runefall::DataStore::Get().LoadAll(Runefall::RunefallDataDir());
		Runefall::FormCatalog::Get().Load();
		Runefall::StageEngine::Get().Load();
		Runefall::RuntimeContentResolver::Get().Load();
		Runefall::RaceClassifier::Get().Load();
		Runefall::DamagePipeline::Get().LoadProtection();
		Runefall::MaterialRegistry::Get().LoadDefinitions();
		Runefall::RuneForge::Get().ReloadSocketRecipes();
		Runefall::LootEngine::Get().Reload();
		Runefall::TierEngine::Get().Reload();
		Runefall::HuntEngine::Get().Reload();
		if (Runefall::Config::Get().EnableProbes() && Runefall::Config::Get().DebugSelfTest())
			Runefall::DebugHarness::RunAll();
	}

	void OnMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		if (!a_msg) {
			logger::error("message: null message");
			return;
		}
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			OnDataLoaded();
			break;
		case SKSE::MessagingInterface::kNewGame:
		case SKSE::MessagingInterface::kPostLoadGame:
			// Never touch runtime forms/player here: even a loaded kNewGame/kPostLoadGame message can
			// fire while the world is still streaming in. Wait the configured delay on a worker,
			// then enqueue one main-thread activation task.
			Runefall::LootService::Get().ResetSessionState();
			ActivateGameplayAfterDelay();
			break;
		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	SetupLog();

	const auto* plugin = SKSE::PluginDeclaration::GetSingleton();
	if (!plugin) {
		logger::error("Failed to get plugin declaration");
		return false;
	}
	logger::info("{} v{} loading (api {})", plugin->GetName(), plugin->GetVersion(), Runefall::kApiVersion);
	logger::info("runtime: {} version {}", REL::Module::IsAE() ? "AE" : (REL::Module::IsVR() ? "VR" : "SE"), REL::Module::get().version().string());

	const auto config = Runefall::Config::Get().Load();
	logger::info("config: {} - {}", config.StatusText(), config.detail);

	if (Runefall::Config::Get().EnablePapyrus()) {
		if (const auto* papyrus = SKSE::GetPapyrusInterface(); !papyrus || !papyrus->Register(Runefall::PapyrusBridge::Register)) {
			logger::error("Failed to register Papyrus interface");
			return false;
		}
	} else {
		logger::warn("Papyrus bridge disabled by config");
	}

	if (Runefall::Config::Get().EnableCoSave())
		Runefall::CoSave::Get().Register();
	else
		logger::warn("co-save disabled by config");

	const auto* messaging = SKSE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(OnMessage)) {
		logger::error("Failed to register messaging listener");
		return false;
	}

	return true;
}