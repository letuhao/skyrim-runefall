#include "CoSaveCodec.h"
#include "catch_amalgamated.hpp"

using namespace Runefall;

namespace
{
	CoSaveState Sample()
	{
		CoSaveState s;
		s.playerRank = 42;
		s.playerStage = 42;
		s.playerXp = 1234;
		s.difficultyProfile = 2;
		s.curveId = 1;
		s.characterClass = "warrior";
		s.skillRanks["Cleave"] = 3;
		s.runeTier = 5;
		s.bossKillTier = 2;
		s.generatedContainers = { 0x12345, 0xABCDE };
		s.rewardedQuests = { 0xF00D1 };
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
		s.items.push_back(item);
		return s;
	}
}

TEST_CASE("CoSaveCodec header and round-trip", "[cosave]")
{
	const auto sample = Sample();
	const auto bytes = CoSaveCodec::Encode(sample);
	REQUIRE(!bytes.empty());
	REQUIRE(bytes.size() <= CoSaveCodec::kMaxEncodedBytes);
	REQUIRE(bytes[0] == 'R');
	REQUIRE(bytes[1] == 'N');
	REQUIRE(bytes[2] == 'F');
	REQUIRE(bytes[3] == 'S');
	REQUIRE(bytes[4] == 3);
	REQUIRE(bytes[5] == 0);
	REQUIRE(bytes[10] == 3);
	CoSaveState decoded;
	REQUIRE(CoSaveCodec::Decode(bytes, decoded));
	REQUIRE(decoded.playerRank == sample.playerRank);
	REQUIRE(decoded.playerXp == sample.playerXp);
	REQUIRE(decoded.skillRanks == sample.skillRanks);
	REQUIRE(decoded.items.size() == 1);
	REQUIRE(decoded.items[0].registryId == 3);
	REQUIRE(CoSaveCodec::Encode(decoded) == bytes);
}

TEST_CASE("CoSaveCodec failure purity", "[cosave]")
{
	CoSaveState sentinel;
	sentinel.playerRank = 77;
	sentinel.playerXp = 99;
	const auto before = CoSaveCodec::Encode(sentinel);
	auto bad = before;
	bad.resize(before.size() / 2);
	REQUIRE_FALSE(CoSaveCodec::Decode(bad, sentinel));
	REQUIRE(CoSaveCodec::Encode(sentinel) == before);

	auto schemaZero = before;
	schemaZero[4] = 0;
	schemaZero[5] = 0;
	REQUIRE_FALSE(CoSaveCodec::Decode(schemaZero, sentinel));
	REQUIRE(CoSaveCodec::Encode(sentinel) == before);
}

TEST_CASE("CoSaveCodec record-version gate token", "[cosave]")
{
	const auto sample = Sample();
	const auto bytes = CoSaveCodec::Encode(sample);
	CoSaveState out;
	REQUIRE_FALSE(CoSaveCodec::LoadRecord(bytes, 0, out));
	REQUIRE(CoSaveCodec::LastRejectToken() == "co-save: reject record-version");
	REQUIRE_FALSE(CoSaveCodec::LoadRecord(bytes, 4, out));
	REQUIRE(CoSaveCodec::LastRejectToken() == "co-save: reject record-version");
}

TEST_CASE("CoSaveCodec LoadRecords zero/one/malformed-then-valid/unknown", "[cosave]")
{
	const auto sample = Sample();
	const auto bytes = CoSaveCodec::Encode(sample);
	CoSaveState out;
	REQUIRE_FALSE(CoSaveCodec::LoadRecords({}, 3, out));

	std::vector<CoSaveRecord> one{ { CoSaveCodec::kSerializationRecord, 3, bytes } };
	REQUIRE(CoSaveCodec::LoadRecords(one, 3, out));
	REQUIRE(out.playerRank == sample.playerRank);

	auto bad = bytes;
	bad.resize(bad.size() / 2);
	std::vector<CoSaveRecord> repair{ { CoSaveCodec::kSerializationRecord, 3, bad }, { CoSaveCodec::kSerializationRecord, 3, bytes } };
	REQUIRE(CoSaveCodec::LoadRecords(repair, 3, out));
	REQUIRE(out.playerRank == sample.playerRank);

	std::vector<CoSaveRecord> unknown{ { 'NOPE', 3, bytes } };
	REQUIRE_FALSE(CoSaveCodec::LoadRecords(unknown, 3, out));
}

TEST_CASE("CoSaveCodec deterministic encoding", "[cosave]")
{
	auto a = Sample();
	auto b = Sample();
	b.generatedContainers = { 0xABCDE, 0x12345 };
	b.skillRanks.clear();
	b.skillRanks["Cleave"] = 3;
	REQUIRE(CoSaveCodec::Encode(a) == CoSaveCodec::Encode(b));
}
