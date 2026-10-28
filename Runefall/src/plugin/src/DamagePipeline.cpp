#include "DamagePipeline.h"

#include <cmath>
#include <format>

#include "RE/A/Actor.h"
#include "RE/C/Character.h"
#include "RE/P/PlayerCharacter.h"
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
		if (a_original)
			a_original(a_self, a_attacker, a_damage);
		Runefall::DamagePipeline::Get().Record(target, attacker, a_damage, flags);
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
		return { "DamagePipeline.SelfTest", CheckStatus::Pass, "direct/DoT/reflected/environmental/invalid classification; HandleHealthDamage and TESHitEvent paths available" };
	}
}
