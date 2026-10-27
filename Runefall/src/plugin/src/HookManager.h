#pragma once
#include "Core.h"

namespace Runefall
{
	class HookManager
	{
	public:
		static HookManager& Get();

		// Installs the two proven Load3D vtable hooks (container/actor spawn path) and registers
		// read-only event sinks. Every sink only captures FormIDs/handles and queues a main-thread
		// task; no game mutation happens inside a dispatch callback.
		void Register();

		[[nodiscard]] std::uint32_t ContainerEvents() const noexcept;
		[[nodiscard]] std::uint32_t MenuEvents() const noexcept;
		[[nodiscard]] std::uint32_t CellEvents() const noexcept;
		[[nodiscard]] std::uint32_t ObjectEvents() const noexcept;
		[[nodiscard]] std::uint32_t PlayerItemEvents() const noexcept;
		[[nodiscard]] std::uint32_t ResetEvents() const noexcept;
		[[nodiscard]] std::uint32_t QuestStageEvents() const noexcept;
		[[nodiscard]] std::uint32_t Load3DRefrEvents() const noexcept;
		[[nodiscard]] std::uint32_t Load3DCharacterEvents() const noexcept;
		[[nodiscard]] bool          Load3DHooksInstalled() const noexcept;
	};
}
