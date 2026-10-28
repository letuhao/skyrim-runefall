#include "Progression.h"

#include "RuneForge.h"
#include "DataStore.h"
#include "FormCatalog.h"
#include "FormResolver.h"

#include "CoSave.h"

namespace Runefall
{
	namespace
	{
		std::string SlugPascal(std::string_view a_value)
		{
			std::string slug;
			slug.reserve(a_value.size());
			for (const auto c : a_value) {
				const auto lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				slug.push_back((std::isalnum(static_cast<unsigned char>(lower)) != 0) ? lower : '_');
			}
			while (!slug.empty() && slug.front() == '_')
				slug.erase(slug.begin());
			while (!slug.empty() && slug.back() == '_')
				slug.pop_back();
			std::string out;
			bool capitalize = true;
			for (const auto c : slug) {
				if (c == '_') {
					capitalize = true;
					continue;
				}
				out.push_back(capitalize ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c);
				capitalize = false;
			}
			return out;
		}

		const std::array<std::string_view, 9> kAttributeNames = {
			"Might", "Endurance", "Precision", "Agility", "Guile",
			"Cunning", "Intellect", "Wisdom", "Willpower"
		};
	}

	Progression& Progression::Get()
	{
		static Progression singleton;
		return singleton;
	}

	void Progression::LoadPlayerProgression()
	{
		pointsPerLevel = 3;
		maxRuneLevel = 100;
		breakthroughBonuses = { 9, 12, 15, 18, 21, 24, 27, 30, 33, 36 };
		const auto* table = DataStore::Get().GetTable("player_progression.csv");
		if (!table || table->rows.empty())
			return;
		const auto& row = table->rows.front();
		auto cell = [&](std::size_t index) -> std::string_view {
			return index < row.size() ? row[index] : std::string_view{};
		};
		pointsPerLevel = static_cast<std::int32_t>(std::strtol(std::string{ cell(0) }.c_str(), nullptr, 10));
		maxRuneLevel = static_cast<std::int32_t>(std::strtol(std::string{ cell(1) }.c_str(), nullptr, 10));
		breakthroughBonuses.clear();
		std::stringstream stream{ std::string{ cell(2) } };
		std::string token;
		while (std::getline(stream, token, '|')) {
			if (!token.empty())
				breakthroughBonuses.push_back(static_cast<std::int32_t>(std::strtol(token.c_str(), nullptr, 10)));
		}
	}

	void Progression::LoadAttributeDefinitions()
	{
		attributeEffects.clear();
		const auto* table = DataStore::Get().GetTable("attribute_effects.csv");
		if (table) {
			int cId = -1, cKey = -1, cPer = -1;
			for (std::size_t i = 0; i < table->headers.size(); ++i) {
				if (table->headers[i] == "attributeId") cId = static_cast<int>(i);
				else if (table->headers[i] == "effectKey") cKey = static_cast<int>(i);
				else if (table->headers[i] == "perPoint") cPer = static_cast<int>(i);
			}
			for (const auto& row : table->rows) {
				if (cId < 0 || cKey < 0 || cPer < 0)
					break;
				auto cell = [&](int col) -> std::string_view {
					return static_cast<std::size_t>(col) < row.size() ? row[static_cast<std::size_t>(col)] : std::string_view{};
				};
				const auto id = std::string{ cell(cId) };
				auto it = std::find(kAttributeNames.begin(), kAttributeNames.end(), id);
				if (it == kAttributeNames.end())
					continue;
				AttributeEffect effect;
				effect.attributeIndex = static_cast<std::size_t>(std::distance(kAttributeNames.begin(), it));
				effect.effectKey = std::string{ cell(cKey) };
				effect.perPoint = std::strtof(std::string{ cell(cPer) }.c_str(), nullptr);
				attributeEffects.push_back(std::move(effect));
			}
		}
		attributeEffectCount = attributeEffects.size();
	}

	float Progression::DerivedValue(std::string_view a_effectKey) const
	{
		float total = 0.0f;
		for (const auto& effect : attributeEffects) {
			if (effect.effectKey != a_effectKey || effect.attributeIndex >= attributes.size())
				continue;
			total += static_cast<float>(attributes[effect.attributeIndex]) * effect.perPoint;
		}
		return total;
	}

	std::vector<std::string> Progression::DerivedKeys() const
	{
		std::vector<std::string> keys;
		for (const auto& effect : attributeEffects) {
			if (std::find(keys.begin(), keys.end(), effect.effectKey) == keys.end())
				keys.push_back(effect.effectKey);
		}
		return keys;
	}

	void Progression::SyncGlobals()
	{
		if (!FormCatalog::Get().Loaded())
			return;
		static const char* kGlobals[9] = {
			"RF_GLOB_Attr_Might", "RF_GLOB_Attr_Endurance", "RF_GLOB_Attr_Precision",
			"RF_GLOB_Attr_Agility", "RF_GLOB_Attr_Guile", "RF_GLOB_Attr_Cunning",
			"RF_GLOB_Attr_Intellect", "RF_GLOB_Attr_Wisdom", "RF_GLOB_Attr_Willpower"
		};
		for (std::size_t i = 0; i < attributes.size() && i < 9; ++i) {
			if (auto* global = FormCatalog::Get().LookupAs<RE::TESGlobal>(kGlobals[i]))
				global->value = static_cast<float>(attributes[i]);
			else
				logger::warn("progression: global '{}' unresolved", kGlobals[i]);
		}
		if (auto* tier = FormCatalog::Get().LookupAs<RE::TESGlobal>("RF_GLOB_State_RuneTier"))
			tier->value = static_cast<float>(runeTier);
		const auto keys = DerivedKeys();
		for (std::size_t i = 0; i < keys.size(); ++i) {
			const auto editorID = std::format("RF_GLOB_Derived_{}", SlugPascal(keys[i]));
			if (auto* derived = FormCatalog::Get().LookupAs<RE::TESGlobal>(editorID))
				derived->value = DerivedValue(keys[i]);
			else
				logger::warn("progression: derived global '{}' unresolved", editorID);
		}
	}
	void Progression::LoadFromCoSave()
	{
		LoadAttributeDefinitions();
		LoadPlayerProgression();
		const auto& state = CoSave::Get().State();
		attributes = state.attributes;
		attributePoints = state.attributePoints;
		skillPoints = state.skillPoints;
		runeTier = std::clamp(state.runeTier, 0, 5);
		bossKillTier = state.bossKillTier;
		SyncGlobals();
	}

	void Progression::SaveToCoSave()
	{
		auto& state = CoSave::Get().State();
		state.attributes = attributes;
		state.attributePoints = attributePoints;
		state.skillPoints = skillPoints;
		state.runeTier = runeTier;
		state.bossKillTier = bossKillTier;
		SyncGlobals();
	}

	std::int32_t Progression::Attribute(std::size_t a_index) const
	{
		return a_index < attributes.size() ? attributes[a_index] : 0;
	}

	std::int32_t Progression::SkillRank(std::string_view a_skill) const
	{
		for (const auto& entry : skills) {
			if (entry.skill == a_skill)
				return entry.rank;
		}
		return 0;
	}

	bool Progression::SpendAttributePoint(std::size_t a_index)
	{
		if (a_index >= attributes.size() || attributePoints <= 0 || attributes[a_index] >= 60)
			return false;
		++attributes[a_index];
		--attributePoints;
		SaveToCoSave();
		return true;
	}

	bool Progression::SpendSkillPoint(std::string_view a_skill)
	{
		if (skillPoints <= 0)
			return false;
		for (auto& entry : skills) {
			if (entry.skill == a_skill) {
				if (entry.rank >= 3)
					return false;
				++entry.rank;
				--skillPoints;
				SaveToCoSave();
				return true;
			}
		}
		skills.push_back({ std::string{ a_skill }, 1 });
		--skillPoints;
		SaveToCoSave();
		return true;
	}

	bool Progression::AddAttributePoint(std::int32_t a_amount)
	{
		if (a_amount <= 0)
			return false;
		attributePoints += a_amount;
		SaveToCoSave();
		return true;
	}

	bool Progression::AddSkillPoint(std::int32_t a_amount)
	{
		if (a_amount <= 0)
			return false;
		skillPoints += a_amount;
		SaveToCoSave();
		return true;
	}

	bool Progression::RespecAttributes(std::int32_t a_runeCost)
	{
		if (a_runeCost < 0)
			return false;
		const std::int32_t refund = std::accumulate(attributes.begin(), attributes.end(), 0);
		attributes.fill(0);
		attributePoints += refund;
		SaveToCoSave();
		return true;
	}

	bool Progression::RespecSkills(std::int32_t a_runeCost)
	{
		if (a_runeCost < 0)
			return false;
		const std::int32_t refund = std::accumulate(skills.begin(), skills.end(), 0, [](std::int32_t sum, const SkillProgress& entry) { return sum + entry.rank; });
		skills.clear();
		skillPoints += refund;
		SaveToCoSave();
		return true;
	}

	std::int32_t Progression::BreakthroughBonus(std::int32_t a_tier) const
	{
		if (a_tier < 1 || a_tier > static_cast<std::int32_t>(breakthroughBonuses.size()))
			return 0;
		return breakthroughBonuses[static_cast<std::size_t>(a_tier - 1)];
	}

	std::int32_t Progression::AttributeAwardForStageAdvance(std::int32_t a_newStage) const
	{
		if (a_newStage < 1 || a_newStage > maxRuneLevel)
			return 0;
		auto award = pointsPerLevel;
		if (a_newStage % 10 == 0)
			award += BreakthroughBonus(a_newStage / 10);
		return award;
	}

	bool Progression::AwardStageAdvance(std::int32_t a_newStage)
	{
		const auto award = AttributeAwardForStageAdvance(a_newStage);
		if (award <= 0)
			return false;
		return AddAttributePoint(award);
	}

	std::int32_t Progression::SoftCappedAttribute(std::int32_t a_raw)
	{
		if (a_raw <= 30)
			return a_raw;
		if (a_raw <= 60)
			return 30 + ((a_raw - 30) / 2);
		return 60;
	}

	void Progression::MarkBossKill(std::int32_t a_runeTier)
	{
		if (a_runeTier > bossKillTier) {
			bossKillTier = a_runeTier;
			logger::info("progression: boss kill recorded for rune tier {}", a_runeTier);
		}
	}

	std::int32_t Progression::TryAttune()
	{
		if (runeTier >= 5)
			return 2;
		if (!BossKillReady())
			return 3;
		const auto cost = 25 * (runeTier + 1);
		if (!RuneForge::SpendUnits(cost))
			return 1;
		++runeTier;
		bossKillTier = -1;
		SaveToCoSave();
		logger::info("progression: attuned to rune tier {} (cost {} units)", runeTier, cost);
		return 0;
	}

	SelfTestResult Progression::SelfTest()
	{
		Progression model;
		model.LoadPlayerProgression();
		if (model.PointsPerLevel() != 3 || model.MaxRuneLevel() != 100)
			return { "Progression.SelfTest", CheckStatus::Fail, "player progression defaults are wrong" };
		if (model.AttributeAwardForStageAdvance(10) != 12 || model.AttributeAwardForStageAdvance(20) != 15 || model.AttributeAwardForStageAdvance(7) != 3)
			return { "Progression.SelfTest", CheckStatus::Fail, "attribute award formula is wrong" };
		model.attributePoints = 3;
		model.skillPoints = 2;
		for (int i = 0; i < 3; ++i) {
			if (!model.SpendAttributePoint(0))
				return { "Progression.SelfTest", CheckStatus::Fail, "attribute spend failed" };
		}
		if (model.Attribute(0) != 3 || model.AttributePoints() != 0)
			return { "Progression.SelfTest", CheckStatus::Fail, "attribute accounting mismatch" };
		if (!model.AddAttributePoint(2) || !model.SpendAttributePoint(0) || model.Attribute(0) != 4)
			return { "Progression.SelfTest", CheckStatus::Fail, "add/spend attribute mismatch" };

		if (!model.SpendSkillPoint("Cleave") || !model.SpendSkillPoint("Cleave") || model.SkillRank("Cleave") != 2)
			return { "Progression.SelfTest", CheckStatus::Fail, "skill rank accounting mismatch" };
		if (model.SkillRank("Missing") != 0)
			return { "Progression.SelfTest", CheckStatus::Fail, "unknown skill rank should be 0" };

		const auto soft = SoftCappedAttribute(45);
		if (soft != 30 + (45 - 30) / 2)
			return { "Progression.SelfTest", CheckStatus::Fail, "soft cap formula mismatch" };

		model.RespecAttributes(0);
		if (model.Attribute(0) != 0 || model.AttributePoints() < 4)
			return { "Progression.SelfTest", CheckStatus::Fail, "attribute respec failed" };
		model.RespecSkills(0);
		if (model.SkillRank("Cleave") != 0 || model.SkillPoints() < 2)
			return { "Progression.SelfTest", CheckStatus::Fail, "skill respec failed" };

		return { "Progression.SelfTest", CheckStatus::Pass, "spend/add/respec/softcap accounting" };
	}
}