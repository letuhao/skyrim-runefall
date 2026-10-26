#pragma once
#include "Core.h"

namespace Runefall
{
	class MenuUI
	{
	public:
		static void Register();
		[[nodiscard]] static SelfTestResult SelfTest();
	};
}
