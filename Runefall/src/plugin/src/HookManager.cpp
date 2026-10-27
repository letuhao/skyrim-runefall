#include "HookManager.h"

#include "RE/S/ScriptEventSourceHolder.h"
#include "RE/T/TESCellFullyLoadedEvent.h"
#include "RE/T/TESContainerChangedEvent.h"
#include "RE/T/TESObjectLoadedEvent.h"
#include "RE/M/MenuOpenCloseEvent.h"
#include "RE/U/UI.h"

#include <atomic>

namespace Runefall
{
	namespace
	{
		std::atomic<std::uint32_t> g_containerEvents{ 0 };
		std::atomic<std::uint32_t> g_menuEvents{ 0 };
		std::atomic<std::uint32_t> g_cellEvents{ 0 };
		std::atomic<std::uint32_t> g_objectEvents{ 0 };
		std::atomic_bool           g_registered{ false };

		class ContainerSink : public RE::BSTEventSink<RE::TESContainerChangedEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* a_event, RE::BSTEventSource<RE::TESContainerChangedEvent>*) override
			{
				if (!a_event)
					return RE::BSEventNotifyControl::kContinue;
				const auto count = g_containerEvents.fetch_add(1) + 1;
				if (count <= 3 || count % 1000 == 0)
					logger::debug("hooks: container change #{} old={:08X} new={:08X} base={:08X} count={}",
						count, a_event->oldContainer, a_event->newContainer, a_event->baseObj, a_event->itemCount);
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		class MenuSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
			{
				if (!a_event)
					return RE::BSEventNotifyControl::kContinue;
				const auto count = g_menuEvents.fetch_add(1) + 1;
				if (count <= 5 || count % 1000 == 0)
					logger::debug("hooks: menu {} '{}' #{}", a_event->opening ? "open" : "close", a_event->menuName.c_str(), count);
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		class CellSink : public RE::BSTEventSink<RE::TESCellFullyLoadedEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESCellFullyLoadedEvent* a_event, RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) override
			{
				if (!a_event)
					return RE::BSEventNotifyControl::kContinue;
				g_cellEvents.fetch_add(1);
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		class ObjectSink : public RE::BSTEventSink<RE::TESObjectLoadedEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESObjectLoadedEvent* a_event, RE::BSTEventSource<RE::TESObjectLoadedEvent>*) override
			{
				if (!a_event)
					return RE::BSEventNotifyControl::kContinue;
				g_objectEvents.fetch_add(1);
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		ContainerSink g_containerSink;
		MenuSink      g_menuSink;
		CellSink      g_cellSink;
		ObjectSink    g_objectSink;
	}

	HookManager& HookManager::Get()
	{
		static HookManager singleton;
		return singleton;
	}

	void HookManager::Register()
	{
		if (g_registered.exchange(true))
			return;
		auto* holder = RE::ScriptEventSourceHolder::GetSingleton();
		if (!holder) {
			logger::error("hooks: ScriptEventSourceHolder unavailable");
			g_registered.store(false);
			return;
		}
		holder->AddEventSink<RE::TESContainerChangedEvent>(&g_containerSink);
		if (auto* ui = RE::UI::GetSingleton())
			ui->AddEventSink<RE::MenuOpenCloseEvent>(&g_menuSink);
		else
			logger::warn("hooks: UI singleton unavailable; menu events disabled");
		holder->AddEventSink<RE::TESCellFullyLoadedEvent>(&g_cellSink);
		holder->AddEventSink<RE::TESObjectLoadedEvent>(&g_objectSink);
		logger::info("hooks: read-only event sinks registered (container/menu/cell/object)");
	}

	std::uint32_t HookManager::ContainerEvents() const noexcept { return g_containerEvents.load(); }
	std::uint32_t HookManager::MenuEvents() const noexcept { return g_menuEvents.load(); }
	std::uint32_t HookManager::CellEvents() const noexcept { return g_cellEvents.load(); }
	std::uint32_t HookManager::ObjectEvents() const noexcept { return g_objectEvents.load(); }
}