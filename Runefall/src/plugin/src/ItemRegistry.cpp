#include "ItemRegistry.h"

namespace Runefall
{
	ItemRegistry& ItemRegistry::Get()
	{
		static ItemRegistry singleton;
		return singleton;
	}

	std::uint32_t ItemRegistry::Add(ItemMetadata a_metadata)
	{
		constexpr std::size_t kMaxEntries = 10000;
		if (entries.size() >= kMaxEntries) {
			logger::warn("item-registry: cap {} reached; dropping new item", kMaxEntries);
			return 0;
		}
		a_metadata.registryId = nextId++;
		entries.push_back(std::move(a_metadata));
		return entries.back().registryId;
	}

	const ItemMetadata* ItemRegistry::Find(std::uint32_t a_registryId) const
	{
		for (const auto& entry : entries) {
			if (entry.registryId == a_registryId)
				return &entry;
		}
		return nullptr;
	}

	void ItemRegistry::Clear()
	{
		entries.clear();
		nextId = 1;
	}

	void ItemRegistry::Load(std::vector<ItemMetadata> a_entries)
	{
		entries = std::move(a_entries);
		nextId = 1;
		for (const auto& entry : entries) {
			const auto candidate = entry.registryId + 1;
			if (candidate > nextId)
				nextId = candidate;
		}
	}

	SelfTestResult ItemRegistry::SelfTest()
	{
		ItemRegistry test;
		test.Clear();

		ItemMetadata sample;
		sample.rarity = 3;
		sample.ilvl = 42;
		sample.enhancementCount = 2;
		sample.enhancements[0] = { "rf_affix_001", "RF_MGEF_Affix_001", 1, 3, 12.5f, 0, 0 };
		sample.enhancements[1] = { "rf_affix_047", "RF_MGEF_Affix_047", 2, 2, 7.0f, 10, 0 };
		sample.socketCount = 2;
		sample.socketRunes = { 3, 0, 0 };
		sample.tags = static_cast<std::uint16_t>(ItemTag::HuntDrop | ItemTag::UniqueAffix);

		const auto id = test.Add(sample);
		const auto* found = test.Find(id);
		if (!found)
			return { "ItemRegistry.SelfTest", CheckStatus::Fail, "inserted entry not found" };
		if (found->rarity != sample.rarity || found->ilvl != sample.ilvl ||
			found->enhancementCount != sample.enhancementCount ||
			found->enhancements[1].affixId != sample.enhancements[1].affixId ||
			found->socketRunes[0] != 3 || found->tags != sample.tags)
			return { "ItemRegistry.SelfTest", CheckStatus::Fail, "metadata round-trip mismatch" };

		const auto secondId = test.Add(ItemMetadata{});
		if (secondId == id)
			return { "ItemRegistry.SelfTest", CheckStatus::Fail, "registry ids are not unique" };

		const auto sizeBefore = test.Size();
		test.Clear();
		if (test.Size() != 0 || sizeBefore != 2)
			return { "ItemRegistry.SelfTest", CheckStatus::Fail, "clear failed" };

		return { "ItemRegistry.SelfTest", CheckStatus::Pass, "insert/find/unique/clear OK" };
	}
}