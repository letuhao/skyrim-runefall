#pragma once
#include "Core.h"

namespace Runefall
{
	struct MaterialDef
	{
		std::string id;
		std::string name;
		std::int32_t tier = 0;
		std::string family;
		std::string itemEditorId;
	};

	class MaterialRegistry
	{
	public:
		static MaterialRegistry& Get();

		void LoadDefinitions();
		void LoadFromCoSave();
		void SaveToCoSave();

		[[nodiscard]] bool        Has(std::string_view a_id) const;
		[[nodiscard]] std::int32_t Count(std::string_view a_id) const;
		[[nodiscard]] std::int32_t Total() const;
		[[nodiscard]] std::size_t DefinitionCount() const noexcept { return definitions.size(); }

		bool Add(std::string_view a_id, std::int32_t a_amount);
		bool Consume(std::string_view a_id, std::int32_t a_amount);
		bool AddDefaultForTier(std::int32_t a_tier, std::int32_t a_amount);

		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		std::vector<MaterialDef> definitions;
	};
}