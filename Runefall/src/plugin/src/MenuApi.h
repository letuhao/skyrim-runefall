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

		// Item detail for the menu/Papyrus diagnostics path; empty registry ID returns {}.
		[[nodiscard]] static std::string BuildItemDetailJson(std::uint32_t a_registryId);

		// Static diagnostics: data-loaded state, feature flags and last probe summary.
		[[nodiscard]] static std::string BuildDiagnosticsJson();

		[[nodiscard]] static SelfTestResult SelfTest();
	};
}