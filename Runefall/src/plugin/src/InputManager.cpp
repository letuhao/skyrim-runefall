#include "InputManager.h"

#include "DebugHarness.h"
#include "LootService.h"
#include "MenuApi.h"
#include "RuntimeContentResolver.h"
#include "EncounterManager.h"
#include "HuntContractManager.h"
#include "AnomalyEngine.h"
#include "ClassRegistry.h"

namespace Runefall
{
	namespace
	{
		// DirectInput scan codes for the default debug/UI hotkeys.
		constexpr std::uint32_t kKeyF2 = 0x3C;
		constexpr std::uint32_t kKeyF3 = 0x3D;
		constexpr std::uint32_t kKeyF4 = 0x3E;
		constexpr std::uint32_t kKeyF5 = 0x3F;
		constexpr std::uint32_t kKeyF6 = 0x40;
		constexpr std::uint32_t kKeyF7 = 0x41;
		constexpr std::uint32_t kKeyF8 = 0x42;
		constexpr std::uint32_t kKeyF9 = 0x43;
		constexpr std::uint32_t kKeyF10 = 0x44;
		constexpr std::uint32_t kKeyF11 = 0x57;
		constexpr std::uint32_t kKeyF12 = 0x58;

		class InputSink : public RE::BSTEventSink<RE::InputEvent*>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) override
			{
				if (!a_event)
					return RE::BSEventNotifyControl::kContinue;
				for (auto* event = *a_event; event; event = event->next) {
					const auto* button = event->AsButtonEvent();
					if (!button || !button->IsDown())
						continue;
					if (button->GetDevice() != RE::INPUT_DEVICE::kKeyboard)
						continue;
					const auto code = button->GetIDCode();
					if (auto* tasks = SKSE::GetTaskInterface()) {
						if (code == kKeyF2) {
							tasks->AddTask([]() {
								ClassRegistry::Get().ApplyClass("warrior");
								logger::info("input: class choice requested");
							});
						} else if (code == kKeyF3) {
							tasks->AddTask([]() {
								AnomalyEngine::Get().DebugSpawn();
								logger::info("input: anomaly spawn requested");
							});
						} else if (code == kKeyF4) {
							tasks->AddTask([]() {
								HuntContractManager::Get().DebugStartFirst();
								logger::info("input: hunt contract start requested");
							});
						} else if (code == kKeyF5) {
							tasks->AddTask([]() {
								EncounterManager::Get().DebugSpawn();
								logger::info("input: encounter debug spawn requested");
							});
						} else if (code == kKeyF6) {
							tasks->AddTask([]() {
								RuntimeContentResolver::Get().Rescan();
								logger::info("input: runtime content rescan requested");
								if (auto* console = RE::ConsoleLog::GetSingleton())
									console->Print("[Runefall] runtime content cache rescanned");
							});
						} else if (code == kKeyF7) {
							tasks->AddTask([]() {
								const auto diagnostics = MenuApi::BuildDiagnosticsJson();
								logger::info("input: diagnostics {}", diagnostics);
								if (auto* console = RE::ConsoleLog::GetSingleton())
									console->Print("[Runefall] diagnostics: %s", diagnostics.c_str());
							});
						} else if (code == kKeyF8) {
							tasks->AddTask([]() {
								const bool ok = LootService::Get().DebugGiveRuneItem(3);
								if (auto* console = RE::ConsoleLog::GetSingleton())
									console->Print("[Runefall] debug rune item: %s", ok ? "queued" : "skipped");
							});
						} else if (code == kKeyF9) {
							tasks->AddTask([]() {
								const bool ok = LootService::Get().GrantQuestReward(4, "debug");
								if (auto* console = RE::ConsoleLog::GetSingleton())
									console->Print("[Runefall] debug quest reward: %s", ok ? "granted" : "skipped");
							});
						} else if (code == kKeyF10) {
							tasks->AddTask([]() {
								const auto snapshot = MenuApi::BuildSnapshotJson();
								logger::info("input: menu snapshot {}", snapshot);
								if (auto* console = RE::ConsoleLog::GetSingleton())
									console->Print("[Runefall] %s", snapshot.c_str());
							});
						} else if (code == kKeyF11) {
							tasks->AddTask([]() {
								DebugHarness::RunAll();
								if (auto* console = RE::ConsoleLog::GetSingleton())
									console->Print("[Runefall] probe rerun: %s", DebugHarness::LastSummary().c_str());
							});
						} else if (code == kKeyF12) {
							tasks->AddTask([]() {
								const bool ok = LootService::Get().InjectProbeItem();
								if (auto* console = RE::ConsoleLog::GetSingleton())
									console->Print("[Runefall] probe item: %s", ok ? "injected" : "skipped");
							});
						}
					}
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		InputSink g_inputSink;
	}

	InputManager& InputManager::Get()
	{
		static InputManager singleton;
		return singleton;
	}

	void InputManager::Register()
	{
		if (registered)
			return;
		auto* deviceManager = RE::BSInputDeviceManager::GetSingleton();
		if (!deviceManager) {
			logger::warn("input: BSInputDeviceManager unavailable; hotkeys disabled");
			return;
		}
		deviceManager->AddEventSink<RE::InputEvent*>(&g_inputSink);
		registered = true;
		logger::info("input: hotkeys registered (F2 class, F3 anomaly, F4 hunt contract, F5 encounter spawn, F6 runtime rescan, F7 diagnostics, F8 rune debug, F9 quest debug, F10 snapshot, F11 probe, F12 probe item)");
	}

	SelfTestResult InputManager::SelfTest()
	{
		if (!RE::BSInputDeviceManager::GetSingleton())
			return { "InputManager.SelfTest", CheckStatus::Warn, "input device manager unavailable" };
		if (!registered)
			return { "InputManager.SelfTest", CheckStatus::Warn, "hotkeys not registered yet" };
		return { "InputManager.SelfTest", CheckStatus::Pass, "F10/F11/F12 hotkeys registered" };
	}
}