#include "FormResolver.h"

namespace Runefall
{
	RE::TESForm* FormResolver::Resolve(std::string_view a_editorID)
	{
		if (a_editorID.empty())
			return nullptr;
		return RE::TESForm::LookupByEditorID(std::string_view{ a_editorID });
	}

	SelfTestResult FormResolver::SelfTest()
	{
		if (!RE::TESDataHandler::GetSingleton())
			return { "FormResolver.SelfTest", CheckStatus::Fail, "TESDataHandler unavailable" };

		const auto* skyrim = RE::TESDataHandler::GetSingleton()->LookupModByName("Skyrim.esm");
		if (!skyrim)
			return { "FormResolver.SelfTest", CheckStatus::Fail, "Skyrim.esm not loaded" };

		const auto* race = ResolveAs<RE::TESRace>("DraugrRace");
		if (!race)
			return { "FormResolver.SelfTest", CheckStatus::Fail, "EditorID lookup failed for DraugrRace" };

		return { "FormResolver.SelfTest", CheckStatus::Pass, "resolved DraugrRace form " + std::to_string(race->GetFormID()) };
	}
}