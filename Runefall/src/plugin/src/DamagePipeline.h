#pragma once
#include "Core.h"

namespace Runefall
{
	enum class DamageCategory : std::uint8_t
	{
		Direct = 0,
		Dot,
		Reflected,
		Environmental,
		Invalid
	};

	enum DamageFlags : std::uint16_t
	{
		kDamageFlagNone = 0,
		kDamageFlagDot = 1 << 0,
		kDamageFlagReflected = 1 << 1,
		kDamageFlagEnvironmental = 1 << 2
	};

	class DamagePipeline
	{
	public:
		static DamagePipeline& Get();

		[[nodiscard]] bool Install();
		void LoadProtection();
		void LoadPenetration();
		[[nodiscard]] int PlayerPenetration() const;
		[[nodiscard]] float AdjustDamage(RE::FormID a_target, RE::FormID a_attacker, float a_damage, std::uint16_t a_flags) const;
		[[nodiscard]] float ComputeMultiplier(int a_targetStage, int a_attackerStage, std::uint16_t a_flags, int a_penetration) const;
		[[nodiscard]] bool ProtectionLoaded() const noexcept { return protectionLoaded; }
		[[nodiscard]] bool Installed() const noexcept { return installed; }
		void SetHitEventFallback(bool a_enabled) noexcept { hitEventFallback.store(a_enabled); }
		[[nodiscard]] bool HitEventFallback() const noexcept { return hitEventFallback.load(); }
		void Record(RE::FormID a_target, RE::FormID a_attacker, float a_damage, std::uint16_t a_flags = kDamageFlagNone);

		[[nodiscard]] static DamageCategory Classify(RE::FormID a_target, RE::FormID a_attacker, float a_damage, std::uint16_t a_flags);
		[[nodiscard]] static SelfTestResult SelfTest();

		[[nodiscard]] std::uint32_t Samples() const noexcept { return samples; }
		[[nodiscard]] std::uint32_t DirectSamples() const noexcept { return directSamples; }
		[[nodiscard]] std::uint32_t DotSamples() const noexcept { return dotSamples; }
		[[nodiscard]] std::uint32_t ReflectedSamples() const noexcept { return reflectedSamples; }
		[[nodiscard]] std::uint32_t EnvironmentalSamples() const noexcept { return environmentalSamples; }
		[[nodiscard]] std::uint32_t InvalidSamples() const noexcept { return invalidSamples; }

	private:
		bool installed = false;
		bool protectionLoaded = false;
		float protectionCap = 0.70f;
		float protectionPerStage = 0.03f;
		float protectionFloor = 0.05f;
		float protectionDotFactor = 0.50f;
		bool playerProtected = false;
		bool anomalyProtected = false;
		int penetrationCap = 20;
		bool protectionEnabled = true;
		int penetrationBase = 1;
		int penetrationPer10Stages = 1;
		bool penetrationMage = true;
		bool penetrationWeapon = true;
		bool penetrationConsumable = false;
		std::atomic_bool hitEventFallback{ true };
		std::atomic<std::uint32_t> samples{ 0 };
		std::atomic<std::uint32_t> directSamples{ 0 };
		std::atomic<std::uint32_t> dotSamples{ 0 };
		std::atomic<std::uint32_t> reflectedSamples{ 0 };
		std::atomic<std::uint32_t> environmentalSamples{ 0 };
		std::atomic<std::uint32_t> invalidSamples{ 0 };
	};
}
