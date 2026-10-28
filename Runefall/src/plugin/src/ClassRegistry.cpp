#include "ClassRegistry.h"

#include "DataStore.h"
#include "Progression.h"

#include <algorithm>
#include <format>
#include <sstream>

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
		std::int32_t AttributeIndex(std::string_view a_name)
		{
			static const std::array<std::string_view, 9> kNames = {
				"Might", "Endurance", "Precision", "Agility", "Guile",
				"Cunning", "Intellect", "Wisdom", "Willpower"
			};
			for (std::size_t i = 0; i < kNames.size(); ++i)
				if (kNames[i] == a_name)
					return static_cast<std::int32_t>(i);
			return -1;
		}
	}

	ClassRegistry& ClassRegistry::Get()
	{
		static ClassRegistry singleton;
		return singleton;
	}

	void ClassRegistry::Load()
	{
		classes.clear();
		loaded = false;
		const auto* table = DataStore::Get().GetTable("classes.csv");
		if (!table) {
			logger::error("class-registry: classes.csv missing");
			return;
		}
		const int cId = FindColumn(*table, "classId");
		const int cName = FindColumn(*table, "nameKey");
		const int cPrimary = FindColumn(*table, "primaryAttribute");
		const int cBonuses = FindColumn(*table, "attributeBonuses");
		const int cSkills = FindColumn(*table, "startingSkills");
		const int cTags = FindColumn(*table, "roleTags");
		for (const auto& row : table->rows) {
			ClassDef def;
			def.classId = Cell(row, cId);
			def.nameKey = Cell(row, cName);
			def.primaryAttribute = Cell(row, cPrimary);
			def.attributeBonuses = Cell(row, cBonuses);
			def.startingSkills = Cell(row, cSkills);
			def.roleTags = Cell(row, cTags);
			if (!def.classId.empty())
				classes.push_back(std::move(def));
		}
		loaded = !classes.empty();
		if (loaded)
			logger::info("class-registry: loaded {} classes", classes.size());
	}

	const ClassRegistry::ClassDef* ClassRegistry::Find(std::string_view a_classId) const
	{
		for (const auto& def : classes)
			if (def.classId == a_classId)
				return &def;
		return nullptr;
	}

	bool ClassRegistry::ApplyClass(std::string_view a_classId)
	{
		const auto* def = Find(a_classId);
		if (!def)
			return false;
		auto& progression = Progression::Get();
		if (!progression.ClassId().empty())
			return false; // class choice is permanent without an explicit respec path
		progression.SetClassId(def->classId);
		std::stringstream bonuses{ def->attributeBonuses };
		std::string token;
		while (std::getline(bonuses, token, '|')) {
			const auto equal = token.find('=');
			if (equal == std::string::npos)
				continue;
			const auto index = AttributeIndex(token.substr(0, equal));
			const auto value = static_cast<std::int32_t>(std::strtol(token.substr(equal + 1).c_str(), nullptr, 10));
			if (index >= 0 && value > 0)
				progression.AddAttribute(static_cast<std::size_t>(index), value);
		}
		std::stringstream skills{ def->startingSkills };
		while (std::getline(skills, token, '|'))
			if (!token.empty())
				progression.UnlockSkill(token);
		logger::info("class-registry: applied class {} ({}), {} starting skills", def->classId, def->nameKey, def->startingSkills);
		return true;
	}

	SelfTestResult ClassRegistry::SelfTest()
	{
		auto& registry = Get();
		if (!registry.Loaded())
			return { "ClassRegistry.SelfTest", CheckStatus::Fail, "class table not loaded" };
		if (registry.ClassCount() < 3)
			return { "ClassRegistry.SelfTest", CheckStatus::Fail, "class coverage too small" };
		const auto* warrior = registry.Find("warrior");
		const auto* rogue = registry.Find("rogue");
		const auto* mage = registry.Find("mage");
		if (!warrior || !rogue || !mage || warrior->startingSkills.empty() || mage->attributeBonuses.empty())
			return { "ClassRegistry.SelfTest", CheckStatus::Fail, "class definitions invalid" };
		return { "ClassRegistry.SelfTest", CheckStatus::Pass,
			std::format("{} classes; warrior/rogue/mage skills and bonuses present", registry.ClassCount()) };
	}
}
