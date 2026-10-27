#include "StageEngine.h"

#include "DataStore.h"

#include <algorithm>
#include <cmath>
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

		std::int32_t ToInt(const std::vector<std::string>& a_row, int a_column, std::int32_t a_default = 0)
		{
			const auto text = Cell(a_row, a_column);
			if (text.empty())
				return a_default;
			char* end = nullptr;
			const auto value = std::strtol(text.c_str(), &end, 10);
			return end == text.c_str() ? a_default : static_cast<std::int32_t>(value);
		}

		float ToFloat(const std::vector<std::string>& a_row, int a_column, float a_default = 0.0f)
		{
			const auto text = Cell(a_row, a_column);
			if (text.empty())
				return a_default;
			char* end = nullptr;
			const auto value = std::strtof(text.c_str(), &end);
			return end == text.c_str() ? a_default : value;
		}

		StageSource SourceFromText(std::string_view a_text)
		{
			if (a_text == "worldband") return StageSource::WorldBand;
			if (a_text == "encounter") return StageSource::Encounter;
			if (a_text == "override") return StageSource::Override;
			if (a_text == "playerrank") return StageSource::PlayerRank;
			return StageSource::Default;
		}

		std::uint64_t TextSalt(std::string_view a_text)
		{
			std::uint64_t hash = 1469598103934665603ull;
			for (const auto c : a_text) {
				hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
				hash *= 1099511628211ull;
			}
			return hash;
		}

		bool IsNear(float a_left, float a_right, float a_tolerance)
		{
			return std::fabs(a_left - a_right) <= a_tolerance;
		}

		bool NearU32(std::uint32_t a_left, std::uint32_t a_right)
		{
			return a_left == a_right || (a_left > a_right ? a_left - a_right : a_right - a_left) <= 1;
		}

		bool IsTierKeyword(std::string_view a_editorId, std::uint8_t a_tier)
		{
			if (a_editorId.size() != 13)
				return false;
			if (a_editorId.rfind("RF_KW_Tier_", 0) != 0)
				return false;
			return a_editorId.ends_with(std::format("{:02}", a_tier));
		}
	}

	StageEngine& StageEngine::Get()
	{
		static StageEngine singleton;
		return singleton;
	}

	void StageEngine::Load()
	{
		stages.clear();
		curves.clear();
		tiers.clear();
		difficulties.clear();
		bands.clear();
		rules.clear();
		legacyTiers.clear();
		loaded = false;

		if (!LoadCurves() || !LoadStages() || !LoadTiers() || !LoadDifficulties() || !LoadBands() || !LoadRules() || !LoadLegacyTiers()) {
			logger::error("stage-engine: load failed; ladder disabled");
			stages.clear();
			curves.clear();
			tiers.clear();
			difficulties.clear();
			bands.clear();
			rules.clear();
			legacyTiers.clear();
			return;
		}
		std::sort(rules.begin(), rules.end(), [](const AssignmentRule& a, const AssignmentRule& b) {
			return a.priority > b.priority;
		});
		if (!ValidateTables()) {
			logger::error("stage-engine: validation failed; ladder disabled");
			stages.clear();
			curves.clear();
			tiers.clear();
			difficulties.clear();
			bands.clear();
			rules.clear();
			legacyTiers.clear();
			return;
		}
		loaded = true;
		logger::info("stage-engine: loaded {} stages, {} tiers, {} curves, {} difficulties, {} bands, {} rules, {} legacy tiers",
			stages.size(), tiers.size(), curves.size(), difficulties.size(), bands.size(), rules.size(), legacyTiers.size());
	}

	bool StageEngine::LoadCurves()
	{
		const auto* table = DataStore::Get().GetTable("curve_profiles.csv");
		if (!table) {
			logger::error("stage-engine: curve_profiles.csv missing");
			return false;
		}
		const int cId = FindColumn(*table, "curveId");
		const int cBase = FindColumn(*table, "powerBase");
		const int cHealth = FindColumn(*table, "healthExp");
		const int cDamage = FindColumn(*table, "damageExp");
		const int cRegen = FindColumn(*table, "regenExp");
		const int cArmor = FindColumn(*table, "armorExp");
		const int cMax = FindColumn(*table, "maxStage");
		const int cEnabled = FindColumn(*table, "enabled");
		const int cNotes = FindColumn(*table, "notes");
		if (cId < 0 || cBase < 0 || cHealth < 0 || cDamage < 0 || cRegen < 0 || cArmor < 0 || cMax < 0) {
			logger::error("stage-engine: curve_profiles.csv missing required columns");
			return false;
		}
		for (const auto& row : table->rows) {
			CurveDefinition value;
			value.curveId = static_cast<std::uint8_t>((std::clamp)(ToInt(row, cId), 1, 255));
			value.powerBase = ToFloat(row, cBase, 15.0f);
			value.healthExp = ToFloat(row, cHealth, 0.65f);
			value.damageExp = ToFloat(row, cDamage, 0.45f);
			value.regenExp = ToFloat(row, cRegen, 0.30f);
			value.armorExp = ToFloat(row, cArmor, 0.25f);
			value.maxStage = static_cast<std::uint8_t>((std::clamp)(ToInt(row, cMax, 100), 1, 100));
			value.enabled = ToInt(row, cEnabled, 1) != 0;
			value.notes = Cell(row, cNotes);
			curves.push_back(std::move(value));
		}
		return !curves.empty();
	}

	bool StageEngine::LoadStages()
	{
		const auto* table = DataStore::Get().GetTable("stage_ladder.csv");
		if (!table) {
			logger::error("stage-engine: stage_ladder.csv missing");
			return false;
		}
		const int sStage = FindColumn(*table, "stage");
		const int sTier = FindColumn(*table, "tier");
		const int sCurve = FindColumn(*table, "curveId");
		const int sPower = FindColumn(*table, "powerIndex");
		const int sHealth = FindColumn(*table, "healthMult");
		const int sDamage = FindColumn(*table, "damageMult");
		const int sRegen = FindColumn(*table, "regenMult");
		const int sArmor = FindColumn(*table, "armorMult");
		const int sStat = FindColumn(*table, "statCapacity");
		const int sOffense = FindColumn(*table, "offenseCapacity");
		const int sDefense = FindColumn(*table, "defenseCapacity");
		const int sUtility = FindColumn(*table, "utilityCapacity");
		const int sAbility = FindColumn(*table, "abilityCapacity");
		const int sLoot = FindColumn(*table, "lootCapacity");
		const int sReward = FindColumn(*table, "rewardCapacity");
		const int sEncounter = FindColumn(*table, "encounterCost");
		if (sStage < 0 || sTier < 0 || sCurve < 0 || sPower < 0) {
			logger::error("stage-engine: stage_ladder.csv missing required columns");
			return false;
		}
		for (const auto& row : table->rows) {
			StageRow value;
			value.stage = static_cast<std::uint8_t>((std::clamp)(ToInt(row, sStage), 1, 100));
			value.tier = static_cast<std::uint8_t>((std::clamp)(ToInt(row, sTier), 1, 10));
			value.curve = static_cast<std::uint8_t>((std::clamp)(ToInt(row, sCurve, 1), 1, 255));
			value.power = ToFloat(row, sPower, 1.0f);
			value.health = ToFloat(row, sHealth, 1.0f);
			value.damage = ToFloat(row, sDamage, 1.0f);
			value.regen = ToFloat(row, sRegen, 1.0f);
			value.armor = ToFloat(row, sArmor, 1.0f);
			value.statCapacity = static_cast<std::uint32_t>((std::max)(0, ToInt(row, sStat, 100)));
			value.offenseCapacity = static_cast<std::uint32_t>((std::max)(0, ToInt(row, sOffense, 100)));
			value.defenseCapacity = static_cast<std::uint32_t>((std::max)(0, ToInt(row, sDefense, 100)));
			value.utilityCapacity = static_cast<std::uint32_t>((std::max)(0, ToInt(row, sUtility, 100)));
			value.abilityCapacity = static_cast<std::uint32_t>((std::max)(0, ToInt(row, sAbility, 100)));
			value.lootCapacity = static_cast<std::uint32_t>((std::max)(0, ToInt(row, sLoot, 100)));
			value.rewardCapacity = static_cast<std::uint32_t>((std::max)(0, ToInt(row, sReward, 100)));
			value.encounterCost = static_cast<std::uint32_t>((std::max)(0, ToInt(row, sEncounter, 10)));
			stages.push_back(value);
		}
		return stages.size() == 100;
	}

	bool StageEngine::LoadTiers()
	{
		const auto* table = DataStore::Get().GetTable("tier_definitions.csv");
		if (!table) {
			logger::error("stage-engine: tier_definitions.csv missing");
			return false;
		}
		const int tTier = FindColumn(*table, "tier");
		const int tMin = FindColumn(*table, "stageMin");
		const int tMax = FindColumn(*table, "stageMax");
		const int tName = FindColumn(*table, "name");
		const int tNameKey = FindColumn(*table, "nameKey");
		const int tKeyword = FindColumn(*table, "keywordEditorId");
		const int tTag = FindColumn(*table, "breakpointTag");
		const int tUnlock = FindColumn(*table, "unlockTag");
		const int tStep = FindColumn(*table, "budgetStep");
		for (const auto& row : table->rows) {
			TierDefinition value;
			value.tier = static_cast<std::uint8_t>((std::clamp)(ToInt(row, tTier), 1, 10));
			value.stageMin = static_cast<std::uint8_t>((std::clamp)(ToInt(row, tMin), 1, 100));
			value.stageMax = static_cast<std::uint8_t>((std::clamp)(ToInt(row, tMax), 1, 100));
			value.name = Cell(row, tName);
			value.nameKey = Cell(row, tNameKey);
			value.keywordEditorId = Cell(row, tKeyword);
			value.breakpointTag = Cell(row, tTag);
			value.unlockTag = Cell(row, tUnlock);
			value.budgetStep = static_cast<std::uint32_t>((std::max)(0, ToInt(row, tStep, 100)));
			tiers.push_back(std::move(value));
		}
		return tiers.size() == 10;
	}

	bool StageEngine::LoadDifficulties()
	{
		const auto* table = DataStore::Get().GetTable("difficulty_profiles.csv");
		if (!table) {
			logger::error("stage-engine: difficulty_profiles.csv missing");
			return false;
		}
		const int dId = FindColumn(*table, "profileId");
		const int dName = FindColumn(*table, "name");
		const int dNameKey = FindColumn(*table, "nameKey");
		const int dDelta = FindColumn(*table, "stageDelta");
		const int dStat = FindColumn(*table, "statMult");
		const int dDealt = FindColumn(*table, "damageDealtMult");
		const int dTaken = FindColumn(*table, "damageTakenMult");
		const int dXp = FindColumn(*table, "xpMult");
		const int dReward = FindColumn(*table, "rewardMult");
		const int dDrop = FindColumn(*table, "dropMult");
		const int dMax = FindColumn(*table, "maxStage");
		const int dEnabled = FindColumn(*table, "enabled");
		for (const auto& row : table->rows) {
			DifficultyDefinition value;
			value.profileId = static_cast<std::uint8_t>((std::clamp)(ToInt(row, dId), 0, 255));
			value.name = Cell(row, dName);
			value.nameKey = Cell(row, dNameKey);
			value.stageDelta = ToInt(row, dDelta, 0);
			value.statMult = ToFloat(row, dStat, 1.0f);
			value.damageDealtMult = ToFloat(row, dDealt, 1.0f);
			value.damageTakenMult = ToFloat(row, dTaken, 1.0f);
			value.xpMult = ToFloat(row, dXp, 1.0f);
			value.rewardMult = ToFloat(row, dReward, 1.0f);
			value.dropMult = ToFloat(row, dDrop, 1.0f);
			value.maxStage = static_cast<std::uint8_t>((std::clamp)(ToInt(row, dMax, 100), 1, 100));
			value.enabled = ToInt(row, dEnabled, 1) != 0;
			difficulties.push_back(std::move(value));
		}
		return !difficulties.empty();
	}

	bool StageEngine::LoadBands()
	{
		const auto* table = DataStore::Get().GetTable("stage_content_bands.csv");
		if (!table) {
			logger::error("stage-engine: stage_content_bands.csv missing");
			return false;
		}
		const int bMin = FindColumn(*table, "stageMin");
		const int bMax = FindColumn(*table, "stageMax");
		const int bTier = FindColumn(*table, "tier");
		const int bPlayerMin = FindColumn(*table, "playerLevelMin");
		const int bPlayerMax = FindColumn(*table, "playerLevelMax");
		const int bItemMin = FindColumn(*table, "itemLevelMin");
		const int bItemMax = FindColumn(*table, "itemLevelMax");
		const int bGear = FindColumn(*table, "gearCheckBand");
		const int bUnlock = FindColumn(*table, "unlockTag");
		const int bV1 = FindColumn(*table, "v1Populated");
		for (const auto& row : table->rows) {
			ContentBand value;
			value.stageMin = static_cast<std::uint8_t>((std::clamp)(ToInt(row, bMin), 1, 100));
			value.stageMax = static_cast<std::uint8_t>((std::clamp)(ToInt(row, bMax), 1, 100));
			value.tier = static_cast<std::uint8_t>((std::clamp)(ToInt(row, bTier), 1, 10));
			value.playerLevelMin = static_cast<std::uint8_t>((std::clamp)(ToInt(row, bPlayerMin), 1, 100));
			value.playerLevelMax = static_cast<std::uint8_t>((std::clamp)(ToInt(row, bPlayerMax), 1, 100));
			value.itemLevelMin = static_cast<std::uint8_t>((std::clamp)(ToInt(row, bItemMin), 1, 100));
			value.itemLevelMax = static_cast<std::uint8_t>((std::clamp)(ToInt(row, bItemMax), 1, 100));
			value.gearCheckBand = Cell(row, bGear);
			value.unlockTag = Cell(row, bUnlock);
			value.v1Populated = ToInt(row, bV1, 1) != 0;
			bands.push_back(std::move(value));
		}
		return bands.size() == 10;
	}

	bool StageEngine::LoadRules()
	{
		const auto* table = DataStore::Get().GetTable("stage_assignment_rules.csv");
		if (!table) {
			logger::error("stage-engine: stage_assignment_rules.csv missing");
			return false;
		}
		const int rId = FindColumn(*table, "ruleId");
		const int rSource = FindColumn(*table, "source");
		const int rMin = FindColumn(*table, "minStage");
		const int rMax = FindColumn(*table, "maxStage");
		const int rOffMin = FindColumn(*table, "offsetMin");
		const int rOffMax = FindColumn(*table, "offsetMax");
		const int rJitter = FindColumn(*table, "jitter");
		const int rSalt = FindColumn(*table, "seedSalt");
		const int rPriority = FindColumn(*table, "priority");
		const int rEnabled = FindColumn(*table, "enabled");
		for (const auto& row : table->rows) {
			AssignmentRule value;
			value.ruleId = static_cast<std::uint16_t>((std::max)(0, ToInt(row, rId)));
			value.source = SourceFromText(Cell(row, rSource));
			value.minStage = static_cast<std::uint8_t>((std::clamp)(ToInt(row, rMin, 1), 1, 100));
			value.maxStage = static_cast<std::uint8_t>((std::clamp)(ToInt(row, rMax, 100), 1, 100));
			value.offsetMin = static_cast<std::int8_t>((std::clamp)(ToInt(row, rOffMin), -100, 100));
			value.offsetMax = static_cast<std::int8_t>((std::clamp)(ToInt(row, rOffMax), -100, 100));
			value.jitter = static_cast<std::int8_t>((std::clamp)(ToInt(row, rJitter), 0, 100));
			value.seedSalt = TextSalt(Cell(row, rSalt));
			value.priority = ToInt(row, rPriority, 0);
			value.enabled = ToInt(row, rEnabled, 1) != 0;
			rules.push_back(std::move(value));
		}
		return !rules.empty();
	}

	bool StageEngine::LoadLegacyTiers()
	{
		const auto* table = DataStore::Get().GetTable("legacy_rf_tier_map.csv");
		if (!table) {
			logger::error("stage-engine: legacy_rf_tier_map.csv missing");
			return false;
		}
		const int lTier = FindColumn(*table, "rfTier");
		const int lMin = FindColumn(*table, "stageMin");
		const int lMax = FindColumn(*table, "stageMax");
		const int lRank = FindColumn(*table, "playerRankStage");
		for (const auto& row : table->rows) {
			LegacyTierMap value;
			value.rfTier = static_cast<std::uint8_t>((std::clamp)(ToInt(row, lTier), 0, 5));
			value.stageMin = static_cast<std::uint8_t>((std::clamp)(ToInt(row, lMin), 1, 100));
			value.stageMax = static_cast<std::uint8_t>((std::clamp)(ToInt(row, lMax), 1, 100));
			value.playerRankStage = static_cast<std::uint8_t>((std::clamp)(ToInt(row, lRank), 1, 100));
			legacyTiers.push_back(value);
		}
		return legacyTiers.size() == 6;
	}

	bool StageEngine::ValidateTables() const
	{
		if (stages.size() != 100) {
			logger::error("stage-engine: expected 100 stages, got {}", stages.size());
			return false;
		}
		if (tiers.size() != 10) {
			logger::error("stage-engine: expected 10 tiers, got {}", tiers.size());
			return false;
		}
		if (curves.empty() || legacyTiers.size() != 6) {
			logger::error("stage-engine: curves or legacy tier map missing");
			return false;
		}
		for (std::size_t i = 0; i < stages.size(); ++i) {
			const auto& row = stages[i];
			if (row.stage != i + 1) {
				logger::error("stage-engine: stage row {} is {} (not contiguous)", i + 1, row.stage);
				return false;
			}
			if (row.tier != (row.stage - 1) / 10 + 1) {
				logger::error("stage-engine: stage {} tier {} disagrees with formula", row.stage, row.tier);
				return false;
			}
			const auto* curve = Curve(row.curve);
			if (!curve || !curve->enabled || row.stage > curve->maxStage) {
				logger::error("stage-engine: stage {} references invalid curve {}", row.stage, row.curve);
				return false;
			}
			const auto stageCount = static_cast<float>((std::max)(1, static_cast<int>(curve->maxStage) - 1));
			const auto expectedPower = std::pow(curve->powerBase, static_cast<float>(row.stage - 1) / stageCount);
			if (!IsNear(row.power, expectedPower, 2e-3f)) {
				logger::error("stage-engine: stage {} power {} != curve {}", row.stage, row.power, expectedPower);
				return false;
			}
			if (!IsNear(row.health, std::pow(row.power, curve->healthExp), 2e-3f) ||
				!IsNear(row.damage, std::pow(row.power, curve->damageExp), 2e-3f) ||
				!IsNear(row.regen, std::pow(row.power, curve->regenExp), 2e-3f) ||
				!IsNear(row.armor, std::pow(row.power, curve->armorExp), 2e-3f)) {
				logger::error("stage-engine: stage {} multipliers do not match curve exponents", row.stage);
				return false;
			}
			const auto expectedStat = static_cast<std::uint32_t>(std::llround(100.0 * row.power));
			const auto expectedOffense = static_cast<std::uint32_t>(std::llround(100.0 * std::pow(row.power, 0.45)));
			const auto expectedDefense = static_cast<std::uint32_t>(std::llround(100.0 * std::pow(row.power, 0.65)));
			const auto expectedUtility = static_cast<std::uint32_t>(std::llround(100.0 * std::pow(row.power, 0.30)));
			const auto expectedAbility = static_cast<std::uint32_t>(std::llround(100.0 * std::pow(row.power, 0.25)));
			const auto expectedLoot = static_cast<std::uint32_t>(std::llround(100.0 * std::pow(row.power, 0.50)));
			const auto expectedReward = static_cast<std::uint32_t>(std::llround(100.0 * std::pow(row.power, 0.40)));
			const auto expectedEncounter = static_cast<std::uint32_t>(std::llround(10.0 * std::pow(row.power, 0.35)));
			if (!NearU32(row.statCapacity, expectedStat) || !NearU32(row.offenseCapacity, expectedOffense) || !NearU32(row.defenseCapacity, expectedDefense) ||
				!NearU32(row.utilityCapacity, expectedUtility) || !NearU32(row.abilityCapacity, expectedAbility) || !NearU32(row.lootCapacity, expectedLoot) ||
				!NearU32(row.rewardCapacity, expectedReward) || !NearU32(row.encounterCost, expectedEncounter)) {
				logger::error("stage-engine: stage {} capacity columns do not match formulas", row.stage);
				return false;
			}
			if (i > 0) {
				const auto& previous = stages[i - 1];
				if (row.power + 1e-5f < previous.power || row.health + 1e-5f < previous.health ||
					row.damage + 1e-5f < previous.damage || row.regen + 1e-5f < previous.regen ||
					row.armor + 1e-5f < previous.armor || row.statCapacity < previous.statCapacity) {
					logger::error("stage-engine: non-monotonic row at stage {}", row.stage);
					return false;
				}
			}
		}
		for (std::size_t i = 0; i < tiers.size(); ++i) {
			const auto& tier = tiers[i];
			if (tier.tier != i + 1 || tier.stageMin != i * 10 + 1 || tier.stageMax != (i + 1) * 10) {
				logger::error("stage-engine: tier {} range is not contiguous", i + 1);
				return false;
			}
			if (tier.name.empty() || tier.nameKey.empty() || tier.unlockTag.empty() || tier.breakpointTag.empty() ||
				!IsTierKeyword(tier.keywordEditorId, static_cast<std::uint8_t>(i + 1)) || tier.budgetStep == 0) {
				logger::error("stage-engine: tier {} has invalid name/keyword/metadata", i + 1);
				return false;
			}
		}
		if (bands.size() != 10) {
			logger::error("stage-engine: expected 10 content bands, got {}", bands.size());
			return false;
		}
		for (std::size_t i = 0; i < bands.size(); ++i) {
			const auto& band = bands[i];
			if (band.stageMin != i * 10 + 1 || band.stageMax != (i + 1) * 10 || band.tier != i + 1) {
				logger::error("stage-engine: content band {} is not contiguous", i + 1);
				return false;
			}
			if (band.playerLevelMin > band.playerLevelMax || band.itemLevelMin > band.itemLevelMax || band.gearCheckBand.empty()) {
				logger::error("stage-engine: content band {} has invalid level ranges", i + 1);
				return false;
			}
		}
		std::vector<std::uint8_t> difficultyIds;
		for (const auto& difficulty : difficulties) {
			if (std::find(difficultyIds.begin(), difficultyIds.end(), difficulty.profileId) != difficultyIds.end()) {
				logger::error("stage-engine: duplicate difficulty profile {}", difficulty.profileId);
				return false;
			}
			difficultyIds.push_back(difficulty.profileId);
			if (difficulty.name.empty() || difficulty.nameKey.empty() || difficulty.maxStage < 1 || difficulty.statMult <= 0.0f) {
				logger::error("stage-engine: difficulty profile {} is invalid", difficulty.profileId);
				return false;
			}
		}
		std::vector<std::uint16_t> ruleIds;
		std::vector<StageSource>   sources;
		for (const auto& rule : rules) {
			if (rule.ruleId == 0 || std::find(ruleIds.begin(), ruleIds.end(), rule.ruleId) != ruleIds.end()) {
				logger::error("stage-engine: duplicate or zero assignment rule id {}", rule.ruleId);
				return false;
			}
			ruleIds.push_back(rule.ruleId);
			if (!rule.enabled)
				continue;
			sources.push_back(rule.source);
			if (rule.offsetMin > rule.offsetMax || rule.jitter < 0 || rule.minStage > rule.maxStage) {
				logger::error("stage-engine: assignment rule {} has invalid range", rule.ruleId);
				return false;
			}
		}
		for (const auto source : { StageSource::Default, StageSource::WorldBand, StageSource::Encounter, StageSource::Override, StageSource::PlayerRank }) {
			if (std::find(sources.begin(), sources.end(), source) == sources.end()) {
				logger::error("stage-engine: no enabled rule for source {}", static_cast<int>(source));
				return false;
			}
		}
		if (!legacyTiers.empty()) {
			std::uint8_t expectedMin = 1;
			for (const auto& legacy : legacyTiers) {
				if (legacy.stageMin != expectedMin || legacy.stageMax < legacy.stageMin) {
					logger::error("stage-engine: legacy tier map is not contiguous at rfTier {}", legacy.rfTier);
					return false;
				}
				expectedMin = static_cast<std::uint8_t>(legacy.stageMax + 1);
			}
			if (legacyTiers.back().stageMax != 100) {
				logger::error("stage-engine: legacy tier map does not end at stage 100");
				return false;
			}
		}
		return true;
	}

	const StageRow* StageEngine::Stage(std::uint8_t a_stage) const
	{
		if (a_stage < 1 || a_stage > stages.size())
			return nullptr;
		return &stages[static_cast<std::size_t>(a_stage - 1)];
	}

	float StageEngine::Power(std::uint8_t a_stage) const
	{
		const auto* row = Stage(a_stage);
		return row ? row->power : 1.0f;
	}

	float StageEngine::StatMultiplier(std::uint8_t a_stage, StatKind a_kind) const
	{
		const auto* row = Stage(a_stage);
		if (!row)
			return 1.0f;
		switch (a_kind) {
		case StatKind::Health: return row->health;
		case StatKind::Damage: return row->damage;
		case StatKind::Regen: return row->regen;
		case StatKind::Armor: return row->armor;
		default: return 1.0f;
		}
	}

	std::uint8_t StageEngine::TierOf(std::uint8_t a_stage) const
	{
		const auto* row = Stage(a_stage);
		return row ? row->tier : 1;
	}

	std::uint32_t StageEngine::Capacity(std::uint8_t a_stage, CapacityKind a_kind) const
	{
		const auto* row = Stage(a_stage);
		if (!row)
			return 0;
		switch (a_kind) {
		case CapacityKind::Stat: return row->statCapacity;
		case CapacityKind::Offense: return row->offenseCapacity;
		case CapacityKind::Defense: return row->defenseCapacity;
		case CapacityKind::Utility: return row->utilityCapacity;
		case CapacityKind::Ability: return row->abilityCapacity;
		case CapacityKind::Loot: return row->lootCapacity;
		case CapacityKind::Reward: return row->rewardCapacity;
		case CapacityKind::Encounter: return row->encounterCost;
		default: return 0;
		}
	}

	std::uint8_t StageEngine::StageGap(std::uint8_t a_target, std::uint8_t a_attacker) const
	{
		return a_target > a_attacker ? static_cast<std::uint8_t>(a_target - a_attacker) : 0;
	}

	float StageEngine::PowerRatio(std::uint8_t a_target, std::uint8_t a_attacker) const
	{
		const auto attacker = Power(a_attacker);
		return attacker <= 0.0f ? 0.0f : Power(a_target) / attacker;
	}

	const CurveDefinition* StageEngine::Curve(std::uint8_t a_curve) const
	{
		for (const auto& value : curves)
			if (value.curveId == a_curve)
				return &value;
		return nullptr;
	}

	const TierDefinition* StageEngine::Tier(std::uint8_t a_tier) const
	{
		if (a_tier < 1 || a_tier > tiers.size())
			return nullptr;
		return &tiers[static_cast<std::size_t>(a_tier - 1)];
	}

	const DifficultyDefinition* StageEngine::Difficulty(std::uint8_t a_profile) const
	{
		for (const auto& value : difficulties)
			if (value.profileId == a_profile)
				return &value;
		return nullptr;
	}

	const ContentBand* StageEngine::ContentBandFor(std::uint8_t a_stage) const
	{
		for (const auto& value : bands)
			if (a_stage >= value.stageMin && a_stage <= value.stageMax)
				return &value;
		return nullptr;
	}

	const LegacyTierMap* StageEngine::LegacyTier(std::uint8_t a_rfTier) const
	{
		for (const auto& value : legacyTiers)
			if (value.rfTier == a_rfTier)
				return &value;
		return nullptr;
	}

	std::uint64_t StageEngine::MixSeed(std::uint64_t a_seed) const
	{
		std::uint64_t hash = 1469598103934665603ull;
		for (int i = 0; i < 8; ++i) {
			hash ^= (a_seed >> (i * 8)) & 0xFF;
			hash *= 1099511628211ull;
		}
		hash ^= hash >> 33;
		hash *= 0xff51afd7ed558ccdull;
		hash ^= hash >> 33;
		return hash;
	}

	const AssignmentRule* StageEngine::FindRuleById(std::uint16_t a_ruleId) const
	{
		for (const auto& rule : rules)
			if (rule.ruleId == a_ruleId)
				return &rule;
		return nullptr;
	}

	const AssignmentRule* StageEngine::FindRule(StageSource a_source) const
	{
		for (const auto& rule : rules)
			if (rule.enabled && rule.source == a_source)
				return &rule;
		return nullptr;
	}

	StageProfile StageEngine::RequestStage(const StageRequest& a_request) const
	{
		StageProfile profile;
		profile.source = a_request.source;
		profile.flags = kStageProfileNone;

		if (static_cast<std::uint8_t>(a_request.source) > static_cast<std::uint8_t>(StageSource::PlayerRank)) {
			profile.flags = kStageProfileRejected;
			return profile;
		}

		const AssignmentRule* rule = nullptr;
		if (a_request.ruleId != 0) {
			rule = FindRuleById(a_request.ruleId);
			if (!rule || !rule->enabled) {
				profile.flags = kStageProfileRejected;
				return profile;
			}
		} else {
			rule = FindRule(a_request.source);
		}

		auto stage = static_cast<std::int32_t>(1);
		bool explicitStage = false;
		if (a_request.overrideStage >= 1 && a_request.overrideStage <= 100) {
			stage = a_request.overrideStage;
			explicitStage = true;
		} else if (a_request.requestedStage >= 1 && a_request.requestedStage <= 100) {
			stage = a_request.requestedStage;
			explicitStage = true;
		} else if (a_request.source == StageSource::PlayerRank) {
			if (a_request.playerRank >= 1 && a_request.playerRank <= 100) {
				stage = a_request.playerRank;
				explicitStage = true;
			}
		} else if (a_request.band >= 1 && a_request.band <= 100) {
			stage = a_request.band;
		}

		if (!explicitStage && a_request.source != StageSource::Default && stage == 1) {
			if ((a_request.source == StageSource::WorldBand || a_request.source == StageSource::Encounter) && a_request.band == 0) {
				profile.flags = kStageProfileRejected;
				return profile;
			}
			if (a_request.source == StageSource::Override && a_request.overrideStage == 0) {
				profile.flags = kStageProfileRejected;
				return profile;
			}
			if (a_request.source == StageSource::PlayerRank && a_request.playerRank == 0 && a_request.band == 0) {
				profile.flags = kStageProfileRejected;
				return profile;
			}
		}

		if (!explicitStage && rule) {
			const auto hash = MixSeed(a_request.seed ^ rule->seedSalt ^ static_cast<std::uint64_t>(rule->ruleId));
			const auto span = static_cast<std::int32_t>(rule->offsetMax) - rule->offsetMin + 1;
			const auto offset = span > 0 ? rule->offsetMin + static_cast<std::int32_t>(hash % static_cast<std::uint64_t>(span)) : rule->offsetMin;
			const auto jitter = rule->jitter > 0 ? static_cast<std::int32_t>(hash >> 16) % (rule->jitter * 2 + 1) - rule->jitter : 0;
			const auto adjusted = stage + offset + jitter;
			const auto clamped = (std::clamp)(adjusted, static_cast<std::int32_t>(rule->minStage), static_cast<std::int32_t>(rule->maxStage));
			if (clamped != adjusted)
				profile.flags = static_cast<std::uint16_t>(profile.flags | kStageProfileClamped);
			stage = clamped;
			profile.flags = static_cast<std::uint16_t>(profile.flags | kStageProfileRuleApplied);
		}

		if (a_request.difficultyProfile != 255) {
			const auto* difficulty = Difficulty(a_request.difficultyProfile);
			if (!difficulty || !difficulty->enabled) {
				profile.flags = kStageProfileRejected;
				return profile;
			}
			const auto adjusted = stage + difficulty->stageDelta;
			stage = (std::clamp)(adjusted, 1, static_cast<std::int32_t>(difficulty->maxStage));
			profile.flags = static_cast<std::uint16_t>(profile.flags | kStageProfileDifficultyApplied);
		}

		const auto curveId = Stage(1) ? Stage(1)->curve : 1;
		const auto* curve = Curve(curveId);
		const auto maxStage = curve ? static_cast<std::int32_t>(curve->maxStage) : 100;
		const auto clamped = (std::clamp)(stage, 1, maxStage);
		if (clamped != stage)
			profile.flags = static_cast<std::uint16_t>(profile.flags | kStageProfileClamped);
		profile.stage = static_cast<std::uint8_t>(clamped);
		profile.tier = TierOf(profile.stage);
		profile.curve = Stage(profile.stage) ? Stage(profile.stage)->curve : 1;
		return profile;
	}

	SelfTestResult StageEngine::SelfTest()
	{
		auto& engine = Get();
		if (!engine.Loaded())
			return { "StageEngine.SelfTest", CheckStatus::Fail, "stage tables not loaded/validated" };
		if (engine.StageCount() != 100 || engine.TierCount() != 10 || engine.CurveCount() == 0 || engine.LegacyTierCount() != 6)
			return { "StageEngine.SelfTest", CheckStatus::Fail, "expected 100 stages, 10 tiers, curve, 6 legacy tiers" };
		if (std::fabs(engine.Power(1) - 1.0f) > 1e-3f || std::fabs(engine.Power(100) - 15.0f) > 2e-3f)
			return { "StageEngine.SelfTest", CheckStatus::Fail, "power endpoints are wrong" };
		if (engine.TierOf(1) != 1 || engine.TierOf(10) != 1 || engine.TierOf(11) != 2 || engine.TierOf(100) != 10)
			return { "StageEngine.SelfTest", CheckStatus::Fail, "tier mapping is wrong" };

		StageRequest request;
		request.difficultyProfile = 255;
		request.overrideStage = 50;
		auto profile = engine.RequestStage(request);
		if ((profile.flags & kStageProfileRejected) || profile.stage != 50 || profile.tier != 5)
			return { "StageEngine.SelfTest", CheckStatus::Fail, "override request did not resolve" };

		request.overrideStage = 0;
		request.requestedStage = 75;
		profile = engine.RequestStage(request);
		if (profile.stage != 75 || profile.tier != 8)
			return { "StageEngine.SelfTest", CheckStatus::Fail, "explicit stage request did not resolve" };

		request.requestedStage = 0;
		request.source = StageSource::PlayerRank;
		request.playerRank = 37;
		profile = engine.RequestStage(request);
		if (profile.stage != 37 || profile.source != StageSource::PlayerRank)
			return { "StageEngine.SelfTest", CheckStatus::Fail, "player rank request did not resolve" };

		request.source = StageSource::Encounter;
		request.playerRank = 0;
		request.band = 50;
		request.seed = 424242;
		const auto first = engine.RequestStage(request);
		const auto second = engine.RequestStage(request);
		if (first.stage != second.stage || first.flags != second.flags || first.stage < 1 || first.stage > 100)
			return { "StageEngine.SelfTest", CheckStatus::Fail, "encounter request is not deterministic" };

		request.difficultyProfile = 0;
		request.requestedStage = 50;
		request.band = 0;
		request.seed = 0;
		profile = engine.RequestStage(request);
		if (profile.stage != 45)
			return { "StageEngine.SelfTest", CheckStatus::Fail, "difficulty profile did not apply stage delta" };

		request.difficultyProfile = 255;
		request.requestedStage = 0;
		request.ruleId = 999;
		profile = engine.RequestStage(request);
		if (!(profile.flags & kStageProfileRejected))
			return { "StageEngine.SelfTest", CheckStatus::Fail, "unknown rule id was not rejected" };

		request.ruleId = 0;
		request.source = static_cast<StageSource>(99);
		profile = engine.RequestStage(request);
		if (!(profile.flags & kStageProfileRejected))
			return { "StageEngine.SelfTest", CheckStatus::Fail, "unknown source was not rejected" };

		const auto* legacy = engine.LegacyTier(5);
		if (!legacy || legacy->stageMax != 100 || legacy->playerRankStage != 100)
			return { "StageEngine.SelfTest", CheckStatus::Fail, "legacy tier map is not usable" };
		const auto* curve = engine.Curve(1);
		if (!curve || curve->maxStage != 100)
			return { "StageEngine.SelfTest", CheckStatus::Fail, "curve profile is not usable" };

		return { "StageEngine.SelfTest", CheckStatus::Pass,
			std::format("100 stages, 10 tiers, 1 curve, 6 legacy tiers, validated; encounter stage {}; rejection/difficulty/player-rank paths OK", first.stage) };
	}
}
