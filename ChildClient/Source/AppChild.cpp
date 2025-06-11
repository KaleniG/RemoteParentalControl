#include <rpc_net.h>

#include "Core/ScreenRecorder.h"
#include "Core/ChildNetClient.h"

int main()
{
  uint32_t currentFrameHeight = 0;
  uint32_t currentFrameWidth = 0;
  uint32_t currentFrameQuality = 50;

  rpc::ChildNetClient netClient;
  rpc::ScreenRecorder screenRecorder(currentFrameQuality);

  netClient.Connect(rpc::net::parent_id, rpc::net::parent_port);

  while (true)
  {
    if (netClient.IsConnected())
    {
      rpc::frame_data frame = screenRecorder.GetFrame();

      if (frame.is_valid())
      {
        if (frame.height != currentFrameHeight || frame.width != currentFrameWidth || frame.quality != currentFrameQuality)
        {
          currentFrameHeight = frame.height;
          currentFrameWidth = frame.width;
          currentFrameQuality = frame.quality;
          netClient.SendFrameData(frame);
        }

        netClient.SendFramePixels(frame);
      }

      if (!netClient.Incoming().empty())
      {
        auto msg = netClient.Incoming().pop_front().msg;

        switch (msg.header.id)
        {
          case rpc::net::message_type::server_frame_quality_change:
          {
            uint32_t frameQuality = 0;
            msg >> frameQuality;
            screenRecorder.SetFrameQuality(frameQuality);
            RPC_INFO("[NETWORK] Recieved a request to set the quality to '{}'", frameQuality);
            break;
          }
        }
      }
    }
    else
    {
      std::this_thread::sleep_for(std::chrono::seconds(5));
      netClient.Connect(rpc::net::parent_id, rpc::net::parent_port);
    }
  }
}