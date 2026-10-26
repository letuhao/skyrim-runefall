#include "InputManager.h"

#include "DebugHarness.h"
#include "LootService.h"
#include "MenuApi.h"

namespace Runefall
{
	namespace
	{
		// DirectInput scan codes for the default debug/UI hotkeys.
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
					if (code == kKeyF10) {
						const auto snapshot = MenuApi::BuildSnapshotJson();
						logger::info("input: menu snapshot {}", snapshot);
						if (auto* console = RE::ConsoleLog::GetSingleton())
							console->Print("[Runefall] %s", snapshot.c_str());
					} else if (code == kKeyF11) {
						DebugHarness::RunAll();
						if (auto* console = RE::ConsoleLog::GetSingleton())
							console->Print("[Runefall] probe rerun: %s", DebugHarness::LastSummary().c_str());
					} else if (code == kKeyF12) {
						const bool ok = LootService::Get().InjectProbeItem();
						if (auto* console = RE::ConsoleLog::GetSingleton())
							console->Print("[Runefall] probe item: %s", ok ? "injected" : "skipped");
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
		logger::info("input: hotkeys registered (F10 snapshot, F11 probe, F12 probe item)");
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
