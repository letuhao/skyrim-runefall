#include "MenuApi.h"

#include "CoSave.h"
#include "HuntEngine.h"
#include "ItemRegistry.h"
#include "LootEngine.h"
#include "Progression.h"
#include "TierEngine.h"
#include "DebugHarness.h"
#include "Config.h"
#include "DataStore.h"
#include "HookManager.h"

namespace Runefall
{
	std::string MenuApi::BuildSnapshotJson()
	{
		const auto& state = CoSave::Get().State();
		std::string out;
		out.reserve(768);
		out += "{\"api\":";
		out += std::to_string(kApiVersion);
		out += ",\"tier\":";
		out += std::to_string(Progression::Get().RuneTier());
		out += ",\"attributePoints\":";
		out += std::to_string(Progression::Get().AttributePoints());
		out += ",\"skillPoints\":";
		out += std::to_string(Progression::Get().SkillPoints());
		out += ",\"attributes\":[";
		for (std::size_t i = 0; i < state.attributes.size(); ++i) {
			if (i)
				out += ',';
			out += std::to_string(state.attributes[i]);
		}
		out += "],\"runes\":[";
		for (std::size_t i = 0; i < state.runes.size(); ++i) {
			if (i)
				out += ',';
			out += std::to_string(state.runes[i]);
		}
		out += "],\"items\":";
		out += std::to_string(ItemRegistry::Get().Size());
		out += ",\"huntFamilies\":";
		out += std::to_string(HuntEngine::Get().FamilyCount());
		out += ",\"affixes\":";
		out += std::to_string(LootEngine::Get().AffixCount());
		out += ",\"ranks\":";
		out += std::to_string(TierEngine::Get().RankCount());
		out += '}';
		return out;
	}

	std::string MenuApi::BuildItemDetailJson(std::uint32_t a_registryId)
	{
		const auto* item = ItemRegistry::Get().Find(a_registryId);
		if (!item)
			return "{}";
		std::string out;
		out.reserve(512);
		out += "{\"registryId\":" + std::to_string(item->registryId);
		out += ",\"baseFormId\":" + std::to_string(item->baseFormId);
		out += ",\"rarity\":" + std::to_string(item->rarity);
		out += ",\"ilvl\":" + std::to_string(item->ilvl);
		out += ",\"socketCount\":" + std::to_string(item->socketCount);
		out += ",\"tags\":" + std::to_string(item->tags);
		out += ",\"enhancements\":[";
		for (std::uint8_t i = 0; i < item->enhancementCount && i < 5; ++i) {
			const auto& enhancement = item->enhancements[i];
			if (i)
				out += ',';
			out += "{\"affixId\":\"" + enhancement.affixId + "\",\"effectEditorId\":\"" + enhancement.effectEditorId + "\"";
			out += ",\"magnitude\":" + std::to_string(enhancement.magnitude);
			out += ",\"duration\":" + std::to_string(enhancement.duration);
			out += ",\"area\":" + std::to_string(enhancement.area) + '}';
		}
		out += "]}";
		return out;
	}

	std::string MenuApi::BuildDiagnosticsJson()
	{
		const auto& cfg = Config::Get();
		std::string out;
		out.reserve(512);
		out += "{\"pluginVersion\":";
		out += std::to_string(kApiVersion);
		out += ",\"dataLoaded\":";
		out += DataStore::Get().IsLoaded() ? "true" : "false";
		out += ",\"tables\":";
		out += std::to_string(DataStore::Get().TableCount());
		out += ",\"runtimeLoot\":";
		out += cfg.EnableRuntimeLoot() ? "true" : "false";
		out += ",\"menu\":";
		out += cfg.EnableMenu() ? "true" : "false";
		out += ",\"input\":";
		out += cfg.EnableInput() ? "true" : "false";
		out += ",\"coSave\":";
		out += cfg.EnableCoSave() ? "true" : "false";
		out += ",\"attributeEffects\":";
		out += std::to_string(Progression::Get().AttributeEffectCount());
		out += ",\"hookEvents\":{\"container\":";
		out += std::to_string(HookManager::Get().ContainerEvents());
		out += ",\"menu\":" + std::to_string(HookManager::Get().MenuEvents());
		out += ",\"cell\":" + std::to_string(HookManager::Get().CellEvents());
		out += ",\"object\":" + std::to_string(HookManager::Get().ObjectEvents()) + '}';
		out += ",\"probeFailures\":";
		out += std::to_string(DebugHarness::LastFailures());
		out += ",\"probeSummary\":\"" + DebugHarness::LastSummary() + "\"";
		out += '}';
		return out;
	}

	SelfTestResult MenuApi::SelfTest()
	{
		const auto json = BuildSnapshotJson();
		if (json.size() < 20 || json.front() != '{' || json.back() != '}')
			return { "MenuApi.SelfTest", CheckStatus::Fail, "snapshot JSON is malformed" };
		if (json.find("\"api\":") == std::string::npos || json.find("\"attributes\":") == std::string::npos)
			return { "MenuApi.SelfTest", CheckStatus::Fail, "snapshot JSON is missing required keys" };
		const auto detail = BuildItemDetailJson(0);
		if (detail.empty() || detail.front() != '{' || detail.back() != '}')
			return { "MenuApi.SelfTest", CheckStatus::Fail, "item detail JSON is malformed" };
		const auto diagnostics = BuildDiagnosticsJson();
		if (diagnostics.find("\"dataLoaded\":") == std::string::npos || diagnostics.find("\"probeSummary\":") == std::string::npos)
			return { "MenuApi.SelfTest", CheckStatus::Fail, "snapshot JSON is missing required keys" };
		return { "MenuApi.SelfTest", CheckStatus::Pass, std::to_string(json.size()) + " byte snapshot" };
	}
}