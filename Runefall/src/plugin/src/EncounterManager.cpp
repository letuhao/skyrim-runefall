#include "EncounterManager.h"

#include "Config.h"
#include "DataStore.h"
#include "FormCatalog.h"
#include "WorldBandEngine.h"
#include "RareSpawnEngine.h"
#include "LootService.h"

#include "RE/P/PlayerCharacter.h"
#include "RE/T/TESNPC.h"

#include <algorithm>
#include <cstdlib>
#include <format>

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
		int Int(const std::vector<std::string>& a_row, int a_column, int a_default = 0)
		{
			const auto text = Cell(a_row, a_column);
			if (text.empty())
				return a_default;
			return static_cast<int>(std::strtol(text.c_str(), nullptr, 10));
		}
		bool Bool(const std::vector<std::string>& a_row, int a_column)
		{
			return Cell(a_row, a_column) == "1";
		}
		float Float(const std::vector<std::string>& a_row, int a_column, float a_default = 0.0f)
		{
			const auto text = Cell(a_row, a_column);
			return text.empty() ? a_default : std::strtof(text.c_str(), nullptr);
		}
		std::uint64_t Hash(std::uint64_t a_seed, std::string_view a_text)
		{
			auto hash = a_seed;
			for (const auto c : a_text) {
				hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
				hash *= 1099511628211ull;
			}
			return hash;
		}
	}

	EncounterManager& EncounterManager::Get()
	{
		static EncounterManager singleton;
		return singleton;
	}

	void EncounterManager::Load()
	{
		entries.clear();
		loaded = false;
		const auto* table = DataStore::Get().GetTable("encounter_tables.csv");
		if (!table) {
			logger::error("encounter: encounter_tables.csv missing");
			return;
		}
		const int cId = FindColumn(*table, "encounterId");
		const int cBand = FindColumn(*table, "bandId");
		const int cClass = FindColumn(*table, "class");
		const int cActor = FindColumn(*table, "actorEditorId");
		const int cMin = FindColumn(*table, "stageMin");
		const int cMax = FindColumn(*table, "stageMax");
		const int cWeight = FindColumn(*table, "weight");
		const int cMaxCell = FindColumn(*table, "maxPerCell");
		const int cCooldown = FindColumn(*table, "cooldownSeconds");
		const int cEnabled = FindColumn(*table, "enabled");
		for (const auto& row : table->rows) {
			Entry entry;
			entry.encounterId = Cell(row, cId);
			entry.bandId = Cell(row, cBand);
			entry.className = Cell(row, cClass);
			entry.actorEditorId = Cell(row, cActor);
			entry.stageMin = static_cast<std::uint8_t>((std::clamp)(Int(row, cMin, 1), 1, 100));
			entry.stageMax = static_cast<std::uint8_t>((std::clamp)(Int(row, cMax, 20), 1, 100));
			entry.weight = Float(row, cWeight);
			entry.maxPerCell = (std::max)(1, Int(row, cMaxCell, 1));
			entry.cooldownSeconds = (std::max)(0, Int(row, cCooldown, 60));
			entry.enabled = Bool(row, cEnabled);
			if (!entry.encounterId.empty() && entry.stageMin <= entry.stageMax)
				entries.push_back(std::move(entry));
		}
		loaded = !entries.empty();
		if (loaded)
			logger::info("encounter: loaded {} entries", entries.size());
		else
			logger::error("encounter: no valid entries");
	}

	const EncounterManager::Entry* EncounterManager::Select(std::string_view a_bandId, std::uint8_t a_stage, std::uint64_t a_seed) const
	{
		std::vector<const Entry*> candidates;
		for (const auto& entry : entries)
			if (entry.enabled && entry.bandId == a_bandId && a_stage >= entry.stageMin && a_stage <= entry.stageMax)
				candidates.push_back(&entry);
		if (candidates.empty())
			return nullptr;
		const auto hash = Hash(a_seed, a_bandId);
		const auto index = static_cast<std::size_t>(hash % candidates.size());
		return candidates[index];
	}

	std::string EncounterManager::SelectEncounterId(std::string_view a_bandId, std::uint8_t a_stage, std::uint64_t a_seed) const
	{
		const auto* entry = Select(a_bandId, a_stage, a_seed);
		return entry ? entry->encounterId : std::string{};
	}

	bool EncounterManager::DebugSpawn()
	{
		if (!loaded || !Config::Get().EnableEncounters())
			return false;
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return false;
		auto* cell = player->GetParentCell();
		if (!cell || !cell->IsExteriorCell())
			return false;
		const auto stage = WorldBandEngine::Get().StageForCell(cell, player->GetFormID());
		if (stage == 0)
			return false;
		const auto seed = static_cast<std::uint64_t>(player->GetFormID()) ^ static_cast<std::uint64_t>(cell->GetFormID());
		RareSpawn rare;
		if (RareSpawnEngine::Get().TryPick("tamriel", seed, rare)) {
			auto* rareBase = FormCatalog::Get().LookupAs<RE::TESNPC>(rare.actorEditorId);
			if (rareBase) {
				const auto placedRare = player->PlaceObjectAtMe(rareBase, true);
				if (placedRare) {
					LootService::Get().SetActorStageOverride(placedRare->GetFormID(), rare.stage);
					// rare spawns use the debug spawn path but still obey one-per-cell semantics
					const auto cellID = cell->GetFormID();
					++cellCounts[cellID];
					cooldowns[cellID] = std::chrono::steady_clock::now() + std::chrono::seconds(60);
					logger::info("rare-spawn: spawned {} ({}) at {:08X} stage {}", rare.rareId, rare.actorEditorId, placedRare->GetFormID(), rare.stage);
					return true;
				}
			}
		}
		const auto* entry = Select("tamriel", stage, seed);
		if (!entry)
			return false;
		const auto cellID = cell->GetFormID();
		if (const auto it = cooldowns.find(cellID); it != cooldowns.end() && std::chrono::steady_clock::now() < it->second) {
			logger::info("encounter: cell {:08X} cooldown active", cellID);
			return false;
		}
		if (cellCounts[cellID] >= entry->maxPerCell) {
			logger::info("encounter: cell {:08X} cap reached ({})", cellID, entry->maxPerCell);
			return false;
		}
		auto* actorBase = FormCatalog::Get().LookupAs<RE::TESNPC>(entry->actorEditorId);
		if (!actorBase) {
			logger::warn("encounter: actor base {} unresolved", entry->actorEditorId);
			return false;
		}
		const auto placed = player->PlaceObjectAtMe(actorBase, true);
		if (!placed) {
			logger::warn("encounter: PlaceObjectAtMe failed for {}", entry->encounterId);
			return false;
		}
		++cellCounts[cellID];
		cooldowns[cellID] = std::chrono::steady_clock::now() + std::chrono::seconds(entry->cooldownSeconds);
		logger::info("encounter: spawned {} ({}) at {:08X} stage {}", entry->encounterId, entry->actorEditorId, placed->GetFormID(), stage);
		return true;
	}

	SelfTestResult EncounterManager::SelfTest()
	{
		auto& manager = Get();
		if (!manager.Loaded())
			return { "EncounterManager.SelfTest", CheckStatus::Fail, "encounter tables not loaded" };
		if (manager.TableCount() < 1)
			return { "EncounterManager.SelfTest", CheckStatus::Fail, "no encounters" };
		const auto first = manager.SelectEncounterId("tamriel", 10, 42);
		const auto second = manager.SelectEncounterId("tamriel", 10, 42);
		if (first.empty() || first != second)
			return { "EncounterManager.SelfTest", CheckStatus::Fail, "encounter selection is not deterministic" };
		return { "EncounterManager.SelfTest", CheckStatus::Pass,
			std::format("{} entries; tamriel stage 10 seed 42 -> {}", manager.TableCount(), first) };
	}
}
