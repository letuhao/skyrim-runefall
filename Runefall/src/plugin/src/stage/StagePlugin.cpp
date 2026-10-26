#include "PCH.h"
#include "Core.h"

#include "CoSave.h"
#include "Config.h"
#include "DataStore.h"
#include "DebugHarness.h"
#include "InputManager.h"
#include "LootService.h"
#include "MenuUI.h"
#include "PackManager.h"
#include "PapyrusBridge.h"

using namespace Runefall;

namespace
{
	std::string StageLogPath()
	{
		auto logs = logger::log_directory();
		if (!logs)
			return {};
		const auto* plugin = SKSE::PluginDeclaration::GetSingleton();
		const auto name = plugin ? std::string{ plugin->GetName() } + ".log" : std::string{ "RunefallStage.log" };
		return (*logs / name).string();
	}

	void SetupLog()
	{
		auto logsFolder = logger::log_directory();
		if (!logsFolder)
			util::report_and_fail("SKSE log_directory not provided, logs can't be written");
		const auto path = StageLogPath();
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true);
		auto logger = std::make_shared<spdlog::logger>("stage", sink);
		spdlog::set_default_logger(std::move(logger));
		spdlog::set_pattern("[%H:%M:%S.%e] [%l] [%s:%#] %v");
		spdlog::set_level(spdlog::level::info);
		spdlog::flush_on(spdlog::level::info);
	}

	int ReadStageLayer()
	{
		const auto path = RunefallDataDir().parent_path() / "stage.ini";
		std::ifstream file{ path };
		if (!file.is_open())
			return 0;
		std::string line;
		while (std::getline(file, line)) {
			const auto eq = line.find('=');
			if (eq == std::string::npos)
				continue;
			if (Trim(line.substr(0, eq)) == "iStageLayer")
				return std::atoi(std::string{ Trim(line.substr(eq + 1)) }.c_str());
		}
		return 0;
	}

	int g_layer = 0;

	void OnDataLoaded()
	{
		logger::info("stage: kDataLoaded (layer {})", g_layer);
		if (g_layer >= 2) {
			const auto result = Runefall::DataStore::Get().LoadAll(RunefallDataDir());
			logger::info("stage: DataStore {}", result.detail);
		}
		if (g_layer >= 3) {
			const auto result = Runefall::PackManager::Get().Scan();
			logger::info("stage: PackManager {}", result.detail);
		}
		if (g_layer >= 6) {
			Runefall::InputManager::Get().Register();
			Runefall::MenuUI::Register();
		}
		if (g_layer >= 7)
			Runefall::DebugHarness::RunAll();
	}

	void OnMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		if (!a_msg)
			return;
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			OnDataLoaded();
			break;
		case SKSE::MessagingInterface::kNewGame:
		case SKSE::MessagingInterface::kPostLoadGame:
			if (g_layer >= 7)
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
	g_layer = ReadStageLayer();
	logger::info("RunefallStage loaded (layer {})", g_layer);

	if (g_layer >= 1) {
		const auto config = Runefall::Config::Get().Load();
		logger::info("stage: Config {}", config.detail);
	}
	if (g_layer >= 4)
		Runefall::CoSave::Get().Register();
	if (g_layer >= 5) {
		if (const auto* papyrus = SKSE::GetPapyrusInterface(); !papyrus || !papyrus->Register(Runefall::PapyrusBridge::Register)) {
			logger::error("stage: Papyrus interface registration failed");
			return false;
		}
	}
	if (const auto* messaging = SKSE::GetMessagingInterface(); !messaging || !messaging->RegisterListener(OnMessage)) {
		logger::error("stage: messaging listener registration failed");
		return false;
	}
	return true;
}