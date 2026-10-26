#include "MenuUI.h"

#ifdef RUNEFALL_MENU_FRAMEWORK
#	include <imgui/imgui.h>
#	include "SKSEMenuFramework.h"
#endif

#include "CoSave.h"
#include "DebugHarness.h"
#include "HuntEngine.h"
#include "ItemRegistry.h"
#include "LootEngine.h"
#include "MenuApi.h"
#include "Progression.h"
#include "TierEngine.h"

namespace Runefall
{
#ifdef RUNEFALL_MENU_FRAMEWORK
	namespace
	{
		bool labelValue(const char* a_label, std::int32_t a_value)
		{
			ImGui::TextUnformatted(a_label);
			ImGui::SameLine(180.0f);
			ImGui::Text("%d", a_value);
			return true;
		}

		void RenderCharacter()
		{
			ImGui::TextUnformatted("Runefall - Character");
			ImGui::Separator();
			ImGui::Text("Attribute points: %d", Progression::Get().AttributePoints());
			ImGui::Text("Skill points: %d", Progression::Get().SkillPoints());
			ImGui::Text("RF tier: %d", Progression::Get().RuneTier());
			ImGui::Separator();

			static const std::array<const char*, 9> kNames = { "Might", "Endurance", "Precision", "Agility", "Guile", "Cunning", "Intellect", "Wisdom", "Willpower" };
			for (std::size_t i = 0; i < kNames.size(); ++i)
				labelValue(kNames[i], Progression::Get().Attribute(i));

			ImGui::Separator();
			ImGui::TextUnformatted("Runes:");
			for (int rank = 1; rank <= 5; ++rank)
				labelValue(("R" + std::to_string(rank)).c_str(), CoSave::Get().State().runes[static_cast<std::size_t>(rank - 1)]);
		}

		void RenderRuneForge()
		{
			ImGui::TextUnformatted("Runefall - Rune Forge");
			ImGui::Separator();
			ImGui::Text("Tracked items: %d", static_cast<int>(ItemRegistry::Get().Size()));
			ImGui::Text("Affixes loaded: %d", static_cast<int>(LootEngine::Get().AffixCount()));
			ImGui::Text("Probe: %s", DebugHarness::LastSummary().c_str());
			ImGui::Separator();
			ImGui::TextUnformatted("Transactions are exposed through the Rune Forge native API;");
			ImGui::TextUnformatted("interactive item selection lands with the inventory integration.");
		}

		void RenderHuntCodex()
		{
			ImGui::TextUnformatted("Runefall - Hunt Codex");
			ImGui::Separator();
			ImGui::Text("Hunt families: %d", static_cast<int>(HuntEngine::Get().FamilyCount()));
			ImGui::Text("Affixes: %d   Ranks: %d", static_cast<int>(LootEngine::Get().AffixCount()), static_cast<int>(TierEngine::Get().RankCount()));
			ImGui::Separator();
			for (const auto& family : HuntEngine::Get().Families()) {
				ImGui::Text("%-20s first-kill: %s", family.group.c_str(), HuntEngine::Get().HasFirstKill(family.group) ? "yes" : "no");
			}
		}

		void RenderItemDetail()
		{
			ImGui::TextUnformatted("Runefall - Item Detail");
			ImGui::Separator();
			const auto& items = ItemRegistry::Get().All();
			ImGui::Text("Tracked items: %d", static_cast<int>(items.size()));
			ImGui::Separator();
			const std::size_t first = items.size() > 10 ? items.size() - 10 : 0;
			for (std::size_t i = first; i < items.size(); ++i) {
				const auto& item = items[i];
				ImGui::Text("#%u  rarity=%u ilvl=%u sockets=%u enhancements=%u",
					item.registryId, item.rarity, item.ilvl, item.socketCount, item.enhancementCount);
			}
		}
	}
#endif

	void MenuUI::Register()
	{
#ifdef RUNEFALL_MENU_FRAMEWORK
		SKSEMenuFramework::SetSection("Runefall");
		SKSEMenuFramework::AddSectionItem("Character", RenderCharacter);
		SKSEMenuFramework::AddSectionItem("Rune Forge", RenderRuneForge);
		SKSEMenuFramework::AddSectionItem("Hunt Codex", RenderHuntCodex);
		SKSEMenuFramework::AddSectionItem("Item Detail", RenderItemDetail);
		logger::info("menu: Runefall section registered (4 pages)");
#else
		logger::warn("menu: built without RUNEFALL_MENU_FRAMEWORK; pages disabled");
#endif
	}

	SelfTestResult MenuUI::SelfTest()
	{
#ifdef RUNEFALL_MENU_FRAMEWORK
		return { "MenuUI.SelfTest", CheckStatus::Pass, "menu pages compiled against SKSE Menu Framework" };
#else
		return { "MenuUI.SelfTest", CheckStatus::Warn, "menu framework support not compiled in" };
#endif
	}
}