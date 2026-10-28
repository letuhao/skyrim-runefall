#include "DamagePipeline.h"

#include <cmath>
#include <format>

#include "RE/A/Actor.h"
#include "RE/C/Character.h"
#include "RE/P/PlayerCharacter.h"

#include "CoSave.h"
#include "LootService.h"
#include "DataStore.h"
#include "REL/Relocation.h"

namespace
{
	using DamageFn = void (*)(RE::Actor*, RE::Actor*, float);
	DamageFn g_originalActor = nullptr;
	using CharacterFn = void (*)(RE::Character*, RE::Actor*, float);
	CharacterFn g_originalCharacter = nullptr;
	using PlayerFn = void (*)(RE::PlayerCharacter*, RE::Actor*, float);
	PlayerFn g_originalPlayer = nullptr;

	template <class T, class Fn>
	void CallAndRecord(T* a_self, RE::Actor* a_attacker, float a_damage, Fn a_original, bool a_player)
	{
		const auto target = a_self ? a_self->GetFormID() : 0;
		const auto attacker = a_attacker ? a_attacker->GetFormID() : 0;
		const std::uint16_t flags = attacker == 0 ? Runefall::kDamageFlagEnvironmental :
			(attacker == target ? Runefall::kDamageFlagReflected : Runefall::kDamageFlagNone);
		const auto adjusted = Runefall::DamagePipeline::Get().AdjustDamage(target, attacker, a_damage, flags);
		if (a_original)
			a_original(a_self, a_attacker, adjusted);
		Runefall::DamagePipeline::Get().Record(target, attacker, adjusted, flags);
		(void)a_player;
	}

	void ThunkActor(RE::Actor* a_self, RE::Actor* a_attacker, float a_damage)
	{
		CallAndRecord(a_self, a_attacker, a_damage, g_originalActor, false);
	}

	void ThunkCharacter(RE::Character* a_self, RE::Actor* a_attacker, float a_damage)
	{
		CallAndRecord(a_self, a_attacker, a_damage, g_originalCharacter, false);
	}

	void ThunkPlayer(RE::PlayerCharacter* a_self, RE::Actor* a_attacker, float a_damage)
	{
		CallAndRecord(a_self, a_attacker, a_damage, g_originalPlayer, true);
	}
}



namespace Runefall
{
	DamagePipeline& DamagePipeline::Get()
	{
		static DamagePipeline singleton;
		return singleton;
	}

	bool DamagePipeline::Install()
	{
		if (installed)
			return true;
		try {
			const auto index = REL::Module::IsVR() ? 0x106u : 0x104u;
			bool any = false;
			{
				static REL::Relocation<std::uintptr_t> table{ RE::VTABLE_Actor[0] };
				if (table.address() != 0) {
					const auto original = table.write_vfunc(index, reinterpret_cast<std::uintptr_t>(&ThunkActor));
					if (original) {
						g_originalActor = reinterpret_cast<DamageFn>(original);
						any = true;
					}
				}
			}
			{
				static REL::Relocation<std::uintptr_t> table{ RE::VTABLE_Character[0] };
				if (table.address() != 0) {
					const auto original = table.write_vfunc(index, reinterpret_cast<std::uintptr_t>(&ThunkCharacter));
					if (original) {
						g_originalCharacter = reinterpret_cast<CharacterFn>(original);
						any = true;
					}
				}
			}
			{
				static REL::Relocation<std::uintptr_t> table{ RE::VTABLE_PlayerCharacter[0] };
				if (table.address() != 0) {
					const auto original = table.write_vfunc(index, reinterpret_cast<std::uintptr_t>(&ThunkPlayer));
					if (original) {
						g_originalPlayer = reinterpret_cast<PlayerFn>(original);
						any = true;
					}
				}
			}
			installed = any;
			if (any)
				logger::info("damage-pipeline: installed HandleHealthDamage hooks (Actor/Character/PlayerCharacter, slot 0x{:X})", index);
			else
				logger::warn("damage-pipeline: HandleHealthDamage hook unavailable; TESHitEvent sink remains");
			return any;
		} catch (const std::exception& e) {
			logger::error("damage-pipeline: hook install failed: {}", e.what());
			return false;
		}
	}

	void DamagePipeline::LoadProtection()
	{
		protectionLoaded = false;
		const auto* table = DataStore::Get().GetTable("threat_protection.csv");
		if (!table || table->rows.empty()) {
			logger::error("damage-pipeline: threat_protection.csv missing; protection disabled");
			return;
		}
		const auto find = [&](std::string_view name) {
			for (std::size_t i = 0; i < table->headers.size(); ++i)
				if (table->headers[i] == name)
					return static_cast<int>(i);
			return -1;
		};
		const auto& row = table->rows.front();
		const auto cell = [&](std::string_view name) {
			const auto index = find(name);
			return index >= 0 && static_cast<std::size_t>(index) < row.size() ? row[static_cast<std::size_t>(index)] : std::string{};
		};
		const auto toFloat = [](const std::string& text, float fallback) {
			char* end = nullptr;
			const auto value = std::strtof(text.c_str(), &end);
			return end == text.c_str() ? fallback : value;
		};
		const auto toInt = [](const std::string& text, int fallback) {
			char* end = nullptr;
			const auto value = std::strtol(text.c_str(), &end, 10);
			return end == text.c_str() ? fallback : static_cast<int>(value);
		};
		protectionCap = toFloat(cell("cap"), 0.70f);
		protectionPerStage = toFloat(cell("perStage"), 0.03f);
		protectionFloor = toFloat(cell("floor"), 0.05f);
		protectionDotFactor = toFloat(cell("dotFactor"), 0.50f);
		playerProtected = cell("playerProtected") == "1";
		anomalyProtected = cell("anomalyProtected") == "1";
		penetrationCap = toInt(cell("penetrationCap"), 20);
		protectionEnabled = cell("enabled") == "1";
		protectionLoaded = true;
		logger::info("damage-pipeline: protection loaded cap={} perStage={} floor={} dotFactor={} playerProtected={} anomalyProtected={}",
			protectionCap, protectionPerStage, protectionFloor, protectionDotFactor, playerProtected, anomalyProtected);
	}


	void DamagePipeline::LoadPenetration()
	{
		const auto* table = DataStore::Get().GetTable("penetration_profiles.csv");
		if (!table || table->rows.empty())
			return;
		const auto find = [&](std::string_view name) {
			for (std::size_t i = 0; i < table->headers.size(); ++i)
				if (table->headers[i] == name)
					return static_cast<int>(i);
			return -1;
		};
		const auto& row = table->rows.front();
		const auto cell = [&](std::string_view name) {
			const auto index = find(name);
			return index >= 0 && static_cast<std::size_t>(index) < row.size() ? row[static_cast<std::size_t>(index)] : std::string{};
		};
		penetrationBase = static_cast<int>(std::strtol(cell("base").c_str(), nullptr, 10));
		penetrationPer10Stages = static_cast<int>(std::strtol(cell("per10Stages").c_str(), nullptr, 10));
		penetrationCap = static_cast<int>(std::strtol(cell("cap").c_str(), nullptr, 10));
		penetrationMage = cell("mageChannel") == "1";
		penetrationWeapon = cell("weaponChannel") == "1";
		penetrationConsumable = cell("consumableChannel") == "1";
		logger::info("damage-pipeline: penetration loaded base={} per10={} cap={} mage={} weapon={}",
			penetrationBase, penetrationPer10Stages, penetrationCap, penetrationMage, penetrationWeapon);
	}

	int DamagePipeline::PlayerPenetration() const
	{
		const auto stage = static_cast<int>(CoSave::Get().State().playerStage);
		const auto raw = penetrationBase + (std::max)(0, stage / 10) * penetrationPer10Stages;
		return (std::clamp)(raw, 0, penetrationCap);
	}

	float DamagePipeline::ComputeMultiplier(int a_targetStage, int a_attackerStage, std::uint16_t a_flags, int a_penetration) const
	{
		if (!protectionLoaded || !protectionEnabled || a_targetStage <= 0 || a_attackerStage <= 0)
			return 1.0f;
		if ((a_flags & kDamageFlagEnvironmental) != 0 || (a_flags & kDamageFlagReflected) != 0)
			return 1.0f;
		const auto penetration = (std::clamp)(a_penetration, 0, penetrationCap);
		const auto gap = (std::max)(0, a_targetStage - a_attackerStage - penetration);
		if (gap <= 0)
			return 1.0f;
		auto dr = (std::min)(protectionCap, protectionPerStage * static_cast<float>(gap));
		if ((a_flags & kDamageFlagDot) != 0)
			dr *= protectionDotFactor;
		dr = (std::max)(dr, 0.0f);
		return (std::max)(1.0f - dr, protectionFloor);
	}

	float DamagePipeline::AdjustDamage(RE::FormID a_target, RE::FormID a_attacker, float a_damage, std::uint16_t a_flags) const
	{
		if (!protectionLoaded || a_damage == 0.0f)
			return a_damage;
		if (LootService::Get().IsAnomalyFormID(a_target))
			return a_damage;
		const bool targetIsPlayer = a_target == 0x14;
		if (targetIsPlayer && !playerProtected)
			return a_damage;
		const auto stageOf = [](RE::FormID a_formID) -> int {
			if (a_formID == 0x14)
				return static_cast<int>(CoSave::Get().State().playerStage);
			return static_cast<int>(LootService::Get().StageForFormID(a_formID));
		};
		const auto targetStage = stageOf(a_target);
		const auto attackerStage = stageOf(a_attacker);
		if (targetStage <= 0 || attackerStage <= 0)
			return a_damage;
		const auto penetration = a_attacker == 0x14 ? PlayerPenetration() : 0;
		const auto multiplier = ComputeMultiplier(targetStage, attackerStage, a_flags, penetration);
		return a_damage * multiplier;
	}

	DamageCategory DamagePipeline::Classify(RE::FormID a_target, RE::FormID a_attacker, float a_damage, std::uint16_t a_flags)
	{
		if (a_target == 0 || a_damage == 0.0f)
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
		a_damage = std::fabs(a_damage);
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
		const auto& protection = Get();
		const auto noGap = protection.ComputeMultiplier(10, 10, kDamageFlagNone, 0);
		const auto gap10 = protection.ComputeMultiplier(11, 1, kDamageFlagNone, 0);
		const auto capped = protection.ComputeMultiplier(50, 1, kDamageFlagNone, 0);
		const auto dot = protection.ComputeMultiplier(11, 1, kDamageFlagDot, 0);
		const auto bypass = protection.ComputeMultiplier(50, 1, kDamageFlagReflected, 0);
		const auto gap20 = protection.ComputeMultiplier(21, 1, kDamageFlagNone, 0);
		const auto gap20pen10 = protection.ComputeMultiplier(21, 1, kDamageFlagNone, 10);
		if (std::fabs(noGap - 1.0f) > 1e-4f || std::fabs(gap10 - 0.70f) > 1e-3f ||
			std::fabs(capped - 0.30f) > 1e-3f || std::fabs(dot - 0.85f) > 1e-3f || std::fabs(bypass - 1.0f) > 1e-4f ||
			std::fabs(gap20 - 0.40f) > 1e-3f || std::fabs(gap20pen10 - 0.70f) > 1e-3f)
			return { "DamagePipeline.SelfTest", CheckStatus::Fail, "protection multiplier output is wrong" };
		return { "DamagePipeline.SelfTest", CheckStatus::Pass, "classification + stage-gap protection (cap 70%, 3%/stage, 5% floor, DoT 50%)" };
	}
}
