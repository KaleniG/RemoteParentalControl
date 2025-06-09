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
    int32_t quality = 0;
    int32_t height = 0;
    int32_t width = 0;
    std::vector<uint8_t> pixels;

    bool is_valid() const
    {
      return !pixels.empty() && quality <= 100 && quality >= 1 && height > 0 && width > 0;
    }
  };

  class ScreenRecorder
  {
  public:
    ScreenRecorder(int32_t frame_quality);
    ~ScreenRecorder();

    void SetFrameQuality(int32_t quality);

    frame_data GetFrame();

  private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_D3DDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_D3DContext;
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> m_DXGIOutputDuplication;
    int32_t m_FrameQuality;
  };
}