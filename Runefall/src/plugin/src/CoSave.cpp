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
	using Record = CoSaveRecord;

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

		void MarkReject(std::string_view) {}

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

		void PutI32(std::vector<std::uint8_t>& a_out, std::int32_t a_value)
		{
			PutU32(a_out, static_cast<std::uint32_t>(a_value));
		}

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
		return CoSaveCodec::LastRejectToken();
	}

	std::vector<std::uint8_t> CoSave::Encode(const CoSaveState& a_state)
	{
		return CoSaveCodec::Encode(a_state);
	}

	std::vector<std::uint8_t> CoSave::EncodeWithBudget(const CoSaveState& a_state, std::size_t a_budget)
	{
		return CoSaveCodec::EncodeWithBudget(a_state, a_budget);
	}

	bool CoSave::Decode(const std::vector<std::uint8_t>& a_bytes, CoSaveState& a_out)
	{
		return CoSaveCodec::Decode(a_bytes, a_out);
	}

	bool CoSave::LoadRecord(const std::vector<std::uint8_t>& a_bytes, std::uint32_t a_recordVersion, CoSaveState& a_out)
	{
		return CoSaveCodec::LoadRecord(a_bytes, a_recordVersion, a_out);
	}

	bool CoSave::LoadRecords(const std::vector<CoSaveRecord>& a_records, CoSaveState& a_out, std::uint32_t& a_recordVersion)
	{
		const auto version = a_records.empty() ? static_cast<std::uint16_t>(CoSaveCodec::kSaveVersion) : a_records.front().version;
		const bool ok = CoSaveCodec::LoadRecords(a_records, version, a_out);
		if (ok && !a_records.empty())
			a_recordVersion = a_records.front().version;
		else
			a_recordVersion = version;
		return ok;
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

	void CoSave::ClearLoadedState()
	{
		Reset();
		ItemRegistry::Get().Clear();
		Progression::Get().LoadFromCoSave();
		MaterialRegistry::Get().LoadFromCoSave();
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
		std::vector<CoSaveRecord> records;
		while (a_intfc->GetNextRecordInfo(type, version, size)) {
			if (version > kSaveVersion) logger::warn("co-save: warn newer-save version={}", version);
			if (size > kMaxRecordBytes) { MarkReject("co-save: reject record-size"); logger::error("co-save: reject record-size {}", size); continue; }
			CoSaveRecord record{ type, version, std::vector<std::uint8_t>(size) };
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
			CoSave::Get().ClearLoadedState();
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
		std::vector<CoSaveRecord> noRecords;
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

		std::vector<CoSaveRecord> emptyRecords;
		CoSaveState emptyTarget;
		std::uint32_t emptyVersion = 0;
		if (LoadRecords(emptyRecords, emptyTarget, emptyVersion)) return { "CoSave.SelfTest", CheckStatus::Fail, "empty records accepted" };
		std::vector<CoSaveRecord> oneRecord{ { kSerializationRecord, kSaveVersion, encoded } };
		CoSaveState oneTarget;
		std::uint32_t oneVersion = 0;
		if (!LoadRecords(oneRecord, oneTarget, oneVersion) || oneVersion != kSaveVersion || oneTarget.playerRank != sample.playerRank) return { "CoSave.SelfTest", CheckStatus::Fail, "single record load failed" };
		CoSaveState secondState = sample;
		secondState.playerRank = 99;
		auto secondBytes = Encode(secondState);
		std::vector<CoSaveRecord> manyRecords{ { kSerializationRecord, kSaveVersion, encoded }, { kSerializationRecord, kSaveVersion, secondBytes } };
		CoSaveState manyTarget;
		std::uint32_t manyVersion = 0;
		if (!LoadRecords(manyRecords, manyTarget, manyVersion) || manyTarget.playerRank != sample.playerRank) return { "CoSave.SelfTest", CheckStatus::Fail, "multiple records did not keep first" };
		std::vector<CoSaveRecord> unknownRecords{ { 0xDEADBEEF, kSaveVersion, encoded } };
		CoSaveState unknownTarget;
		std::uint32_t unknownVersion = 0;
		if (LoadRecords(unknownRecords, unknownTarget, unknownVersion)) return { "CoSave.SelfTest", CheckStatus::Fail, "unknown record accepted" };
		std::vector<CoSaveRecord> repairRecords{ { kSerializationRecord, kSaveVersion, truncated }, { kSerializationRecord, kSaveVersion, encoded } };
		CoSaveState repairTarget;
		std::uint32_t repairVersion = 0;
		if (!LoadRecords(repairRecords, repairTarget, repairVersion) || repairTarget.playerRank != sample.playerRank)
			return { "CoSave.SelfTest", CheckStatus::Fail, "malformed-then-valid record did not recover" };

		const auto savedState = CoSave::Get().State();
		const auto savedItems = ItemRegistry::Get().All();
		CoSave::Get().ClearLoadedState();
		const CoSaveState defaults;
		const auto& cleared = CoSave::Get().State();
		const bool clearOk = cleared.playerRank == defaults.playerRank && cleared.playerStage == defaults.playerStage &&
			cleared.playerXp == defaults.playerXp && cleared.characterClass == defaults.characterClass &&
			cleared.endgameComplete == defaults.endgameComplete && cleared.items.empty() &&
			ItemRegistry::Get().All().empty() && Progression::Get().AttributePoints() == 0 &&
			MaterialRegistry::Get().Total() == 0;
		CoSave::Get().State() = savedState;
		ItemRegistry::Get().Load(savedItems);
		Progression::Get().LoadFromCoSave();
		MaterialRegistry::Get().LoadFromCoSave();
		if (!clearOk) return { "CoSave.SelfTest", CheckStatus::Fail, "ClearLoadedState did not reset state/registries" };

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

