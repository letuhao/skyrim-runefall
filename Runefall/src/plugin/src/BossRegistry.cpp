#include "BossRegistry.h"

#include "DataStore.h"
#include "FormCatalog.h"

#include "RE/A/Actor.h"
#include "RE/T/TESNPC.h"
#include "RE/T/TESRace.h"

#include <algorithm>
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
		bool Bool(const std::vector<std::string>& a_row, int a_column)
		{
			return Cell(a_row, a_column) == "1";
		}
	}

	BossRegistry& BossRegistry::Get()
	{
		static BossRegistry singleton;
		return singleton;
	}

	void BossRegistry::Load()
	{
		bosses.clear();
		loaded = false;
		const auto* table = DataStore::Get().GetTable("bosses.csv");
		if (!table) {
			logger::error("boss-registry: bosses.csv missing");
			return;
		}
		const int bId = FindColumn(*table, "bossId");
		const int bActor = FindColumn(*table, "actorEditorId");
		const int bStage = FindColumn(*table, "fixedStage");
		const int bUnique = FindColumn(*table, "unique");
		const int bQuest = FindColumn(*table, "questBand");
		const int bPackage = FindColumn(*table, "packageId");
		const int bEnabled = FindColumn(*table, "enabled");
		for (const auto& row : table->rows) {
			BossDef def;
			def.bossId = Cell(row, bId);
			def.actorEditorId = Cell(row, bActor);
			if (auto* npc = FormCatalog::Get().LookupAs<RE::TESNPC>(def.actorEditorId))
				def.actorFormId = npc->GetFormID();
			def.fixedStage = static_cast<std::uint8_t>((std::clamp)(Int(row, bStage, 1), 1, 100));
			def.unique = Bool(row, bUnique);
			def.questBand = Int(row, bQuest);
			def.packageId = Cell(row, bPackage);
			def.enabled = Bool(row, bEnabled);
			if (!def.actorEditorId.empty() && def.enabled)
				bosses.push_back(std::move(def));
		}
		loaded = !bosses.empty();
		if (loaded)
			logger::info("boss-registry: loaded {} bosses", bosses.size());
	}

	const BossRegistry::BossDef* BossRegistry::Find(std::string_view a_editorId) const
	{
		for (const auto& boss : bosses)
			if (boss.actorEditorId == a_editorId)
				return &boss;
		return nullptr;
	}

	std::uint8_t BossRegistry::FixedStageForActor(RE::Actor* a_actor) const
	{
		if (!loaded || !a_actor)
			return 0;
		const auto* actorBase = a_actor->GetActorBase();
		if (!actorBase)
			return 0;
		for (const auto& boss : bosses)
			if (boss.actorFormId != 0 && boss.actorFormId == actorBase->GetFormID())
				return boss.fixedStage;
		const char* editor = actorBase->GetFormEditorID();
		if (!editor || !*editor)
			return 0;
		const auto* boss = Find(editor);
		return boss ? boss->fixedStage : 0;
	}

	bool BossRegistry::IsBossActor(RE::Actor* a_actor) const
	{
		return FixedStageForActor(a_actor) > 0;
	}

	std::uint8_t BossRegistry::FixedStageForEditorId(std::string_view a_editorId) const
	{
		if (const auto* boss = Find(a_editorId))
			return boss->fixedStage;
		return 0;
	}

	SelfTestResult BossRegistry::SelfTest()
	{
		auto& registry = Get();
		if (!registry.Loaded())
			return { "BossRegistry.SelfTest", CheckStatus::Fail, "boss table not loaded" };
		if (registry.BossCount() < 3)
			return { "BossRegistry.SelfTest", CheckStatus::Fail, "boss coverage too small" };
		const auto* first = registry.Find("RF_NPC_Boss_01");
		if (!first || first->fixedStage < 1)
			return { "BossRegistry.SelfTest", CheckStatus::Fail, "boss_01 fixed stage missing" };
		return { "BossRegistry.SelfTest", CheckStatus::Pass,
			std::format("{} bosses; boss_01 fixed stage {}", registry.BossCount(), static_cast<int>(first->fixedStage)) };
	}
}
