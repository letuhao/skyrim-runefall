#include "HuntEngine.h"

#include "CoSave.h"
#include "DataStore.h"

namespace Runefall
{
	namespace
	{
		struct HuntRestore
		{
			std::unordered_map<std::string, std::uint8_t>* map = nullptr;
			std::unordered_map<std::string, std::uint8_t>  saved;
			~HuntRestore() { if (map) *map = std::move(saved); }
		};
	}

	namespace
	{
		int FindColumn(const CsvTable& a_table, std::string_view a_name)
		{
			for (std::size_t i = 0; i < a_table.headers.size(); ++i) {
				if (a_table.headers[i] == a_name)
					return static_cast<int>(i);
			}
			return -1;
		}

		std::string Cell(const std::vector<std::string>& a_row, int a_column)
		{
			if (a_column < 0 || static_cast<std::size_t>(a_column) >= a_row.size())
				return {};
			return a_row[static_cast<std::size_t>(a_column)];
		}

		float CellFloat(const std::vector<std::string>& a_row, int a_column, float a_default = 0.0f)
		{
			const auto text = Cell(a_row, a_column);
			if (text.empty())
				return a_default;
			char* end = nullptr;
			const auto value = std::strtof(text.c_str(), &end);
			return end == text.c_str() ? a_default : value;
		}

		std::int32_t CellInt(const std::vector<std::string>& a_row, int a_column, std::int32_t a_default = 0)
		{
			const auto text = Cell(a_row, a_column);
			if (text.empty())
				return a_default;
			char* end = nullptr;
			const auto value = std::strtol(text.c_str(), &end, 10);
			return end == text.c_str() ? a_default : static_cast<std::int32_t>(value);
		}
	}

	HuntEngine& HuntEngine::Get()
	{
		static HuntEngine singleton;
		return singleton;
	}

	void HuntEngine::Reload()
	{
		groups.clear();
		rewards.clear();
		loaded = false;

		const auto* groupTable = DataStore::Get().GetTable("race_groups.csv");
		const auto* rewardTable = DataStore::Get().GetTable("drops_races.csv");
		if (!groupTable || !rewardTable)
			return;

		const int gName = FindColumn(*groupTable, "raceGroup");
		const int gRaces = FindColumn(*groupTable, "memberRaceEditorIds");
		const int gTier = FindColumn(*groupTable, "minRfTier");
		const int gUnique = FindColumn(*groupTable, "uniqueAffixId");
		const int gMaterial = FindColumn(*groupTable, "materialId");
		for (const auto& row : groupTable->rows) {
			RaceGroupDef def;
			def.group = Cell(row, gName);
			for (const auto& race : Split(Cell(row, gRaces), '|'))
				if (!race.empty())
					def.memberRaces.push_back(race);
			def.minRfTier = CellInt(row, gTier, 1);
			def.uniqueAffixId = Cell(row, gUnique);
			def.materialId = Cell(row, gMaterial);
			if (!def.group.empty() && !def.memberRaces.empty())
				groups.push_back(std::move(def));
		}

		const int rGroup = FindColumn(*rewardTable, "raceGroup");
		const int rType = FindColumn(*rewardTable, "rewardType");
		const int rId = FindColumn(*rewardTable, "rewardId");
		const int rChance = FindColumn(*rewardTable, "dropChancePct");
		const int rFirst = FindColumn(*rewardTable, "firstOnly");
		const int rNote = FindColumn(*rewardTable, "note");
		for (const auto& row : rewardTable->rows) {
			RaceRewardDef def;
			def.group = Cell(row, rGroup);
			def.rewardType = Cell(row, rType);
			def.rewardId = Cell(row, rId);
			def.dropChancePct = CellFloat(row, rChance);
			def.firstOnly = CellInt(row, rFirst) != 0;
			def.note = Cell(row, rNote);
			if (!def.group.empty() && !def.rewardId.empty())
				rewards.push_back(std::move(def));
		}

		loaded = !groups.empty() && !rewards.empty();
	}

	const RaceGroupDef* HuntEngine::FindFamilyForRace(std::string_view a_raceEditorId) const
	{
		for (const auto& group : groups) {
			for (const auto& race : group.memberRaces) {
				if (race == a_raceEditorId)
					return &group;
			}
		}
		return nullptr;
	}

	const RaceGroupDef* HuntEngine::FindFamily(std::string_view a_group) const
	{
		for (const auto& group : groups) {
			if (group.group == a_group)
				return &group;
		}
		return nullptr;
	}

	const RaceRewardDef* HuntEngine::FindReward(std::string_view a_group, std::string_view a_rewardType) const
	{
		for (const auto& reward : rewards) {
			if (reward.group == a_group && reward.rewardType == a_rewardType)
				return &reward;
		}
		return nullptr;
	}

	bool HuntEngine::HasFirstKill(std::string_view a_group) const
	{
		const auto& state = CoSave::Get().State();
		const auto it = state.huntFirstKill.find(std::string{ a_group });
		return it != state.huntFirstKill.end() && it->second != 0;
	}

	bool HuntEngine::MarkFirstKill(std::string_view a_group)
	{
		if (!FindFamily(a_group))
			return false;
		auto& state = CoSave::Get().State();
		state.huntFirstKill[std::string{ a_group }] = 1;
		return true;
	}

	SelfTestResult HuntEngine::SelfTest()
	{
		HuntEngine engine;
		engine.groups.push_back({ "Draugr", { "DraugrRace", "SkeletonArmorRace" }, 1, "rf_hunt_unique_draugr", "RF_MISC_Mat_Hunt_Draugr" });
		engine.groups.push_back({ "Dragon", { "DragonRace" }, 4, "rf_hunt_unique_dragon", "RF_MISC_Mat_Hunt_Dragon" });
		engine.rewards.push_back({ "Draugr", "unique_affix", "rf_hunt_unique_draugr", 100.0f, true, "first kill only" });
		engine.rewards.push_back({ "Draugr", "material", "RF_MISC_Mat_Hunt_Draugr", 15.0f, false, "" });
		engine.loaded = true;

		const auto* draugr = engine.FindFamilyForRace("SkeletonArmorRace");
		if (!draugr || draugr->group != "Draugr")
			return { "HuntEngine.SelfTest", CheckStatus::Fail, "race family lookup failed" };
		if (engine.FindFamilyForRace("DragonRace") == nullptr)
			return { "HuntEngine.SelfTest", CheckStatus::Fail, "dragon race lookup failed" };
		if (engine.FindFamilyForRace("NotARace") != nullptr)
			return { "HuntEngine.SelfTest", CheckStatus::Fail, "unknown race matched a family" };
		const auto* unique = engine.FindReward("Draugr", "unique_affix");
		if (!unique || !unique->firstOnly || unique->rewardId != "rf_hunt_unique_draugr")
			return { "HuntEngine.SelfTest", CheckStatus::Fail, "unique reward lookup failed" };
		const auto* material = engine.FindReward("Draugr", "material");
		if (!material || material->firstOnly)
			return { "HuntEngine.SelfTest", CheckStatus::Fail, "material reward lookup failed" };

		auto& firstKill = CoSave::Get().State().huntFirstKill;
		HuntRestore huntRestore{ &firstKill, firstKill };
		firstKill.clear();
		if (engine.HasFirstKill("Draugr") || !engine.MarkFirstKill("Draugr") || !engine.HasFirstKill("Draugr"))
			return { "HuntEngine.SelfTest", CheckStatus::Fail, "first-kill state failed" };
		if (engine.MarkFirstKill("Unknown"))
			return { "HuntEngine.SelfTest", CheckStatus::Fail, "unknown family was marked" };
		return { "HuntEngine.SelfTest", CheckStatus::Pass, "family mapping, rewards, first-kill state" };
	}
}