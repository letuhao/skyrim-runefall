#include "SkillPerkEngine.h"

#include "DataStore.h"
#include "Progression.h"

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
			return text.empty() ? a_default : static_cast<int>(std::strtol(text.c_str(), nullptr, 10));
		}
		float Float(const std::vector<std::string>& a_row, int a_column, float a_default = 0.0f)
		{
			const auto text = Cell(a_row, a_column);
			return text.empty() ? a_default : std::strtof(text.c_str(), nullptr);
		}
	}

	SkillPerkEngine& SkillPerkEngine::Get()
	{
		static SkillPerkEngine singleton;
		return singleton;
	}

	void SkillPerkEngine::Load()
	{
		perks.clear();
		loaded = false;
		const auto* table = DataStore::Get().GetTable("skill_perks.csv");
		if (!table) {
			logger::error("skill-perks: skill_perks.csv missing");
			return;
		}
		const int pId = FindColumn(*table, "perkId");
		const int pSkill = FindColumn(*table, "skill");
		const int pRank = FindColumn(*table, "rankRequired");
		const int pEffect = FindColumn(*table, "effectKey");
		const int pMagnitude = FindColumn(*table, "magnitude");
		const int pDesc = FindColumn(*table, "descriptionKey");
		for (const auto& row : table->rows) {
			Perk perk;
			perk.perkId = Cell(row, pId);
			perk.skill = Cell(row, pSkill);
			perk.rankRequired = (std::max)(1, Int(row, pRank, 1));
			perk.effectKey = Cell(row, pEffect);
			perk.magnitude = Float(row, pMagnitude);
			perk.descriptionKey = Cell(row, pDesc);
			if (!perk.perkId.empty() && !perk.skill.empty())
				perks.push_back(std::move(perk));
		}
		loaded = !perks.empty();
		if (loaded)
			logger::info("skill-perks: loaded {} perks", perks.size());
	}

	float SkillPerkEngine::BonusFor(std::string_view a_effectKey) const
	{
		if (!loaded)
			return 0.0f;
		float total = 0.0f;
		for (const auto& perk : perks) {
			if (perk.effectKey != a_effectKey)
				continue;
			if (Progression::Get().SkillRank(perk.skill) >= perk.rankRequired)
				total += perk.magnitude;
		}
		return total;
	}

	SelfTestResult SkillPerkEngine::SelfTest()
	{
		auto& engine = Get();
		if (!engine.Loaded())
			return { "SkillPerkEngine.SelfTest", CheckStatus::Fail, "skill perks not loaded" };
		if (engine.PerkCount() < 3)
			return { "SkillPerkEngine.SelfTest", CheckStatus::Fail, "perk coverage too small" };
		const auto hasEffect = [&](std::string_view key) {
			for (const auto& perk : engine.perks)
				if (perk.effectKey == key && perk.magnitude > 0.0f)
					return true;
			return false;
		};
		if (!hasEffect("skill.cleave.power") || !hasEffect("skill.block.mitigation"))
			return { "SkillPerkEngine.SelfTest", CheckStatus::Fail, "expected perk effects missing" };
		return { "SkillPerkEngine.SelfTest", CheckStatus::Pass,
			std::format("{} perks; cleave/block effect keys present", engine.PerkCount()) };
	}
}
