#include "ActorEquipmentDistributor.h"

#include "Config.h"
#include "DataStore.h"
#include "FormCatalog.h"
#include "FormResolver.h"

#include "RE/A/Actor.h"
#include "RE/A/ActorEquipManager.h"
#include "RE/B/BGSPerk.h"
#include "RE/T/TESBoundObject.h"
#include "RE/T/TESDataHandler.h"
#include "RE/T/TESObjectARMO.h"
#include "RE/T/TESObjectWEAP.h"

#include <algorithm>
#include <format>
#include <sstream>

namespace Runefall
{
	namespace
	{
		int FindColumn(const CsvTable& a_table, std::string_view a_name)
		{
			for (std::size_t i = 0; i < a_table.headers.size(); ++i)
				if (a_table.headers[i] == a_name)
					return static_cast<int>(i);
			return -1;
		}

		std::string Cell(const std::vector<std::string>& a_row, int a_column)
		{
			if (a_column < 0 || static_cast<std::size_t>(a_column) >= a_row.size())
				return {};
			return a_row[static_cast<std::size_t>(a_column)];
		}

		std::vector<std::string> Split(const std::string& a_text)
		{
			std::vector<std::string> values;
			std::stringstream stream{ a_text };
			std::string value;
			while (std::getline(stream, value, '|'))
				if (!value.empty())
					values.push_back(value);
			return values;
		}

		int Int(const std::vector<std::string>& a_row, int a_column, int a_default = 0)
		{
			const auto text = Cell(a_row, a_column);
			if (text.empty())
				return a_default;
			return static_cast<int>(std::strtol(text.c_str(), nullptr, 10));
		}

		RE::TESBoundObject* ResolveItem(const std::string& a_key)
		{
			if (const auto separator = a_key.find(':'); separator != std::string::npos) {
				const auto plugin = a_key.substr(0, separator);
				const auto local = std::strtoul(a_key.substr(separator + 1).c_str(), nullptr, 0);
				auto* dataHandler = RE::TESDataHandler::GetSingleton();
				if (!dataHandler)
					return nullptr;
				auto* form = dataHandler->LookupForm(static_cast<RE::FormID>(local), plugin);
				if (!form)
					return nullptr;
				switch (form->GetFormType()) {
				case RE::FormType::Weapon: return form->As<RE::TESObjectWEAP>();
				case RE::FormType::Armor: return form->As<RE::TESObjectARMO>();
				case RE::FormType::Misc: return form->As<RE::TESObjectMISC>();
				default: return nullptr;
				}
			}
			return FormResolver::ResolveAs<RE::TESBoundObject>(a_key);
		}

		bool HasItem(RE::Actor* a_actor, RE::TESBoundObject* a_item)
		{
			if (!a_actor || !a_item)
				return false;
			const auto counts = a_actor->GetInventoryCounts([a_item](RE::TESBoundObject& a_object) { return &a_object == a_item; });
			for (const auto& [object, count] : counts) {
				(void)object;
				if (count > 0)
					return true;
			}
			return false;
		}
	}

	ActorEquipmentDistributor& ActorEquipmentDistributor::Get()
	{
		static ActorEquipmentDistributor singleton;
		return singleton;
	}

	void ActorEquipmentDistributor::Load()
	{
		sets.clear();
		loaded = false;
		const auto* table = DataStore::Get().GetTable("equipment_sets.csv");
		if (!table) {
			logger::error("equipment: equipment_sets.csv missing");
			return;
		}
		const int sId = FindColumn(*table, "setId");
		const int sRole = FindColumn(*table, "roleKey");
		const int sMin = FindColumn(*table, "stageMin");
		const int sMax = FindColumn(*table, "stageMax");
		const int sItems = FindColumn(*table, "itemEditorIds");
		const int sPerks = FindColumn(*table, "perkEditorIds");
		const int sWeight = FindColumn(*table, "weight");
		const int sChance = FindColumn(*table, "applyChance");
		for (const auto& row : table->rows) {
			SetDef def;
			def.setId = Cell(row, sId);
			def.roleKey = Cell(row, sRole);
			def.stageMin = static_cast<std::uint8_t>((std::clamp)(Int(row, sMin, 1), 1, 100));
			def.stageMax = static_cast<std::uint8_t>((std::clamp)(Int(row, sMax, 100), 1, 100));
			def.items = Split(Cell(row, sItems));
			def.perks = Split(Cell(row, sPerks));
			def.weight = std::strtof(Cell(row, sWeight).c_str(), nullptr);
			def.applyChance = std::strtof(Cell(row, sChance).c_str(), nullptr);
			if (!def.setId.empty() && def.stageMin <= def.stageMax)
				sets.push_back(std::move(def));
		}
		std::sort(sets.begin(), sets.end(), [](const SetDef& a, const SetDef& b) { return a.weight > b.weight; });
		loaded = !sets.empty();
		if (loaded)
			logger::info("equipment: loaded {} sets", sets.size());
		else
			logger::error("equipment: no valid sets");
	}

	const ActorEquipmentDistributor::SetDef* ActorEquipmentDistributor::Select(std::string_view a_roleKey, std::uint8_t a_stage) const
	{
		for (const auto& set : sets)
			if (set.roleKey == a_roleKey && a_stage >= set.stageMin && a_stage <= set.stageMax)
				return &set;
		for (const auto& set : sets)
			if (set.roleKey == "any" && a_stage >= set.stageMin && a_stage <= set.stageMax)
				return &set;
		return nullptr;
	}

	std::string ActorEquipmentDistributor::SelectSetId(std::string_view a_roleKey, std::uint8_t a_stage) const
	{
		const auto* set = Select(a_roleKey, a_stage);
		return set ? set->setId : std::string{};
	}

	bool ActorEquipmentDistributor::Apply(RE::Actor* a_actor, std::string_view a_roleKey, std::uint8_t a_stage)
	{
		logger::info("equipment: apply-check role={} stage={} enabled={} loaded={}", a_roleKey, a_stage, Config::Get().EnableActorEquipment(), loaded);
		if (!loaded || !a_actor || a_actor->IsPlayerRef() || !Config::Get().EnableActorEquipment())
			return false;
		const auto* set = Select(a_roleKey, a_stage);
		if (!set)
			return false;
		logger::info("equipment: selected set {} for {:08X}", set->setId, a_actor->GetFormID());
		auto* equipManager = RE::ActorEquipManager::GetSingleton();
		if (!equipManager)
			return false;
		if (set->applyChance < 100.0f) {
			const auto roll = static_cast<float>(a_actor->GetFormID() % 100u);
			if (roll >= set->applyChance)
				return false;
		}
		std::size_t added = 0;
		for (const auto& editorId : set->items) {
			auto* item = ResolveItem(editorId);
			logger::info("equipment: resolve '{}' -> {}", editorId, item ? item->GetFormID() : 0);
			if (!item || HasItem(a_actor, item))
				continue;
			a_actor->AddObjectToContainer(item, nullptr, 1, nullptr);
			equipManager->EquipObject(a_actor, item, nullptr, 1, nullptr, false, false, false, true);
			++added;
		}
		std::size_t perksAdded = 0;
		for (const auto& editorId : set->perks) {
			auto* perk = FormCatalog::Get().LookupAs<RE::BGSPerk>(editorId);
			if (!perk)
				continue;
			if (!a_actor->HasPerk(perk)) {
				a_actor->AddPerk(perk, 1);
				++perksAdded;
			}
		}
		if (added > 0 || perksAdded > 0)
			logger::info("equipment: set {} applied to {:08X} ({} items, {} perks, stage {})", set->setId, a_actor->GetFormID(), added, perksAdded, a_stage);
		return added > 0 || perksAdded > 0;
	}

	SelfTestResult ActorEquipmentDistributor::SelfTest()
	{
		auto& engine = Get();
		if (!engine.Loaded())
			return { "ActorEquipmentDistributor.SelfTest", CheckStatus::Fail, "equipment sets not loaded" };
		if (engine.SetCount() < 5)
			return { "ActorEquipmentDistributor.SelfTest", CheckStatus::Fail, "equipment set coverage too small" };
		const auto humanoid = engine.SelectSetId("rf:role/humanoid", 50);
		const auto fallback = engine.SelectSetId("rf:role/unknown", 50);
		if (humanoid.empty() || fallback.empty())
			return { "ActorEquipmentDistributor.SelfTest", CheckStatus::Fail, "equipment selection failed" };
		return { "ActorEquipmentDistributor.SelfTest", CheckStatus::Pass,
			std::format("{} sets; humanoid stage 50 -> {}, unknown -> {}", engine.SetCount(), humanoid, fallback) };
	}
}
