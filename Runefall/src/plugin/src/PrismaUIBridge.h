#pragma once
#include "Core.h"

namespace Runefall
{
	class PrismaUIBridge
	{
	public:
		static PrismaUIBridge& Get();
		[[nodiscard]] bool WriteViewModel();
		[[nodiscard]] std::string ViewModelPath() const;
		[[nodiscard]] bool LastWriteOk() const noexcept { return lastWriteOk; }
		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		bool lastWriteOk = false;
	};
}
