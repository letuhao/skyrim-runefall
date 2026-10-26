#pragma once
#include "Core.h"
#include "ItemMetadata.h"

namespace Runefall
{
	class ItemRegistry
	{
	public:
		static ItemRegistry& Get();

		std::uint32_t Add(ItemMetadata a_metadata);
		[[nodiscard]] const ItemMetadata* Find(std::uint32_t a_registryId) const;
		[[nodiscard]] std::size_t          Size() const noexcept { return entries.size(); }
		void                               Clear();

		[[nodiscard]] const std::vector<ItemMetadata>& All() const noexcept { return entries; }
		void Load(std::vector<ItemMetadata> a_entries);

		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		std::vector<ItemMetadata> entries;
		std::uint32_t             nextId = 1;
	};
}
