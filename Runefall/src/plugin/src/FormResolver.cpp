#include "FormResolver.h"

namespace Runefall
{
	RE::TESForm* FormResolver::Resolve(std::string_view a_editorID)
	{
		if (a_editorID.empty())
			return nullptr;
		auto* form = RE::TESForm::LookupByEditorID(std::string_view{ a_editorID });
		if (!form) {
			// Light-plugin (ESL) EditorIDs are not reliably present in the global editor-ID map.
			// Runefall affix MGEFs are allocated from local FormID 0x900 in Runefall.esp (L3 §1.1).
			if (a_editorID.starts_with("RF_MGEF_Affix_")) {
				const auto index = std::atoi(std::string{ a_editorID.substr(a_editorID.size() - 3) }.c_str());
				if (auto* dataHandler = RE::TESDataHandler::GetSingleton()) {
					if (auto* effect = dataHandler->LookupForm<RE::EffectSetting>(static_cast<RE::FormID>(0x900 + index), "Runefall.esp")) {
						logger::info("form-resolver: '{}' resolved via Runefall.esp local FormID", a_editorID);
						return effect;
					}
				}
			}
			// Hunt materials are MISC records; light-plugin EditorIDs also miss the global map.
			if (a_editorID.starts_with("RF_MISC_")) {
				if (auto* dataHandler = RE::TESDataHandler::GetSingleton()) {
					for (auto* misc : dataHandler->GetFormArray<RE::TESObjectMISC>()) {
						if (!misc)
							continue;
						const char* editorID = misc->GetFormEditorID();
						if (editorID && a_editorID == editorID) {
							logger::info("form-resolver: '{}' resolved via MISC array scan", a_editorID);
							return misc;
						}
					}
				}
			}
			// Fall back to scanning the loaded effect array and matching the runtime EditorID.
			if (auto* dataHandler = RE::TESDataHandler::GetSingleton()) {
				const auto effects = dataHandler->GetFormArray<RE::EffectSetting>();
				logger::info("form-resolver: fallback scan {} effects for '{}'", effects.size(), a_editorID);
				for (auto* effect : effects) {
					if (!effect)
						continue;
					const char* editorID = effect->GetFormEditorID();
					if (editorID && a_editorID == editorID) {
						logger::info("form-resolver: '{}' resolved via form-array scan", a_editorID);
						form = effect;
						break;
					}
				}
			}
		}
		if (form && form->IsDeleted()) {
			logger::warn("form-resolver: '{}' resolved to a deleted form", a_editorID);
			return nullptr;
		}
		return form;
	}

	SelfTestResult FormResolver::SelfTest()
	{
		if (!RE::TESDataHandler::GetSingleton())
			return { "FormResolver.SelfTest", CheckStatus::Fail, "TESDataHandler unavailable" };

		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			logger::error("form-resolver: TESDataHandler null");
			return { "FormResolver.SelfTest", CheckStatus::Fail, "TESDataHandler unavailable" };
		}
		const auto* skyrim = dataHandler->LookupModByName("Skyrim.esm");
		if (!skyrim) {
			logger::error("form-resolver: Skyrim.esm not loaded");
			return { "FormResolver.SelfTest", CheckStatus::Fail, "Skyrim.esm not loaded" };
		}

		const auto* race = ResolveAs<RE::TESRace>("DraugrRace");
		if (!race)
			return { "FormResolver.SelfTest", CheckStatus::Fail, "EditorID lookup failed for DraugrRace" };

		return { "FormResolver.SelfTest", CheckStatus::Pass, "resolved DraugrRace form " + std::to_string(race->GetFormID()) };
	}
}