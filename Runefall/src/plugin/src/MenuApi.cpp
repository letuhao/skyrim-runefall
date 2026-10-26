#include "MenuApi.h"

#include "CoSave.h"
#include "HuntEngine.h"
#include "ItemRegistry.h"
#include "LootEngine.h"
#include "Progression.h"
#include "TierEngine.h"

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

	SelfTestResult MenuApi::SelfTest()
	{
		const auto json = BuildSnapshotJson();
		if (json.size() < 20 || json.front() != '{' || json.back() != '}')
			return { "MenuApi.SelfTest", CheckStatus::Fail, "snapshot JSON is malformed" };
		if (json.find("\"api\":") == std::string::npos || json.find("\"attributes\":") == std::string::npos)
			return { "MenuApi.SelfTest", CheckStatus::Fail, "snapshot JSON is missing required keys" };
		return { "MenuApi.SelfTest", CheckStatus::Pass, std::to_string(json.size()) + " byte snapshot" };
	}
}
