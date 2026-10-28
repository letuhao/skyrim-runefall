#include "CombatRatingEngine.h"

#include "DataStore.h"
#include "StageEngine.h"

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

		float Float(const std::vector<std::string>& a_row, int a_column, float a_default = 0.0f)
		{
			const auto text = Cell(a_row, a_column);
			if (text.empty())
				return a_default;
			return std::strtof(text.c_str(), nullptr);
		}

		bool Bool(const std::vector<std::string>& a_row, int a_column)
		{
			return Cell(a_row, a_column) == "1";
		}
	}

	CombatRatingEngine& CombatRatingEngine::Get()
	{
		static CombatRatingEngine singleton;
		return singleton;
	}

	void CombatRatingEngine::Load()
	{
		baseDPS = 10.0f;
		baseEHP = 150.0f;
		paceProfiles.clear();
		displayProfiles.clear();
		loaded = false;

		if (const auto* ratings = DataStore::Get().GetTable("rating_profiles.csv"); ratings && !ratings->rows.empty()) {
			const int pId = FindColumn(*ratings, "profileId");
			const int pDps = FindColumn(*ratings, "baseDPS");
			const int pEhp = FindColumn(*ratings, "baseEHP");
			const auto& row = ratings->rows.front();
			baseDPS = Float(row, pDps, 10.0f);
			baseEHP = Float(row, pEhp, 150.0f);
			(void)pId;
		}
		if (const auto* pace = DataStore::Get().GetTable("combat_pace_profiles.csv")) {
			const int pProfile = FindColumn(*pace, "profileId");
			const int pClass = FindColumn(*pace, "encounterClass");
			const int pTtkMin = FindColumn(*pace, "ttkMin");
			const int pTtkMax = FindColumn(*pace, "ttkMax");
			const int pTtdMin = FindColumn(*pace, "ttdMin");
			const int pTtdMax = FindColumn(*pace, "ttdMax");
			const int pQMin = FindColumn(*pace, "qMin");
			const int pQMax = FindColumn(*pace, "qMax");
			for (const auto& row : pace->rows) {
				PaceBand band;
				band.profileId = Cell(row, pProfile);
				band.encounterClass = Cell(row, pClass);
				band.ttkMin = Float(row, pTtkMin);
				band.ttkMax = Float(row, pTtkMax);
				band.ttdMin = Float(row, pTtdMin);
				band.ttdMax = Float(row, pTtdMax);
				band.qMin = Float(row, pQMin);
				band.qMax = Float(row, pQMax);
				if (!band.profileId.empty())
					paceProfiles.push_back(std::move(band));
			}
		}
		if (const auto* display = DataStore::Get().GetTable("rating_display_profiles.csv")) {
			const int pId = FindColumn(*display, "profileId");
			const int pNumber = FindColumn(*display, "numberVisible");
			const int pLabel = FindColumn(*display, "labelVisible");
			const int pDecimals = FindColumn(*display, "decimals");
			for (const auto& row : display->rows) {
				DisplayProfile profile;
				profile.profileId = Cell(row, pId);
				profile.numberVisible = Bool(row, pNumber);
				profile.labelVisible = Bool(row, pLabel);
				profile.decimals = static_cast<int>(std::strtol(Cell(row, pDecimals).c_str(), nullptr, 10));
				if (!profile.profileId.empty())
					displayProfiles.push_back(std::move(profile));
			}
		}
		loaded = !paceProfiles.empty() && !displayProfiles.empty();
		if (loaded)
			logger::info("combat-ratings: loaded baseDPS={} baseEHP={} paceBands={} displayProfiles={}", baseDPS, baseEHP, paceProfiles.size(), displayProfiles.size());
		else
			logger::error("combat-ratings: rating/pace/display tables missing");
	}

	float CombatRatingEngine::ReferenceDPS(std::uint8_t a_stage) const
	{
		return baseDPS * static_cast<float>(StageEngine::Get().Capacity(a_stage, CapacityKind::Offense)) / 100.0f;
	}

	float CombatRatingEngine::ReferenceEHP(std::uint8_t a_stage) const
	{
		return baseEHP * static_cast<float>(StageEngine::Get().Capacity(a_stage, CapacityKind::Defense)) / 100.0f;
	}

	float CombatRatingEngine::PCR(float a_offenseDPS, float a_effectiveHP, float a_utility, std::uint8_t a_stage) const
	{
		const auto refDps = ReferenceDPS(a_stage);
		const auto refEhp = ReferenceEHP(a_stage);
		if (refDps <= 0.0f || refEhp <= 0.0f || a_offenseDPS <= 0.0f || a_effectiveHP <= 0.0f)
			return 0.0f;
		return 100.0f * std::sqrt((a_offenseDPS / refDps) * (a_effectiveHP / refEhp)) * a_utility;
	}

	float CombatRatingEngine::NCR(float a_offenseDPS, float a_effectiveHP, float a_utility, std::uint8_t a_ratingStage) const
	{
		return PCR(a_offenseDPS, a_effectiveHP, a_utility, a_ratingStage);
	}

	float CombatRatingEngine::RCR(float a_offenseDPS, float a_effectiveHP, float a_utility, std::uint8_t a_runeStage) const
	{
		return PCR(a_offenseDPS, a_effectiveHP, a_utility, a_runeStage);
	}

	float CombatRatingEngine::TTK(float a_targetEffectiveHP, float a_attackerEffectiveDPS)
	{
		return a_attackerEffectiveDPS > 0.0f ? a_targetEffectiveHP / a_attackerEffectiveDPS : 0.0f;
	}

	float CombatRatingEngine::TTD(float a_attackerEffectiveHP, float a_targetEffectiveDPS)
	{
		return a_targetEffectiveDPS > 0.0f ? a_attackerEffectiveHP / a_targetEffectiveDPS : 0.0f;
	}

	float CombatRatingEngine::ChallengeRatio(float a_ttk, float a_ttd)
	{
		return a_ttd > 0.0f ? a_ttk / a_ttd : 0.0f;
	}

	std::string CombatRatingEngine::ThreatLabel(float a_ratingOrRatio, bool a_isRatio) const
	{
		const auto value = a_isRatio ? a_ratingOrRatio : a_ratingOrRatio;
		if (value < 50.0f) return "Trivial";
		if (value < 80.0f) return "Manageable";
		if (value < 120.0f) return "Fair";
		if (value < 180.0f) return "Dangerous";
		return "Lethal";
	}

	SelfTestResult CombatRatingEngine::SelfTest()
	{
		auto& engine = Get();
		if (!engine.Loaded())
			return { "CombatRatingEngine.SelfTest", CheckStatus::Fail, "rating data not loaded" };
		if (engine.PaceProfileCount() < 10 || engine.DisplayProfileCount() < 1)
			return { "CombatRatingEngine.SelfTest", CheckStatus::Fail, "pace/display data coverage too small" };
		const auto stage = static_cast<std::uint8_t>(10);
		const auto refDps = engine.ReferenceDPS(stage);
		const auto refEhp = engine.ReferenceEHP(stage);
		if (refDps <= 0.0f || refEhp <= 0.0f)
			return { "CombatRatingEngine.SelfTest", CheckStatus::Fail, "reference stats are zero" };
		if (std::fabs(engine.PCR(refDps, refEhp, 1.0f, stage) - 100.0f) > 0.01f)
			return { "CombatRatingEngine.SelfTest", CheckStatus::Fail, "baseline PCR is not 100" };
		if (engine.PCR(refDps * 2.0f, refEhp, 1.0f, stage) <= 100.0f)
			return { "CombatRatingEngine.SelfTest", CheckStatus::Fail, "PCR does not scale with offense" };
		if (std::fabs(TTK(100.0f, 20.0f) - 5.0f) > 0.001f || std::fabs(TTD(50.0f, 10.0f) - 5.0f) > 0.001f || std::fabs(ChallengeRatio(5.0f, 5.0f) - 1.0f) > 0.001f)
			return { "CombatRatingEngine.SelfTest", CheckStatus::Fail, "TTK/TTD/Q math is wrong" };
		if (engine.ThreatLabel(150.0f, false) != "Dangerous")
			return { "CombatRatingEngine.SelfTest", CheckStatus::Fail, "threat label mapping is wrong" };
		return { "CombatRatingEngine.SelfTest", CheckStatus::Pass,
			std::format("PCR baseline 100 at stage {}, {} pace bands, {} display profiles, TTK/TTD/Q OK", static_cast<int>(stage), engine.PaceProfileCount(), engine.DisplayProfileCount()) };
	}
}
