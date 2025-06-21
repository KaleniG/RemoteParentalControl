#include "Core/ScreenRecorder.h"

#define YK_ENABLE_DEBUG_LOG
#define YK_ENABLE_DEBUG_PROFILING_LOG

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <YKLib.h>

namespace rpc
{
  ScreenRecorder::ScreenRecorder(int32_t frame_quality)
  {
    YK_ASSERT(frame_quality >= 1 && frame_quality <= 100, "[SCREEN RECORDER] Frame quality should be in range of 1 to 100");

    m_FrameQuality = frame_quality;

    m_Compressor = tjInitCompress();
    YK_ASSERT(m_Compressor, "[SCREEN RECORDER] TurboJPEG error: failed to initialize the compressor");

    stbi_flip_vertically_on_write(true);

    HRESULT result;

    D3D_FEATURE_LEVEL featureLevel;
    result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &m_D3DDevice, &featureLevel, &m_D3DContext);
    YK_ASSERT(!FAILED(result), "[SCREEN RECORDER] D3D11 error: {}", HRESULTToString(result));

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    result = m_D3DDevice.As(&dxgiDevice);
    YK_ASSERT(!FAILED(result), "[SCREEN RECORDER] DXGI error: {}", HRESULTToString(result));

    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    result = dxgiDevice->GetAdapter(&adapter);
    YK_ASSERT(!FAILED(result), "[SCREEN RECORDER] DXGI error: {}", HRESULTToString(result));

    Microsoft::WRL::ComPtr<IDXGIOutput> output;
    result = adapter->EnumOutputs(0, &output);
    YK_ASSERT(!FAILED(result), "[SCREEN RECORDER] DXGI error: {}", HRESULTToString(result));

    Microsoft::WRL::ComPtr<IDXGIOutput1> output1;
    result = output.As(&output1);
    YK_ASSERT(!FAILED(result), "[SCREEN RECORDER] DXGI error: {}", HRESULTToString(result));

    result = output1->DuplicateOutput(m_D3DDevice.Get(), &m_DXGIOutputDuplication);
    YK_ASSERT(!FAILED(result), "[SCREEN RECORDER] DXGI error: {}", HRESULTToString(result));
  }

  ScreenRecorder::~ScreenRecorder()
  {
    tjDestroy(m_Compressor);
  }

  void ScreenRecorder::SetFrameQuality(uint32_t quality)
  {
    YK_ASSERT(quality >= 1 && quality <= 100, "[SCREEN RECORDER] Frame quality should be in range of 1 to 100");
    m_FrameQuality = quality;
  }

  uint32_t ScreenRecorder::GetFrameQuality() const
  {
    return m_FrameQuality;
  }

  frame_data ScreenRecorder::GetFrame()
  {
    HRESULT result;

    DXGI_OUTDUPL_FRAME_INFO frameInfo = {};
    Microsoft::WRL::ComPtr<IDXGIResource> desktopResource;

    result = m_DXGIOutputDuplication->AcquireNextFrame(100, &frameInfo, &desktopResource);

    if (result == DXGI_ERROR_WAIT_TIMEOUT)
    {
      return frame_data();
    }

    if (FAILED(result))
    {
      YK_WARN("[SCREEN CAPTURE] Failed to acquire frame: {}", HRESULTToString(result));
      return frame_data();
    }

    if (frameInfo.AccumulatedFrames == 0)
    {
      result = m_DXGIOutputDuplication->ReleaseFrame();
      YK_ASSERT(!FAILED(result), "[SCREEN CAPTURE] Failed to release the frame, error: {}", HRESULTToString(result));
      return frame_data();
    }

    Microsoft::WRL::ComPtr<ID3D11Texture2D> frame;
    result = desktopResource.As(&frame);
    if (FAILED(result))
    {
      YK_WARN("[SCREEN CAPTURE] Failed to obtaining a texture, error: {}", HRESULTToString(result));
      result = m_DXGIOutputDuplication->ReleaseFrame();
      YK_ASSERT(!FAILED(result), "[SCREEN CAPTURE] Failed to release the frame, error: {}", HRESULTToString(result));
      return frame_data();
    }

    D3D11_TEXTURE2D_DESC desc;
    frame->GetDesc(&desc);

    D3D11_TEXTURE2D_DESC cpuDesc = desc;
    cpuDesc.Usage = D3D11_USAGE_STAGING;
    cpuDesc.BindFlags = 0;
    cpuDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    cpuDesc.MiscFlags = 0;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> cpuTexture;
    result = m_D3DDevice->CreateTexture2D(&cpuDesc, nullptr, &cpuTexture);
    if (FAILED(result))
    {
      YK_WARN("[SCREEN CAPTURE] Failed to create a staging texture, error: {}", HRESULTToString(result));
      result = m_DXGIOutputDuplication->ReleaseFrame();
      YK_ASSERT(!FAILED(result), "[SCREEN CAPTURE] Failed to release the frame, error: {}", HRESULTToString(result));
      return frame_data();
    }

    m_D3DContext->CopyResource(cpuTexture.Get(), frame.Get());

    D3D11_MAPPED_SUBRESOURCE mapped;
    result = m_D3DContext->Map(cpuTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(result))
    {
      YK_WARN("[SCREEN CAPTURE] Failed to map a texture, error: {}", HRESULTToString(result));
      result = m_DXGIOutputDuplication->ReleaseFrame();
      YK_ASSERT(!FAILED(result), "[SCREEN CAPTURE] Failed to release the frame, error: {}", HRESULTToString(result));
      return frame_data();
    }

    std::vector<uint8_t> BGRAData(mapped.RowPitch * desc.Height);
    std::memcpy(BGRAData.data(), mapped.pData, BGRAData.size());

    std::vector<uint8_t> RGBData(desc.Width * desc.Height * 3);
    for (uint32_t i = 0; i < desc.Width * desc.Height; i++)
    {
      RGBData[i * 3 + 0] = BGRAData[i * 4 + 2];
      RGBData[i * 3 + 1] = BGRAData[i * 4 + 1];
      RGBData[i * 3 + 2] = BGRAData[i * 4 + 0];
    }

    m_D3DContext->Unmap(cpuTexture.Get(), 0);
    result = m_DXGIOutputDuplication->ReleaseFrame();
    YK_ASSERT(!FAILED(result), "[SCREEN CAPTURE] Failed to release the frame, error: {}", HRESULTToString(result));

    struct MemoryBuffer
    {
      std::vector<uint8_t> data;

      static void WriteCallback(void* context, void* data, int32_t size)
      {
        MemoryBuffer* buffer = static_cast<MemoryBuffer*>(context);
        buffer->data.insert(buffer->data.end(), static_cast<uint8_t*>(data), static_cast<uint8_t*>(data) + size);
      }
    } JPEGBuffer;

    yk::Timer timer;
    timer.Start();

    uint8_t* jpegBuf = nullptr;
    unsigned long jpegSize = 0;

    if (tjCompress2(m_Compressor, RGBData.data(), desc.Width, 0, desc.Height, TJPF_RGB, &jpegBuf, &jpegSize, TJSAMP_444, m_FrameQuality, TJFLAG_FASTDCT) != 0)
    {
      YK_ERROR("[SCREEN RECORDER] Compression failed: {}", tjGetErrorStr());
      return frame_data();
    }

    JPEGBuffer.data.assign(jpegBuf, jpegBuf + jpegSize);
    tjFree(jpegBuf);

    frame_data frameData;
    frameData.height = desc.Height;
    frameData.width = desc.Width;
    frameData.quality = m_FrameQuality;
    frameData.pixels = std::move(JPEGBuffer.data);
    frameData.size = frameData.pixels.size();

    YK_INFO("{}ms", static_cast<int32_t>(timer.ElapsedMilliseconds()));
    return frameData;
  }
}