#include "CoSaveCodec.h"
#ifdef RUNEFALL_VENDORED_CATCH2
#include "catch_amalgamated.hpp"
#else
#include <catch2/catch_test_macros.hpp>
#endif

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


namespace
{
	using Bytes = std::vector<std::uint8_t>;

	Bytes U16v(std::uint16_t v) { return { static_cast<std::uint8_t>(v & 0xFF), static_cast<std::uint8_t>(v >> 8) }; }
	Bytes U32v(std::uint32_t v) { Bytes b; for (int i = 0; i < 4; ++i) b.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF)); return b; }
	Bytes Hdr(std::uint32_t sections, std::uint16_t schema = 3, std::uint16_t version = 3, std::uint32_t flags = 0)
	{
		Bytes b{ 'R', 'N', 'F', 'S' };
		Bytes s = U16v(schema); b.insert(b.end(), s.begin(), s.end());
		Bytes f = U32v(flags); b.insert(b.end(), f.begin(), f.end());
		Bytes v = U16v(version); b.insert(b.end(), v.begin(), v.end());
		Bytes c = U32v(sections); b.insert(b.end(), c.begin(), c.end());
		return b;
	}
	Bytes Progression()
	{
		Bytes b;
		for (int i = 0; i < 9; ++i) { Bytes v = U32v(0); b.insert(b.end(), v.begin(), v.end()); }
		Bytes a = U32v(0); b.insert(b.end(), a.begin(), a.end());
		Bytes s = U32v(0); b.insert(b.end(), s.begin(), s.end());
		return b;
	}
	Bytes I32v(std::int32_t v) { return U32v(static_cast<std::uint32_t>(v)); }
	Bytes Str(const std::string& s) { Bytes b = U16v(static_cast<std::uint16_t>(s.size())); b.insert(b.end(), s.begin(), s.end()); return b; }
	Bytes Cat(std::initializer_list<Bytes> parts)
	{
		std::size_t total = 0;
		for (const auto& p : parts) total += p.size();
		Bytes b;
		b.reserve(total);
		for (const auto& p : parts) b.insert(b.end(), p.begin(), p.end());
		return b;
	}
	Bytes Sec(std::uint16_t id, const Bytes& payload)
	{
		Bytes b = U16v(id);
		Bytes len = U32v(static_cast<std::uint32_t>(payload.size()));
		b.insert(b.end(), len.begin(), len.end());
		b.insert(b.end(), payload.begin(), payload.end());
		return b;
	}
	Bytes Save(std::initializer_list<Bytes> sections)
	{
		Bytes b = Hdr(static_cast<std::uint32_t>(sections.size()));
		for (const auto& section : sections) b.insert(b.end(), section.begin(), section.end());
		return b;
	}
}

TEST_CASE("CoSaveCodec A6 rejection token classes", "[cosave][a6]")
{
	CoSaveState out;
	auto expect = [&](const Bytes& bytes, const std::string& token) {
		out = CoSaveState{};
		REQUIRE_FALSE(CoSaveCodec::Decode(bytes, out));
		REQUIRE(CoSaveCodec::LastRejectToken() == token);
	};

	auto base = Save({ Sec(1, Progression()) });
	REQUIRE(CoSaveCodec::Decode(base, out));
	REQUIRE(out.playerRank == 1);

	auto flags = base; flags[6] = 1; expect(flags, "co-save: reject flags");
	auto mirror = base; mirror[10] = 2; expect(mirror, "co-save: reject version-mirror");
	auto count0 = base; count0[12] = count0[13] = count0[14] = count0[15] = 0; expect(count0, "co-save: reject section-count");
	auto count65 = base; count65[12] = 65; expect(count65, "co-save: reject section-count");
	expect(Save({ Sec(1, {}) }), "co-save: reject empty-section");
	expect(Save({ Sec(2, Bytes(20, 0)) }), "co-save: reject missing-section");
	expect(Save({ Sec(1, Progression()), Sec(1, Progression()) }), "co-save: reject duplicate-section");
	expect(Save({ Sec(1, Progression()), Sec(3, U32v(1024)) }), "co-save: reject count-remaining");
	expect(Save({ Sec(1, Progression()), Sec(5, Cat({ U32v(2), Str("a"), I32v(1), Str("a"), I32v(2) })) }), "co-save: reject duplicate-key");
	auto big = Bytes(CoSaveCodec::kMaxRecordBytes + 1, 0); expect(big, "co-save: reject record-size");
	REQUIRE(CoSaveCodec::EncodeWithBudget(Sample(), 8).empty());
	REQUIRE(CoSaveCodec::LastRejectToken() == "co-save: reject budget");
}

TEST_CASE("CoSaveCodec A6 accepted classes", "[cosave][a6]")
{
	CoSaveState out;
	auto unknown = Save({ Sec(99, { 1, 2, 3 }), Sec(1, Progression()) });
	REQUIRE(CoSaveCodec::Decode(unknown, out));
	REQUIRE(CoSaveCodec::LastRejectToken() == "co-save: drain unknown-record");
	REQUIRE(out.playerRank == 1);

	std::vector<CoSaveRecord> two{ { CoSaveCodec::kSerializationRecord, 3, CoSaveCodec::Encode(Sample()) },
		{ CoSaveCodec::kSerializationRecord, 3, CoSaveCodec::Encode(Sample()) } };
	REQUIRE(CoSaveCodec::LoadRecords(two, 3, out));
	REQUIRE(out.playerRank == Sample().playerRank);
}

TEST_CASE("CoSaveCodec A5 override duplicate", "[cosave][a5]")
{
	const Bytes ladder = { 1, 1, 1, 255, 1, 2, 0, 7, 0, 0, 0, 0, 0, 0, 0, 20, 0,
		7, 0, 0, 0, 0, 0, 0, 0, 20, 0 };
	const auto bytes = Save({ Sec(1, Bytes(44, 0)), Sec(11, ladder) });
	CoSaveState out;
	REQUIRE_FALSE(CoSaveCodec::Decode(bytes, out));
	REQUIRE(CoSaveCodec::LastRejectToken() == "co-save: reject duplicate-key");
}
