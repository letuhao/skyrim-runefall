#pragma once
#include "Core.h"

namespace Runefall
{
	class CombatRatingEngine
	{
	public:
		static CombatRatingEngine& Get();

		void Load();
		[[nodiscard]] bool Loaded() const noexcept { return loaded; }
		[[nodiscard]] float ReferenceDPS(std::uint8_t a_stage) const;
		[[nodiscard]] float ReferenceEHP(std::uint8_t a_stage) const;
		[[nodiscard]] float PCR(float a_offenseDPS, float a_effectiveHP, float a_utility, std::uint8_t a_stage) const;
		[[nodiscard]] float NCR(float a_offenseDPS, float a_effectiveHP, float a_utility, std::uint8_t a_ratingStage) const;
		[[nodiscard]] float RCR(float a_offenseDPS, float a_effectiveHP, float a_utility, std::uint8_t a_runeStage) const;
		[[nodiscard]] static float TTK(float a_targetEffectiveHP, float a_attackerEffectiveDPS);
		[[nodiscard]] static float TTD(float a_attackerEffectiveHP, float a_targetEffectiveDPS);
		[[nodiscard]] static float ChallengeRatio(float a_ttk, float a_ttd);
		[[nodiscard]] std::string ThreatLabel(float a_ratingOrRatio, bool a_isRatio) const;
		[[nodiscard]] std::size_t PaceProfileCount() const noexcept { return paceProfiles.size(); }
		[[nodiscard]] std::size_t DisplayProfileCount() const noexcept { return displayProfiles.size(); }
		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		struct PaceBand
		{
			std::string profileId;
			std::string encounterClass;
			float ttkMin = 0, ttkMax = 0, ttdMin = 0, ttdMax = 0, qMin = 0, qMax = 0;
		};
		struct DisplayProfile
		{
			std::string profileId;
			bool numberVisible = true;
			bool labelVisible = true;
			int decimals = 0;
		};

		float baseDPS = 10.0f;
		float baseEHP = 150.0f;
		std::vector<PaceBand> paceProfiles;
		std::vector<DisplayProfile> displayProfiles;
		bool loaded = false;
	};
}
