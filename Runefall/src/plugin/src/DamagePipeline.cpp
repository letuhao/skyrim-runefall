#include "DamagePipeline.h"

#include <format>

namespace Runefall
{
	DamagePipeline& DamagePipeline::Get()
	{
		static DamagePipeline singleton;
		return singleton;
	}

	DamageCategory DamagePipeline::Classify(RE::FormID a_target, RE::FormID a_attacker, float a_damage, std::uint16_t a_flags)
	{
		if (a_target == 0 || a_damage <= 0.0f)
			return DamageCategory::Invalid;
		if ((a_flags & kDamageFlagEnvironmental) != 0 || a_attacker == 0)
			return DamageCategory::Environmental;
		if ((a_flags & kDamageFlagReflected) != 0 || a_attacker == a_target)
			return DamageCategory::Reflected;
		if ((a_flags & kDamageFlagDot) != 0)
			return DamageCategory::Dot;
		return DamageCategory::Direct;
	}

	void DamagePipeline::Record(RE::FormID a_target, RE::FormID a_attacker, float a_damage, std::uint16_t a_flags)
	{
		const auto count = samples.fetch_add(1) + 1;
		switch (Classify(a_target, a_attacker, a_damage, a_flags)) {
		case DamageCategory::Direct: directSamples.fetch_add(1); break;
		case DamageCategory::Dot: dotSamples.fetch_add(1); break;
		case DamageCategory::Reflected: reflectedSamples.fetch_add(1); break;
		case DamageCategory::Environmental: environmentalSamples.fetch_add(1); break;
		default: invalidSamples.fetch_add(1); break;
		}
		if (count <= 10 || count % 500 == 0)
			logger::info("damage-pipeline: sample #{} target={:08X} attacker={:08X} damage={:.2f} category={} flags={}",
				count, a_target, a_attacker, a_damage, static_cast<int>(Classify(a_target, a_attacker, a_damage, a_flags)), a_flags);
	}

	SelfTestResult DamagePipeline::SelfTest()
	{
		if (Classify(0x14, 0x100, 10.0f, kDamageFlagNone) != DamageCategory::Direct)
			return { "DamagePipeline.SelfTest", CheckStatus::Fail, "direct classification failed" };
		if (Classify(0x14, 0x100, 10.0f, kDamageFlagDot) != DamageCategory::Dot)
			return { "DamagePipeline.SelfTest", CheckStatus::Fail, "DoT classification failed" };
		if (Classify(0x14, 0x14, 10.0f, kDamageFlagNone) != DamageCategory::Reflected)
			return { "DamagePipeline.SelfTest", CheckStatus::Fail, "reflected classification failed" };
		if (Classify(0x14, 0, 10.0f, kDamageFlagNone) != DamageCategory::Environmental)
			return { "DamagePipeline.SelfTest", CheckStatus::Fail, "environmental classification failed" };
		if (Classify(0, 0x100, 10.0f, kDamageFlagNone) != DamageCategory::Invalid)
			return { "DamagePipeline.SelfTest", CheckStatus::Fail, "invalid classification failed" };
		return { "DamagePipeline.SelfTest", CheckStatus::Pass, "direct/DoT/reflected/environmental/invalid classification; TESHitEvent sink records direct hits" };
	}
}
