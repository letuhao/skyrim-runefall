#pragma once
#include "Core.h"

namespace Runefall
{
	class FormResolver
	{
	public:
		[[nodiscard]] static RE::TESForm* Resolve(std::string_view a_editorID);
		template <class T>
		[[nodiscard]] static T* ResolveAs(std::string_view a_editorID)
		{
			auto* form = Resolve(a_editorID);
			return form ? form->As<T>() : nullptr;
		}
		[[nodiscard]] static SelfTestResult SelfTest();
	};
}
