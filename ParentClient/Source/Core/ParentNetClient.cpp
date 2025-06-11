#include "Core/ParentNetClient.h"
#include "Core/Common.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace rpc
{
  ParentClient::ParentClient(uint16_t port) 
    : net::ServerInterface<net::message_type>(port) {}

  void ParentClient::ChangeFrameQuality(uint32_t quality)
  {
    net::message<net::message_type> msg;
    msg.header.id = net::message_type::server_frame_quality_change;

    msg << quality;
    ParentClient::MessageClient(m_ConnectedClient, msg);
  }

  bool ParentClient::NewFrameAvailable()
  {
    return m_NewFrameAvailable.exchange(false);
  }

  bool ParentClient::OnClientConnect(std::shared_ptr<net::connection<net::message_type>> client)
  {
    m_ConnectedClient = client;
    return true;
  }

  void ParentClient::OnClientDisconnect(std::shared_ptr<net::connection<net::message_type>> client)
  {
    m_ConnectedClient.reset();
  }

  void ParentClient::OnMessage(std::shared_ptr<net::connection<net::message_type>> client, net::message<net::message_type>& msg)
  {
    switch (msg.header.id)
    {
    case net::message_type::client_frame_data_update:
    {
      std::lock_guard<std::mutex> lock1(g_frameSizeMutex);
      std::lock_guard<std::mutex> lock2(g_frameQualityMutex);
      msg >> g_frameQuality >> g_frameHeight >> g_frameWidth;
      break;
    }
    case net::message_type::client_frame_pixels_update:
    {
      uint64_t size = 0;
      std::vector<uint8_t> jpegData;
      msg >> size;
      msg.pull_back(jpegData, size);

      std::thread([&, jpegData = std::move(jpegData)]()
        {
          int32_t width, height, channels;
          stbi_uc* data = stbi_load_from_memory(jpegData.data(), static_cast<int32_t>(jpegData.size()), &width, &height, &channels, 3);
          RPC_ASSERT(data, "[NETWORK] Failed to load image: {}", stbi_failure_reason());

          {
            std::lock_guard<std::mutex> lock(g_frameSizeMutex);
            if (g_frameHeight != height || g_frameWidth != width)
            {
              RPC_WARN("[NETWORK] Invalid image size");
              stbi_image_free(data);
              return;
            }
          }

          {
            std::lock_guard<std::mutex> lock(g_framePixelsMutex);
            g_framePixelsData = std::vector<uint8_t>(data, data + width * height * 3);
            m_NewFrameAvailable.store(true);
          }

          stbi_image_free(data);
        }).detach();
      
      break;
    }
    case net::message_type::client_input_update:
    {

      break;
    }
    default:
    {
      RPC_WARN("[NETWORK] Invalid message recieved, header id '{}'", static_cast<uint32_t>(msg.header.id));
      break;
    }
    }
  }
}