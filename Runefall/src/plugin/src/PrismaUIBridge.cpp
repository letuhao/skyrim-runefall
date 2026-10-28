#include "PrismaUIBridge.h"

#include "DataStore.h"
#include "MenuApi.h"
#include "Progression.h"

#include <fstream>
#include <format>

namespace Runefall
{
	PrismaUIBridge& PrismaUIBridge::Get()
	{
		static PrismaUIBridge singleton;
		return singleton;
	}

	std::string PrismaUIBridge::ViewModelPath() const
	{
		const auto directory = DataStore::Get().DataDirectory().parent_path() / "ui";
		return (directory / "view-model.json").string();
	}

	bool PrismaUIBridge::WriteViewModel()
	{
		const auto path = ViewModelPath();
		std::error_code error;
		fs::create_directories(fs::path{ path }.parent_path(), error);
		if (error) {
			logger::error("prisma-ui: cannot create ui directory: {}", error.message());
			lastWriteOk = false;
			return false;
		}
		std::ofstream file{ path, std::ios::trunc };
		if (!file) {
			logger::error("prisma-ui: cannot open view model {}", path);
			lastWriteOk = false;
			return false;
		}
		file << "{\n";
		file << "  \"schemaVersion\": 1,\n";
		file << "  \"viewId\": \"runefall-hud\",\n";
		file << "  \"playerRank\": " << Progression::Get().RuneLevel() << ",\n";
		file << "  \"playerXp\": " << Progression::Get().RuneXp() << ",\n";
		file << "  \"classId\": \"" << Progression::Get().ClassId() << "\",\n";
		file << "  \"snapshot\": " << MenuApi::BuildSnapshotJson() << "\n";
		file << "}\n";
		lastWriteOk = static_cast<bool>(file);
		if (lastWriteOk)
			logger::info("prisma-ui: wrote view model {}", path);
		else
			logger::error("prisma-ui: failed writing view model {}", path);
		return lastWriteOk;
	}

	SelfTestResult PrismaUIBridge::SelfTest()
	{
		auto& bridge = Get();
		if (!bridge.WriteViewModel() || !bridge.LastWriteOk())
			return { "PrismaUIBridge.SelfTest", CheckStatus::Fail, "view model was not written" };
		std::ifstream file{ bridge.ViewModelPath() };
		if (!file)
			return { "PrismaUIBridge.SelfTest", CheckStatus::Fail, "view model cannot be read back" };
		std::string text{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
		if (text.find("runefall-hud") == std::string::npos || text.find("playerRank") == std::string::npos || text.find("snapshot") == std::string::npos)
			return { "PrismaUIBridge.SelfTest", CheckStatus::Fail, "view model contract is incomplete" };
		return { "PrismaUIBridge.SelfTest", CheckStatus::Pass, std::format("view model {} bytes", text.size()) };
	}
}
