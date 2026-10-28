#include "AnomalyEngine.h"

#include "DataStore.h"

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
		int Int(const std::vector<std::string>& a_row, int a_column, int a_default = 0)
		{
			const auto text = Cell(a_row, a_column);
			return text.empty() ? a_default : static_cast<int>(std::strtol(text.c_str(), nullptr, 10));
		}
		bool Bool(const std::vector<std::string>& a_row, int a_column)
		{
			return Cell(a_row, a_column) == "1";
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
		std::uint64_t Fnv(std::string_view a_text)
		{
			std::uint64_t hash = 1469598103934665603ull;
			for (const auto c : a_text) {
				hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
				hash *= 1099511628211ull;
			}
			return hash;
		}
	}

	AnomalyEngine& AnomalyEngine::Get()
	{
		static AnomalyEngine singleton;
		return singleton;
	}

	void AnomalyEngine::Load()
	{
		families.clear();
		loaded = false;
		const auto* table = DataStore::Get().GetTable("anomaly_families.csv");
		if (!table) {
			logger::error("anomaly: anomaly_families.csv missing");
			return;
		}
		const int fName = FindColumn(*table, "family");
		const int fKey = FindColumn(*table, "nameKey");
		const int fOffset = FindColumn(*table, "stageOffset");
		const int fLabels = FindColumn(*table, "tierLabels");
		const int fCore = FindColumn(*table, "coreItemEditorId");
		const int fTrophy = FindColumn(*table, "trophyItemEditorId");
		const int fActor = FindColumn(*table, "encounterActorEditorId");
		const int fEnabled = FindColumn(*table, "enabled");
		for (const auto& row : table->rows) {
			Family fam;
			fam.family = Cell(row, fName);
			fam.nameKey = Cell(row, fKey);
			fam.stageOffset = static_cast<std::uint8_t>((std::clamp)(Int(row, fOffset), 0, 50));
			fam.tierLabels = Split(Cell(row, fLabels));
			fam.coreItem = Cell(row, fCore);
			fam.trophyItem = Cell(row, fTrophy);
			fam.encounterActor = Cell(row, fActor);
			fam.enabled = Bool(row, fEnabled);
			if (!fam.family.empty() && fam.enabled)
				families.push_back(std::move(fam));
		}
		loaded = !families.empty();
		if (loaded)
			logger::info("anomaly: loaded {} families", families.size());
	}

	const AnomalyEngine::Family* AnomalyEngine::Find(std::string_view a_family) const
	{
		for (const auto& fam : families)
			if (fam.family == a_family)
				return &fam;
		return nullptr;
	}

	const std::string& AnomalyEngine::FamilyForSeed(std::uint64_t a_seed) const
	{
		static const std::string empty;
		if (families.empty())
			return empty;
		return families[Fnv(std::to_string(a_seed)) % families.size()].family;
	}

	std::uint8_t AnomalyEngine::AdjustStage(std::uint8_t a_baseStage, std::string_view a_family) const
	{
		const auto* fam = Find(a_family);
		if (!fam)
			return a_baseStage;
		return static_cast<std::uint8_t>((std::clamp)(static_cast<int>(a_baseStage) + fam->stageOffset, 1, 100));
	}

	std::string AnomalyEngine::TierLabel(std::uint8_t a_stage, std::string_view a_family) const
	{
		const auto* fam = Find(a_family);
		if (!fam || fam->tierLabels.empty())
			return {};
		const auto tier = (std::min)(fam->tierLabels.size(), static_cast<std::size_t>((a_stage - 1) / 34 + 1));
		return fam->tierLabels[tier - 1];
	}

	std::string AnomalyEngine::CoreItem(std::string_view a_family) const
	{
		const auto* fam = Find(a_family);
		return fam ? fam->coreItem : std::string{};
	}

	std::string AnomalyEngine::EncounterActor(std::string_view a_family) const
	{
		const auto* fam = Find(a_family);
		return fam ? fam->encounterActor : std::string{};
	}

	SelfTestResult AnomalyEngine::SelfTest()
	{
		auto& engine = Get();
		if (!engine.Loaded())
			return { "AnomalyEngine.SelfTest", CheckStatus::Fail, "anomaly families not loaded" };
		if (engine.FamilyCount() < 3)
			return { "AnomalyEngine.SelfTest", CheckStatus::Fail, "anomaly family coverage too small" };
		const auto& first = engine.FamilyForSeed(42);
		const auto& second = engine.FamilyForSeed(42);
		if (first != second || first.empty())
			return { "AnomalyEngine.SelfTest", CheckStatus::Fail, "anomaly family selection is not deterministic" };
		const auto stage = engine.AdjustStage(10, first);
		if (stage <= 10 || engine.TierLabel(stage, first).empty() || engine.CoreItem(first).empty())
			return { "AnomalyEngine.SelfTest", CheckStatus::Fail, "anomaly stage/tier/core data is invalid" };
		return { "AnomalyEngine.SelfTest", CheckStatus::Pass,
			std::format("{} families; seed 42 -> {} stage {} tier {}", engine.FamilyCount(), first, static_cast<int>(stage), engine.TierLabel(stage, first)) };
	}
}
