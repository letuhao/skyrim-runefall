#pragma once
#include "Core.h"

namespace Runefall
{
	class InputManager
	{
	public:
		static InputManager& Get();

		void Register();
		[[nodiscard]] bool Registered() const noexcept { return registered; }

		[[nodiscard]] static SelfTestResult SelfTest();

	private:
		inline static bool registered = false;
	};
}