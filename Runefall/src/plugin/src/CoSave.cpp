#include "CoSave.h"

#include "ItemRegistry.h"
#include "Progression.h"
#include "MaterialRegistry.h"

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
			state.generatedContainers.erase(state.generatedContainers.begin());
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
			state.rewardedQuests.erase(state.rewardedQuests.begin());
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
		std::vector<std::uint8_t> out;
		out.insert(out.end(), { 'R', 'N', 'F', 'S' });
		PutU16(out, kSaveVersion);
		PutU32(out, 0);  // flags, reserved
		PutU16(out, kSaveVersion);

		std::vector<std::vector<std::uint8_t>> sections;

		std::vector<std::uint8_t> progress;
		for (const auto value : a_state.attributes)
			PutI32(progress, value);
		PutI32(progress, a_state.attributePoints);
		PutI32(progress, a_state.skillPoints);
		PutI32(progress, a_state.runeTier);
		PutI32(progress, a_state.bossKillTier);
		sections.push_back(BuildSection(kSectionProgression, progress));

		std::vector<std::uint8_t> runes;
		for (const auto value : a_state.runes)
			PutI32(runes, value);
		sections.push_back(BuildSection(kSectionRunes, runes));

		std::vector<std::uint8_t> items;
		PutU32(items, static_cast<std::uint32_t>(a_state.items.size()));
		for (const auto& item : a_state.items) {
			PutU32(items, item.registryId);
			PutU32(items, item.baseFormId);
			items.push_back(item.rarity);
			PutU16(items, item.ilvl);
			items.push_back(item.socketCount);
			for (const auto rune : item.socketRunes)
				PutI32(items, rune);
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
		PutU32(hunt, static_cast<std::uint32_t>(a_state.huntFirstKill.size()));
		for (const auto& [id, flag] : a_state.huntFirstKill) {
			PutString(hunt, id);
			hunt.push_back(flag);
		}
		sections.push_back(BuildSection(kSectionHunt, hunt));

		std::vector<std::uint8_t> materials;
		PutU32(materials, static_cast<std::uint32_t>(a_state.materials.size()));
		for (const auto& [id, count] : a_state.materials) {
			PutString(materials, id);
			PutI32(materials, count);
		}
		sections.push_back(BuildSection(kSectionMaterials, materials));

		std::vector<std::uint8_t> packs;
		PutU32(packs, static_cast<std::uint32_t>(a_state.packVersions.size()));
		for (const auto& [id, version] : a_state.packVersions) {
			PutString(packs, id);
			PutString(packs, version);
		}
		sections.push_back(BuildSection(kSectionPackState, packs));

		std::vector<std::uint8_t> hash;
		PutU32(hash, a_state.configHash);
		sections.push_back(BuildSection(kSectionConfigHash, hash));

		std::vector<std::uint8_t> containers;
		PutU32(containers, static_cast<std::uint32_t>(a_state.generatedContainers.size()));
		for (const auto id : a_state.generatedContainers)
			PutU32(containers, id);
		sections.push_back(BuildSection(kSectionContainers, containers));

		std::vector<std::uint8_t> quests;
		PutU32(quests, static_cast<std::uint32_t>(a_state.rewardedQuests.size()));
		for (const auto id : a_state.rewardedQuests)
			PutU32(quests, id);
		sections.push_back(BuildSection(kSectionQuests, quests));

		std::vector<std::uint8_t> ladder;
		PutU8(ladder, 1);  // ladder section schema
		PutU8(ladder, a_state.playerRank);
		PutU8(ladder, a_state.playerStage);
		PutU8(ladder, a_state.difficultyProfile);
		PutU8(ladder, a_state.curveId);
		PutU16(ladder, static_cast<std::uint16_t>(a_state.ladderOverrides.size()));
		for (const auto& entry : a_state.ladderOverrides) {
			PutU64(ladder, entry.key);
			PutU8(ladder, entry.stage);
			PutU8(ladder, entry.flags);
		}
		PutU32(ladder, a_state.playerXp);
		PutString(ladder, a_state.characterClass);
		PutU16(ladder, static_cast<std::uint16_t>((std::min)(a_state.skillRanks.size(), static_cast<std::size_t>(256))));
		for (const auto& [skill, rank] : a_state.skillRanks) {
			PutString(ladder, skill);
			PutI32(ladder, rank);
		}
		PutU8(ladder, a_state.endgameComplete ? 1 : 0);
		sections.push_back(BuildSection(kSectionLadder, ladder));

		PutU32(out, static_cast<std::uint32_t>(sections.size()));
		for (const auto& section : sections)
			out.insert(out.end(), section.begin(), section.end());
		return out;
	}

	bool CoSave::Decode(const std::vector<std::uint8_t>& a_bytes, CoSaveState& a_out)
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
		if (!reader.U16(schema) || schema > kSaveVersion)
			return false;
		if (schema >= 3 && !reader.U32(flags))
			return false;
		if (!reader.U16(saveVersion) || !reader.U32(sectionCount))
			return false;

		constexpr std::uint32_t kMaxSections = 64;
		if (sectionCount > kMaxSections) {
			logger::error("co-save: sectionCount {} exceeds cap {}", sectionCount, kMaxSections);
			return false;
		}
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
				if (!sectionReader.U32(count) || count > 1000)
					return false;
				for (std::uint32_t i = 0; i < count; ++i) {
					std::string id;
					std::int32_t value = 0;
					if (!sectionReader.String(id) || id.size() > 128 || !sectionReader.I32(value))
						return false;
					a_out.materials[id] = value;
				}
				break;
			}
			case kSectionItems: {
				std::uint32_t count = 0;
				if (!sectionReader.U32(count) || count > 100000)
					return false;
				a_out.items.resize(count);
				for (auto& item : a_out.items) {
					if (!sectionReader.U32(item.registryId) || !sectionReader.U32(item.baseFormId))
						return false;
					if (!sectionReader.U8(item.rarity) || !sectionReader.U16(item.ilvl) || !sectionReader.U8(item.socketCount))
						return false;
					if (item.rarity > 5 || item.socketCount > 3) {
						logger::error("co-save: invalid item rarity {} / socketCount {}", item.rarity, item.socketCount);
						return false;
					}
					for (auto& rune : item.socketRunes)
						if (!sectionReader.I32(rune))
							return false;
					if (!sectionReader.U8(item.enhancementCount) || item.enhancementCount > 5)
						return false;
					for (auto& enhancement : item.enhancements) {
						if (!sectionReader.String(enhancement.affixId) || !sectionReader.String(enhancement.effectEditorId))
							return false;
						if (!sectionReader.I32(enhancement.category) || !sectionReader.I32(enhancement.tier) || !sectionReader.Float(enhancement.magnitude))
							return false;
						if (!sectionReader.I32(enhancement.duration) || !sectionReader.I32(enhancement.area))
							return false;
					}
					if (!sectionReader.U16(item.tags))
						return false;
				}
				break;
			}
			case kSectionHunt: {
				std::uint32_t count = 0;
				if (!sectionReader.U32(count) || count > 10000)
					return false;
				for (std::uint32_t n = 0; n < count; ++n) {
					std::string   key;
					std::uint8_t  flag = 0;
					if (!sectionReader.String(key) || !sectionReader.U8(flag))
						return false;
					a_out.huntFirstKill[std::move(key)] = flag;
				}
				break;
			}
			case kSectionPackState: {
				std::uint32_t count = 0;
				if (!sectionReader.U32(count) || count > 1000)
					return false;
				for (std::uint32_t n = 0; n < count; ++n) {
					std::string key;
					std::string version;
					if (!sectionReader.String(key) || !sectionReader.String(version))
						return false;
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
				if (!sectionReader.U32(count) || count > kMaxPersistedRefs)
					return false;
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
				if (!sectionReader.U32(count) || count > kMaxPersistedQuests)
					return false;
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
				std::uint16_t count = 0;
				if (!sectionReader.U16(count) || count > kMaxLadderOverrides)
					return false;
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
					if (!sectionReader.U32(a_out.playerXp))
						return false;
				}
				if (sectionReader.Remaining() > 0) {
					if (!sectionReader.String(a_out.characterClass) || a_out.characterClass.size() > 64)
						return false;
					std::uint16_t skillCount = 0;
					if (!sectionReader.U16(skillCount) || skillCount > 256)
						return false;
					for (std::uint16_t s = 0; s < skillCount; ++s) {
						std::string skill;
						std::int32_t rank = 0;
						if (!sectionReader.String(skill) || skill.size() > 64 || !sectionReader.I32(rank))
							return false;
						if (rank > 0)
							a_out.skillRanks[skill] = rank;
					}
					if (sectionReader.Remaining() >= 1) {
						std::uint8_t complete = 0;
						if (!sectionReader.U8(complete))
							return false;
						a_out.endgameComplete = complete != 0;
					}
				}
				break;
			}
			default:
				break;
			}
		}
		MigrateLegacy(a_out, schema);
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

	void CoSave::OnSave(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc)
			return;
		auto& state = CoSave::Get().State();
		Progression::Get().SaveToCoSave();
		MaterialRegistry::Get().SaveToCoSave();
		state.items = ItemRegistry::Get().All();
		state.configHash = HashConfig();
		const auto bytes = Encode(state);
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
		if (!a_intfc)
			return;
		CoSave::Get().Reset();

		std::uint32_t type = 0;
		std::uint32_t size = 0;
		std::uint32_t version = 0;
		while (a_intfc->GetNextRecordInfo(type, version, size)) {
			constexpr std::uint32_t kMaxRecordBytes = 16u * 1024u * 1024u;
			if (size > kMaxRecordBytes) {
				logger::error("co-save: record {:08X} size {} exceeds cap; skipped", type, size);
				continue;
			}
			std::vector<std::uint8_t> payload(size);
			if (size > 0 && a_intfc->ReadRecordData(payload.data(), size) != size) {
				logger::error("co-save: short read on record {:08X}", type);
				continue;
			}
			if (type != kSerializationRecord)
				continue;
			if (version > kSaveVersion) {
				logger::error("co-save: schema {} newer than supported {}", version, kSaveVersion);
				continue;
			}
			if (Decode(payload, CoSave::Get().State())) {
				ItemRegistry::Get().Load(CoSave::Get().State().items);
				Progression::Get().LoadFromCoSave();
				MaterialRegistry::Get().LoadFromCoSave();
				logger::info("co-save: loaded schema {} ({} bytes, {} item entries)", version, payload.size(), CoSave::Get().State().items.size());
			} else {
				logger::error("co-save: malformed section payload, state reset");
			}
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
		sample.generatedContainers = { 0x000ABCDE, 0x00012345 };
		sample.rewardedQuests = { 0x000F00D1 };
		sample.playerRank = 42;
		sample.playerXp = 1234;
		sample.endgameComplete = true;
		sample.playerStage = 42;
		sample.difficultyProfile = 2;
		sample.curveId = 1;
		sample.ladderOverrides.push_back({ 0x00000000000F00D1ull, 50, 1 });

		const auto encoded = Encode(sample);
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
			decoded.playerRank != sample.playerRank || decoded.playerXp != sample.playerXp || decoded.endgameComplete != sample.endgameComplete || decoded.playerStage != sample.playerStage ||
			decoded.difficultyProfile != sample.difficultyProfile || decoded.curveId != sample.curveId ||
			decoded.ladderOverrides.size() != 1 || decoded.ladderOverrides[0].key != sample.ladderOverrides[0].key ||
			decoded.ladderOverrides[0].stage != sample.ladderOverrides[0].stage)
			return { "CoSave.SelfTest", CheckStatus::Fail, "round-trip data mismatch" };

		// Truncated input must be rejected, not crash.
		auto truncated = encoded;
		truncated.resize(truncated.size() / 2);
		CoSaveState ignored;
		if (Decode(truncated, ignored))
			return { "CoSave.SelfTest", CheckStatus::Fail, "truncated payload was accepted" };

		return { "CoSave.SelfTest", CheckStatus::Pass, std::format("round-trip {} bytes; truncation rejected", encoded.size()) };
	}
}