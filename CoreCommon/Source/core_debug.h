#pragma once

#include <string>
#include <format>

#include "core_platform.h"

#if defined(PLATFORM_WINDOWS)
#define RPC_DEBUG_BREAK() __debugbreak()
#else
#include <csignal>
#define RPC_DEBUG_BREAK() raise(SIGTRAP)
#endif

namespace rpc
{
  enum class log_level
  {
    info, warn, error, assert
  };

  void console_print(const std::string& time, const std::string& message, log_level level);
}

#if defined(CONFIG_DEBUG) || defined(CONFIG_RELEASE)
#define RPC_INFO(fmt, ...) {\
std::string message = std::format(fmt, __VA_ARGS__);\
rpc::console_print(__TIME__, message, rpc::log_level::info);}\

#define RPC_WARN(fmt, ...) {\
std::string message = std::format(fmt, __VA_ARGS__);\
rpc::console_print(__TIME__, message, rpc::log_level::warn);}\

#define RPC_ERROR(fmt, ...) {\
std::string message = std::format(fmt, __VA_ARGS__);\
rpc::console_print(__TIME__, message, rpc::log_level::error);}\

#define RPC_ASSERT(x, fmt, ...) { if (!(x)) { \
std::string message = std::format(fmt, __VA_ARGS__);\
rpc::console_print(__TIME__, message, rpc::log_level::assert);\
RPC_DEBUG_BREAK(); } }\

#elif defined(CONFIG_FINAL)
#define RPC_INFO(...)

#define RPC_WARN(...)

#define RPC_ERROR(fmt, ...)

#define RPC_ASSERT(x, fmt, ...)
#endif