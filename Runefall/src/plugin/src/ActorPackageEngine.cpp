#include "ActorPackageEngine.h"

#include "DataStore.h"
#include "FormCatalog.h"

#include "RE/A/Actor.h"
#include "RE/B/BGSPerk.h"
#include "RE/S/SpellItem.h"

#include <algorithm>
#include <cstdlib>
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

		float Float(const std::vector<std::string>& a_row, int a_column, float a_default = 0.0f)
		{
			const auto text = Cell(a_row, a_column);
			if (text.empty())
				return a_default;
			return std::strtof(text.c_str(), nullptr);
		}
	}

	ActorPackageEngine& ActorPackageEngine::Get()
	{
		static ActorPackageEngine singleton;
		return singleton;
	}

	void ActorPackageEngine::Load()
	{
		packages.clear();
		loaded = false;
		const auto* table = DataStore::Get().GetTable("enemy_packages.csv");
		if (!table) {
			logger::error("actor-packager: enemy_packages.csv missing");
			return;
		}
		const int pId = FindColumn(*table, "packageId");
		const int pRole = FindColumn(*table, "roleKey");
		const int pMin = FindColumn(*table, "stageMin");
		const int pMax = FindColumn(*table, "stageMax");
		const int pSpells = FindColumn(*table, "spellEditorIds");
		const int pPerks = FindColumn(*table, "perkEditorIds");
		const int pElite = FindColumn(*table, "eliteSpellEditorIds");
		const int pBoss = FindColumn(*table, "bossSpellEditorIds");
		const int pLoot = FindColumn(*table, "lootTable");
		const int pWeight = FindColumn(*table, "weight");
		const int pBudget = FindColumn(*table, "budgetCost");
		for (const auto& row : table->rows) {
			PackageDef def;
			def.packageId = Cell(row, pId);
			def.roleKey = Cell(row, pRole);
			def.stageMin = static_cast<std::uint8_t>((std::clamp)(Int(row, pMin, 1), 1, 100));
			def.stageMax = static_cast<std::uint8_t>((std::clamp)(Int(row, pMax, 100), 1, 100));
			def.spells = Split(Cell(row, pSpells));
			def.perks = Split(Cell(row, pPerks));
			def.eliteSpells = Split(Cell(row, pElite));
			def.bossSpells = Split(Cell(row, pBoss));
			def.lootTable = Cell(row, pLoot);
			def.weight = Float(row, pWeight, 0.0f);
			def.budgetCost = Float(row, pBudget, 0.0f);
			if (!def.packageId.empty() && def.stageMin <= def.stageMax)
				packages.push_back(std::move(def));
		}
		std::sort(packages.begin(), packages.end(), [](const PackageDef& a, const PackageDef& b) {
			return a.weight > b.weight;
		});
		loaded = !packages.empty();
		if (loaded)
			logger::info("actor-packager: loaded {} packages", packages.size());
		else
			logger::error("actor-packager: no valid packages");
	}

	const ActorPackageEngine::PackageDef* ActorPackageEngine::Select(std::string_view a_roleKey, std::uint8_t a_stage) const
	{
		for (const auto& package : packages) {
			if (package.roleKey == a_roleKey && a_stage >= package.stageMin && a_stage <= package.stageMax)
				return &package;
		}
		for (const auto& package : packages) {
			if (package.roleKey == "any" && a_stage >= package.stageMin && a_stage <= package.stageMax)
				return &package;
		}
		return nullptr;
	}

	std::string ActorPackageEngine::SelectPackageId(std::string_view a_roleKey, std::uint8_t a_stage) const
	{
		const auto* package = Select(a_roleKey, a_stage);
		return package ? package->packageId : std::string{};
	}

	bool ActorPackageEngine::Apply(RE::Actor* a_actor, std::string_view a_roleKey, std::uint8_t a_stage, std::int32_t a_rank)
	{
		if (!loaded || !a_actor)
			return false;
		const auto* package = Select(a_roleKey, a_stage);
		if (!package)
			return false;
		bool applied = false;
		for (const auto& editorId : package->spells) {
			auto* spell = FormCatalog::Get().LookupAs<RE::SpellItem>(editorId);
			if (!spell) {
				logger::warn("actor-packager: spell {} unresolved for package {}", editorId, package->packageId);
				continue;
			}
			if (!a_actor->HasSpell(spell) && a_actor->AddSpell(spell)) {
				applied = true;
				logger::info("actor-packager: applied spell {} from {} to {:08X}", editorId, package->packageId, a_actor->GetFormID());
			}
		}
		if (a_rank >= 4) {
			for (const auto& editorId : package->eliteSpells) {
				auto* spell = FormCatalog::Get().LookupAs<RE::SpellItem>(editorId);
				if (spell && !a_actor->HasSpell(spell) && a_actor->AddSpell(spell)) {
					applied = true;
					logger::info("actor-packager: elite overlay {} from {} applied to {:08X}", editorId, package->packageId, a_actor->GetFormID());
				}
			}
		}
		if (a_rank >= 5) {
			for (const auto& editorId : package->bossSpells) {
				auto* spell = FormCatalog::Get().LookupAs<RE::SpellItem>(editorId);
				if (spell && !a_actor->HasSpell(spell) && a_actor->AddSpell(spell)) {
					applied = true;
					logger::info("actor-packager: boss overlay {} from {} applied to {:08X}", editorId, package->packageId, a_actor->GetFormID());
				}
			}
		}
		for (const auto& editorId : package->perks) {
			auto* perk = FormCatalog::Get().LookupAs<RE::BGSPerk>(editorId);
			if (!perk) {
				logger::warn("actor-packager: perk {} unresolved for package {}", editorId, package->packageId);
				continue;
			}
			a_actor->AddPerk(perk, 1);
			applied = true;
		}
		if (applied)
			logger::info("actor-packager: package {} applied once to {:08X} (stage {})", package->packageId, a_actor->GetFormID(), a_stage);
		return applied;
	}

	SelfTestResult ActorPackageEngine::SelfTest()
	{
		auto& engine = Get();
		if (!engine.Loaded())
			return { "ActorPackageEngine.SelfTest", CheckStatus::Fail, "packages not loaded" };
		if (engine.PackageCount() < 10)
			return { "ActorPackageEngine.SelfTest", CheckStatus::Fail, "package coverage too small" };
		for (const auto& package : engine.packages)
			if (package.budgetCost < 0.0f || package.budgetCost > 100.0f)
				return { "ActorPackageEngine.SelfTest", CheckStatus::Fail, "package budget out of range" };
		const auto humanoid = engine.SelectPackageId("rf:role/humanoid", 50);
		const auto fallback = engine.SelectPackageId("rf:role/unknown", 50);
		if (humanoid.empty() || fallback.empty())
			return { "ActorPackageEngine.SelfTest", CheckStatus::Fail, "package selection failed" };
		if (engine.SelectPackageId("rf:role/humanoid", 50) != humanoid)
			return { "ActorPackageEngine.SelfTest", CheckStatus::Fail, "package selection is not deterministic" };
		return { "ActorPackageEngine.SelfTest", CheckStatus::Pass,
			std::format("{} packages; humanoid stage 50 -> {}, unknown -> {}", engine.PackageCount(), humanoid, fallback) };
	}
}
