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

TEST_CASE("CoSaveCodec A5 duplicate registryId", "[cosave][a5]")
{
	const auto encoded = CoSaveCodec::Encode(Sample());
	REQUIRE(!encoded.empty());
	std::size_t off = 16;
	Bytes itemPayload;
	while (off + 6 <= encoded.size()) {
		const std::uint16_t id = static_cast<std::uint16_t>(encoded[off] | (encoded[off + 1] << 8));
		const std::uint32_t len = static_cast<std::uint32_t>(encoded[off + 2]) |
			(static_cast<std::uint32_t>(encoded[off + 3]) << 8) |
			(static_cast<std::uint32_t>(encoded[off + 4]) << 16) |
			(static_cast<std::uint32_t>(encoded[off + 5]) << 24);
		if (id == 3) { itemPayload.assign(encoded.begin() + off + 6, encoded.begin() + off + 6 + len); break; }
		off += 6 + len;
	}
	REQUIRE(itemPayload.size() > 4);
	const Bytes items(itemPayload.begin() + 4, itemPayload.end());
	const auto bytes = Save({ Sec(1, Bytes(44, 0)), Sec(3, Cat({ U32v(2), items, items })) });
	CoSaveState out;
	REQUIRE_FALSE(CoSaveCodec::Decode(bytes, out));
	REQUIRE(CoSaveCodec::LastRejectToken() == "co-save: reject item.registryId");
}

TEST_CASE("CoSaveCodec A6 caps strings range trailing", "[cosave][a6]")
{
	CoSaveState out;
	auto expect = [&](const Bytes& bytes, const std::string& token) {
		INFO("token " << token);
		out = CoSaveState{};
		REQUIRE_FALSE(CoSaveCodec::Decode(bytes, out));
		REQUIRE(CoSaveCodec::LastRejectToken() == token);
	};
	auto with = [](const Bytes& section) { return Save({ Sec(1, Bytes(44, 0)), section }); };
	const Bytes ladderHeader = { 1, 1, 1, 255, 1, 0, 0 };

	expect(with(Sec(3, U32v(1025))), "co-save: reject item-count");
	expect(with(Sec(4, U32v(257))), "co-save: reject hunt-count");
	expect(with(Sec(5, U32v(257))), "co-save: reject material-count");
	expect(with(Sec(7, U32v(65))), "co-save: reject pack-count");
	expect(with(Sec(9, U32v(4097))), "co-save: reject container-count");
	expect(with(Sec(10, U32v(1025))), "co-save: reject quest-count");
	expect(with(Sec(11, { 1, 1, 1, 255, 1, 1, 1 })), "co-save: reject override-count");

	expect(with(Sec(5, Cat({ U32v(1), Str(std::string(65, 'a')), I32v(1) }))), "co-save: reject string.map-key");
	expect(with(Sec(7, Cat({ U32v(1), Str(std::string(33, 'a')), Str("v") }))), "co-save: reject string.pack");
	expect(with(Sec(11, Cat({ ladderHeader, U32v(0), Str(std::string(65, 'a')) }))), "co-save: reject string.class");

	expect(with(Sec(11, { 1, 0, 1, 255, 1, 0, 0 })), "co-save: reject range");
	expect(with(Sec(11, { 1, 1, 1, 6, 1, 0, 0 })), "co-save: reject range");
	expect(with(Sec(11, { 1, 1, 1, 255, 0, 0, 0 })), "co-save: reject range");

	auto base = Save({ Sec(1, Progression()) });
	base.push_back(0);
	expect(base, "co-save: reject trailing");
}

// append-probe

namespace ExtraTests
{
namespace
{
	using Bytes = std::vector<std::uint8_t>;

	Bytes U8v(std::uint8_t v) { return { v }; }
	Bytes U16v(std::uint16_t v) { return { static_cast<std::uint8_t>(v & 0xFF), static_cast<std::uint8_t>(v >> 8) }; }
	Bytes U32v(std::uint32_t v) { Bytes b; for (int i = 0; i < 4; ++i) b.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF)); return b; }
	Bytes I32v(std::int32_t v) { return U32v(static_cast<std::uint32_t>(v)); }
	Bytes Str(const std::string& s) { Bytes b = U16v(static_cast<std::uint16_t>(s.size())); b.insert(b.end(), s.begin(), s.end()); return b; }
	Bytes Cat(std::initializer_list<Bytes> parts)
	{
		std::size_t n = 0;
		for (const auto& p : parts) n += p.size();
		Bytes b;
		b.reserve(n);
		for (const auto& p : parts) b.insert(b.end(), p.begin(), p.end());
		return b;
	}
	Bytes Hdr(std::uint32_t count, std::uint16_t schema = 3, std::uint16_t version = 3, std::uint32_t flags = 0)
	{
		Bytes b{ 'R', 'N', 'F', 'S' };
		Bytes s = U16v(schema); b.insert(b.end(), s.begin(), s.end());
		Bytes f = U32v(flags); b.insert(b.end(), f.begin(), f.end());
		Bytes v = U16v(version); b.insert(b.end(), v.begin(), v.end());
		Bytes c = U32v(count); b.insert(b.end(), c.begin(), c.end());
		return b;
	}
	Bytes Sec(std::uint16_t id, const Bytes& payload)
	{
		Bytes b = U16v(id);
		Bytes n = U32v(static_cast<std::uint32_t>(payload.size()));
		b.insert(b.end(), n.begin(), n.end());
		b.insert(b.end(), payload.begin(), payload.end());
		return b;
	}
	Bytes Save(std::initializer_list<Bytes> sections)
	{
		Bytes b = Hdr(static_cast<std::uint32_t>(sections.size()));
		for (const auto& s : sections) b.insert(b.end(), s.begin(), s.end());
		return b;
	}
	Bytes Prog() { return Bytes(44, 0); }
	Bytes Item(std::uint32_t rid = 1, std::uint32_t base = 0x1234, std::uint8_t rarity = 1, std::uint16_t ilvl = 1,
		std::uint8_t sockets = 1, std::int32_t rune0 = 1, std::uint8_t enh = 1, std::int32_t cat = 1,
		std::int32_t tier = 1, std::uint16_t tags = 1, std::string affix = "")
	{
		Bytes b;
		for (Bytes p : { U32v(rid), U32v(base), U8v(rarity), U16v(ilvl), U8v(sockets), I32v(rune0), I32v(0), I32v(0), U8v(enh) })
			b.insert(b.end(), p.begin(), p.end());
		for (int i = 0; i < 5; ++i)
			for (Bytes p : { Str(affix), Str(""), I32v(cat), I32v(tier), I32v(0), I32v(0), I32v(0) })
				b.insert(b.end(), p.begin(), p.end());
		Bytes t = U16v(tags); b.insert(b.end(), t.begin(), t.end());
		return b;
	}
	Bytes WithItem(const Bytes& item) { return Save({ Sec(1, Prog()), Sec(3, Cat({ U32v(1), item })) }); }
}

TEST_CASE("A6 item field tokens", "[cosave][a6]")
{
	auto check = [](const Bytes& bytes, const std::string& token) {
		CoSaveState out;
		REQUIRE_FALSE(CoSaveCodec::Decode(bytes, out));
		REQUIRE(CoSaveCodec::LastRejectToken() == token);
	};
	check(WithItem(Item(1, 0)), "co-save: reject item.baseFormId");
	check(WithItem(Item(0)), "co-save: reject item.registryId");
	check(WithItem(Item(1, 0x1234, 6)), "co-save: reject item.rarity");
	check(WithItem(Item(1, 0x1234, 1, 1001)), "co-save: reject item.ilvl");
	check(WithItem(Item(1, 0x1234, 1, 1, 4)), "co-save: reject item.socketCount");
	check(WithItem(Item(1, 0x1234, 1, 1, 1, 6)), "co-save: reject item.socketRunes");
	check(WithItem(Item(1, 0x1234, 1, 1, 1, 1, 6)), "co-save: reject item.enhancementCount");
	check(WithItem(Item(1, 0x1234, 1, 1, 1, 1, 1, 6)), "co-save: reject item.category");
	check(WithItem(Item(1, 0x1234, 1, 1, 1, 1, 1, 1, 6)), "co-save: reject item.tier");
	check(WithItem(Item(1, 0x1234, 1, 1, 1, 1, 1, 1, 1, 0x40)), "co-save: reject item.tags");
}

TEST_CASE("A6 schema/truncated/dedupe", "[cosave][a6]")
{
	auto base = Save({ Sec(1, Prog()) });
	auto schema0 = base; schema0[4] = schema0[5] = 0;
	CoSaveState out;
	REQUIRE_FALSE(CoSaveCodec::Decode(schema0, out));
	REQUIRE(CoSaveCodec::LastRejectToken() == "co-save: reject schema");
	auto schema4 = base; schema4[4] = 4;
	REQUIRE_FALSE(CoSaveCodec::Decode(schema4, out));
	REQUIRE(CoSaveCodec::LastRejectToken() == "co-save: reject schema");
	auto truncated = base; truncated.resize(truncated.size() - 3);
	REQUIRE_FALSE(CoSaveCodec::Decode(truncated, out));
	REQUIRE(CoSaveCodec::LastRejectToken() == "co-save: reject trailing");

	auto dedupe = Save({ Sec(1, Prog()), Sec(9, Cat({ U32v(2), U32v(5), U32v(5) })) });
	REQUIRE(CoSaveCodec::Decode(dedupe, out));
	REQUIRE(out.generatedContainers.size() == 1);
}

TEST_CASE("A6 skill count and string", "[cosave][a6]")
{
	const Bytes ladder = { 1, 1, 1, 255, 1, 0, 0 };
	CoSaveState out;
	auto s1 = Save({ Sec(1, Prog()), Sec(11, Cat({ ladder, U32v(0), Str(""), U16v(257) })) });
	REQUIRE_FALSE(CoSaveCodec::Decode(s1, out));
	REQUIRE(CoSaveCodec::LastRejectToken() == "co-save: reject skill-count");
	auto s2 = Save({ Sec(1, Prog()), Sec(11, Cat({ ladder, U32v(0), Str(""), U16v(1), Str(std::string(65, 'a')), I32v(1) })) });
	REQUIRE_FALSE(CoSaveCodec::Decode(s2, out));
	REQUIRE(CoSaveCodec::LastRejectToken() == "co-save: reject string.skill");
}

TEST_CASE("A1 LoadOrReset defaults", "[cosave][a1]")
{
	CoSaveState out;
	out.playerRank = 55;
	REQUIRE_FALSE(CoSaveCodec::LoadOrReset({}, 3, out));
	REQUIRE(out.playerRank == 1);
	out.playerRank = 55;
	std::vector<CoSaveRecord> bad{ { CoSaveCodec::kSerializationRecord, 3, { 1, 2, 3 } } };
	REQUIRE_FALSE(CoSaveCodec::LoadOrReset(bad, 3, out));
	REQUIRE(out.playerRank == 1);
	const CoSaveState sample{};
	std::vector<CoSaveRecord> good{ { CoSaveCodec::kSerializationRecord, 3, CoSaveCodec::Encode(sample) } };
	REQUIRE(CoSaveCodec::LoadOrReset(good, 3, out));
	REQUIRE(out.playerRank == 1);
}


	TEST_CASE("A6 affix string, at-cap containers, optional section", "[cosave][a6]")
	{
		auto bad = WithItem(Item(1, 0x1234, 1, 1, 1, 1, 1, 1, 1, 1, std::string(65, 'a')));
		CoSaveState out;
		REQUIRE_FALSE(CoSaveCodec::Decode(bad, out));
		REQUIRE(CoSaveCodec::LastRejectToken() == "co-save: reject string.affix");

		Bytes ids;
		for (std::uint32_t i = 1; i <= 4096; ++i) { Bytes v = U32v(i); ids.insert(ids.end(), v.begin(), v.end()); }
		auto full = Save({ Sec(1, Prog()), Sec(9, Cat({ U32v(4096), ids })) });
		REQUIRE(CoSaveCodec::Decode(full, out));
		REQUIRE(out.generatedContainers.size() == 4096);

		auto optional = Save({ Sec(1, Prog()) });
		REQUIRE(CoSaveCodec::Decode(optional, out));
		REQUIRE(out.items.empty());
	}

}
