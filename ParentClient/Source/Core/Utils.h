#pragma once

#include <string>

#include "Core/PlatformDetection.h"

#if defined(PLATFORM_WINDOWS)
#include <Windows.h>
#endif

#include <asio.hpp>

namespace rpc
{
  asio::ip::address_v4 GetNetworkBroadcastAddress(asio::io_context& io_context);
#if defined(PLATFORM_WINDOWS)
  std::string HRESULTToString(HRESULT hr);
#endif
}