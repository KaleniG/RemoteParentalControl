#pragma once

#include <string>
#include <vector>

#if defined(PLATFORM_WINDOWS)
#include <windows.h>
#endif

namespace rpc
{
#if defined(PLATFORM_WINDOWS)
  std::string HRESULTToString(HRESULT hr);
#endif
}