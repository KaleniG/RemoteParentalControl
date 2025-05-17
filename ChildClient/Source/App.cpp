#include <optional>
#include <thread>
#include <memory>
#include <set>

#include <stb_image_write.h>
#include <wrl/client.h>
#include <dxgi1_2.h>
#include <asio.hpp>
#include <d3d11.h>

#include "Core/Debug.h"
#include "Core/Utils.h"

#define CONNECTION_TIMEOUT_SECONDS 15

#define COMMUNICATION_PORT 4000
#define DATA_TRANSFER_PORT 5000

#define MAX_MESSAGE_SIZE 128
#define PARENT_ACCESS_REQUEST_MESSAGE "RPC_PARENT_REMOTE_ACTIVATION_REQUEST"
#define PARENT_ACCESS_REQUEST_ACKNOWLEDGEMENT_MESSAGE "RPC_CHILD_REMOTE_ACTIVATION_REQUEST_ACKNOWLEDGEMENT"

#define UDP_PACKET_MAX_SIZE 65536

int main()
{
  try
  {
    asio::io_context ioContext;
    auto workGuard = asio::make_work_guard(ioContext);

    std::optional<asio::ip::address_v4> parentIP;
    std::unique_ptr<asio::steady_timer> parentConnectionTimer;
    std::mutex parentIPMutex;

    auto socket = std::make_shared<asio::ip::udp::socket>(ioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), COMMUNICATION_PORT));
    auto messageBuffer = std::make_shared<std::array<char, MAX_MESSAGE_SIZE>>();
    auto parentUDPEndpoint = std::make_shared<asio::ip::udp::endpoint>();

    std::function<void()> startReceive;

    startReceive = [&]() {
      socket->async_receive_from(
        asio::buffer(*messageBuffer), *parentUDPEndpoint,
        [&, socket, messageBuffer, parentUDPEndpoint](const asio::error_code& errorCode, std::size_t length)
        {
          if (errorCode)
          {
            RPC_ERROR("[CHILD MESSAGE HANDLER] UDP packet reception error: {}", errorCode.message());
            startReceive();
            return;
          }

          std::string message(messageBuffer->data(), length);

          std::lock_guard<std::mutex> lock(parentIPMutex);

          if (message.compare(0, std::strlen(PARENT_ACCESS_REQUEST_MESSAGE), PARENT_ACCESS_REQUEST_MESSAGE) == 0)
          {
            if (!parentIP.has_value())
            {
              RPC_INFO("[CHILD MESSAGE HANDLER] Received a parent access request from {}:{}", parentUDPEndpoint->address().to_string(), COMMUNICATION_PORT);

              try
              {
                asio::ip::tcp::socket tcpSocket(ioContext);
                asio::ip::tcp::endpoint parentTCPEndpoint(parentUDPEndpoint->address().to_v4(), COMMUNICATION_PORT);
                tcpSocket.connect(parentTCPEndpoint);
                tcpSocket.send(asio::buffer(std::string(PARENT_ACCESS_REQUEST_ACKNOWLEDGEMENT_MESSAGE)));

                RPC_INFO("[CHILD MESSAGE HANDLER] Sent an acknowledgement message to the parent {}:{}", parentUDPEndpoint->address().to_string(), COMMUNICATION_PORT);
              }
              catch (const std::exception& e)
              {
                RPC_ERROR("[CHILD MESSAGE HANDLER] TCP connection with the parent failed: {}", e.what());
                startReceive();
                return;
              }

              parentIP = parentUDPEndpoint->address().to_v4();
              RPC_INFO("[CHILD MESSAGE HANDLER] The parent now is {}:{}", parentIP.value().to_string(), COMMUNICATION_PORT);

              parentConnectionTimer = std::make_unique<asio::steady_timer>(ioContext);
              parentConnectionTimer->expires_after(std::chrono::seconds(CONNECTION_TIMEOUT_SECONDS));
              parentConnectionTimer->async_wait([&](const asio::error_code& error_code)
                {
                  if (!error_code)
                  {
                    std::lock_guard<std::mutex> lock(parentIPMutex);
                    RPC_WARN("[CHILD MESSAGE HANDLER] The parent timed out, resetting the connection");
                    parentIP.reset();
                    parentConnectionTimer.reset();
                  }
                });
            }
            else if (parentIP.value() == parentUDPEndpoint->address().to_v4())
            {
              RPC_INFO("[CHILD MESSAGE HANDLER] Received keepalive broadcast from parent, resetting the timer");

              if (!parentConnectionTimer)
                parentConnectionTimer = std::make_unique<asio::steady_timer>(ioContext);

              parentConnectionTimer->expires_after(std::chrono::seconds(CONNECTION_TIMEOUT_SECONDS));
              parentConnectionTimer->async_wait([&](const asio::error_code& error_code)
                {
                  if (!error_code)
                  {
                    std::lock_guard<std::mutex> lock(parentIPMutex);
                    RPC_WARN("[CHILD MESSAGE HANDLER] The parent timed out, resetting the connection");
                    parentIP.reset();
                    parentConnectionTimer.reset();
                  }
                });
            }
          }
          else
          {
            RPC_ERROR("[CHILD MESSAGE HANDLER] Received an unknown message '{}' from {}:{}", message, parentUDPEndpoint->address().to_string(), COMMUNICATION_PORT);
          }

          startReceive();
        });
      };

    startReceive();

    std::thread asioAsync(&asio::io_context::run, &ioContext);

    stbi_flip_vertically_on_write(true);

    HRESULT result;
    Microsoft::WRL::ComPtr<ID3D11Device> d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3dContext;
    D3D_FEATURE_LEVEL featureLevel;
    result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &d3dDevice, &featureLevel, &d3dContext);
    RPC_ASSERT(!FAILED(result), "[SCREEN CAPTURE] D3D11 error: {}", rpc::HRESULTToString(result));

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    result = d3dDevice.As(&dxgiDevice);
    RPC_ASSERT(!FAILED(result), "[SCREEN CAPTURE] D3D11 error: {}", rpc::HRESULTToString(result));

    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    result = dxgiDevice->GetAdapter(&adapter);
    RPC_ASSERT(!FAILED(result), "[SCREEN CAPTURE] D3D11 error: {}", rpc::HRESULTToString(result));

    Microsoft::WRL::ComPtr<IDXGIOutput> output;
    result = adapter->EnumOutputs(0, &output);
    RPC_ASSERT(!FAILED(result), "[SCREEN CAPTURE] D3D11 error: {}", rpc::HRESULTToString(result));

    Microsoft::WRL::ComPtr<IDXGIOutput1> output1;
    result = output.As(&output1);
    RPC_ASSERT(!FAILED(result), "[SCREEN CAPTURE] D3D11 error: {}", rpc::HRESULTToString(result));

    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> duplication;
    result = output1->DuplicateOutput(d3dDevice.Get(), &duplication);
    RPC_ASSERT(!FAILED(result), "[SCREEN CAPTURE] D3D11 error: {}", rpc::HRESULTToString(result));

    asio::ip::udp::socket sendSocket(ioContext);
    sendSocket.open(asio::ip::udp::v4());

    while (true)
    {
      {
        std::lock_guard<std::mutex> lock(parentIPMutex);
        if (!parentIP.has_value())
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
          continue;
        }
      }

      DXGI_OUTDUPL_FRAME_INFO frameInfo = {};
      Microsoft::WRL::ComPtr<IDXGIResource> desktopResource;

      result = duplication->AcquireNextFrame(100, &frameInfo, &desktopResource);
      if (result == DXGI_ERROR_WAIT_TIMEOUT)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        continue;
      }
      RPC_ASSERT(!FAILED(result), "[SCREEN CAPTURE] Failed to acquire frame: {}", rpc::HRESULTToString(result));

      Microsoft::WRL::ComPtr<ID3D11Texture2D> frame;
      result = desktopResource.As(&frame);
      RPC_ASSERT(!FAILED(result), "[SCREEN CAPTURE] Failed to obtaining a texture, error: {}", rpc::HRESULTToString(result));

      D3D11_TEXTURE2D_DESC desc;
      frame->GetDesc(&desc);

      D3D11_TEXTURE2D_DESC cpuDesc = desc;
      cpuDesc.Usage = D3D11_USAGE_STAGING;
      cpuDesc.BindFlags = 0;
      cpuDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      cpuDesc.MiscFlags = 0;

      Microsoft::WRL::ComPtr<ID3D11Texture2D> cpuTexture;
      result = d3dDevice->CreateTexture2D(&cpuDesc, nullptr, &cpuTexture);
      RPC_ASSERT(!FAILED(result), "[SCREEN CAPTURE] Failed to create a staging texture, error: {}", rpc::HRESULTToString(result));

      d3dContext->CopyResource(cpuTexture.Get(), frame.Get());

      D3D11_MAPPED_SUBRESOURCE mapped;
      result = d3dContext->Map(cpuTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
      RPC_ASSERT(!FAILED(result), "[SCREEN CAPTURE] Failed to map a texture, error: {}", rpc::HRESULTToString(result));

      std::vector<uint8_t> BGRAImageData(mapped.RowPitch * desc.Height);
      std::memcpy(BGRAImageData.data(), mapped.pData, BGRAImageData.size());

      std::vector<uint8_t> JPEGImageData(rpc::BGRAToJPEG(BGRAImageData, desc.Width, desc.Height, 1));

      std::vector<uint8_t> sizeData;
      sizeData.push_back(static_cast<uint8_t>(desc.Width >> 24));
      sizeData.push_back(static_cast<uint8_t>(desc.Width >> 16));
      sizeData.push_back(static_cast<uint8_t>(desc.Width >> 8));
      sizeData.push_back(static_cast<uint8_t>(desc.Width));

      sizeData.push_back(static_cast<uint8_t>(desc.Height >> 24));
      sizeData.push_back(static_cast<uint8_t>(desc.Height >> 16));
      sizeData.push_back(static_cast<uint8_t>(desc.Height >> 8));
      sizeData.push_back(static_cast<uint8_t>(desc.Height));

      sizeData.push_back(static_cast<uint8_t>(static_cast<uint32_t>(JPEGImageData.size()) >> 24));
      sizeData.push_back(static_cast<uint8_t>(static_cast<uint32_t>(JPEGImageData.size()) >> 16));
      sizeData.push_back(static_cast<uint8_t>(static_cast<uint32_t>(JPEGImageData.size()) >> 8));
      sizeData.push_back(static_cast<uint8_t>(static_cast<uint32_t>(JPEGImageData.size())));

      std::vector<uint8_t> FrameData;
      FrameData.insert(FrameData.end(), sizeData.begin(), sizeData.end());
      FrameData.insert(FrameData.end(), JPEGImageData.begin(), JPEGImageData.end());

      d3dContext->Unmap(cpuTexture.Get(), 0);
      result = duplication->ReleaseFrame();
      RPC_ASSERT(!FAILED(result), "[SCREEN CAPTURE] Failed to release the frame, error: {}", rpc::HRESULTToString(result));

      {
        std::lock_guard<std::mutex> lock(parentIPMutex);
        if (!parentIP.has_value())
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
          continue;
        }
        if (FrameData.size() < UDP_PACKET_MAX_SIZE)
          sendSocket.send_to(asio::buffer(FrameData), asio::ip::udp::endpoint(parentIP.value(), DATA_TRANSFER_PORT));
      }
      RPC_INFO("[CHILD IMAGE SEND] Sent an image of {} bytes with resolution {}x{}", FrameData.size() - 8, desc.Width, desc.Height);
    }
  }
  catch (const std::exception& e)
  {
    RPC_ASSERT(false, "[GENERAL] Caught exception: {}", e.what());
  }
}
