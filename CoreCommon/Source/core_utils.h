#pragma once

#include <string>
#include <vector>

#include "core_platform.h"

#if defined(PLATFORM_WINDOWS)
#include <windows.h>
#endif

namespace rpc
{
#if defined(PLATFORM_WINDOWS)
  std::string HRESULTToString(HRESULT hr);
#endif
}