#include "RuntimeContentResolver.h"

#include "DataStore.h"

#include <algorithm>
#include <fstream>
#include <format>

namespace Runefall
{
	namespace
	{
		int FindColumn(const CsvTable& a_table, std::string_view a_name)
		{
			for (std::size_t i = 0; i < a_table.headers.size(); ++i)
				if (a_table.headers[i] == a_name)
					return static_cast<int>(i);
			return -1;
		}

		std::string Cell(const std::vector<std::string>& a_row, int a_column)
		{
			if (a_column < 0 || static_cast<std::size_t>(a_column) >= a_row.size())
				return {};
			return a_row[static_cast<std::size_t>(a_column)];
		}

		std::uint64_t Hash(std::string_view a_text)
		{
			std::uint64_t hash = 1469598103934665603ull;
			for (const auto c : a_text) {
				hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
				hash *= 1099511628211ull;
			}
			return hash;
		}

		std::string Escape(std::string_view a_text)
		{
			std::string out;
			out.reserve(a_text.size() + 8);
			for (const auto c : a_text) {
				switch (c) {
				case '"': out += "\\\""; break;
				case '\\': out += "\\\\"; break;
				case '\n': out += "\\n"; break;
				default: out.push_back(c); break;
				}
			}
			return out;
		}
	}

	RuntimeContentResolver& RuntimeContentResolver::Get()
	{
		static RuntimeContentResolver singleton;
		return singleton;
	}

	void RuntimeContentResolver::Load()
	{
		roles.clear();
		raceProfiles.clear();
		cache.clear();
		loaded = false;

		const auto* roleTable = DataStore::Get().GetTable("roles.csv");
		const auto* raceTable = DataStore::Get().GetTable("race_profiles.csv");
		if (!roleTable || !raceTable) {
			logger::error("runtime-resolver: roles.csv or race_profiles.csv missing");
			return;
		}
		const int rKey = FindColumn(*roleTable, "roleKey");
		const int rParent = FindColumn(*roleTable, "parentRole");
		const int rTags = FindColumn(*roleTable, "tags");
		const int rHost = FindColumn(*roleTable, "hostPolicy");
		const int rProtection = FindColumn(*roleTable, "protectionPolicy");
		for (const auto& row : roleTable->rows) {
			RoleDef role;
			role.key = Cell(row, rKey);
			role.parentKey = Cell(row, rParent);
			role.tags = Cell(row, rTags);
			role.hostPolicy = Cell(row, rHost);
			role.protectionPolicy = Cell(row, rProtection);
			role.family = role.key;
			if (role.key.empty())
				continue;
			roles[role.key] = std::move(role);
		}
		const int pKey = FindColumn(*raceTable, "profileKey");
		const int pEditor = FindColumn(*raceTable, "raceEditorId");
		const int pRoles = FindColumn(*raceTable, "roles");
		const int pFamily = FindColumn(*raceTable, "family");
		for (const auto& row : raceTable->rows) {
			RaceDef race;
			race.key = Cell(row, pKey);
			race.raceEditorId = Cell(row, pEditor);
			race.roles = Cell(row, pRoles);
			race.family = Cell(row, pFamily);
			if (race.raceEditorId.empty())
				continue;
			raceProfiles[race.raceEditorId] = std::move(race);
		}
		loaded = !roles.empty() && !raceProfiles.empty() && roles.contains("rf:role/unknown");
		if (loaded) {
			dataVersion = ComputeDataVersion();
			RescanIfNeeded();
		}
		if (!loaded)
			logger::error("runtime-resolver: tables loaded but no roles/race profiles (or missing unknown role)");
		else
			logger::info("runtime-resolver: loaded {} roles, {} race profiles", roles.size(), raceProfiles.size());
	}

	const RuntimeContentResolver::RoleDef* RuntimeContentResolver::FindRole(std::string_view a_key) const
	{
		const auto it = roles.find(std::string{ a_key });
		return it == roles.end() ? nullptr : &it->second;
	}

	const RuntimeContentResolver::RaceDef* RuntimeContentResolver::FindRace(std::string_view a_editorId) const
	{
		for (const auto& [key, race] : raceProfiles) {
			(void)key;
			if (race.raceEditorId == a_editorId)
				return &race;
		}
		return nullptr;
	}

	std::string RuntimeContentResolver::CacheKey(const RaceDescriptor& a_race)
	{
		return a_race.plugin + ":" + std::to_string(a_race.localFormId) + ":" + a_race.editorId;
	}

	std::string RuntimeContentResolver::CachePath(std::string_view a_key) const
	{
		const auto directory = DataStore::Get().DataDirectory().parent_path() / "runtime";
		return (directory / std::format("race-{:016X}.json", Hash(a_key))).string();
	}

	std::string RuntimeContentResolver::ReadCache(std::string_view a_path) const
	{
		std::ifstream file{ std::string{ a_path } };
		if (!file)
			return {};
		return std::string{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
	}

	bool RuntimeContentResolver::WriteCache(const RuntimeRaceProfile& a_profile)
	{
		const auto path = CachePath(a_profile.cacheKey);
		std::error_code error;
		fs::create_directories(fs::path{ path }.parent_path(), error);
		if (error)
			return false;
		std::ofstream file{ path, std::ios::trunc };
		if (!file)
			return false;
		file << "{\n";
		file << "  \"schemaVersion\": 1,\n";
		file << "  \"cacheKey\": \"" << Escape(a_profile.cacheKey) << "\",\n";
		file << "  \"roleKey\": \"" << Escape(a_profile.roleKey) << "\",\n";
		file << "  \"family\": \"" << Escape(a_profile.family) << "\",\n";
		file << "  \"stageMin\": " << static_cast<int>(a_profile.stageMin) << ",\n";
		file << "  \"stageMax\": " << static_cast<int>(a_profile.stageMax) << ",\n";
		file << "  \"provisional\": " << (a_profile.provisional ? "true" : "false") << ",\n";
		file << "  \"generatorVersion\": \"0.1.0\",\n";
		file << "  \"lootTable\": \"" << Escape(a_profile.lootTable) << "\",\n";
		file << "  \"dataVersion\": \"" << Escape(a_profile.dataVersion) << "\",\n";
		file << "  \"materialRefs\": [";
		for (std::size_t i = 0; i < a_profile.materialRefs.size(); ++i) {
			if (i > 0)
				file << ", ";
			file << "\"" << Escape(a_profile.materialRefs[i]) << "\"";
		}
		file << "]\n}\n";
		return static_cast<bool>(file);
	}

	std::string RuntimeContentResolver::ComputeDataVersion() const
	{
		return std::format("roles={};races={};plugin=0.1.0", roles.size(), raceProfiles.size());
	}

	void RuntimeContentResolver::RescanIfNeeded()
	{
		const auto directory = DataStore::Get().DataDirectory().parent_path() / "runtime";
		std::error_code error;
		if (!fs::exists(directory, error))
			return;
		for (const auto& entry : fs::directory_iterator(directory, error)) {
			if (error || !entry.is_regular_file())
				continue;
			const auto text = ReadCache(entry.path().string());
			if (text.find(dataVersion) == std::string::npos) {
				std::error_code removeError;
				fs::remove(entry.path(), removeError);
				if (!removeError)
					logger::info("runtime-resolver: removed stale cache {}", entry.path().filename().string());
			}
		}
	}

	void RuntimeContentResolver::Rescan()
	{
		const auto directory = DataStore::Get().DataDirectory().parent_path() / "runtime";
		std::error_code error;
		std::size_t removed = 0;
		if (fs::exists(directory, error)) {
			for (const auto& entry : fs::directory_iterator(directory, error)) {
				if (error || !entry.is_regular_file())
					continue;
				std::error_code removeError;
				fs::remove(entry.path(), removeError);
				if (!removeError)
					++removed;
			}
		}
		cache.clear();
		logger::info("runtime-resolver: manual rescan removed {} cached profile(s), dataVersion={}", removed, dataVersion);
	}

	RuntimeRaceProfile RuntimeContentResolver::ResolveRace(const RaceDescriptor& a_race)
	{
		const auto key = CacheKey(a_race);
		if (const auto it = cache.find(key); it != cache.end())
			return it->second;

		RuntimeRaceProfile profile;
		profile.cacheKey = key;
		const auto* race = FindRace(a_race.editorId);
		if (race && !race->roles.empty()) {
			const auto separator = race->roles.find('|');
			profile.roleKey = separator == std::string::npos ? race->roles : race->roles.substr(0, separator);
			if (!profile.roleKey.starts_with("rf:role/"))
				profile.roleKey = "rf:role/" + profile.roleKey;
			profile.family = race->family.empty() ? profile.roleKey : race->family;
			profile.provisional = false;
			profile.stageMin = 1;
			profile.stageMax = 100;
		} else {
			profile.roleKey = "rf:role/unknown";
			profile.family = "unknown";
			profile.provisional = true;
			profile.stageMin = 1;
			profile.stageMax = 10;
		}
		if (!FindRole(profile.roleKey)) {
			profile.roleKey = "rf:role/unknown";
			profile.family = "unknown";
			profile.provisional = true;
			profile.stageMin = 1;
			profile.stageMax = 10;
		}
		const auto familySlug = profile.family;
		profile.materialRefs = {
			"rf:material/" + familySlug + "_part",
			"rf:material/" + familySlug + "_core"
		};
		profile.lootTable = "rf:loot/" + familySlug + "_role";
		profile.dataVersion = dataVersion;
		if (!WriteCache(profile)) {
			logger::error("runtime-resolver: failed to write cache for {}", key);
		} else {
			cache[key] = profile;
		}
		logger::info("runtime-resolver: resolved {} -> role {} ({})", key, profile.roleKey, profile.provisional ? "provisional" : "known");
		return profile;
	}

	SelfTestResult RuntimeContentResolver::SelfTest()
	{
		auto& resolver = Get();
		if (!resolver.Loaded())
			return { "RuntimeContentResolver.SelfTest", CheckStatus::Fail, "roles/race profile tables not loaded" };
		if (resolver.RoleCount() < 10 || resolver.RaceProfileCount() < 10)
			return { "RuntimeContentResolver.SelfTest", CheckStatus::Fail, "role/race profile coverage is too small" };

		RaceDescriptor unknown;
		unknown.plugin = "RFTest.esp";
		unknown.localFormId = 0x800;
		unknown.editorId = "RF_UnknownRace";
		const auto first = resolver.ResolveRace(unknown);
		const auto second = resolver.ResolveRace(unknown);
		if (first.roleKey != "rf:role/unknown" || !first.provisional || first.cacheKey != second.cacheKey)
			return { "RuntimeContentResolver.SelfTest", CheckStatus::Fail, "unknown race did not resolve to provisional unknown" };
		if (first.materialRefs.empty() || first.lootTable.empty())
			return { "RuntimeContentResolver.SelfTest", CheckStatus::Fail, "unknown race generated no material/loot profile" };
		const auto cacheText = resolver.ReadCache(resolver.CachePath(first.cacheKey));
		if (cacheText.empty() || cacheText.find(first.cacheKey) == std::string::npos || cacheText.find("rf:role/unknown") == std::string::npos || cacheText.find("dataVersion") == std::string::npos)
			return { "RuntimeContentResolver.SelfTest", CheckStatus::Fail, "runtime JSON cache was not written/read" };

		RaceDescriptor known;
		known.plugin = "Skyrim.esm";
		known.localFormId = 0;
		known.editorId = "NordRace";
		const auto nord = resolver.ResolveRace(known);
		if (nord.provisional || nord.roleKey != "rf:role/humanoid")
			return { "RuntimeContentResolver.SelfTest", CheckStatus::Fail, "known race did not resolve to humanoid" };

		return { "RuntimeContentResolver.SelfTest", CheckStatus::Pass,
			std::format("{} roles, {} race profiles; unknown provisional + cache reload + known NordRace OK", resolver.RoleCount(), resolver.RaceProfileCount()) };
	}
}
