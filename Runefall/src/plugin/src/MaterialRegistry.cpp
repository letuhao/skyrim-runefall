#include "MaterialRegistry.h"

#include "CoSave.h"
#include "DataStore.h"

namespace Runefall
{
	namespace
	{
		struct MaterialRestore
		{
			std::unordered_map<std::string, std::int32_t>* map = nullptr;
			std::unordered_map<std::string, std::int32_t>  saved;
			~MaterialRestore() { if (map) *map = std::move(saved); }
		};
	}

	MaterialRegistry& MaterialRegistry::Get()
	{
		static MaterialRegistry singleton;
		return singleton;
	}

	void MaterialRegistry::LoadDefinitions()
	{
		definitions.clear();
		const auto* table = DataStore::Get().GetTable("materials.csv");
		if (!table)
			return;
		auto column = [&](std::string_view name) {
			for (std::size_t i = 0; i < table->headers.size(); ++i)
				if (table->headers[i] == name)
					return static_cast<int>(i);
			return -1;
		};
		const int cId = column("materialId");
		const int cName = column("name");
		const int cTier = column("tier");
		const int cFamily = column("family");
		const int cItem = column("itemEditorId");
		for (const auto& row : table->rows) {
			auto cell = [&](int c) -> std::string {
				return c >= 0 && static_cast<std::size_t>(c) < row.size() ? row[static_cast<std::size_t>(c)] : std::string{};
			};
			MaterialDef def;
			def.id = cell(cId);
			def.name = cell(cName);
			def.family = cell(cFamily);
			def.itemEditorId = cell(cItem);
			def.tier = std::atoi(cell(cTier).c_str());
			if (!def.id.empty())
				definitions.push_back(std::move(def));
		}
		logger::info("materials: loaded {} definitions", definitions.size());
	}

	void MaterialRegistry::LoadFromCoSave()
	{
		auto& map = CoSave::Get().State().materials;
		map.clear();
		for (const auto& def : definitions)
			map.try_emplace(def.id, 0);
	}

	void MaterialRegistry::SaveToCoSave()
	{
		// The co-save already points at State().materials; no copy is needed.
	}

	bool MaterialRegistry::Has(std::string_view a_id) const
	{
		for (const auto& def : definitions)
			if (def.id == a_id)
				return true;
		return false;
	}

	std::int32_t MaterialRegistry::Count(std::string_view a_id) const
	{
		const auto& map = CoSave::Get().State().materials;
		const auto it = map.find(std::string{ a_id });
		return it == map.end() ? 0 : it->second;
	}

	std::int32_t MaterialRegistry::Total() const
	{
		std::int32_t total = 0;
		for (const auto& [id, count] : CoSave::Get().State().materials)
			total += count;
		return total;
	}

	bool MaterialRegistry::Add(std::string_view a_id, std::int32_t a_amount)
	{
		if (a_amount <= 0 || !Has(a_id))
			return false;
		auto& map = CoSave::Get().State().materials;
		map[std::string{ a_id }] += a_amount;
		return true;
	}

	bool MaterialRegistry::Consume(std::string_view a_id, std::int32_t a_amount)
	{
		if (a_amount <= 0 || !Has(a_id))
			return false;
		auto& map = CoSave::Get().State().materials;
		auto it = map.find(std::string{ a_id });
		if (it == map.end() || it->second < a_amount)
			return false;
		it->second -= a_amount;
		return true;
	}

	bool MaterialRegistry::AddDefaultForTier(std::int32_t a_tier, std::int32_t a_amount)
	{
		for (const auto& def : definitions) {
			if (def.tier == a_tier)
				return Add(def.id, a_amount);
		}
		return false;
	}

	SelfTestResult MaterialRegistry::SelfTest()
	{
		if (MaterialRegistry::Get().DefinitionCount() == 0)
			return { "MaterialRegistry.SelfTest", CheckStatus::Warn, "materials.csv not loaded" };
		const auto& map = CoSave::Get().State().materials;
		MaterialRestore restore{ const_cast<std::unordered_map<std::string, std::int32_t>*>(&map), map };
		auto& mutableMap = const_cast<std::unordered_map<std::string, std::int32_t>&>(map);
		mutableMap.clear();
		mutableMap["rf_mat_common_ore"] = 2;
		if (!MaterialRegistry::Get().Add("rf_mat_common_ore", 3) || MaterialRegistry::Get().Count("rf_mat_common_ore") != 5)
			return { "MaterialRegistry.SelfTest", CheckStatus::Fail, "material add/count failed" };
		return { "MaterialRegistry.SelfTest", CheckStatus::Pass, "material definitions/add/count" };
	}
}