#pragma once
#include "Core.h"

namespace Runefall
{
	class MenuApi
	{
	public:
		// Compact JSON snapshot for Papyrus/UI consumers. No file access, no engine reads
		// beyond already-cached module state.
		[[nodiscard]] static std::string BuildSnapshotJson();

		[[nodiscard]] static SelfTestResult SelfTest();
	};
}
