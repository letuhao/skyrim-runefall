#include "PCH.h"

#include "CoSave.h"
#include "Config.h"
#include "DebugHarness.h"
#include "InputManager.h"
#include "MenuUI.h"
#include "LootService.h"
#include "PapyrusBridge.h"

namespace
{
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

	void OnDataLoaded()
	{
		logger::info("Runefall data loaded: bootstrap complete, API {}", Runefall::kApiVersion);
		Runefall::LootService::Get().RegisterHooks();
		Runefall::InputManager::Get().Register();
		Runefall::MenuUI::Register();
		if (Runefall::Config::Get().DebugSelfTest())
			Runefall::DebugHarness::RunAll();
	}

	void OnMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			OnDataLoaded();
			break;
		case SKSE::MessagingInterface::kNewGame:
		case SKSE::MessagingInterface::kPostLoadGame:
			Runefall::DebugHarness::RunGameplayProbes();
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

	if (const auto* papyrus = SKSE::GetPapyrusInterface(); !papyrus || !papyrus->Register(Runefall::PapyrusBridge::Register)) {
		logger::error("Failed to register Papyrus interface");
		return false;
	}

	Runefall::CoSave::Get().Register();

	const auto* messaging = SKSE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(OnMessage)) {
		logger::error("Failed to register messaging listener");
		return false;
	}

	return true;
}