#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

std::atomic<std::size_t> g_allocations{ 0 };

namespace
{
	struct Row { int stage=0; int tier=0; double power=0, health=0, damage=0, regen=0, armor=0; };

	nlohmann::json Load(const std::string& path)
	{
		std::ifstream file{ path };
		if (!file) return {};
		nlohmann::json value;
		file >> value;
		return value;
	}

	bool Near(double a, double b, double tolerance) { return std::fabs(a-b) <= tolerance; }

	int Run(const std::string& dataDir)
	{
		const auto stageJson = Load(dataDir + "/stage_ladder.json");
		if (!stageJson.is_object() || stageJson.value("schemaVersion", 0) != 1)
		{
			std::cerr << "FAIL: stage_ladder.json missing/invalid\n";
			return 1;
		}
		const auto& stages = stageJson.at("rows");
		if (!stages.is_array() || stages.size() != 100)
		{
			std::cerr << "FAIL: stages=" << (stages.is_array() ? stages.size() : 0) << "\n";
			return 1;
		}
		std::vector<Row> rows;
		rows.reserve(100);
		for (const auto& item : stages)
		{
			Row row;
			row.stage = std::stoi(item.at("stage").get<std::string>());
			row.tier = std::stoi(item.at("tier").get<std::string>());
			row.power = std::stod(item.at("powerIndex").get<std::string>());
			row.health = std::stod(item.at("healthMult").get<std::string>());
			row.damage = std::stod(item.at("damageMult").get<std::string>());
			row.regen = std::stod(item.at("regenMult").get<std::string>());
			row.armor = std::stod(item.at("armorMult").get<std::string>());
			rows.push_back(row);
		}
		for (std::size_t i = 0; i < rows.size(); ++i)
		{
			if (rows[i].stage != static_cast<int>(i + 1) || rows[i].tier != static_cast<int>(i / 10 + 1))
			{
				std::cerr << "FAIL: stage/tier " << i + 1 << "\n";
				return 1;
			}
			if (!Near(rows[i].power, std::pow(15.0, static_cast<double>(i) / 99.0), 2e-3) ||
				!Near(rows[i].health, std::pow(rows[i].power, 0.65), 2e-3) ||
				!Near(rows[i].damage, std::pow(rows[i].power, 0.45), 2e-3) ||
				!Near(rows[i].regen, std::pow(rows[i].power, 0.30), 2e-3) ||
				!Near(rows[i].armor, std::pow(rows[i].power, 0.25), 2e-3))
			{
				std::cerr << "FAIL: curve " << i + 1 << "\n";
				return 1;
			}
		}
		const auto tiers = Load(dataDir + "/tier_definitions.json");
		const auto bands = Load(dataDir + "/stage_content_bands.json");
		const auto curves = Load(dataDir + "/curve_profiles.json");
		const auto legacy = Load(dataDir + "/legacy_rf_tier_map.json");
		if (!tiers.is_object() || tiers.at("rows").size() != 10 ||
			!bands.is_object() || bands.at("rows").size() != 10 ||
			!curves.is_object() || curves.at("rows").empty() ||
			!legacy.is_object() || legacy.at("rows").size() != 6)
		{
			std::cerr << "FAIL: tier/band/curve/legacy table counts\n";
			return 1;
		}

		g_allocations.store(0);
		const auto start = std::chrono::steady_clock::now();
		volatile double sum = 0.0;
		for (int i = 0; i < 10000000; ++i)
			sum += rows[static_cast<std::size_t>(i % 100)].power;
		const auto end = std::chrono::steady_clock::now();
		const auto allocations = g_allocations.load();
		const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
		const auto perLookup = static_cast<double>(ns) / 10000000.0;
		if (allocations != 0)
		{
			std::cerr << "FAIL: " << allocations << " allocations during lookup\n";
			return 1;
		}
		if (perLookup > 100.0)
		{
			std::cerr << "FAIL: lookup " << perLookup << " ns > 100 ns\n";
			return 1;
		}
		std::cout << "PASS: 100 stages from JSON, formulas valid, 0 allocations, lookup=" << perLookup << " ns\n";
		return 0;
	}
}

void* operator new(std::size_t size)
{
	++g_allocations;
	if (void* pointer = std::malloc(size))
		return pointer;
	throw std::bad_alloc{};
}

void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }

int main(int argc, char** argv)
{
	const std::string dataDir = argc > 1 ? argv[1] : "build/dist/Data/Runefall/data";
	return Run(dataDir);
}
