#pragma once
#include "Core.h"

namespace Runefall
{
	class ClassRegistry
	{
	public:
		static ClassRegistry& Get();
		void Load();
		[[nodiscard]] bool Loaded() const noexcept { return loaded; }
		[[nodiscard]] std::size_t ClassCount() const noexcept { return classes.size(); }
		bool ApplyClass(std::string_view a_classId);
		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		struct ClassDef
		{
			std::string classId;
			std::string nameKey;
			std::string primaryAttribute;
			std::string attributeBonuses;
			std::string startingSkills;
			std::string roleTags;
		};
		[[nodiscard]] const ClassDef* Find(std::string_view a_classId) const;
		std::vector<ClassDef> classes;
		bool loaded = false;
	};
}
