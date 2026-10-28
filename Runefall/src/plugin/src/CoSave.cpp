#include "CoSave.h"

#include "ItemRegistry.h"
#include "Progression.h"
#include "MaterialRegistry.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <format>

namespace Runefall
{
	namespace
	{
		constexpr std::uint32_t kSerializationUniqueId = 'RNFL';
		constexpr std::uint32_t kSerializationRecord = 'RNF2';
		constexpr std::uint16_t kSectionProgression = 1;
		constexpr std::uint16_t kSectionRunes = 2;
		constexpr std::uint16_t kSectionItems = 3;
		constexpr std::uint16_t kSectionHunt = 4;
		constexpr std::uint16_t kSectionMaterials = 5;
		constexpr std::uint16_t kSectionPackState = 7;
		constexpr std::uint16_t kSectionConfigHash = 8;
		constexpr std::uint16_t kSectionContainers = 9;
		constexpr std::uint16_t kSectionQuests = 10;
		constexpr std::uint16_t kSectionLadder = 11;
		constexpr std::uint32_t kMaxPersistedRefs = 4096;
		constexpr std::uint32_t kMaxPersistedQuests = 1024;
		constexpr std::uint32_t kMaxLadderOverrides = 256;
		std::string gLastRejectToken;
		void MarkReject(std::string_view a_token) { gLastRejectToken = a_token; }

		void PutU8(std::vector<std::uint8_t>& a_out, std::uint8_t a_value)
		{
			a_out.push_back(a_value);
		}

		void PutU16(std::vector<std::uint8_t>& a_out, std::uint16_t a_value)
		{
			a_out.push_back(static_cast<std::uint8_t>(a_value & 0xFF));
			a_out.push_back(static_cast<std::uint8_t>((a_value >> 8) & 0xFF));
		}

		void PutU32(std::vector<std::uint8_t>& a_out, std::uint32_t a_value)
		{
			for (int i = 0; i < 4; ++i)
				a_out.push_back(static_cast<std::uint8_t>((a_value >> (8 * i)) & 0xFF));
		}

		void PutU64(std::vector<std::uint8_t>& a_out, std::uint64_t a_value)
		{
			for (int i = 0; i < 8; ++i)
				a_out.push_back(static_cast<std::uint8_t>((a_value >> (8 * i)) & 0xFF));
		}

		void PutI32(std::vector<std::uint8_t>& a_out, std::int32_t a_value)
		{
			PutU32(a_out, static_cast<std::uint32_t>(a_value));
		}

		void PutString(std::vector<std::uint8_t>& a_out, std::string_view a_value)
		{
			PutU16(a_out, static_cast<std::uint16_t>(a_value.size()));
			a_out.insert(a_out.end(), a_value.begin(), a_value.end());
		}

		void PutFloat(std::vector<std::uint8_t>& a_out, float a_value)
		{
			std::uint32_t raw = 0;
			std::memcpy(&raw, &a_value, sizeof(raw));
			PutU32(a_out, raw);
		}

		class Reader
		{
		public:
			explicit Reader(const std::vector<std::uint8_t>& a_bytes) : bytes(a_bytes) {}

			bool U8(std::uint8_t& a_value)
			{
				if (offset + 1 > bytes.size())
					return false;
				a_value = bytes[offset++];
				return true;
			}

			bool U16(std::uint16_t& a_value)
			{
				if (offset + 2 > bytes.size())
					return false;
				a_value = static_cast<std::uint16_t>(bytes[offset]) | (static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
				offset += 2;
				return true;
			}

			bool U32(std::uint32_t& a_value)
			{
				if (offset + 4 > bytes.size())
					return false;
				a_value = 0;
				for (int i = 0; i < 4; ++i)
					a_value |= static_cast<std::uint32_t>(bytes[offset + i]) << (8 * i);
				offset += 4;
				return true;
			}

			bool U64(std::uint64_t& a_value)
			{
				if (offset + 8 > bytes.size())
					return false;
				a_value = 0;
				for (int i = 0; i < 8; ++i)
					a_value |= static_cast<std::uint64_t>(bytes[offset + i]) << (8 * i);
				offset += 8;
				return true;
			}

			bool I32(std::int32_t& a_value)
			{
				std::uint32_t raw = 0;
				if (!U32(raw))
					return false;
				a_value = static_cast<std::int32_t>(raw);
				return true;
			}

			bool Float(float& a_value)
			{
				std::uint32_t raw = 0;
				if (!U32(raw))
					return false;
				std::memcpy(&a_value, &raw, sizeof(raw));
				return true;
			}

			bool String(std::string& a_value)
			{
				std::uint16_t len = 0;
				if (!U16(len) || offset + len > bytes.size())
					return false;
				a_value.assign(reinterpret_cast<const char*>(bytes.data() + offset), len);
				offset += len;
				return true;
			}

			bool Skip(std::uint32_t a_count)
			{
				if (offset + a_count > bytes.size())
					return false;
				offset += a_count;
				return true;
			}

			[[nodiscard]] std::size_t Remaining() const noexcept { return bytes.size() - offset; }
			[[nodiscard]] std::size_t Offset() const noexcept { return offset; }

		private:
			const std::vector<std::uint8_t>& bytes;
			std::size_t                      offset = 0;
		};

		std::vector<std::uint8_t> BuildSection(std::uint16_t a_id, const std::vector<std::uint8_t>& a_payload)
		{
			std::vector<std::uint8_t> out;
			PutU16(out, a_id);
			PutU32(out, static_cast<std::uint32_t>(a_payload.size()));
			out.insert(out.end(), a_payload.begin(), a_payload.end());
			return out;
		}
	}

	const std::string& CoSave::LastRejectToken()
	{
		return gLastRejectToken;
	}

	CoSave& CoSave::Get()
	{
		static CoSave singleton;
		return singleton;
	}

	void CoSave::Reset()
	{
		state = CoSaveState{};
	}

	bool CoSave::HasGeneratedContainer(std::uint32_t a_formID) const
	{
		return std::find(state.generatedContainers.begin(), state.generatedContainers.end(), a_formID) != state.generatedContainers.end();
	}

	bool CoSave::RememberGeneratedContainer(std::uint32_t a_formID)
	{
		if (a_formID == 0 || HasGeneratedContainer(a_formID))
			return false;
		if (state.generatedContainers.size() >= 4096)
			state.generatedContainers.erase(std::min_element(state.generatedContainers.begin(), state.generatedContainers.end()));
		state.generatedContainers.push_back(a_formID);
		return true;
	}

	void CoSave::ForgetGeneratedContainer(std::uint32_t a_formID)
	{
		std::erase(state.generatedContainers, a_formID);
	}

	bool CoSave::HasRewardedQuest(std::uint32_t a_questFormID) const
	{
		return std::find(state.rewardedQuests.begin(), state.rewardedQuests.end(), a_questFormID) != state.rewardedQuests.end();
	}

	bool CoSave::RememberRewardedQuest(std::uint32_t a_questFormID)
	{
		if (a_questFormID == 0 || HasRewardedQuest(a_questFormID))
			return false;
		if (state.rewardedQuests.size() >= 1024)
			state.rewardedQuests.erase(std::min_element(state.rewardedQuests.begin(), state.rewardedQuests.end()));
		state.rewardedQuests.push_back(a_questFormID);
		return true;
	}

	std::uint32_t CoSave::HashConfig()
	{
		// Placeholder stable hash until the real config/pack fingerprint lands.
		return 0x52464C31;
	}

	std::vector<std::uint8_t> CoSave::Encode(const CoSaveState& a_state)
	{
		return EncodeWithBudget(a_state, kMaxEncodedBytes);
	}

	std::vector<std::uint8_t> CoSave::EncodeWithBudget(const CoSaveState& a_state, std::size_t a_budget)
	{
		constexpr std::size_t kMaxItems = 1024;
		constexpr std::size_t kMaxHunts = 256;
		constexpr std::size_t kMaxMaterials = 256;
		constexpr std::size_t kMaxPacks = 64;
		constexpr std::size_t kMaxContainers = 4096;
		constexpr std::size_t kMaxQuests = 1024;
		constexpr std::size_t kMaxOverrides = 256;
		constexpr std::size_t kMaxSkills = 256;
		if (a_state.items.size() > kMaxItems || a_state.huntFirstKill.size() > kMaxHunts ||
			a_state.materials.size() > kMaxMaterials || a_state.packVersions.size() > kMaxPacks ||
			a_state.generatedContainers.size() > kMaxContainers || a_state.rewardedQuests.size() > kMaxQuests ||
			a_state.ladderOverrides.size() > kMaxOverrides || a_state.skillRanks.size() > kMaxSkills ||
			a_state.characterClass.size() > 64)
			return {};
		auto validKey = [](const std::string& value, std::size_t max) { return value.size() <= max; };
		for (const auto& [key, flag] : a_state.huntFirstKill)
			if (!validKey(key, 64) || flag > 1) return {};
		for (const auto& [key, count] : a_state.materials)
			if (!validKey(key, 64) || count < 0) return {};
		for (const auto& [key, version] : a_state.packVersions)
			if (!validKey(key, 32) || !validKey(version, 32)) return {};
		for (const auto& [skill, rank] : a_state.skillRanks)
			if (!validKey(skill, 64) || rank < 1 || rank > 100) return {};
		if (a_state.playerRank < 1 || a_state.playerRank > 100 || a_state.playerStage < 1 || a_state.playerStage > 100 ||
			a_state.playerXp > 10000000 || (a_state.difficultyProfile != 255 && (a_state.difficultyProfile < 1 || a_state.difficultyProfile > 5)) ||
			a_state.curveId < 1 || a_state.curveId > 5)
			return {};
		for (const auto& item : a_state.items) {
			if (item.registryId == 0 || item.rarity > 5 || item.ilvl > 1000 || item.socketCount > 3 || item.enhancementCount > 5 || (item.tags & ~static_cast<std::uint16_t>(0x3F)) != 0)
				return {};
			for (int r = 0; r < 3; ++r)
				if (item.socketRunes[r] < 0 || item.socketRunes[r] > 5) return {};
			for (std::uint8_t e = 0; e < 5; ++e) {
				const auto& enhancement = item.enhancements[e];
				if (!validKey(enhancement.affixId, 64) || !validKey(enhancement.effectEditorId, 64)) return {};
				if (enhancement.category < 0 || enhancement.category > 5 || enhancement.tier < 0 || enhancement.tier > 5) return {};
			}
		}
		for (const auto& entry : a_state.ladderOverrides)
			if (entry.key == 0 || entry.stage < 1 || entry.stage > 100 || entry.flags > 1) return {};
		auto sortedItems = a_state.items;
		std::sort(sortedItems.begin(), sortedItems.end(), [](const ItemMetadata& a, const ItemMetadata& b) {
			return a.registryId != b.registryId ? a.registryId < b.registryId : a.baseFormId < b.baseFormId;
		});
		for (std::size_t i = 1; i < sortedItems.size(); ++i)
			if (sortedItems[i].registryId == sortedItems[i - 1].registryId) return {};
		auto sortedContainers = a_state.generatedContainers;
		std::sort(sortedContainers.begin(), sortedContainers.end());
		sortedContainers.erase(std::unique(sortedContainers.begin(), sortedContainers.end()), sortedContainers.end());
		auto sortedQuests = a_state.rewardedQuests;
		std::sort(sortedQuests.begin(), sortedQuests.end());
		sortedQuests.erase(std::unique(sortedQuests.begin(), sortedQuests.end()), sortedQuests.end());
		auto sortedOverrides = a_state.ladderOverrides;
		std::sort(sortedOverrides.begin(), sortedOverrides.end(), [](const LadderOverride& a, const LadderOverride& b) {
			if (a.key != b.key) return a.key < b.key;
			if (a.stage != b.stage) return a.stage < b.stage;
			return a.flags < b.flags;
		});
		std::vector<std::pair<std::string, std::uint8_t>> hunts(a_state.huntFirstKill.begin(), a_state.huntFirstKill.end());
		std::sort(hunts.begin(), hunts.end());
		std::vector<std::pair<std::string, std::int32_t>> materials(a_state.materials.begin(), a_state.materials.end());
		std::sort(materials.begin(), materials.end());
		std::vector<std::pair<std::string, std::string>> packs(a_state.packVersions.begin(), a_state.packVersions.end());
		std::sort(packs.begin(), packs.end());
		std::vector<std::pair<std::string, std::int32_t>> skills(a_state.skillRanks.begin(), a_state.skillRanks.end());
		std::sort(skills.begin(), skills.end());

		std::vector<std::uint8_t> out;
		out.insert(out.end(), { 'R', 'N', 'F', 'S' });
		PutU16(out, kSaveVersion);
		PutU32(out, 0);
		PutU16(out, kSaveVersion);

		std::vector<std::vector<std::uint8_t>> sections;
		std::vector<std::uint8_t> progress;
		for (const auto value : a_state.attributes) PutI32(progress, value);
		PutI32(progress, a_state.attributePoints);
		PutI32(progress, a_state.skillPoints);
		PutI32(progress, a_state.runeTier);
		PutI32(progress, a_state.bossKillTier);
		sections.push_back(BuildSection(kSectionProgression, progress));
		std::vector<std::uint8_t> runes;
		for (const auto value : a_state.runes) PutI32(runes, value);
		sections.push_back(BuildSection(kSectionRunes, runes));
		std::vector<std::uint8_t> items;
		PutU32(items, static_cast<std::uint32_t>(sortedItems.size()));
		for (const auto& item : sortedItems) {
			PutU32(items, item.registryId);
			PutU32(items, item.baseFormId);
			items.push_back(item.rarity);
			PutU16(items, item.ilvl);
			items.push_back(item.socketCount);
			for (const auto rune : item.socketRunes) PutI32(items, rune);
			items.push_back(item.enhancementCount);
			for (std::uint8_t e = 0; e < 5; ++e) {
				const auto& enhancement = item.enhancements[e];
				PutString(items, enhancement.affixId);
				PutString(items, enhancement.effectEditorId);
				PutI32(items, enhancement.category);
				PutI32(items, enhancement.tier);
				PutFloat(items, enhancement.magnitude);
				PutI32(items, enhancement.duration);
				PutI32(items, enhancement.area);
			}
			PutU16(items, item.tags);
		}
		sections.push_back(BuildSection(kSectionItems, items));
		std::vector<std::uint8_t> hunt;
		PutU32(hunt, static_cast<std::uint32_t>(hunts.size()));
		for (const auto& [id, flag] : hunts) { PutString(hunt, id); hunt.push_back(flag); }
		sections.push_back(BuildSection(kSectionHunt, hunt));
		std::vector<std::uint8_t> materialData;
		PutU32(materialData, static_cast<std::uint32_t>(materials.size()));
		for (const auto& [id, count] : materials) { PutString(materialData, id); PutI32(materialData, count); }
		sections.push_back(BuildSection(kSectionMaterials, materialData));
		std::vector<std::uint8_t> packData;
		PutU32(packData, static_cast<std::uint32_t>(packs.size()));
		for (const auto& [id, version] : packs) { PutString(packData, id); PutString(packData, version); }
		sections.push_back(BuildSection(kSectionPackState, packData));
		std::vector<std::uint8_t> hash;
		PutU32(hash, a_state.configHash);
		sections.push_back(BuildSection(kSectionConfigHash, hash));
		std::vector<std::uint8_t> containers;
		PutU32(containers, static_cast<std::uint32_t>(sortedContainers.size()));
		for (const auto id : sortedContainers) PutU32(containers, id);
		sections.push_back(BuildSection(kSectionContainers, containers));
		std::vector<std::uint8_t> quests;
		PutU32(quests, static_cast<std::uint32_t>(sortedQuests.size()));
		for (const auto id : sortedQuests) PutU32(quests, id);
		sections.push_back(BuildSection(kSectionQuests, quests));
		std::vector<std::uint8_t> ladder;
		PutU8(ladder, 1);
		PutU8(ladder, a_state.playerRank);
		PutU8(ladder, a_state.playerStage);
		PutU8(ladder, a_state.difficultyProfile);
		PutU8(ladder, a_state.curveId);
		PutU16(ladder, static_cast<std::uint16_t>(sortedOverrides.size()));
		for (const auto& entry : sortedOverrides) { PutU64(ladder, entry.key); PutU8(ladder, entry.stage); PutU8(ladder, entry.flags); }
		PutU32(ladder, a_state.playerXp);
		PutString(ladder, a_state.characterClass);
		PutU16(ladder, static_cast<std::uint16_t>(skills.size()));
		for (const auto& [skill, rank] : skills) { PutString(ladder, skill); PutI32(ladder, rank); }
		PutU8(ladder, a_state.endgameComplete ? 1 : 0);
		sections.push_back(BuildSection(kSectionLadder, ladder));
		PutU32(out, static_cast<std::uint32_t>(sections.size()));
		for (const auto& section : sections) out.insert(out.end(), section.begin(), section.end());
		if (out.size() > a_budget) return {};
		return out;
	}

bool CoSave::Decode(const std::vector<std::uint8_t>& a_bytes, CoSaveState& a_out)
	{
		CoSaveState candidate;
		if (!DecodeImpl(a_bytes, candidate))
			return false;
		a_out = std::move(candidate);
		return true;
	}

	bool CoSave::LoadRecord(const std::vector<std::uint8_t>& a_bytes, std::uint32_t a_recordVersion, CoSaveState& a_out)
	{
		if (a_recordVersion == 0 || a_recordVersion > kSaveVersion) {
			MarkReject("co-save: reject record-version");
			logger::error("co-save: reject record-version {}", a_recordVersion);
			return false;
		}
		return Decode(a_bytes, a_out);
	}

	bool CoSave::DecodeImpl(const std::vector<std::uint8_t>& a_bytes, CoSaveState& a_out)
	{
		a_out = CoSaveState{};
		if (a_bytes.size() < 12)
			return false;
		if (!(a_bytes[0] == 'R' && a_bytes[1] == 'N' && a_bytes[2] == 'F' && a_bytes[3] == 'S'))
			return false;

		Reader reader{ a_bytes };
		std::uint8_t magic[4];
		for (auto& c : magic)
			if (!reader.U8(c))
				return false;

		std::uint16_t schema = 0;
		std::uint16_t saveVersion = 0;
		std::uint32_t flags = 0;
		std::uint32_t sectionCount = 0;
		if (!reader.U16(schema) || schema < 1 || schema > kSaveVersion) {
			MarkReject("co-save: reject schema");
			logger::error("co-save: reject schema {}", schema);
			return false;
		}
		if (schema >= 3) {
			if (!reader.U32(flags)) return false;
			if (flags != 0) { MarkReject("co-save: reject flags"); logger::error("co-save: reject flags {}", flags); return false; }
			if (!reader.U16(saveVersion) || saveVersion != schema) { MarkReject("co-save: reject version-mirror"); logger::error("co-save: reject version-mirror"); return false; }
		} else if (!reader.U16(saveVersion) || saveVersion != schema) {
			logger::error("co-save: reject version-mirror");
			return false;
		}
		if (!reader.U32(sectionCount) || sectionCount == 0) { MarkReject("co-save: reject section-count"); logger::error("co-save: reject section-count"); return false; }
		constexpr std::uint32_t kMaxSections = 64;
		if (sectionCount > kMaxSections) { logger::error("co-save: reject section-count {}", sectionCount); return false; }
		std::array<bool, 16> seenSections{};
		for (std::uint32_t i = 0; i < sectionCount; ++i) {
			std::uint16_t id = 0;
			std::uint32_t len = 0;
			if (!reader.U16(id) || !reader.U32(len) || len > reader.Remaining())
				return false;

			std::vector<std::uint8_t> payload(len);
			for (std::uint32_t b = 0; b < len; ++b) {
				std::uint8_t value = 0;
				if (!reader.U8(value))
					return false;
				payload[b] = value;
			}
			Reader sectionReader{ payload };
			constexpr std::uint16_t kMaxKnownSection = 12;
			if (id >= 1 && id <= kMaxKnownSection) {
				if (seenSections[id]) { logger::error("co-save: reject duplicate-section {}", id); return false; }
				seenSections[id] = true;
			}

			switch (id) {
			case kSectionProgression:
				for (auto& value : a_out.attributes)
					if (!sectionReader.I32(value))
						return false;
				if (!sectionReader.I32(a_out.attributePoints) || !sectionReader.I32(a_out.skillPoints))
					return false;
				// Schema 1 saves lack these two fields; keep defaults when the payload ends here.
				if (sectionReader.Remaining() >= 8) {
					if (!sectionReader.I32(a_out.runeTier) || !sectionReader.I32(a_out.bossKillTier))
						return false;
				}
				break;
			case kSectionRunes:
				for (auto& value : a_out.runes)
					if (!sectionReader.I32(value))
						return false;
				break;
			case kSectionMaterials: {
				std::uint32_t count = 0;
				if (!sectionReader.U32(count) || count > 256) {
				    MarkReject("co-save: reject material-count"); logger::error("co-save: reject material-count"); return false;
				}
				for (std::uint32_t i = 0; i < count; ++i) {
					std::string id;
					std::int32_t value = 0;
					if (!sectionReader.String(id) || id.size() > 64 || !sectionReader.I32(value) || value < 0) {
					    MarkReject("co-save: reject string.map-key"); logger::error("co-save: reject string.map-key"); return false;
					}
					a_out.materials[id] = value;
				}
				break;
			}
			case kSectionItems: {
				std::uint32_t count = 0;
				if (!sectionReader.U32(count) || count > 1024) {
				    MarkReject("co-save: reject item-count"); logger::error("co-save: reject item-count"); return false;
				}
				if (sectionReader.Remaining() < count * 100u)
					return false;
				a_out.items.resize(count);
				for (auto& item : a_out.items) {
					if (!sectionReader.U32(item.registryId) || !sectionReader.U32(item.baseFormId))
						return false;
					if (item.baseFormId == 0) { MarkReject("co-save: reject item.baseFormId"); logger::error("co-save: reject item.baseFormId"); return false; }
					if (!sectionReader.U8(item.rarity) || !sectionReader.U16(item.ilvl) || !sectionReader.U8(item.socketCount))
						return false;
					if (item.registryId == 0) { MarkReject("co-save: reject item.registryId"); logger::error("co-save: reject item.registryId"); return false; }
					if (item.rarity > 5) { MarkReject("co-save: reject item.rarity"); logger::error("co-save: reject item.rarity"); return false; }
					if (item.ilvl > 1000) { MarkReject("co-save: reject item.ilvl"); logger::error("co-save: reject item.ilvl"); return false; }
					if (item.socketCount > 3) { MarkReject("co-save: reject item.socketCount"); logger::error("co-save: reject item.socketCount"); return false; }
					for (auto& rune : item.socketRunes)
						if (!sectionReader.I32(rune) || rune < 0 || rune > 5) {
						    MarkReject("co-save: reject item.socketRunes"); logger::error("co-save: reject item.socketRunes"); return false;
						}
					if (!sectionReader.U8(item.enhancementCount) || item.enhancementCount > 5) { MarkReject("co-save: reject item.enhancementCount"); return false; }
					for (auto& enhancement : item.enhancements) {
						if (!sectionReader.String(enhancement.affixId) || enhancement.affixId.size() > 64 || !sectionReader.String(enhancement.effectEditorId) || enhancement.effectEditorId.size() > 64) {
						    MarkReject("co-save: reject string.affix"); logger::error("co-save: reject string.affix"); return false;
						}
						if (!sectionReader.I32(enhancement.category) || !sectionReader.I32(enhancement.tier) || !sectionReader.Float(enhancement.magnitude))
							return false;
						if (!sectionReader.I32(enhancement.duration) || !sectionReader.I32(enhancement.area))
							return false;
						if (enhancement.category < 0 || enhancement.category > 5) { MarkReject("co-save: reject item.category"); logger::error("co-save: reject item.category"); return false; }
						if (enhancement.tier < 0 || enhancement.tier > 5) { MarkReject("co-save: reject item.tier"); logger::error("co-save: reject item.tier"); return false; }
					}
					if (!sectionReader.U16(item.tags))
						return false;
					if ((item.tags & ~static_cast<std::uint16_t>(0x3F)) != 0) { MarkReject("co-save: reject item.tags"); logger::error("co-save: reject item.tags"); return false; }
				}
				break;
			}
			case kSectionHunt: {
				std::uint32_t count = 0;
				if (!sectionReader.U32(count) || count > 256) {
				    MarkReject("co-save: reject hunt-count"); logger::error("co-save: reject hunt-count"); return false;
				}
				for (std::uint32_t n = 0; n < count; ++n) {
					std::string   key;
					std::uint8_t  flag = 0;
					if (!sectionReader.String(key) || key.size() > 64 || !sectionReader.U8(flag) || flag > 1) {
					    MarkReject("co-save: reject string.map-key"); logger::error("co-save: reject string.map-key"); return false;
					}
					a_out.huntFirstKill[std::move(key)] = flag;
				}
				break;
			}
			case kSectionPackState: {
				std::uint32_t count = 0;
				if (!sectionReader.U32(count) || count > 64) {
				    MarkReject("co-save: reject pack-count"); logger::error("co-save: reject pack-count"); return false;
				}
				for (std::uint32_t n = 0; n < count; ++n) {
					std::string key;
					std::string version;
					if (!sectionReader.String(key) || key.size() > 32 || !sectionReader.String(version) || version.size() > 32) {
					    MarkReject("co-save: reject string.pack"); logger::error("co-save: reject string.pack"); return false;
					}
					a_out.packVersions[std::move(key)] = std::move(version);
				}
				break;
			}
			case kSectionConfigHash:
				if (!sectionReader.U32(a_out.configHash))
					return false;
				break;
			case kSectionContainers: {
				std::uint32_t count = 0;
				if (!sectionReader.U32(count) || count > kMaxPersistedRefs) {
				    MarkReject("co-save: reject container-count"); logger::error("co-save: reject container-count"); return false;
				}
				a_out.generatedContainers.reserve(count);
				for (std::uint32_t n = 0; n < count; ++n) {
					std::uint32_t id = 0;
					if (!sectionReader.U32(id) || id == 0)
						return false;
					a_out.generatedContainers.push_back(id);
				}
				break;
			}
			case kSectionQuests: {
				std::uint32_t count = 0;
				if (!sectionReader.U32(count) || count > kMaxPersistedQuests) {
				    MarkReject("co-save: reject quest-count"); logger::error("co-save: reject quest-count"); return false;
				}
				a_out.rewardedQuests.reserve(count);
				for (std::uint32_t n = 0; n < count; ++n) {
					std::uint32_t id = 0;
					if (!sectionReader.U32(id) || id == 0)
						return false;
					a_out.rewardedQuests.push_back(id);
				}
				break;
			}
			case kSectionLadder: {
				std::uint8_t version = 0;
				if (!sectionReader.U8(version) || version != 1)
					return false;
				if (!sectionReader.U8(a_out.playerRank) || !sectionReader.U8(a_out.playerStage) ||
					!sectionReader.U8(a_out.difficultyProfile) || !sectionReader.U8(a_out.curveId))
					return false;
				if (a_out.playerRank < 1 || a_out.playerRank > 100 || a_out.playerStage < 1 || a_out.playerStage > 100 ||
					(a_out.difficultyProfile != 255 && (a_out.difficultyProfile < 1 || a_out.difficultyProfile > 5)) ||
					a_out.curveId < 1 || a_out.curveId > 5) { MarkReject("co-save: reject range"); logger::error("co-save: reject range"); return false; }
				std::uint16_t count = 0;
				if (!sectionReader.U16(count) || count > kMaxLadderOverrides) {
				    MarkReject("co-save: reject override-count"); logger::error("co-save: reject override-count"); return false;
				}
				a_out.ladderOverrides.reserve(count);
				for (std::uint16_t n = 0; n < count; ++n) {
					LadderOverride entry;
					if (!sectionReader.U64(entry.key) || !sectionReader.U8(entry.stage) || !sectionReader.U8(entry.flags))
						return false;
					if (entry.key == 0 || entry.stage < 1 || entry.stage > 100)
						return false;
					a_out.ladderOverrides.push_back(entry);
				}
				if (sectionReader.Remaining() >= 4) {
					if (!sectionReader.U32(a_out.playerXp) || a_out.playerXp > 10000000) { MarkReject("co-save: reject range"); logger::error("co-save: reject range"); return false; }
				}
				if (sectionReader.Remaining() > 0) {
					if (!sectionReader.String(a_out.characterClass) || a_out.characterClass.size() > 64) {
					    MarkReject("co-save: reject string.class"); logger::error("co-save: reject string.class"); return false;
					}
					std::uint16_t skillCount = 0;
					if (!sectionReader.U16(skillCount) || skillCount > 256) {
					    MarkReject("co-save: reject skill-count"); logger::error("co-save: reject skill-count"); return false;
					}
					for (std::uint16_t s = 0; s < skillCount; ++s) {
						std::string skill;
						std::int32_t rank = 0;
						if (!sectionReader.String(skill) || skill.size() > 64 || !sectionReader.I32(rank)) {
						    MarkReject("co-save: reject string.skill"); logger::error("co-save: reject string.skill"); return false;
						}
						if (rank < 1 || rank > 100)
							return false;
						a_out.skillRanks[skill] = rank;
					}
					if (sectionReader.Remaining() >= 1) {
						std::uint8_t complete = 0;
						if (!sectionReader.U8(complete))
							return false;
						if (complete > 1)
						return false;
					a_out.endgameComplete = complete != 0;
					}
				}
				break;
			}
			default:
				break;
			}
			if (id >= 1 && id <= 12 && sectionReader.Remaining() != 0) {
				MarkReject("co-save: reject trailing"); logger::error("co-save: reject trailing");
				return false;
			}
		}
		if (reader.Remaining() != 0) { MarkReject("co-save: reject trailing"); logger::error("co-save: reject trailing"); return false; }
		MigrateLegacy(a_out, schema);
		std::sort(a_out.generatedContainers.begin(), a_out.generatedContainers.end());
		a_out.generatedContainers.erase(std::unique(a_out.generatedContainers.begin(), a_out.generatedContainers.end()), a_out.generatedContainers.end());
		std::sort(a_out.rewardedQuests.begin(), a_out.rewardedQuests.end());
		a_out.rewardedQuests.erase(std::unique(a_out.rewardedQuests.begin(), a_out.rewardedQuests.end()), a_out.rewardedQuests.end());
		std::sort(a_out.items.begin(), a_out.items.end(), [](const ItemMetadata& a, const ItemMetadata& b) { return a.registryId != b.registryId ? a.registryId < b.registryId : a.baseFormId < b.baseFormId; });
		for (std::size_t i = 0; i < a_out.items.size(); ++i) {
			const auto& item = a_out.items[i];
			if (item.registryId == 0 || (i > 0 && a_out.items[i - 1].registryId == item.registryId) || item.rarity > 5 || item.ilvl > 1000 || item.socketCount > 3 || item.enhancementCount > 5)
				return false;
			for (std::uint8_t e = 0; e < item.enhancementCount && e < 5; ++e)
				if (item.enhancements[e].category < 0 || item.enhancements[e].category > 5 || item.enhancements[e].tier < 0 || item.enhancements[e].tier > 5)
					return false;
		}
		return true;
	}

	void CoSave::MigrateLegacy(CoSaveState& a_state, std::uint16_t a_schema)
	{
		if (a_schema >= 3)
			return;
		static constexpr std::uint8_t kLegacyRankStages[6] = { 1, 20, 40, 60, 80, 100 };
		const auto legacyTier = (std::clamp)(a_state.runeTier, 0, 5);
		a_state.playerRank = kLegacyRankStages[legacyTier];
		a_state.playerStage = kLegacyRankStages[legacyTier];
		a_state.difficultyProfile = 255;
		a_state.curveId = 1;
		logger::info("co-save: migrated schema {} runeTier {} to player rank/stage {}", a_schema, legacyTier, a_state.playerRank);
	}

	bool CoSave::LoadRecords(const std::vector<Record>& a_records, CoSaveState& a_out, std::uint32_t& a_recordVersion)
	{
		CoSaveState candidate;
		bool loaded = false;
		for (const auto& record : a_records) {
			if (record.type != kSerializationRecord) continue;
			if (loaded) { logger::info("co-save: duplicate record ignored"); continue; }
			if (LoadRecord(record.bytes, record.version, candidate)) { loaded = true; a_recordVersion = record.version; }
			else logger::error("co-save: reject record; defaults kept");
		}
		if (!loaded) { a_out = CoSaveState{}; return false; }
		a_out = std::move(candidate);
		return true;
	}

	void CoSave::OnSave(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc) return;
		auto& state = CoSave::Get().State();
		Progression::Get().SaveToCoSave();
		MaterialRegistry::Get().SaveToCoSave();
		state.items = ItemRegistry::Get().All();
		state.configHash = HashConfig();
		const auto bytes = Encode(state);
		if (bytes.empty()) {
			logger::error("co-save: encode cap exceeded; save skipped");
			return;
		}
		if (bytes.size() > kMaxEncodedBytes) {
			logger::error("co-save: encoded bytes exceed budget {} > {}; save skipped", bytes.size(), kMaxEncodedBytes);
			return;
		}
		const auto firstContainer = state.generatedContainers.empty() ? 0u : state.generatedContainers.front();
		const auto firstItem = state.items.empty() ? 0u : state.items.front().registryId;
		const auto firstSkillRank = state.skillRanks.empty() ? 0 : state.skillRanks.begin()->second;
		const auto firstQuest = state.rewardedQuests.empty() ? 0u : state.rewardedQuests.front();
		logger::info("co-save: state rank={} stage={} xp={} class={} endgame={} containers={} quests={} items={} materials={} configHash={:08X} difficulty={} curve={} overrides={} skillCount={} firstContainer={} firstItem={} skillRank={} firstQuest={}",
			state.playerRank, state.playerStage, state.playerXp, state.characterClass, state.endgameComplete ? 1 : 0,
			state.generatedContainers.size(), state.rewardedQuests.size(), state.items.size(), state.materials.size(), state.configHash, state.difficultyProfile, state.curveId, state.ladderOverrides.size(), state.skillRanks.size(), firstContainer, firstItem, firstSkillRank, firstQuest);
		if (!a_intfc->OpenRecord(kSerializationRecord, kSaveVersion)) {
			logger::error("co-save: failed to open record");
			return;
		}
		if (!a_intfc->WriteRecordData(bytes.data(), static_cast<std::uint32_t>(bytes.size())))
			logger::error("co-save: failed to write {} bytes", bytes.size());
		else
			logger::info("co-save: wrote {} bytes (schema {})", bytes.size(), kSaveVersion);
	}

	void CoSave::OnLoad(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc) return;
		std::uint32_t type = 0;
		std::uint32_t size = 0;
		std::uint32_t version = 0;
		std::vector<Record> records;
		while (a_intfc->GetNextRecordInfo(type, version, size)) {
			if (size > kMaxRecordBytes) { logger::error("co-save: reject record-size {}", size); continue; }
			Record record{ type, version, std::vector<std::uint8_t>(size) };
			if (size > 0 && a_intfc->ReadRecordData(record.bytes.data(), size) != size) {
				logger::error("co-save: short read on record {:08X}", type);
				continue;
			}
			records.push_back(std::move(record));
		}
		CoSaveState candidate;
		std::uint32_t loadedVersion = 0;
		if (LoadRecords(records, candidate, loadedVersion)) {
			CoSave::Get().State() = candidate;
			ItemRegistry::Get().Load(candidate.items);
			Progression::Get().LoadFromCoSave();
			MaterialRegistry::Get().LoadFromCoSave();
			logger::info("co-save: loaded schema {} ({} item entries)", loadedVersion, candidate.items.size());
			const auto firstContainer = candidate.generatedContainers.empty() ? 0u : candidate.generatedContainers.front();
			const auto firstItem = candidate.items.empty() ? 0u : candidate.items.front().registryId;
			const auto firstSkillRank = candidate.skillRanks.empty() ? 0 : candidate.skillRanks.begin()->second;
			const auto firstQuest = candidate.rewardedQuests.empty() ? 0u : candidate.rewardedQuests.front();
			logger::info("co-save: state rank={} stage={} xp={} class={} endgame={} containers={} quests={} items={} materials={} configHash={:08X} difficulty={} curve={} overrides={} skillCount={} firstContainer={} firstItem={}",
				candidate.playerRank, candidate.playerStage, candidate.playerXp, candidate.characterClass, candidate.endgameComplete ? 1 : 0,
				candidate.generatedContainers.size(), candidate.rewardedQuests.size(), candidate.items.size(), candidate.materials.size(), candidate.configHash,
				candidate.difficultyProfile, candidate.curveId, candidate.ladderOverrides.size(), candidate.skillRanks.size(), firstContainer, firstItem, firstSkillRank, firstQuest);
		} else {
			CoSave::Get().Reset();
			ItemRegistry::Get().Clear();
			Progression::Get().LoadFromCoSave();
			MaterialRegistry::Get().LoadFromCoSave();
			logger::info("co-save: no record; defaults");
		}
	}

	void CoSave::OnRevert(SKSE::SerializationInterface*)
	{
		CoSave::Get().Reset();
		ItemRegistry::Get().Clear();
		logger::info("co-save: reverted");
	}

	void CoSave::Register()
	{
		auto* serialization = SKSE::GetSerializationInterface();
		if (!serialization) {
			logger::error("co-save: serialization interface unavailable");
			return;
		}
		serialization->SetUniqueID(kSerializationUniqueId);
		serialization->SetSaveCallback(OnSave);
		serialization->SetLoadCallback(OnLoad);
		serialization->SetRevertCallback(OnRevert);
		logger::info("co-save: callbacks registered (record RNF2)");
	}

	SelfTestResult CoSave::SelfTest()
	{
		CoSaveState sample;
		sample.attributes[0] = 7;
		sample.attributes[8] = -3;
		sample.attributePoints = 5;
		sample.skillPoints = 2;
		sample.runes[0] = 11;
		sample.runes[4] = 1;
		sample.huntFirstKill["Draugr"] = 1;
		sample.huntFirstKill["Dragon"] = 0;
		sample.packVersions["unique"] = "0.2.0";
		ItemMetadata item;
		item.registryId = 3;
		item.baseFormId = 0x12EB7;
		item.rarity = 2;
		item.ilvl = 17;
		item.socketCount = 1;
		item.socketRunes[0] = 2;
		item.enhancementCount = 1;
		item.enhancements[0] = { "rf_affix_003", "RF_MGEF_Affix_003", 2, 1, 3.5f, 0, 4 };
		item.tags = static_cast<std::uint16_t>(ItemTag::ContainerLoot);
		sample.items.push_back(item);
		sample.configHash = HashConfig();
		sample.generatedContainers = { 0x00012345, 0x000ABCDE };
		sample.rewardedQuests = { 0x000F00D1 };
		sample.playerRank = 42;
		sample.playerXp = 1234;
		sample.endgameComplete = true;
		sample.playerStage = 42;
		sample.difficultyProfile = 2;
		sample.curveId = 1;
		sample.ladderOverrides.push_back({ 0x00000000000F00D1ull, 50, 1 });
		sample.skillRanks["Cleave"] = 3;
		sample.characterClass = "warrior";

		const auto encoded = Encode(sample);
		if (encoded.empty())
			return { "CoSave.SelfTest", CheckStatus::Fail, "encode returned empty" };
		if (encoded.size() > kMaxEncodedBytes)
			return { "CoSave.SelfTest", CheckStatus::Fail, "sample exceeds budget" };
		if (!(encoded[0] == 'R' && encoded[1] == 'N' && encoded[2] == 'F' && encoded[3] == 'S') ||
			encoded[4] != 3 || encoded[5] != 0 || encoded[6] != 0 || encoded[7] != 0 ||
			encoded[8] != 0 || encoded[9] != 0 || encoded[10] != 3 || encoded[11] != 0)
			return { "CoSave.SelfTest", CheckStatus::Fail, "header layout mismatch" };
		logger::info("co-save: header hex {:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}",
			encoded[0], encoded[1], encoded[2], encoded[3], encoded[4], encoded[5], encoded[6], encoded[7],
			encoded[8], encoded[9], encoded[10], encoded[11], encoded[12], encoded[13], encoded[14], encoded[15]);

		CoSaveState decoded;
		if (!Decode(encoded, decoded))
			return { "CoSave.SelfTest", CheckStatus::Fail, "round-trip decode failed" };
		if (decoded.attributes != sample.attributes || decoded.attributePoints != sample.attributePoints ||
			decoded.skillPoints != sample.skillPoints || decoded.runes != sample.runes ||
			decoded.huntFirstKill != sample.huntFirstKill || decoded.packVersions != sample.packVersions ||
			decoded.configHash != sample.configHash || decoded.items.size() != 1 ||
			decoded.items[0].registryId != item.registryId || decoded.items[0].enhancements[0].affixId != item.enhancements[0].affixId ||
			decoded.items[0].enhancements[0].magnitude != item.enhancements[0].magnitude || decoded.items[0].tags != item.tags ||
			decoded.generatedContainers != sample.generatedContainers || decoded.rewardedQuests != sample.rewardedQuests ||
			decoded.playerRank != sample.playerRank || decoded.playerXp != sample.playerXp || decoded.endgameComplete != sample.endgameComplete ||
			decoded.playerStage != sample.playerStage || decoded.difficultyProfile != sample.difficultyProfile || decoded.curveId != sample.curveId ||
			decoded.ladderOverrides.size() != 1 || decoded.ladderOverrides[0].key != sample.ladderOverrides[0].key ||
			decoded.ladderOverrides[0].stage != sample.ladderOverrides[0].stage || decoded.skillRanks != sample.skillRanks || decoded.characterClass != sample.characterClass)
			return { "CoSave.SelfTest", CheckStatus::Fail, "round-trip data mismatch" };
		if (Encode(decoded) != encoded)
			return { "CoSave.SelfTest", CheckStatus::Fail, "encode(decode(encode)) not byte-identical" };

		auto truncated = encoded;
		truncated.resize(truncated.size() / 2);
		CoSaveState sentinel;
		sentinel.playerRank = 77;
		sentinel.playerXp = 123;
		const auto sentinelBytes = Encode(sentinel);
		if (Decode(truncated, sentinel) || Encode(sentinel) != sentinelBytes)
			return { "CoSave.SelfTest", CheckStatus::Fail, "decode not failure-pure" };

		auto schemaZero = encoded;
		schemaZero[4] = 0;
		schemaZero[5] = 0;
		CoSaveState reject;
		CoSaveState decodeSentinel;
		decodeSentinel.playerRank = 77;
		const auto decodeSentinelBefore = Encode(decodeSentinel);
		if (Decode(schemaZero, decodeSentinel) || Encode(decodeSentinel) != decodeSentinelBefore)
			return { "CoSave.SelfTest", CheckStatus::Fail, "schema 0 accepted or mutated state" };
		if (LastRejectToken() != "co-save: reject schema")
			return { "CoSave.SelfTest", CheckStatus::Fail, "schema rejection token missing" };
		if (LoadRecord(encoded, 4, reject) || LoadRecord(encoded, 0, reject))
			return { "CoSave.SelfTest", CheckStatus::Fail, "record version gate invalid" };
		if (LastRejectToken() != "co-save: reject record-version")
			return { "CoSave.SelfTest", CheckStatus::Fail, "record-version rejection token missing" };
		auto schemaFour = encoded;
		schemaFour[4] = 4; schemaFour[5] = 0;
		CoSaveState schemaSentinel;
		schemaSentinel.playerRank = 77;
		const auto schemaSentinelBefore = Encode(schemaSentinel);
		if (Decode(schemaFour, schemaSentinel) || Encode(schemaSentinel) != schemaSentinelBefore)
			return { "CoSave.SelfTest", CheckStatus::Fail, "schema 4 accepted or mutated state" };

		// v2 migration fixture: 12-byte header, progression with runeTier=5, bossKillTier=2, no ladder.
		std::vector<std::uint8_t> v2;
		v2.insert(v2.end(), { 'R', 'N', 'F', 'S' });
		PutU16(v2, 2); PutU16(v2, 2);
		std::vector<std::vector<std::uint8_t>> v2sections;
		std::vector<std::uint8_t> v2progress;
		for (int i = 0; i < 9; ++i) PutI32(v2progress, 0);
		PutI32(v2progress, 0); PutI32(v2progress, 0); PutI32(v2progress, 5); PutI32(v2progress, 2);
		v2sections.push_back(BuildSection(kSectionProgression, v2progress));
		std::vector<std::uint8_t> v2runes; for (int i = 0; i < 5; ++i) PutI32(v2runes, 0);
		v2sections.push_back(BuildSection(kSectionRunes, v2runes));
		std::vector<std::uint8_t> zeroCount; PutU32(zeroCount, 0);
		v2sections.push_back(BuildSection(kSectionItems, zeroCount));
		v2sections.push_back(BuildSection(kSectionHunt, zeroCount));
		v2sections.push_back(BuildSection(kSectionMaterials, zeroCount));
		v2sections.push_back(BuildSection(kSectionPackState, zeroCount));
		std::vector<std::uint8_t> v2hash; PutU32(v2hash, 0); v2sections.push_back(BuildSection(kSectionConfigHash, v2hash));
		v2sections.push_back(BuildSection(kSectionContainers, zeroCount));
		v2sections.push_back(BuildSection(kSectionQuests, zeroCount));
		PutU32(v2, static_cast<std::uint32_t>(v2sections.size()));
		for (const auto& section : v2sections) v2.insert(v2.end(), section.begin(), section.end());
		CoSaveState migrated;
		if (!Decode(v2, migrated) || migrated.playerRank != 100 || migrated.playerStage != 100 ||
			migrated.difficultyProfile != 255 || migrated.curveId != 1 || migrated.bossKillTier != 2)
			return { "CoSave.SelfTest", CheckStatus::Fail, "v2 migration fixture failed" };

		std::vector<std::uint8_t> v1;
		v1.insert(v1.end(), { 'R', 'N', 'F', 'S' });
		PutU16(v1, 1); PutU16(v1, 1);
		std::vector<std::vector<std::uint8_t>> v1sections;
		std::vector<std::uint8_t> v1progress;
		for (int i = 0; i < 11; ++i) PutI32(v1progress, 0);
		v1sections.push_back(BuildSection(kSectionProgression, v1progress));
		std::vector<std::uint8_t> v1runes; for (int i = 0; i < 5; ++i) PutI32(v1runes, 0);
		v1sections.push_back(BuildSection(kSectionRunes, v1runes));
		std::vector<std::uint8_t> v1zero; PutU32(v1zero, 0);
		v1sections.push_back(BuildSection(kSectionItems, v1zero));
		v1sections.push_back(BuildSection(kSectionHunt, v1zero));
		v1sections.push_back(BuildSection(kSectionMaterials, v1zero));
		v1sections.push_back(BuildSection(kSectionPackState, v1zero));
		std::vector<std::uint8_t> v1hash; PutU32(v1hash, 0); v1sections.push_back(BuildSection(kSectionConfigHash, v1hash));
		v1sections.push_back(BuildSection(kSectionContainers, v1zero));
		v1sections.push_back(BuildSection(kSectionQuests, v1zero));
		PutU32(v1, static_cast<std::uint32_t>(v1sections.size()));
		for (const auto& section : v1sections) v1.insert(v1.end(), section.begin(), section.end());
		CoSaveState v1Decoded;
		if (!Decode(v1, v1Decoded) || v1Decoded.playerRank != 1 || v1Decoded.playerStage != 1 ||
			v1Decoded.difficultyProfile != 255 || v1Decoded.curveId != 1)
			return { "CoSave.SelfTest", CheckStatus::Fail, "v1 migration fixture failed" };

		auto v2tier2 = v2;
		const std::int32_t two = 2;
		std::memcpy(v2tier2.data() + 62, &two, sizeof(two));
		CoSaveState tier2Decoded;
		if (!Decode(v2tier2, tier2Decoded) || tier2Decoded.playerRank != 40 || tier2Decoded.playerStage != 40)
			return { "CoSave.SelfTest", CheckStatus::Fail, "v2 tier2 migration fixture failed" };

		// Deterministic encoding independent of unordered_map insertion order.
		CoSaveState reordered;
		reordered.packVersions["unique"] = "0.2.0";
		reordered.huntFirstKill["Dragon"] = 0;
		reordered.huntFirstKill["Draugr"] = 1;
		for (const auto& [key, value] : sample.skillRanks) reordered.skillRanks[key] = value;
				reordered.characterClass = "warrior";
		reordered.playerRank = 42;
		reordered.playerStage = 42;
		reordered.playerXp = 1234;
		reordered.endgameComplete = true;
		reordered.difficultyProfile = 2;
		reordered.curveId = 1;
		reordered.generatedContainers = { 0x00012345, 0x000ABCDE };
		reordered.rewardedQuests = { 0x000F00D1 };
		reordered.items = sample.items;
		reordered.runes[0] = 11;
		reordered.runes[4] = 1;
		reordered.attributePoints = 5;
		reordered.skillPoints = 2;
		reordered.attributes[0] = 7;
		reordered.attributes[8] = -3;
		reordered.configHash = sample.configHash;
		reordered.ladderOverrides = sample.ladderOverrides;
		if (Encode(sample) != Encode(reordered))
			return { "CoSave.SelfTest", CheckStatus::Fail, "encoding is not deterministic" };

		CoSaveState canonical;
		canonical.runeTier = 5;
		canonical.bossKillTier = 2;
		canonical.playerRank = 42;
		canonical.playerStage = 42;
		canonical.difficultyProfile = 2;
		canonical.curveId = 1;
		const std::string canonicalHex = "524E465303000000000003000A0000000100340000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000050000000200000002001400000000000000000000000000000000000000000000000300040000000000000004000400000000000000050004000000000000000700040000000000000008000400000000000000090004000000000000000A0004000000000000000B0010000000012A2A02010000000000000000000000";
		std::vector<std::uint8_t> canonicalExpected;
		for (std::size_t i = 0; i + 1 < canonicalHex.size(); i += 2)
			canonicalExpected.push_back(static_cast<std::uint8_t>(std::stoul(canonicalHex.substr(i, 2), nullptr, 16)));
		if (Encode(canonical) != canonicalExpected)
			return { "CoSave.SelfTest", CheckStatus::Fail, "canonical golden mismatch" };
		CoSaveState canonicalDecoded;
		if (!Decode(canonicalExpected, canonicalDecoded) || canonicalDecoded.playerRank != 42 || canonicalDecoded.playerStage != 42)
			return { "CoSave.SelfTest", CheckStatus::Fail, "canonical golden decode failed" };

		CoSaveState rejectedSentinel;
		rejectedSentinel.playerRank = 77;
		const auto rejectedBefore = Encode(rejectedSentinel);
		if (LoadRecord(encoded, 4, rejectedSentinel) || Encode(rejectedSentinel) != rejectedBefore)
			return { "CoSave.SelfTest", CheckStatus::Fail, "LoadRecord rejection mutated state" };
		CoSaveState duplicateItems = sample;
		duplicateItems.items.push_back(item);
		if (!Encode(duplicateItems).empty())
			return { "CoSave.SelfTest", CheckStatus::Fail, "duplicate registryId accepted" };
		std::vector<Record> noRecords;
		CoSaveState emptyOut;
		std::uint32_t emptyDefaultVersion = 0;
		if (LoadRecords(noRecords, emptyOut, emptyDefaultVersion)) return { "CoSave.SelfTest", CheckStatus::Fail, "empty records accepted" };
		if (Encode(emptyOut) != Encode(CoSaveState{})) return { "CoSave.SelfTest", CheckStatus::Fail, "empty record post-state not default" };

		// Encode caps and gate states.
		CoSaveState tooManySkills = sample;
		for (int i = 0; i < 258; ++i) tooManySkills.skillRanks["skill_" + std::to_string(i)] = 1;
		if (!Encode(tooManySkills).empty())
			return { "CoSave.SelfTest", CheckStatus::Fail, "skill cap not enforced" };
		CoSaveState tooManyItems = sample;
		for (int i = 0; i < 1025; ++i) tooManyItems.items.push_back(item);
		if (!Encode(tooManyItems).empty())
			return { "CoSave.SelfTest", CheckStatus::Fail, "item cap not enforced" };

		// 1000-item Phase-0 case remains within the 1 MiB budget.
		CoSaveState thousand;
		thousand.playerRank = 1;
		thousand.playerStage = 1;
		thousand.curveId = 1;
		for (int i = 1; i <= 1000; ++i) {
			ItemMetadata cheap;
			cheap.registryId = static_cast<std::uint32_t>(i);
			cheap.baseFormId = 0x12EB7;
			cheap.rarity = 0;
			cheap.ilvl = 1;
			cheap.tags = 0;
			thousand.items.push_back(cheap);
		}
		const auto thousandBytes = Encode(thousand);
		if (thousandBytes.empty() || thousandBytes.size() > kMaxEncodedBytes)
			return { "CoSave.SelfTest", CheckStatus::Fail, "1000-item budget failed" };

		CoSaveState badRank = sample;
		badRank.playerRank = 0;
		if (!Encode(badRank).empty()) return { "CoSave.SelfTest", CheckStatus::Fail, "rank range not enforced" };
		CoSaveState badSocket = sample;
		badSocket.items[0].socketRunes[0] = 9;
		if (!Encode(badSocket).empty()) return { "CoSave.SelfTest", CheckStatus::Fail, "socket range not enforced" };
		CoSaveState badEnhancement = sample;
		badEnhancement.items[0].enhancements[0].category = 7;
		if (!Encode(badEnhancement).empty()) return { "CoSave.SelfTest", CheckStatus::Fail, "enhancement range not enforced" };
		CoSaveState badOverride = sample;
		badOverride.ladderOverrides[0].flags = 2;
		if (!Encode(badOverride).empty()) return { "CoSave.SelfTest", CheckStatus::Fail, "override range not enforced" };
		auto zeroSections = encoded;
		zeroSections[12] = 0; zeroSections[13] = 0; zeroSections[14] = 0; zeroSections[15] = 0;
		CoSaveState zeroTarget;
		if (Decode(zeroSections, zeroTarget)) return { "CoSave.SelfTest", CheckStatus::Fail, "sectionCount 0 accepted" };
		std::vector<std::uint8_t> oversizedRecord(kMaxRecordBytes + 1, 0);
		CoSaveState oversizedTarget;
		if (LoadRecord(oversizedRecord, kSaveVersion, oversizedTarget)) return { "CoSave.SelfTest", CheckStatus::Fail, "oversized record accepted" };

		if (!EncodeWithBudget(sample, 100).empty()) return { "CoSave.SelfTest", CheckStatus::Fail, "budget refusal not enforced" };
		std::vector<std::uint8_t> semantic;
		semantic.insert(semantic.end(), { 'R', 'N', 'F', 'S' });
		PutU16(semantic, 3); PutU32(semantic, 0); PutU16(semantic, 3); PutU32(semantic, 1);
		PutU16(semantic, 1); PutU32(semantic, 4);
		for (int i = 0; i < 4; ++i) semantic.push_back(0);
		CoSaveState semanticSentinel;
		semanticSentinel.playerRank = 77;
		const auto semanticBefore = Encode(semanticSentinel);
		if (Decode(semantic, semanticSentinel) || Encode(semanticSentinel) != semanticBefore)
			return { "CoSave.SelfTest", CheckStatus::Fail, "semantic-invalid decode not failure-pure" };

		std::vector<Record> emptyRecords;
		CoSaveState emptyTarget;
		std::uint32_t emptyVersion = 0;
		if (LoadRecords(emptyRecords, emptyTarget, emptyVersion)) return { "CoSave.SelfTest", CheckStatus::Fail, "empty records accepted" };
		std::vector<Record> oneRecord{ { kSerializationRecord, kSaveVersion, encoded } };
		CoSaveState oneTarget;
		std::uint32_t oneVersion = 0;
		if (!LoadRecords(oneRecord, oneTarget, oneVersion) || oneVersion != kSaveVersion || oneTarget.playerRank != sample.playerRank) return { "CoSave.SelfTest", CheckStatus::Fail, "single record load failed" };
		CoSaveState secondState = sample;
		secondState.playerRank = 99;
		auto secondBytes = Encode(secondState);
		std::vector<Record> manyRecords{ { kSerializationRecord, kSaveVersion, encoded }, { kSerializationRecord, kSaveVersion, secondBytes } };
		CoSaveState manyTarget;
		std::uint32_t manyVersion = 0;
		if (!LoadRecords(manyRecords, manyTarget, manyVersion) || manyTarget.playerRank != sample.playerRank) return { "CoSave.SelfTest", CheckStatus::Fail, "multiple records did not keep first" };
		std::vector<Record> unknownRecords{ { 0xDEADBEEF, kSaveVersion, encoded } };
		CoSaveState unknownTarget;
		std::uint32_t unknownVersion = 0;
		if (LoadRecords(unknownRecords, unknownTarget, unknownVersion)) return { "CoSave.SelfTest", CheckStatus::Fail, "unknown record accepted" };
		std::vector<Record> repairRecords{ { kSerializationRecord, kSaveVersion, truncated }, { kSerializationRecord, kSaveVersion, encoded } };
		CoSaveState repairTarget;
		std::uint32_t repairVersion = 0;
		if (!LoadRecords(repairRecords, repairTarget, repairVersion) || repairTarget.playerRank != sample.playerRank)
			return { "CoSave.SelfTest", CheckStatus::Fail, "malformed-then-valid record did not recover" };

		for (const auto* token : {
			"co-save: reject section-count",
			"co-save: reject schema",
			"co-save: reject flags",
			"co-save: reject version-mirror",
			"co-save: reject item-count",
			"co-save: reject item.registryId",
			"co-save: reject item.baseFormId",
			"co-save: reject item.rarity",
			"co-save: reject item.ilvl",
			"co-save: reject item.socketCount",
			"co-save: reject item.enhancementCount",
			"co-save: reject item.socketRunes",
			"co-save: reject string.affix",
			"co-save: reject item.category",
			"co-save: reject item.tier",
			"co-save: reject item.tags",
			"co-save: reject hunt-count",
			"co-save: reject string.map-key",
			"co-save: reject material-count",
			"co-save: reject pack-count",
			"co-save: reject string.pack",
			"co-save: reject container-count",
			"co-save: reject quest-count",
			"co-save: reject override-count",
			"co-save: reject string.class",
			"co-save: reject skill-count",
			"co-save: reject string.skill",
			"co-save: reject range",
			"co-save: reject trailing",
			"co-save: reject duplicate-section",
		}) {
			MarkReject(token);
			if (LastRejectToken() != token) return { "CoSave.SelfTest", CheckStatus::Fail, "reject token table mismatch" };
		}
		// Eviction is smallest FormID, not FIFO.
		auto savedContainers = CoSave::Get().State().generatedContainers;
		CoSave::Get().State().generatedContainers.clear();
		for (std::uint32_t id = 1; id <= 4096; ++id) CoSave::Get().RememberGeneratedContainer(id);
		CoSave::Get().RememberGeneratedContainer(0xABCDEF);
		if (CoSave::Get().HasGeneratedContainer(1) || !CoSave::Get().HasGeneratedContainer(0xABCDEF) ||
			CoSave::Get().State().generatedContainers.size() != 4096)
			return { "CoSave.SelfTest", CheckStatus::Fail, "eviction policy failed" };
		CoSave::Get().State().generatedContainers = std::move(savedContainers);

		return { "CoSave.SelfTest", CheckStatus::Pass,
			std::format("round-trip {} bytes; header/sentinel/v2/deterministic/caps/budget/eviction OK", encoded.size()) };
	}
}
