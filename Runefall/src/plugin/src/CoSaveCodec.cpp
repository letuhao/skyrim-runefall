#include "CoSaveCodec.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <format>
#include <string_view>

namespace Runefall
{
	namespace logger
	{
		template <class... Args> void error(const char*, Args&&...) {}
		template <class... Args> void info(const char*, Args&&...) {}
		template <class... Args> void warn(const char*, Args&&...) {}
	}

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


	const std::string& CoSaveCodec::LastRejectToken()
	{
		return gLastRejectToken;
	}

	std::vector<std::uint8_t> CoSaveCodec::Encode(const CoSaveState& a_state)
	{
		return EncodeWithBudget(a_state, kMaxEncodedBytes);
	}

	std::vector<std::uint8_t> CoSaveCodec::EncodeWithBudget(const CoSaveState& a_state, std::size_t a_budget)
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
		if (out.size() > a_budget) { MarkReject("co-save: reject budget"); return {}; }
		return out;
	}

bool CoSaveCodec::Decode(const std::vector<std::uint8_t>& a_bytes, CoSaveState& a_out)
	{
		CoSaveState candidate;
		if (!DecodeImpl(a_bytes, candidate))
			return false;
		a_out = std::move(candidate);
		return true;
	}

	bool CoSaveCodec::LoadRecord(const std::vector<std::uint8_t>& a_bytes, std::uint32_t a_recordVersion, CoSaveState& a_out)
	{
		if (a_recordVersion == 0 || a_recordVersion > kSaveVersion) {
			MarkReject("co-save: reject record-version");
			logger::error("co-save: reject record-version {}", a_recordVersion);
			return false;
		}
		return Decode(a_bytes, a_out);
	}

	bool CoSaveCodec::DecodeImpl(const std::vector<std::uint8_t>& a_bytes, CoSaveState& a_out)
	{
		a_out = CoSaveState{};
		if (a_bytes.size() > kMaxRecordBytes) { MarkReject("co-save: reject record-size"); return false; }
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
			MarkReject("co-save: reject version-mirror"); logger::error("co-save: reject version-mirror");
			return false;
		}
		if (!reader.U32(sectionCount) || sectionCount == 0) { MarkReject("co-save: reject section-count"); logger::error("co-save: reject section-count"); return false; }
		constexpr std::uint32_t kMaxSections = 64;
		if (sectionCount > kMaxSections) { MarkReject("co-save: reject section-count"); logger::error("co-save: reject section-count {}", sectionCount); return false; }
		std::array<bool, 16> seenSections{};
		for (std::uint32_t i = 0; i < sectionCount; ++i) {
			std::uint16_t id = 0;
			std::uint32_t len = 0;
			if (!reader.U16(id) || !reader.U32(len) || len > reader.Remaining()) {
				MarkReject("co-save: reject trailing"); logger::error("co-save: reject trailing");
				return false;
			}
			if (len == 0) { MarkReject("co-save: reject empty-section"); logger::error("co-save: reject empty-section"); return false; }

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
				if (seenSections[id]) { MarkReject("co-save: reject duplicate-section"); logger::error("co-save: reject duplicate-section"); return false; }
				seenSections[id] = true;
			} else {
				MarkReject("co-save: drain unknown-record");
				continue;
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
					if (a_out.materials.contains(id)) { MarkReject("co-save: reject duplicate-key"); return false; }
					a_out.materials[id] = value;
				}
				break;
			}
			case kSectionItems: {
				std::uint32_t count = 0;
				if (!sectionReader.U32(count) || count > 1024) {
				    MarkReject("co-save: reject item-count"); logger::error("co-save: reject item-count"); return false;
				}
				if (sectionReader.Remaining() < count * 100u) { MarkReject("co-save: reject count-remaining"); return false; }
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
				for (std::size_t i = 0; i < a_out.items.size(); ++i)
					for (std::size_t j = i + 1; j < a_out.items.size(); ++j)
						if (a_out.items[i].registryId == a_out.items[j].registryId) { MarkReject("co-save: reject item.registryId"); return false; }
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
					if (a_out.huntFirstKill.contains(key)) { MarkReject("co-save: reject duplicate-key"); return false; }
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
					if (a_out.packVersions.contains(key)) { MarkReject("co-save: reject duplicate-key"); return false; }
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
					    MarkReject("co-save: reject string"); logger::error("co-save: reject string"); return false;
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
						if (a_out.skillRanks.contains(skill)) { MarkReject("co-save: reject duplicate-key"); return false; }
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
		if (!seenSections[kSectionProgression]) { MarkReject("co-save: reject missing-section"); return false; }
		return true;
	}

	void CoSaveCodec::MigrateLegacy(CoSaveState& a_state, std::uint16_t a_schema)
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

	bool CoSaveCodec::LoadRecords(const std::vector<CoSaveRecord>& a_records, std::uint16_t a_recordVersion, CoSaveState& a_out)
	{
		if (a_recordVersion == 0 || a_recordVersion > kSaveVersion) { MarkReject("co-save: reject record-version"); return false; }
		CoSaveState candidate;
		bool loaded = false;
		for (const auto& record : a_records) {
			if (record.type != kSerializationRecord) continue;
			if (loaded) { logger::info("co-save: duplicate record ignored"); continue; }
			if (LoadRecord(record.bytes, record.version, candidate)) { loaded = true; }
			else logger::error("co-save: reject record; defaults kept");
		}
		if (!loaded) { a_out = CoSaveState{}; return false; }
		a_out = std::move(candidate);
		return true;
	}


}
