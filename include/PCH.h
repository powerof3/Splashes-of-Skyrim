#pragma once

#include "RE/Skyrim.h"
#include <xbyak/xbyak.h>
#include "SKSE/SKSE.h"

#include <SimpleIni.h>
#include <frozen/set.h>
#include <frozen/string.h>
#include <nonstd/span.hpp>
#include <spdlog/sinks/basic_file_sink.h>

namespace logger = SKSE::log;
using namespace SKSE::util;
namespace stl = SKSE::stl;
using namespace std::string_view_literals;

#define DLLEXPORT __declspec(dllexport)