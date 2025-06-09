#include "core_debug.h"

#if defined(PLATFORM_WINDOWS)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace rpc
{
#if defined(PLATFORM_WINDOWS)
  static HANDLE s_ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

  void console_print(const std::string& time, const std::string& message, log_level level)
  {
    std::string finalMessage;
#if !defined(PLATFORM_WINDOWS)
    switch (level)
    {
    case log_level::info:
      finalMessage.append("\033[38;5;15m\033[48;5;0m");
      break;
    case log_level::warn:
      finalMessage.append("\033[38;5;11m\033[48;5;0m");
      break;
    case log_level::error:
      finalMessage.append("\033[38;5;9m\033[48;5;0m");
      break;
    case log_level::assert:
      finalMessage.append("\033[38;5;15m\033[48;5;9m");
      break;
    }
#else
    switch (level)
    {
    case log_level::info:
      SetConsoleTextAttribute(s_ConsoleHandle, 15 | (0 << 4));
      break;
    case log_level::warn:
      SetConsoleTextAttribute(s_ConsoleHandle, 14 | (0 << 4));
      break;
    case log_level::error:
      SetConsoleTextAttribute(s_ConsoleHandle, 12 | (0 << 4));
      break;
    case log_level::assert:
      SetConsoleTextAttribute(s_ConsoleHandle, 15 | (12 << 4));
      break;
    }
#endif
    finalMessage.append("[" + time + "]");
    finalMessage.append(message);
#if !defined(PLATFORM_WINDOWS)
    finalMessage.append("\033[0m");
#endif
    finalMessage.append("\n");
#if defined(PLATFORM_WINDOWS)
    DWORD written;
    WriteConsoleA(s_ConsoleHandle, finalMessage.c_str(), static_cast<DWORD>(finalMessage.size()), &written, nullptr);
    SetConsoleTextAttribute(s_ConsoleHandle, 7 | (0 << 4));
#else
    write(STDOUT_FILENO, finalMessage.c_str(), finalMessage.size());
#endif
  }
}