#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#pragma warning(push)
#include <SimpleIni.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <xbyak/xbyak.h>
#pragma warning(pop)

#define DLLEXPORT __declspec(dllexport)

namespace logger = SKSE::log;
namespace numeric = SKSE::stl::numeric;
namespace string = SKSE::stl::string;

using namespace std::literals;

namespace stl
{
	using namespace SKSE::stl;

	template <class T>
	void write_thunk_call(std::uintptr_t a_src)
	{
		auto& trampoline = SKSE::GetTrampoline();
		SKSE::AllocTrampoline(14);
		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}

	template <class F, class T>
	void write_vfunc(std::size_t a_size)
	{
		REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[0] };
		T::func = vtbl.write_vfunc(a_size, T::thunk);
	}
}

#ifdef SKYRIM_AE
#	define OFFSET(se, ae) ae
#else
#	define OFFSET(se, ae) se
#endif

#include "Version.h"
