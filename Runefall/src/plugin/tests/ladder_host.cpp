#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	struct Row { int stage=0; int tier=0; double power=0, health=0, damage=0, regen=0, armor=0; };

	std::vector<std::string> Split(const std::string& line)
	{
		std::vector<std::string> cells;
		std::stringstream stream(line);
		std::string cell;
		while (std::getline(stream, cell, ',')) cells.push_back(cell);
		return cells;
	}

	std::vector<Row> Load(const std::string& path)
	{
		std::ifstream file{path};
		std::vector<Row> rows;
		std::string line;
		if (!std::getline(file, line)) return rows;
		auto headers = Split(line);
		auto column = [&](const char* name) { return static_cast<int>(std::find(headers.begin(), headers.end(), name) - headers.begin()); };
		const auto cStage=column("stage"), cTier=column("tier"), cPower=column("powerIndex"), cHealth=column("healthMult"), cDamage=column("damageMult"), cRegen=column("regenMult"), cArmor=column("armorMult");
		while (std::getline(file, line)) {
			if (line.empty()) continue;
			auto cells=Split(line);
			rows.push_back({std::stoi(cells[cStage]), std::stoi(cells[cTier]), std::stod(cells[cPower]), std::stod(cells[cHealth]), std::stod(cells[cDamage]), std::stod(cells[cRegen]), std::stod(cells[cArmor])});
		}
		return rows;
	}

	bool Near(double a, double b, double tolerance) { return std::fabs(a-b) <= tolerance; }
}

int main(int argc, char** argv)
{
	const std::string dataDir = argc > 1 ? argv[1] : "build/dist/Data/Runefall/data";
	const auto rows = Load(dataDir + "/stage_ladder.csv");
	if (rows.size() != 100) { std::cerr << "FAIL rows=" << rows.size() << "\n"; return 1; }
	for (size_t i=0; i<rows.size(); ++i) {
		if (rows[i].stage != static_cast<int>(i+1) || rows[i].tier != static_cast<int>(i/10+1)) { std::cerr << "FAIL stage/tier " << i+1 << "\n"; return 1; }
		if (!Near(rows[i].power, std::pow(15.0, static_cast<double>(i)/99.0), 2e-3) ||
			!Near(rows[i].health, std::pow(rows[i].power, 0.65), 2e-3) ||
			!Near(rows[i].damage, std::pow(rows[i].power, 0.45), 2e-3) ||
			!Near(rows[i].regen, std::pow(rows[i].power, 0.30), 2e-3) ||
			!Near(rows[i].armor, std::pow(rows[i].power, 0.25), 2e-3)) { std::cerr << "FAIL curve " << i+1 << "\n"; return 1; }
	}
	const auto start = std::chrono::steady_clock::now();
	volatile double sum = 0.0;
	for (int i=0; i<10000000; ++i) sum += rows[static_cast<size_t>(i%100)].power;
	const auto end = std::chrono::steady_clock::now();
	const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	const auto perLookup = static_cast<double>(ns)/10000000.0;
	if (perLookup > 100.0) { std::cerr << "FAIL lookup " << perLookup << " ns\n"; return 1; }
	std::cout << "PASS: 100 stages, formulas valid, lookup=" << perLookup << " ns\n";
	return 0;
}
