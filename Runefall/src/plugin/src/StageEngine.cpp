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
	}

	StageEngine& StageEngine::Get()
	{
		static StageEngine singleton;
		return singleton;
	}

	void StageEngine::Load()
	{
		stages.clear();
		tiers.clear();
		difficulties.clear();
		bands.clear();
		rules.clear();
		loaded = false;

		const auto* stageTable = DataStore::Get().GetTable("stage_ladder.csv");
		const auto* tierTable = DataStore::Get().GetTable("tier_definitions.csv");
		if (!stageTable || !tierTable) {
			logger::error("stage-engine: stage_ladder.csv or tier_definitions.csv missing");
			return;
		}

		const int sStage = FindColumn(*stageTable, "stage");
		const int sTier = FindColumn(*stageTable, "tier");
		const int sCurve = FindColumn(*stageTable, "curveId");
		const int sPower = FindColumn(*stageTable, "powerIndex");
		const int sHealth = FindColumn(*stageTable, "healthMult");
		const int sDamage = FindColumn(*stageTable, "damageMult");
		const int sRegen = FindColumn(*stageTable, "regenMult");
		const int sArmor = FindColumn(*stageTable, "armorMult");
		const int sStat = FindColumn(*stageTable, "statCapacity");
		const int sOffense = FindColumn(*stageTable, "offenseCapacity");
		const int sDefense = FindColumn(*stageTable, "defenseCapacity");
		const int sUtility = FindColumn(*stageTable, "utilityCapacity");
		const int sAbility = FindColumn(*stageTable, "abilityCapacity");
		const int sLoot = FindColumn(*stageTable, "lootCapacity");
		const int sReward = FindColumn(*stageTable, "rewardCapacity");
		const int sEncounter = FindColumn(*stageTable, "encounterCost");
		if (sStage < 0 || sTier < 0 || sPower < 0) {
			logger::error("stage-engine: stage_ladder.csv missing required columns");
			return;
		}

		for (const auto& row : stageTable->rows) {
			StageRow value;
			value.stage = static_cast<std::uint8_t>((std::clamp)(ToInt(row, sStage), 1, 100));
			value.tier = static_cast<std::uint8_t>((std::clamp)(ToInt(row, sTier), 1, 10));
			value.curve = static_cast<std::uint8_t>((std::max)(1, ToInt(row, sCurve, 1)));
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
		if (stages.size() != 100) {
			logger::error("stage-engine: expected 100 stage rows, got {}", stages.size());
			stages.clear();
			return;
		}
		for (std::size_t i = 0; i < stages.size(); ++i) {
			if (stages[i].stage != i + 1) {
				logger::error("stage-engine: stage row {} has stage {} (not contiguous)", i + 1, stages[i].stage);
				stages.clear();
				return;
			}
		}

		const int tTier = FindColumn(*tierTable, "tier");
		const int tMin = FindColumn(*tierTable, "stageMin");
		const int tMax = FindColumn(*tierTable, "stageMax");
		const int tName = FindColumn(*tierTable, "name");
		const int tNameKey = FindColumn(*tierTable, "nameKey");
		const int tKeyword = FindColumn(*tierTable, "keywordEditorId");
		const int tTag = FindColumn(*tierTable, "breakpointTag");
		const int tUnlock = FindColumn(*tierTable, "unlockTag");
		const int tStep = FindColumn(*tierTable, "budgetStep");
		for (const auto& row : tierTable->rows) {
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
			tiers.push_back(value);
		}
		if (tiers.size() != 10) {
			logger::error("stage-engine: expected 10 tier rows, got {}", tiers.size());
			tiers.clear();
			return;
		}

		if (const auto* difficultyTable = DataStore::Get().GetTable("difficulty_profiles.csv")) {
			const int dId = FindColumn(*difficultyTable, "profileId");
			const int dName = FindColumn(*difficultyTable, "name");
			const int dNameKey = FindColumn(*difficultyTable, "nameKey");
			const int dDelta = FindColumn(*difficultyTable, "stageDelta");
			const int dStat = FindColumn(*difficultyTable, "statMult");
			const int dDealt = FindColumn(*difficultyTable, "damageDealtMult");
			const int dTaken = FindColumn(*difficultyTable, "damageTakenMult");
			const int dXp = FindColumn(*difficultyTable, "xpMult");
			const int dReward = FindColumn(*difficultyTable, "rewardMult");
			const int dDrop = FindColumn(*difficultyTable, "dropMult");
			const int dMax = FindColumn(*difficultyTable, "maxStage");
			const int dEnabled = FindColumn(*difficultyTable, "enabled");
			for (const auto& row : difficultyTable->rows) {
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
				difficulties.push_back(value);
			}
		}

		if (const auto* bandTable = DataStore::Get().GetTable("stage_content_bands.csv")) {
			const int bMin = FindColumn(*bandTable, "stageMin");
			const int bMax = FindColumn(*bandTable, "stageMax");
			const int bTier = FindColumn(*bandTable, "tier");
			const int bPlayerMin = FindColumn(*bandTable, "playerLevelMin");
			const int bPlayerMax = FindColumn(*bandTable, "playerLevelMax");
			const int bItemMin = FindColumn(*bandTable, "itemLevelMin");
			const int bItemMax = FindColumn(*bandTable, "itemLevelMax");
			const int bGear = FindColumn(*bandTable, "gearCheckBand");
			const int bUnlock = FindColumn(*bandTable, "unlockTag");
			const int bV1 = FindColumn(*bandTable, "v1Populated");
			for (const auto& row : bandTable->rows) {
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
				bands.push_back(value);
			}
		}

		if (const auto* ruleTable = DataStore::Get().GetTable("stage_assignment_rules.csv")) {
			const int rId = FindColumn(*ruleTable, "ruleId");
			const int rSource = FindColumn(*ruleTable, "source");
			const int rMin = FindColumn(*ruleTable, "minStage");
			const int rMax = FindColumn(*ruleTable, "maxStage");
			const int rOffMin = FindColumn(*ruleTable, "offsetMin");
			const int rOffMax = FindColumn(*ruleTable, "offsetMax");
			const int rJitter = FindColumn(*ruleTable, "jitter");
			const int rSalt = FindColumn(*ruleTable, "seedSalt");
			const int rPriority = FindColumn(*ruleTable, "priority");
			const int rEnabled = FindColumn(*ruleTable, "enabled");
			for (const auto& row : ruleTable->rows) {
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
				rules.push_back(value);
			}
		}

		std::sort(rules.begin(), rules.end(), [](const AssignmentRule& a, const AssignmentRule& b) {
			return a.priority > b.priority;
		});
		loaded = true;
		logger::info("stage-engine: loaded {} stages, {} tiers, {} difficulties, {} bands, {} rules",
			stages.size(), tiers.size(), difficulties.size(), bands.size(), rules.size());
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

		if (a_request.overrideStage >= 1 && a_request.overrideStage <= 100) {
			profile.stage = a_request.overrideStage;
		} else if (a_request.requestedStage >= 1 && a_request.requestedStage <= 100) {
			profile.stage = a_request.requestedStage;
		} else {
			const auto* rule = FindRule(a_request.source);
			const auto base = a_request.band >= 1 && a_request.band <= 100 ? a_request.band : 1;
			auto stage = base;
			if (rule) {
				const auto hash = MixSeed(a_request.seed ^ rule->seedSalt ^ static_cast<std::uint64_t>(rule->ruleId));
				const auto span = static_cast<std::int32_t>(rule->offsetMax) - rule->offsetMin + 1;
				const auto offset = span > 0 ? rule->offsetMin + static_cast<std::int32_t>(hash % static_cast<std::uint64_t>(span)) : rule->offsetMin;
				const auto jitter = rule->jitter > 0 ? static_cast<std::int32_t>(hash >> 16) % (rule->jitter * 2 + 1) - rule->jitter : 0;
				stage = (std::clamp)(static_cast<std::int32_t>(stage) + offset + jitter, static_cast<std::int32_t>(rule->minStage), static_cast<std::int32_t>(rule->maxStage));
			}
			profile.stage = static_cast<std::uint8_t>((std::clamp)(stage, 1, 100));
		}

		if (const auto* difficulty = Difficulty(a_request.difficultyProfile); difficulty && difficulty->enabled) {
			const auto adjusted = static_cast<std::int32_t>(profile.stage) + difficulty->stageDelta;
			profile.stage = static_cast<std::uint8_t>((std::clamp)(adjusted, 1, static_cast<std::int32_t>(difficulty->maxStage)));
		}

		profile.tier = TierOf(profile.stage);
		profile.curve = Stage(profile.stage) ? Stage(profile.stage)->curve : 1;
		return profile;
	}

	SelfTestResult StageEngine::SelfTest()
	{
		auto& engine = Get();
		if (!engine.Loaded())
			return { "StageEngine.SelfTest", CheckStatus::Warn, "stage tables not loaded" };
		if (engine.StageCount() != 100 || engine.TierCount() != 10)
			return { "StageEngine.SelfTest", CheckStatus::Fail, "expected 100 stages and 10 tiers" };
		if (std::fabs(engine.Power(1) - 1.0f) > 1e-4f || std::fabs(engine.Power(100) - 15.0f) > 1e-3f)
			return { "StageEngine.SelfTest", CheckStatus::Fail, "power endpoints are wrong" };
		if (engine.TierOf(1) != 1 || engine.TierOf(10) != 1 || engine.TierOf(11) != 2 || engine.TierOf(100) != 10)
			return { "StageEngine.SelfTest", CheckStatus::Fail, "tier mapping is wrong" };
		for (std::uint8_t stage = 2; stage <= 100; ++stage) {
			if (engine.Power(stage) + 1e-5f < engine.Power(static_cast<std::uint8_t>(stage - 1)))
				return { "StageEngine.SelfTest", CheckStatus::Fail, std::format("power not monotonic at stage {}", stage) };
			if (engine.Capacity(stage, CapacityKind::Stat) < engine.Capacity(static_cast<std::uint8_t>(stage - 1), CapacityKind::Stat))
				return { "StageEngine.SelfTest", CheckStatus::Fail, std::format("capacity not monotonic at stage {}", stage) };
		}
		for (std::uint8_t tier = 1; tier <= 10; ++tier) {
			const auto* definition = engine.Tier(tier);
			if (!definition || definition->stageMin != (tier - 1) * 10 + 1 || definition->stageMax != tier * 10)
				return { "StageEngine.SelfTest", CheckStatus::Fail, std::format("tier {} range is wrong", tier) };
		}
		StageRequest request;
		request.source = StageSource::Override;
		request.difficultyProfile = 255;  // 255 = no difficulty overlay
		request.overrideStage = 50;
		const auto overrideProfile = engine.RequestStage(request);
		if (overrideProfile.stage != 50 || overrideProfile.tier != 5)
			return { "StageEngine.SelfTest", CheckStatus::Fail, "override request did not resolve" };
		request.overrideStage = 0;
		request.requestedStage = 75;
		if (engine.RequestStage(request).stage != 75)
			return { "StageEngine.SelfTest", CheckStatus::Fail, "explicit stage request did not resolve" };
		request.requestedStage = 0;
		request.source = StageSource::Encounter;
		request.band = 50;
		request.seed = 424242;
		const auto a = engine.RequestStage(request);
		const auto b = engine.RequestStage(request);
		if (a.stage != b.stage)
			return { "StageEngine.SelfTest", CheckStatus::Fail, "request is not deterministic" };
		return { "StageEngine.SelfTest", CheckStatus::Pass,
			std::format("100 stages, 10 tiers, endpoints 1.0/15.0, deterministic request stage {}", a.stage) };
	}
}
