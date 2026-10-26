#include <nlohmann/json.hpp>
#include "PackManager.h"

namespace Runefall
{
	namespace
	{
		bool StartsWithInsensitive(std::string_view a_value, std::string_view a_prefix)
		{
			if (a_value.size() < a_prefix.size())
				return false;
			for (std::size_t i = 0; i < a_prefix.size(); ++i) {
				if (std::tolower(static_cast<unsigned char>(a_value[i])) != std::tolower(static_cast<unsigned char>(a_prefix[i])))
					return false;
			}
			return true;
		}

		bool EndsWithInsensitive(std::string_view a_value, std::string_view a_suffix)
		{
			if (a_value.size() < a_suffix.size())
				return false;
			const auto offset = a_value.size() - a_suffix.size();
			for (std::size_t i = 0; i < a_suffix.size(); ++i) {
				if (std::tolower(static_cast<unsigned char>(a_value[offset + i])) != std::tolower(static_cast<unsigned char>(a_suffix[i])))
					return false;
			}
			return true;
		}
	}

	PackManager& PackManager::Get()
	{
		static PackManager singleton;
		return singleton;
	}

	SelfTestResult PackManager::Scan()
	{
		packs.clear();
		basePresent = false;

		const auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler)
			return { "PackManager.Scan", CheckStatus::Fail, "TESDataHandler unavailable" };

		const auto* mods = dataHandler->GetLoadedMods();
		const auto  count = dataHandler->GetLoadedModCount();
		for (std::uint8_t i = 0; i < count; ++i) {
			const auto* file = mods[i];
			if (!file)
				continue;
			const std::string filename{ file->GetFilename() };
			if (!StartsWithInsensitive(filename, "Runefall"))
				continue;
			if (filename == basePlugin) {
				basePresent = true;
				continue;
			}
			if (!EndsWithInsensitive(filename, ".esp") && !EndsWithInsensitive(filename, ".esl") && !EndsWithInsensitive(filename, ".esm")) {
				packs.push_back({ filename, {}, 0, {}, {}, "not-a-plugin" });
				continue;
			}

			PackInfo pack;
			pack.plugin = filename;
			pack.packId = filename;
			const auto manifest = RunefallPackDir() / (pack.packId + ".json");
			if (!fs::exists(manifest)) {
				// also try packId.json without extension
				const auto alt = RunefallPackDir() / (pack.packId.substr(0, pack.packId.find_last_of('.')) + ".json");
				if (!fs::exists(alt)) {
					pack.state = "manifest-missing";
					packs.push_back(std::move(pack));
					continue;
				}
				pack.packId = alt.stem().string();
			}
			try {
				const auto manifestPath = RunefallPackDir() / (pack.packId + ".json");
				if (fs::exists(manifestPath)) {
					std::ifstream f{ manifestPath };
					auto          json = nlohmann::json::parse(f);
					pack.packId = json.value("packId", pack.packId);
					pack.format = json.value("format", 0);
					pack.minBaseVersion = json.value("minBaseVersion", std::string{});
					if (json.contains("contentFiles") && json["contentFiles"].is_array())
						for (const auto& item : json["contentFiles"])
							if (item.is_string())
								pack.contentFiles.push_back(item.get<std::string>());
					pack.state = pack.format == 1 ? "ok" : "format-mismatch";
				} else {
					pack.state = "manifest-missing";
				}
			} catch (const std::exception& e) {
				pack.state = std::string{ "manifest-error: " } + e.what();
			}
			packs.push_back(std::move(pack));
		}

		if (!basePresent)
			return { "PackManager.Scan", CheckStatus::Warn, "base plugin Runefall.esp not loaded (content phase pending)" };
		if (packs.empty())
			return { "PackManager.Scan", CheckStatus::Pass, "base present; no pack plugins loaded" };

		std::size_t bad = 0;
		for (const auto& pack : packs)
			if (pack.state != "ok")
				++bad;
		if (bad > 0)
			return { "PackManager.Scan", CheckStatus::Warn, std::format("base present, {} pack(s), {} with issues", packs.size(), bad) };
		return { "PackManager.Scan", CheckStatus::Pass, std::format("base present, {} pack(s)", packs.size()) };
	}

	SelfTestResult PackManager::SelfTest() const
	{
		if (!basePresent)
			return { "PackManager.SelfTest", CheckStatus::Warn, "base plugin not present yet" };
		for (const auto& pack : packs) {
			if (pack.format != 1 && pack.state == "ok")
				return { "PackManager.SelfTest", CheckStatus::Fail, "pack " + pack.packId + " has unsupported format" };
		}
		return { "PackManager.SelfTest", CheckStatus::Pass, std::format("{} pack manifest(s) valid", packs.size()) };
	}
}