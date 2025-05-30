#include "Core/Utils.h"

#include <sstream>

#if defined(PLATFORM_WINDOWS)
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE)
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace rpc
{
  namespace utils
  {
    struct MemoryBuffer 
    {
      std::vector<uint8_t> data;

      static void WriteCallback(void* context, void* data, int size) 
      {
        MemoryBuffer* buffer = static_cast<MemoryBuffer*>(context);
        buffer->data.insert(buffer->data.end(), (uint8_t*)data, (uint8_t*)data + size);
      }
    };
  }

  asio::ip::address_v4 GetNetworkBroadcastAddress(asio::io_context& io_context)
  {
    asio::ip::tcp::resolver resolver(io_context);
    asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(asio::ip::host_name(), "");

#if defined(PLATFORM_WINDOWS)
    IP_ADAPTER_INFO adapter[16];
    DWORD buflen = sizeof(adapter);
    GetAdaptersInfo(adapter, &buflen);

    for (auto ep : endpoints) 
    {
      auto ip = ep.endpoint().address();
      if (!ip.is_v4() || ip.is_loopback() || ip.is_unspecified()) 
        continue;

      for (IP_ADAPTER_INFO* ptr = adapter; ptr != nullptr; ptr = ptr->Next) 
      {
        std::string adapter_ip = ptr->IpAddressList.IpAddress.String;
        std::string adapter_mask = ptr->IpAddressList.IpMask.String;

        if (ip.to_string() == adapter_ip) 
        {
          uint32_t ip_num = ntohl(inet_addr(adapter_ip.c_str()));
          uint32_t mask_num = ntohl(inet_addr(adapter_mask.c_str()));
          uint32_t broadcast = ip_num | ~mask_num;
          asio::ip::address_v4::bytes_type bcast_bytes = {
              static_cast<unsigned char>((broadcast >> 24) & 0xFF),
              static_cast<unsigned char>((broadcast >> 16) & 0xFF),
              static_cast<unsigned char>((broadcast >> 8) & 0xFF),
              static_cast<unsigned char>((broadcast) & 0xFF)
          };
          return asio::ip::address_v4(bcast_bytes);
        }
      }
    }

#elif defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE)
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) return asio::ip::address_v4();

    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) 
    {
      if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;

      auto* sa = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
      auto* mask = reinterpret_cast<sockaddr_in*>(ifa->ifa_netmask);

      uint32_t ip = ntohl(sa->sin_addr.s_addr);
      uint32_t netmask = ntohl(mask->sin_addr.s_addr);
      uint32_t broadcast = ip | ~netmask;

      asio::ip::address_v4::bytes_type bcast_bytes = 
      {
          static_cast<unsigned char>((broadcast >> 24) & 0xFF),
          static_cast<unsigned char>((broadcast >> 16) & 0xFF),
          static_cast<unsigned char>((broadcast >> 8) & 0xFF),
          static_cast<unsigned char>((broadcast) & 0xFF)
      };
      freeifaddrs(ifaddr);
      return asio::ip::address_v4(bcast_bytes);
    }
    freeifaddrs(ifaddr);
#endif

    return asio::ip::address_v4();
  }

#if defined(PLATFORM_WINDOWS)
  std::string HRESULTToString(HRESULT hr)
  {
    std::ostringstream oss;
    oss << "0x" << std::hex << hr;
    return oss.str();
  }
#endif

  std::vector<uint8_t> BGRAToJPEG(const std::vector<uint8_t>& bgra_data, uint32_t bgra_width, uint32_t bgra_height, uint8_t quality)
  {
    std::vector<uint8_t> rgbData(bgra_width * bgra_height * 3);
    for (int i = 0; i < bgra_width * bgra_height; i++)
    {
      rgbData[i * 3 + 0] = bgra_data[i * 4 + 2];
      rgbData[i * 3 + 1] = bgra_data[i * 4 + 1];
      rgbData[i * 3 + 2] = bgra_data[i * 4 + 0];
    }

    utils::MemoryBuffer jpegBuffer;
    stbi_write_jpg_to_func(utils::MemoryBuffer::WriteCallback, &jpegBuffer, bgra_width, bgra_height, 3, rgbData.data(), quality);

    return std::move(jpegBuffer.data);
  }
}