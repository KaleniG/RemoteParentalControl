#include "Core/Debug.h"

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

  void ConsolePrint(const std::string& time, const std::string& message, LogLevel level)
  {
    std::string finalMessage;
#if !defined(PLATFORM_WINDOWS)
    switch (level)
    {
    case LogLevel::Info:
      finalMessage.append("\033[38;5;15m\033[48;5;0m");
      break;
    case LogLevel::Warn:
      finalMessage.append("\033[38;5;11m\033[48;5;0m");
      break;
    case LogLevel::Error:
      finalMessage.append("\033[38;5;9m\033[48;5;0m");
      break;
    case LogLevel::Assert:
      finalMessage.append("\033[38;5;15m\033[48;5;9m");
      break;
    }
#else
    switch (level)
    {
    case LogLevel::Info:
      SetConsoleTextAttribute(s_ConsoleHandle, 15 | (0 << 4));
      break;
    case LogLevel::Warn:
      SetConsoleTextAttribute(s_ConsoleHandle, 14 | (0 << 4));
      break;
    case LogLevel::Error:
      SetConsoleTextAttribute(s_ConsoleHandle, 12 | (0 << 4));
      break;
    case LogLevel::Assert:
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
    WriteConsoleA(s_ConsoleHandle, finalMessage.c_str(), finalMessage.size(), &written, nullptr);
    SetConsoleTextAttribute(s_ConsoleHandle, 7 | (0 << 4));
#else
    write(STDOUT_FILENO, finalMessage.c_str(), finalMessage.size());
#endif
  }
}