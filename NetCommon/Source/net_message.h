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

      void push_back(void* data, size_t size)
      {
        size_t i = body.size();
        body.resize(body.size() + size);
        std::memcpy(body.data() + i, data, size);
        header.size = body.size();
      }

      void pull_back(void* out, size_t size)
      {
        size_t i = body.size() - size;
        std::memcpy(out, body.data() + i, size);
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