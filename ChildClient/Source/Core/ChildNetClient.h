#pragma once

#include <rpc_core.h>
#include <rpc_net.h>

#include "Core/ScreenRecorder.h"

namespace rpc
{
  class ChildNetClient : public net::ClientInterface<net::message_type>
  {
  public:
    void SendFrameData(frame_data& frame);
    void SendFramePixels(frame_data& frame);

  };
}