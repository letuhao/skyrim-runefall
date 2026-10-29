#include "CoSaveCodec.h"

#ifdef RUNEFALL_VENDORED_CATCH2
#include "catch_amalgamated.hpp"
#else
#include <catch2/catch_test_macros.hpp>
#endif

using namespace Runefall;

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
		std::int32_t tier = 1, std::uint16_t tags = 1)
	{
		Bytes b;
		for (Bytes p : { U32v(rid), U32v(base), U8v(rarity), U16v(ilvl), U8v(sockets), I32v(rune0), I32v(0), I32v(0), U8v(enh) })
			b.insert(b.end(), p.begin(), p.end());
		for (int i = 0; i < 5; ++i)
			for (Bytes p : { Str(""), Str(""), I32v(cat), I32v(tier), I32v(0), I32v(0), I32v(0) })
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
