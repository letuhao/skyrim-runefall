#pragma once
#include "Core.h"

namespace Runefall
{
	class HookManager
	{
	public:
		static HookManager& Get();

		// Read-only event sinks. No game mutation happens in dispatch: the sinks only count
		// events and log a bounded sample so downstream hooks can be added without crashing.
		void Register();

		[[nodiscard]] std::uint32_t ContainerEvents() const noexcept;
		[[nodiscard]] std::uint32_t MenuEvents() const noexcept;
		[[nodiscard]] std::uint32_t CellEvents() const noexcept;
		[[nodiscard]] std::uint32_t ObjectEvents() const noexcept;
		[[nodiscard]] std::uint32_t PlayerItemEvents() const noexcept;
	};
}