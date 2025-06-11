#pragma once

#include <vector>

#include <rpc_core.h>

#if defined(PLATFORM_WINDOWS)
  #include <d3d11.h>
  #include <dxgi1_2.h>
  #include <wrl/client.h>
#endif

namespace rpc
{
  struct frame_data
  {
    uint32_t quality = 0;
    uint32_t height = 0;
    uint32_t width = 0;
    uint64_t size = 0;
    std::vector<uint8_t> pixels;

    bool is_valid() const
    {
      return !pixels.empty() && quality <= 100 && quality >= 1 && height > 0 && width > 0 && size > 0;
    }
  };

  class ScreenRecorder
  {
  public:
    ScreenRecorder(int32_t frame_quality);
    ~ScreenRecorder();

    void SetFrameQuality(uint32_t quality);
    uint32_t GetFrameQuality() const;

    frame_data GetFrame();

  private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_D3DDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_D3DContext;
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> m_DXGIOutputDuplication;
    uint32_t m_FrameQuality;
  };
}