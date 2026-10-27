#include "Progression.h"

#include "RuneForge.h"
#include "DataStore.h"

#include "CoSave.h"

namespace Runefall
{
	namespace
	{
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

	void Progression::LoadAttributeDefinitions()
	{
		const auto* table = DataStore::Get().GetTable("attribute_effects.csv");
		attributeEffectCount = table ? table->rows.size() : 0;
	}

	void Progression::LoadFromCoSave()
	{
		LoadAttributeDefinitions();
		const auto& state = CoSave::Get().State();
		attributes = state.attributes;
		attributePoints = state.attributePoints;
		skillPoints = state.skillPoints;
		runeTier = std::clamp(state.runeTier, 0, 5);
		bossKillTier = state.bossKillTier;
	}

	void Progression::SaveToCoSave()
	{
		auto& state = CoSave::Get().State();
		state.attributes = attributes;
		state.attributePoints = attributePoints;
		state.skillPoints = skillPoints;
		state.runeTier = runeTier;
		state.bossKillTier = bossKillTier;
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