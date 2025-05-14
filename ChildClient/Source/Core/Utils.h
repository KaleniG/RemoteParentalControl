#pragma once

#include <string>
#include <vector>

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
  std::vector<uint8_t> BGRAToJPEG(const std::vector<uint8_t>& bgra_data, uint32_t bgra_width, uint32_t bgra_height, uint8_t quality);
}