#pragma once

#include "net_common.h"

namespace rpc
{
  namespace net
  {
    template<typename T>
    struct message_header
    {
      T id = {};
      uint32_t size = 0;
    };

    template<typename T>
    struct message
    {
      message_header<T> header = {};
      std::vector<uint8_t> body;

      void push_back(const std::vector<uint8_t>& buffer)
      {
        body.insert(body.end(), buffer.begin(), buffer.end());
        header.size = body.size();
      }

      void pull_back(std::vector<uint8_t>& out_buffer, size_t size)
      {
        if (size > body.size())
          throw std::runtime_error("Attempt to pull more data than available in message body");

        size_t i = body.size() - size;
        out_buffer.insert(out_buffer.end(), body.begin() + i, body.end());
        body.resize(i);
        header.size = body.size();
      }

      template<typename DataType>
      friend message<T>& operator << (message<T>& msg, const DataType& data)
      {
        static_assert(std::is_standard_layout<DataType>::value, "Data type is too complex to be pushed into a vector");
        size_t i = msg.body.size();
        msg.body.resize(msg.body.size() + sizeof(DataType));
        std::memcpy(msg.body.data() + i, &data, sizeof(DataType));
        msg.header.size = msg.body.size();
        return msg;
      }

      template<typename DataType>
      friend message<T>& operator >> (message<T>& msg, DataType& data)
      {
        static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pulled from a vector");
        size_t i = msg.body.size() - sizeof(DataType);
        std::memcpy(&data, msg.body.data() + i, sizeof(DataType));
        msg.body.resize(i);
        msg.header.size = msg.body.size();
        return msg;
      }
    };

    template<typename T>
    class connection;

    template<typename T>
    struct owned_message
    {
      std::shared_ptr<connection<T>> remote = nullptr;
      message<T> msg;
    };
  }
}