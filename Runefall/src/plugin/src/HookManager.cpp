#include "HookManager.h"

#include "LootService.h"

#include "RE/C/Character.h"
#include "RE/C/ContainerMenu.h"
#include "RE/I/InventoryMenu.h"
#include "RE/M/MenuOpenCloseEvent.h"
#include "RE/S/ScriptEventSourceHolder.h"
#include "RE/T/TESCellFullyLoadedEvent.h"
#include "RE/T/TESContainerChangedEvent.h"
#include "RE/T/TESObjectLoadedEvent.h"
#include "RE/T/TESQuestStageEvent.h"
#include "RE/T/TESResetEvent.h"
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
		std::atomic<std::uint32_t> g_playerItemEvents{ 0 };
		std::atomic<std::uint32_t> g_resetEvents{ 0 };
		std::atomic<std::uint32_t> g_questStageEvents{ 0 };
		std::atomic<std::uint32_t> g_load3dRefrEvents{ 0 };
		std::atomic<std::uint32_t> g_load3dCharacterEvents{ 0 };
		std::atomic_bool           g_registered{ false };
		std::atomic_bool           g_load3dInstalled{ false };

		void QueueObject3DLoaded(RE::FormID a_formID)
		{
			if (auto* tasks = SKSE::GetTaskInterface()) {
				tasks->AddTask([a_formID]() { LootService::Get().OnObject3DLoaded(a_formID); });
			}
		}

		class ContainerSink : public RE::BSTEventSink<RE::TESContainerChangedEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* a_event, RE::BSTEventSource<RE::TESContainerChangedEvent>*) override
			{
				if (!a_event)
					return RE::BSEventNotifyControl::kContinue;
				const auto count = g_containerEvents.fetch_add(1) + 1;
				if (a_event->newContainer == 0x14)
					g_playerItemEvents.fetch_add(1);
				if (count <= 3 || count % 1000 == 0)
					logger::debug("hooks: container change #{} old={:08X} new={:08X} base={:08X} count={}",
						count, a_event->oldContainer, a_event->newContainer, a_event->baseObj, a_event->itemCount);
				if (a_event->newContainer == 0x14 && a_event->itemCount > 0 && a_event->baseObj != 0) {
					const auto base = a_event->baseObj;
					const auto itemCount = a_event->itemCount;
					const auto oldContainer = a_event->oldContainer;
					if (auto* tasks = SKSE::GetTaskInterface()) {
						tasks->AddTask([base, itemCount, oldContainer]() {
							LootService::Get().OnItemAddedToPlayer(base, itemCount, oldContainer);
						});
					}
				}
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
				if (!a_event->opening)
					return RE::BSEventNotifyControl::kContinue;
				if (a_event->menuName == RE::ContainerMenu::MENU_NAME) {
					if (auto* tasks = SKSE::GetTaskInterface())
						tasks->AddTask([]() { LootService::Get().OnContainerMenuOpened(); });
				} else if (a_event->menuName == RE::InventoryMenu::MENU_NAME) {
					if (auto* tasks = SKSE::GetTaskInterface())
						tasks->AddTask([]() { LootService::Get().OnInventoryMenuOpened(); });
				}
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
				const auto cellID = a_event->cell ? a_event->cell->GetFormID() : 0;
				if (cellID == 0)
					return RE::BSEventNotifyControl::kContinue;
				if (auto* tasks = SKSE::GetTaskInterface())
					tasks->AddTask([cellID]() { LootService::Get().OnCellFullyLoaded(cellID); });
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
				if (a_event->loaded)
					QueueObject3DLoaded(a_event->formID);
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		class ResetSink : public RE::BSTEventSink<RE::TESResetEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESResetEvent* a_event, RE::BSTEventSource<RE::TESResetEvent>*) override
			{
				if (!a_event)
					return RE::BSEventNotifyControl::kContinue;
				g_resetEvents.fetch_add(1);
				const auto formID = a_event->object ? a_event->object->GetFormID() : 0;
				if (formID == 0)
					return RE::BSEventNotifyControl::kContinue;
				if (auto* tasks = SKSE::GetTaskInterface())
					tasks->AddTask([formID]() { LootService::Get().OnResetRef(formID); });
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		class QuestStageSink : public RE::BSTEventSink<RE::TESQuestStageEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESQuestStageEvent* a_event, RE::BSTEventSource<RE::TESQuestStageEvent>*) override
			{
				if (!a_event)
					return RE::BSEventNotifyControl::kContinue;
				g_questStageEvents.fetch_add(1);
				const auto formID = a_event->formID;
				const auto stage = a_event->stage;
				if (formID == 0)
					return RE::BSEventNotifyControl::kContinue;
				if (auto* tasks = SKSE::GetTaskInterface())
					tasks->AddTask([formID, stage]() { LootService::Get().OnQuestStage(formID, stage); });
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		ContainerSink   g_containerSink;
		MenuSink        g_menuSink;
		CellSink        g_cellSink;
		ObjectSink      g_objectSink;
		ResetSink       g_resetSink;
		QuestStageSink  g_questSink;

		using Load3DRefrFn = RE::NiAVObject* (*)(RE::TESObjectREFR*, bool);
		using Load3DCharacterFn = RE::NiAVObject* (*)(RE::Character*, bool);
		Load3DRefrFn      g_originalRefrLoad3D = nullptr;
		Load3DCharacterFn g_originalCharacterLoad3D = nullptr;

		RE::NiAVObject* HookRefrLoad3D(RE::TESObjectREFR* a_this, bool a_backgroundLoading)
		{
			RE::NiAVObject* result = nullptr;
			if (a_this && g_originalRefrLoad3D)
				result = g_originalRefrLoad3D(a_this, a_backgroundLoading);
			if (!a_this || a_this->IsPlayerRef())
				return result;
			const auto formID = a_this->GetFormID();
			if (!formID)
				return result;
			auto* base = a_this->GetBaseObject();
			if (!base || !base->As<RE::TESObjectCONT>())
				return result;
			g_load3dRefrEvents.fetch_add(1);
			QueueObject3DLoaded(formID);
			return result;
		}

		RE::NiAVObject* HookCharacterLoad3D(RE::Character* a_this, bool a_backgroundLoading)
		{
			RE::NiAVObject* result = nullptr;
			if (a_this && g_originalCharacterLoad3D)
				result = g_originalCharacterLoad3D(a_this, a_backgroundLoading);
			if (!a_this || a_this->IsPlayerRef())
				return result;
			const auto formID = a_this->GetFormID();
			if (!formID)
				return result;
			g_load3dCharacterEvents.fetch_add(1);
			QueueObject3DLoaded(formID);
			return result;
		}

		bool InstallLoad3DHooks()
		{
			static REL::Relocation<std::uintptr_t> refrVtable{ RE::TESObjectREFR::VTABLE[0] };
			const auto refrOriginal = refrVtable.write_vfunc(0x6A, reinterpret_cast<std::uintptr_t>(&HookRefrLoad3D));
			if (!refrOriginal) {
				logger::error("hooks: TESObjectREFR::Load3D vtable write returned null; container load path disabled");
				return false;
			}
			g_originalRefrLoad3D = reinterpret_cast<Load3DRefrFn>(refrOriginal);

			static REL::Relocation<std::uintptr_t> characterVtable{ RE::Character::VTABLE[0] };
			const auto characterOriginal = characterVtable.write_vfunc(0x6A, reinterpret_cast<std::uintptr_t>(&HookCharacterLoad3D));
			if (!characterOriginal) {
				logger::error("hooks: Character::Load3D vtable write returned null; actor load path disabled");
				g_originalRefrLoad3D = nullptr;
				return false;
			}
			g_originalCharacterLoad3D = reinterpret_cast<Load3DCharacterFn>(characterOriginal);
			logger::info("hooks: Load3D vtable hooks installed (TESObjectREFR + Character, slot 0x6A)");
			return true;
		}
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
		holder->AddEventSink<RE::TESResetEvent>(&g_resetSink);
		holder->AddEventSink<RE::TESQuestStageEvent>(&g_questSink);
		g_load3dInstalled.store(InstallLoad3DHooks());
		logger::info("hooks: event sinks registered (container/menu/cell/object/reset/quest)");
	}

	std::uint32_t HookManager::ContainerEvents() const noexcept { return g_containerEvents.load(); }
	std::uint32_t HookManager::MenuEvents() const noexcept { return g_menuEvents.load(); }
	std::uint32_t HookManager::CellEvents() const noexcept { return g_cellEvents.load(); }
	std::uint32_t HookManager::ObjectEvents() const noexcept { return g_objectEvents.load(); }
	std::uint32_t HookManager::PlayerItemEvents() const noexcept { return g_playerItemEvents.load(); }
	std::uint32_t HookManager::ResetEvents() const noexcept { return g_resetEvents.load(); }
	std::uint32_t HookManager::QuestStageEvents() const noexcept { return g_questStageEvents.load(); }
	std::uint32_t HookManager::Load3DRefrEvents() const noexcept { return g_load3dRefrEvents.load(); }
	std::uint32_t HookManager::Load3DCharacterEvents() const noexcept { return g_load3dCharacterEvents.load(); }
	bool HookManager::Load3DHooksInstalled() const noexcept { return g_load3dInstalled.load(); }
}
