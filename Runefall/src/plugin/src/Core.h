#pragma once
#include "PCH.h"

#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Runefall
{
	inline constexpr std::uint32_t    kApiVersion = 1;
	inline constexpr std::string_view kPluginName = "Runefall";
	inline constexpr std::uint32_t    kSaveVersion = 3;

	namespace fs = std::filesystem;

	enum class CheckStatus
	{
		Pass,
		Warn,
		Fail
	};

	struct SelfTestResult
	{
		std::string name;
		CheckStatus status = CheckStatus::Pass;
		std::string detail;

		[[nodiscard]] const char* StatusText() const noexcept
		{
			switch (status) {
			case CheckStatus::Pass: return "PASS";
			case CheckStatus::Warn: return "WARN";
			case CheckStatus::Fail: return "FAIL";
			default: return "?";
			}
		}
	};

	[[nodiscard]] inline std::string_view Trim(std::string_view a_value) noexcept
	{
		const auto first = a_value.find_first_not_of(" \t\r\n");
		if (first == std::string_view::npos)
			return {};
		const auto last = a_value.find_last_not_of(" \t\r\n");
		return a_value.substr(first, last - first + 1);
	}

	[[nodiscard]] inline std::vector<std::string> Split(std::string_view a_value, char a_sep)
	{
		std::vector<std::string> parts;
		std::string              current;
		for (const char c : a_value) {
			if (c == a_sep) {
				parts.emplace_back(Trim(current));
				current.clear();
			} else {
				current.push_back(c);
			}
		}
		parts.emplace_back(Trim(current));
		return parts;
	}

	[[nodiscard]] inline fs::path FindGameRoot()
	{
		if (const char* env = std::getenv("RUNEFALL_GAME_ROOT"); env && *env) {
			fs::path candidate{ env };
			std::error_code ec;
			if (fs::exists(candidate / "Data" / "Skyrim.esm", ec) && !ec)
				return candidate;
		}

		auto cursor = fs::current_path();
		for (int i = 0; i < 10; ++i) {
			std::error_code ec;
			if (fs::exists(cursor / "Data" / "Skyrim.esm", ec) && !ec)
				return cursor;
			if (!cursor.has_parent_path())
				break;
			cursor = cursor.parent_path();
		}
		return fs::current_path();
	}

	[[nodiscard]] inline fs::path DataRoot()
	{
		return FindGameRoot() / "Data";
	}

	[[nodiscard]] inline fs::path RunefallDataDir()
	{
		return DataRoot() / "Runefall" / "data";
	}

	[[nodiscard]] inline fs::path RunefallPackDir()
	{
		return DataRoot() / "Runefall" / "packs";
	}

	[[nodiscard]] inline fs::path ProbeLogPath()
	{
		return DataRoot() / "SKSE" / "Plugins" / "RunefallProbe.log";
	}

	inline bool EnsureParentDirectory(const fs::path& a_path)
	{
		std::error_code ec;
		if (a_path.has_parent_path())
			fs::create_directories(a_path.parent_path(), ec);
		return !ec;
	}
}