#include <iostream>

#include <asio.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include "Core/Debug.h"
#include "Core/Utils.h"

#define BROADCAST_INTERVAL_SECONDS 5

#define COMMUNICATION_PORT 4000
#define DATA_TRANSFER_PORT 5000

#define MAX_MESSAGE_SIZE 128
#define PARENT_ACCESS_REQUEST_MESSAGE "RPC_PARENT_REMOTE_ACTIVATION_REQUEST"
#define PARENT_ACCESS_REQUEST_ACKNOWLEDGEMENT_MESSAGE "RPC_CHILD_REMOTE_ACTIVATION_REQUEST_ACKNOWLEDGEMENT"

int main()
{
  try
  {
    asio::io_context ioContext;
    std::optional<asio::ip::address_v4> childIP;
    std::mutex childIPMutex;

    std::thread broadcastThread([&]
      {
        asio::ip::udp::socket broadcastSocket(ioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), COMMUNICATION_PORT));
        asio::ip::udp::endpoint broadcastEndpoint(rpc::GetNetworkBroadcastAddress(ioContext), COMMUNICATION_PORT);
        broadcastSocket.set_option(asio::socket_base::broadcast(true));

        while (true) 
        {
          broadcastSocket.send_to(asio::buffer(std::string(PARENT_ACCESS_REQUEST_MESSAGE)), broadcastEndpoint);
          RPC_INFO("[PARENT REQUEST BROADCAST] Sent a broadcast to: {}:{}", broadcastEndpoint.address().to_string(), COMMUNICATION_PORT);
          std::this_thread::sleep_for(std::chrono::seconds(BROADCAST_INTERVAL_SECONDS));
        }
      });

    std::thread childCommunicationThread([&]
      {
        asio::ip::tcp::acceptor childMessageAcceptor(ioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), COMMUNICATION_PORT));
        while (true) 
        {
          try 
          {
            {
              std::lock_guard<std::mutex> lock(childIPMutex);
              if (childIP.has_value())
                continue;
            }

            asio::ip::tcp::socket childMessageSocket(ioContext);
            childMessageAcceptor.accept(childMessageSocket);

            std::array<char, MAX_MESSAGE_SIZE> messageBuffer;

            asio::error_code errorCode;
            size_t length = childMessageSocket.read_some(asio::buffer(messageBuffer), errorCode);

            if (!errorCode) 
            {
              std::string message(messageBuffer.data(), length);

              asio::ip::tcp::endpoint sender = childMessageSocket.remote_endpoint();
              if (message.compare(0, std::strlen(PARENT_ACCESS_REQUEST_ACKNOWLEDGEMENT_MESSAGE), PARENT_ACCESS_REQUEST_ACKNOWLEDGEMENT_MESSAGE) == 0)
              {
                RPC_INFO("[PARENT MESSAGE HANDLER] Recieved an access request acknowledgement from: {}:{}", sender.address().to_string(), COMMUNICATION_PORT);

                std::lock_guard<std::mutex> lock(childIPMutex);
                childIP.emplace(sender.address().to_v4());
                RPC_INFO("[PARENT MESSAGE HANDLER] Estabilished a connection with: {}:{}", sender.address().to_string(), COMMUNICATION_PORT);
              }
              else
              {
                RPC_ERROR("[PARENT MESSAGE HANDLER] Recieved an unknown message '{}' from: {}:{}", message, sender.address().to_string(), COMMUNICATION_PORT);
              }
            }
            else
            {
              RPC_ERROR("[PARENT MESSAGE HANDLER] TCP packet reception error: {}", errorCode.message());
            }

            childMessageSocket.shutdown(asio::ip::tcp::socket::shutdown_both, errorCode);
            childMessageSocket.close(errorCode);
          }
          catch (std::exception& e) 
          {
            RPC_ERROR("[PARENT MESSAGE HANDLER] TCP packet reception error: {}", e.what());
          }
        }
      });

    asio::ip::udp::socket recieveSocket(ioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), DATA_TRANSFER_PORT));
    std::vector<uint8_t> recvBuffer(65536);

    while (true)
    {
      asio::ip::udp::endpoint senderEndpoint;
      size_t lenght = recieveSocket.receive_from(asio::buffer(recvBuffer), senderEndpoint);

      uint32_t widthData = (recvBuffer[0] << 24) | (recvBuffer[1] << 16) | (recvBuffer[2] << 8) | recvBuffer[3];
      uint32_t heightData = (recvBuffer[4] << 24) | (recvBuffer[5] << 16) | (recvBuffer[6] << 8) | recvBuffer[7];
      uint32_t imageSizeData = (recvBuffer[8] << 24) | (recvBuffer[9] << 16) | (recvBuffer[10] << 8) | recvBuffer[11];
      RPC_WARN("[PARENT IMAGE RECIEVE] Recieved an image of {} bytes", imageSizeData + 12);
      std::vector<uint8_t> imageData(recvBuffer.begin() + 12, recvBuffer.end());

      int32_t width, height, channels;
      stbi_uc* data = stbi_load_from_memory(imageData.data(), imageData.size(), &width, &height, &channels, 3);
      RPC_ASSERT(data, "[PARENT IMAGE RECIEVE] Failed to load image: {}", stbi_failure_reason());

      size_t expectedSize = widthData * heightData * 3;
      if (width * height * 3 != expectedSize)
      {
        RPC_WARN("[PARENT IMAGE RECIEVE] Incomplete image received, expected {} bytes, got {} bytes", expectedSize, width * height * 3);
        stbi_image_free(data);
        continue;
      }
      std::vector<uint8_t> decompressedImageData(data, data + width * height * 3);
      stbi_image_free(data);

      stbi_write_jpg("output.jpg", width, height, 3, decompressedImageData.data(), 90);

      RPC_INFO("[PARENT IMAGE RECIEVE] Decompressed image: {}x{} {} bytes", width, height, decompressedImageData.size());
    }
  }
  catch (std::exception& e)
  {
    RPC_ASSERT(false, "[GENERAL] Caught exception: {}", e.what());
  }
}
