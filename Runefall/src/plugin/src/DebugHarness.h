#pragma once
#include "Core.h"

namespace Runefall
{
	class DebugHarness
	{
	public:
		static void RunAll();

		[[nodiscard]] static const std::string& LastSummary() noexcept { return lastSummary; }
		[[nodiscard]] static int                LastFailures() noexcept { return lastFailures; }
		[[nodiscard]] static int                LastPasses() noexcept { return lastPasses; }
		[[nodiscard]] static int                LastWarnings() noexcept { return lastWarnings; }

	private:
		static void WriteReport(const std::vector<SelfTestResult>& a_results);

		inline static std::string lastSummary = "not run";
		inline static int         lastFailures = 0;
		inline static int         lastPasses = 0;
		inline static int         lastWarnings = 0;
	};
}
