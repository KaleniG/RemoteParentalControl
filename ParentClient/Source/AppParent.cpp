#define YK_ENABLE_DEBUG_LOG
#define YK_ENABLE_DEBUG_PROFILING_LOG

#include <rpc_core.h>

#include "Core/ParentNetClient.h"
#include "Core/Renderer.h"
#include "Core/Common.h"

int main()
{
  rpc::ParentClient netClient(rpc::net::parent_port);
  netClient.Start();

  rpc::Renderer renderer(1600, 900, "Parent Client");

  while (renderer.IsRunning())
  {
    netClient.Update(1, false);

    renderer.Clear();

    if (netClient.ClientConnected())
    {
      {
        std::lock_guard<std::mutex> lock(g_frameQualityMutex);
        if (g_frameQuality != g_newFrameQuality)
        {
          netClient.ChangeFrameQuality(g_newFrameQuality);
          g_frameQuality = g_newFrameQuality;
        }
      }

      if (netClient.NewFrameAvailable())
      {
        renderer.UpdateTexture();
      }

      renderer.Render();
    }
    
    renderer.Update();
  }

  netClient.Stop();
}
