#pragma once

#include <filesystem>
#include <string>
#include <format>

#include "Core/PlatformDetection.h"

#if defined(PLATFORM_WINDOWS)
#define RPC_DEBUG_BREAK() __debugbreak()
#else
#include <csignal>
#define RPC_DEBUG_BREAK() raise(SIGTRAP)
#endif

namespace rpc
{
  enum class LogLevel
  {
    Info, Warn, Error, Assert
  };

  void ConsolePrint(const std::string& time, const std::string& message, LogLevel level);
}

#if defined(CONFIG_DEBUG) || defined(CONFIG_RELEASE)
#define RPC_INFO(fmt, ...) {\
std::string message = std::format(fmt, __VA_ARGS__);\
rpc::ConsolePrint(__TIME__, message, rpc::LogLevel::Info);}\

#define RPC_WARN(fmt, ...) {\
std::string message = std::format(fmt, __VA_ARGS__);\
rpc::ConsolePrint(__TIME__, message, rpc::LogLevel::Warn);}\

#define RPC_ERROR(fmt, ...) {\
std::string message = std::format(fmt, __VA_ARGS__);\
rpc::ConsolePrint(__TIME__, message, rpc::LogLevel::Error);}\

#define RPC_ASSERT(x, fmt, ...) { if (!(x)) { \
std::string message = std::format(fmt, __VA_ARGS__);\
rpc::ConsolePrint(__TIME__, message, rpc::LogLevel::Assert);\
RPC_DEBUG_BREAK(); } }\

#elif defined(CONFIG_FINAL)
#define RPC_INFO(...)

#define RPC_WARN(...)

#define RPC_ERROR(fmt, ...)

#define RPC_ASSERT(x, fmt, ...)

#endif