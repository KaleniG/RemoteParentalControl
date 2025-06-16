#pragma once

#include <string>

namespace rpc
{
  namespace net
  {
    enum class message_type
    {
      client_frame_data_update,
      client_frame_pixels_update,
      client_input_update,

      server_frame_quality_change
    };

    static constexpr const char* parent_id = "127.0.0.1"; //192.168.1.11
    static constexpr uint16_t parent_port = 12120;
  }
}