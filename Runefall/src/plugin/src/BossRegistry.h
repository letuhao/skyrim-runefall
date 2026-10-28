#pragma once
#include "Core.h"

namespace Runefall
{
	class BossRegistry
	{
	public:
		static BossRegistry& Get();
		void Load();
		[[nodiscard]] bool Loaded() const noexcept { return loaded; }
		[[nodiscard]] std::uint8_t FixedStageForActor(RE::Actor* a_actor) const;
		[[nodiscard]] bool IsBossActor(RE::Actor* a_actor) const;
		[[nodiscard]] std::uint8_t FixedStageForEditorId(std::string_view a_editorId) const;
		[[nodiscard]] std::size_t BossCount() const noexcept { return bosses.size(); }
		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		struct BossDef
		{
			std::string bossId;
			std::string actorEditorId;
			RE::FormID actorFormId = 0;
			std::uint8_t fixedStage = 1;
			bool unique = false;
			std::int32_t questBand = 0;
			std::string packageId;
			bool enabled = true;
		};
		[[nodiscard]] const BossDef* Find(std::string_view a_editorId) const;
		std::vector<BossDef> bosses;
		bool loaded = false;
	};
}
