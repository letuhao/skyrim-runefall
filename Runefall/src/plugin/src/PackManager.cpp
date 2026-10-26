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

		std::vector<const RE::TESFile*> loadedFiles;
		const auto* fullMods = dataHandler->GetLoadedMods();
		const auto  fullCount = dataHandler->GetLoadedModCount();
		if (!fullMods && fullCount > 0)
			return { "PackManager.Scan", CheckStatus::Fail, "loaded mod array unavailable" };
		for (std::uint8_t i = 0; i < fullCount; ++i) {
			if (fullMods[i])
				loadedFiles.push_back(fullMods[i]);
		}
		const auto* lightMods = dataHandler->GetLoadedLightMods();
		const auto  lightCount = dataHandler->GetLoadedLightModCount();
		if (!lightMods && lightCount > 0)
			return { "PackManager.Scan", CheckStatus::Fail, "loaded light-mod array unavailable" };
		for (std::uint16_t i = 0; i < lightCount; ++i) {
			if (lightMods[i])
				loadedFiles.push_back(lightMods[i]);
		}
		std::unordered_map<std::string, PackInfo> manifestByPlugin;
		try {
		if (fs::exists(RunefallPackDir())) {
			for (const auto& entry : fs::directory_iterator(RunefallPackDir())) {
				if (!entry.is_regular_file() || entry.path().extension() != ".json")
					continue;
				try {
					std::ifstream f{ entry.path() };
					auto          json = nlohmann::json::parse(f);
					PackInfo      info;
					info.packId = json.value("packId", entry.path().stem().string());
					info.plugin = json.value("plugin", std::string{});
					info.format = json.value("format", 0);
					info.minBaseVersion = json.value("minBaseVersion", std::string{});
					if (json.contains("contentFiles") && json["contentFiles"].is_array())
						for (const auto& item : json["contentFiles"])
							if (item.is_string())
								info.contentFiles.push_back(item.get<std::string>());
					info.state = info.format == 1 ? "ok" : "format-mismatch";
					if (!info.plugin.empty())
						manifestByPlugin[info.plugin] = std::move(info);
				} catch (const std::exception& e) {
					logger::warn("pack manifest {}: {}", entry.path().filename().string(), e.what());
				}
			}
		}
		} catch (const std::exception& e) {
			logger::warn("pack manifest scan failed: {}", e.what());
		}

		for (const auto* file : loadedFiles) {
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
			if (const auto it = manifestByPlugin.find(filename); it != manifestByPlugin.end()) {
				pack = it->second;
			} else {
				pack.plugin = filename;
				pack.packId = filename;
				pack.state = "manifest-missing";
			}
			packs.push_back(std::move(pack));
		}

		if (!basePresent) {
			std::string loaded;
			for (const auto* file : loadedFiles) {
				const std::string name{ file->GetFilename() };
				if (!loaded.empty())
					loaded += ",";
				loaded += name;
			}
			return { "PackManager.Scan", CheckStatus::Warn, std::format("base plugin not loaded; mods({})={}", loadedFiles.size(), loaded) };
		}
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