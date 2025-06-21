#include "Core/ChildNetClient.h"

namespace rpc
{
  void ChildNetClient::SendFrameData(frame_data& frame)
  {
    net::message<net::message_type> msg;
    msg.header.id = net::message_type::client_frame_data_update;

    msg << frame.width << frame.height << frame.quality;
    ChildNetClient::Send(msg);
  }

  void ChildNetClient::SendFramePixels(frame_data& frame)
  {
    net::message<net::message_type> msg;
    msg.header.id = net::message_type::client_frame_pixels_update;

    msg.push_back(frame.pixels);
    msg << frame.size;
    ChildNetClient::Send(msg);
  }
}