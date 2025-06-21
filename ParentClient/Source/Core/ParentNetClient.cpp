#include "Core/ParentNetClient.h"
#include "Core/Common.h"

#include <YKLib.h>

namespace rpc
{
  ParentClient::ParentClient(uint16_t port) 
    : net::ServerInterface<net::message_type>(port) 
  {
    m_Decompressor = tjInitDecompress();
    YK_ASSERT(m_Decompressor, "[SCREEN RECORDER] TurboJPEG error: failed to initialize the decompressor");
  }

  ParentClient::~ParentClient()
  {
    tjDestroy(m_Decompressor);
  }

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
          yk::Timer timer;
          timer.Start();

          int32_t width, height, jpegSubsamp, jpegColorspace;
          std::vector<uint8_t> rgbBuffer;
          {
            std::lock_guard<std::mutex> lock(m_DecompressorMutex);
            if (tjDecompressHeader3(m_Decompressor, jpegData.data(), jpegData.size(), &width, &height, &jpegSubsamp, &jpegColorspace) != 0)
            {
              YK_ERROR("[SCREEN RECORDER] Compression failed: {}", tjGetErrorStr());
              return;
            }

            rgbBuffer.resize(width * height * 3);

            if (tjDecompress2(m_Decompressor, jpegData.data(), jpegData.size(), rgbBuffer.data(), width, 0, height, TJPF_RGB, 0) != 0)
            {
              YK_ERROR("[SCREEN RECORDER] Compression failed: {}", tjGetErrorStr());
              return;
            }
          }

          const int rowSize = width * 3;
          std::vector<uint8_t> tempRow(rowSize);
          for (int y = 0; y < height / 2; ++y)
          {
            uint8_t* rowTop = rgbBuffer.data() + y * rowSize;
            uint8_t* rowBottom = rgbBuffer.data() + (height - 1 - y) * rowSize;

            std::memcpy(tempRow.data(), rowTop, rowSize);
            std::memcpy(rowTop, rowBottom, rowSize);
            std::memcpy(rowBottom, tempRow.data(), rowSize);
          }

          {
            std::lock_guard<std::mutex> lock(g_frameSizeMutex);
            if (g_frameHeight != height || g_frameWidth != width)
            {
              YK_WARN("[NETWORK] Invalid image size");
              return;
            }
          }

          {
            std::lock_guard<std::mutex> lock(g_framePixelsMutex);
            g_framePixelsData = std::move(rgbBuffer);
            m_NewFrameAvailable.store(true);
          }

          YK_INFO("{}ms", static_cast<int32_t>(timer.ElapsedMilliseconds()));
        }).detach();
      
      break;
    }
    case net::message_type::client_input_update:
    {

      break;
    }
    default:
    {
      YK_WARN("[NETWORK] Invalid message recieved, header id '{}'", static_cast<uint32_t>(msg.header.id));
      break;
    }
    }
  }
}