outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["asio"]          = "Deps/asio/asio/include"
IncludeDir["stb"]           = "Deps/stb"
IncludeDir["GLFW"]          = "Deps/GLFW/include"
IncludeDir["GLAD"]          = "Deps/GLAD/include"
IncludeDir["libjpeg_turbo"] = "Deps/libjpeg-turbo/src"
IncludeDir["NetCommon"]     = "NetCommon/Source"
IncludeDir["CoreCommon"]    = "CoreCommon/Source"
IncludeDir["YKLib"]         = "Deps/YKLib/YKLib/Source"

LibDir = {}
LibDir["libjpeg_turbo_debug32"]   = "Deps/libjpeg-turbo/build32/Debug"
LibDir["libjpeg_turbo_debug64"]   = "Deps/libjpeg-turbo/build64/Debug"
LibDir["libjpeg_turbo_release32"] = "Deps/libjpeg-turbo/build32/Release"
LibDir["libjpeg_turbo_release64"] = "Deps/libjpeg-turbo/build64/Release"

workspace "RemoteParentalControl"
  startproject "ParentClient"

  configurations 
  { 
    "Debug",
    "DebugDLL",
    "Release",
    "ReleaseDLL",
    "Final",
    "FinalDLL"
  }

  platforms 
  { 
    "Win32",
    "Win64",
    "Linux",
    "MacOS"
  }

  flags
  {
    "MultiProcessorCompile"
  }

  filter { "platforms:Win32" }
    system "Windows"
    architecture "x86"
    systemversion "latest"
    defines { "PLATFORM_WINDOWS", "ARCH_X86", "_CRT_SECURE_NO_WARNINGS", "_SCL_SECURE_NO_WARNINGS", "NOMINMAX" }

  filter { "platforms:Win64" }
    system "Windows"
    architecture "x64"
    systemversion "latest"
    defines { "PLATFORM_WINDOWS", "ARCH_X64", "_CRT_SECURE_NO_WARNINGS", "_SCL_SECURE_NO_WARNINGS", "NOMINMAX" }

  filter { "platforms:Linux" }
    system "Linux"
    architecture "x64"
    defines { "PLATFORM_LINUX", "ARCH_X64" }
    buildoptions { "-std=c++20" }

  filter { "platforms:MacOS" }
    system "macosx"
    architecture "x64"
    defines { "PLATFORM_MACOS", "ARCH_X64" }

    -- Configuration Filters
  filter { "configurations:Debug" }
    symbols "On"
    optimize "Off"
    defines { "CONFIG_DEBUG" }

  filter { "configurations:DebugDLL" }
    symbols "On"
    optimize "Off"
    defines { "CONFIG_DEBUG", "YK_USE_DYNAMIC_LIB" }

  filter { "configurations:Release" }
    symbols "Off"
    optimize "Full"
    defines { "CONFIG_RELEASE" }

  filter { "configurations:ReleaseDLL" }
    symbols "Off"
    optimize "Full"
    defines { "CONFIG_RELEASE", "YK_USE_DYNAMIC_LIB" }

  filter { "configurations:Final" }
    symbols "Off"
    optimize "Full"
    defines { "CONFIG_FINAL" }

  filter { "configurations:FinalDLL" }
    symbols "Off"
    optimize "Full"
    defines { "CONFIG_FINAL", "YK_USE_DYNAMIC_LIB" }

group "Deps"
  include "Deps/GLFW"
  include "Deps/GLAD"
  include "Deps/YKLib/YKLib"
  
  project "NetCommon"
    location "NetCommon"
    language "C++"
    cppdialect "C++latest"
    staticruntime "On"
    kind "StaticLib"
    
    targetdir ("Bin/" .. outputdir .. "/%{prj.name}")
    objdir ("Bin-Int/" .. outputdir .. "/%{prj.name}")
    
    files
    {
      "%{prj.name}/Source/**.cpp",
      "%{prj.name}/Source/**.h"
    }
    
    includedirs
    {
      "%{prj.name}/Source",
      "%{IncludeDir.asio}",
      "%{IncludeDir.YKLib}"
    }
    
    defines
    {
      "ASIO_STANDALONE"
    }

    links
    {
      "YKLib"
    }

    filter { "platforms:Win32 or Win64" }
      defines
      {
        "WIN32_LEAN_AND_MEAN",
        "_WIN32_WINNT=0x0601"
      }
      links
      {
        "Ws2_32.lib"
      }
    
    filter { "configurations:Debug" }
      symbols "On"
      optimize "Off"
      defines
      {
        "CONFIG_DEBUG",
        "YK_ENABLE_DEBUG_LOG"
      }

    filter { "configurations:DebugDLL" }
      symbols "On"
      optimize "Off"
      defines
      {
        "CONFIG_DEBUG",
        "YK_ENABLE_DEBUG_LOG"
      }
    
    filter { "configurations:Release" }
      symbols "Off"
      optimize "Full"
      defines
      {
        "CONFIG_RELEASE",
        "YK_ENABLE_DEBUG_LOG"
      }
    
    filter { "configurations:Final" }
      symbols "Off"
      optimize "Full"
      defines
      {
        "CONFIG_FINAL"
      }

  project "CoreCommon"
    location "CoreCommon"
    language "C++"
    cppdialect "C++latest"
    staticruntime "On"
    kind "StaticLib"
    
    targetdir ("Bin/" .. outputdir .. "/%{prj.name}")
    objdir ("Bin-Int/" .. outputdir .. "/%{prj.name}")
    
    files
    {
      "%{prj.name}/Source/**.cpp",
      "%{prj.name}/Source/**.h"
    }
    
    includedirs
    {
      "%{prj.name}/Source"
    }

    filter { "platforms:Windows" }
    defines
    {
      "WIN32_LEAN_AND_MEAN"
    }
    
    filter { "configurations:Debug" }
    symbols "On"
    optimize "Off"
    defines
    {
      "CONFIG_DEBUG"
    }
    
    filter { "configurations:Release" }
    symbols "Off"
    optimize "Full"
    defines
    {
      "CONFIG_RELEASE"
    }
    
    filter { "configurations:Final" }
    symbols "Off"
    optimize "Full"
    defines
    {
      "CONFIG_FINAL"
    }
group ""
  
project "ParentClient"
  location "ParentClient"
  language "C++"
  cppdialect "C++latest"
  staticruntime "On"
  
  targetdir ("Bin/" .. outputdir .. "/%{prj.name}")
  objdir ("Bin-Int/" .. outputdir .. "/%{prj.name}")
  
  files
  {
    "%{prj.name}/Source/**.cpp",
    "%{prj.name}/Source/**.h"
  }
  
  includedirs
  {
    "%{prj.name}/Source",
    "%{IncludeDir.stb}",
    "%{IncludeDir.GLFW}",
    "%{IncludeDir.GLAD}",
    "%{IncludeDir.asio}",
    "%{IncludeDir.libjpeg_turbo}",
    "%{IncludeDir.NetCommon}",
    "%{IncludeDir.CoreCommon}",
    "%{IncludeDir.YKLib}"
  }

  defines
  {
    "GLFW_INCLUDE_NONE",
    "TJ_STATIC"
  }

  links
  {
    "GLFW",
    "GLAD",
    "turbojpeg-static.lib",
    "NetCommon",
    "CoreCommon",
    "YKLib"
  }

  filter { "platforms:Win32", "configurations:Debug or DebugDLL" }
    libdirs
    {
      "%{LibDir.libjpeg_turbo_debug32}"
    }

  filter { "platforms:Win32", "configurations:Release or ReleaseDLL or Final or FinalDLL" }
    libdirs
    {
      "%{LibDir.libjpeg_turbo_release32}"
    }

  filter { "platforms:Win64", "configurations:Debug or DebugDLL" }
    libdirs
    {
      "%{LibDir.libjpeg_turbo_debug64}"
    }

  filter { "platforms:Win64", "configurations:Release or ReleaseDLL or Final or FinalDLL" }
    libdirs
    {
      "%{LibDir.libjpeg_turbo_release64}"
    }

  filter { "platforms:Win32 or Win64" }
    defines
    {
      "WIN32_LEAN_AND_MEAN"
    }

  filter { "configurations:Debug" }
    symbols "On"
    optimize "Off"
    kind "ConsoleApp"
    defines
    {
      "CONFIG_DEBUG",
      "YK_ENABLE_DEBUG_LOG"
    }

  filter { "configurations:DebugDLL" }
    symbols "On"
    optimize "Off"
    kind "ConsoleApp"
    defines
    {
      "CONFIG_DEBUG",
      "YK_ENABLE_DEBUG_LOG"
    }

  filter { "configurations:Release" }
    symbols "Off"
    optimize "Full"
    kind "ConsoleApp"
    defines
    {
      "CONFIG_RELEASE",
      "YK_ENABLE_DEBUG_LOG"
    }

   filter { "configurations:ReleaseDLL" }
    symbols "Off"
    optimize "Full"
    kind "ConsoleApp"
    defines
    {
      "CONFIG_RELEASE",
      "YK_ENABLE_DEBUG_LOG"
    }

  filter { "configurations:Final" }
    symbols "Off"
    optimize "Full"
    kind "WindowedApp"
    entrypoint "mainCRTStartup"
    defines
    {
      "CONFIG_FINAL"
    }

  filter { "configurations:FinalDLL" }
    symbols "Off"
    optimize "Full"
    kind "WindowedApp"
    entrypoint "mainCRTStartup"
    defines
    {
      "CONFIG_FINAL"
    }

project "ChildClient"
  location "ChildClient"
  language "C++"
  cppdialect "C++latest"
  staticruntime "On"
  
  targetdir ("Bin/" .. outputdir .. "/%{prj.name}")
  objdir ("Bin-Int/" .. outputdir .. "/%{prj.name}")
  
  files
  {
    "%{prj.name}/Source/**.cpp",
    "%{prj.name}/Source/**.h"
  }
  
  includedirs
  {
    "%{prj.name}/Source",
    "%{IncludeDir.stb}",
    "%{IncludeDir.asio}",
    "%{IncludeDir.libjpeg_turbo}",
    "%{IncludeDir.NetCommon}",
    "%{IncludeDir.CoreCommon}",
    "%{IncludeDir.YKLib}"
  }

  defines
  {
    "TJ_STATIC"
  }

  links
  {
    "turbojpeg-static.lib",
    "NetCommon",
    "CoreCommon",
    "YKLib"
  }

  filter { "platforms:Win32", "configurations:Debug or DebugDLL" }
    libdirs
    {
      "%{LibDir.libjpeg_turbo_debug32}"
    }

  filter { "platforms:Win32", "configurations:Release or ReleaseDLL or Final or FinalDLL" }
    libdirs
    {
      "%{LibDir.libjpeg_turbo_release32}"
    }

  filter { "platforms:Win64", "configurations:Debug or DebugDLL" }
    libdirs
    {
      "%{LibDir.libjpeg_turbo_debug64}"
    }

  filter { "platforms:Win64", "configurations:Release or ReleaseDLL or Final or FinalDLL" }
    libdirs
    {
      "%{LibDir.libjpeg_turbo_release64}"
    }

  filter { "platforms:Win32 or Win64" }
    defines
    {
      "WIN32_LEAN_AND_MEAN"
    }
    links
    {
      "dxgi.lib",
      "d3d11.lib"
    }

  filter { "configurations:Debug" }
    symbols "On"
    optimize "Off"
    kind "ConsoleApp"
    defines
    {
      "CONFIG_DEBUG",
      "YK_ENABLE_DEBUG_LOG"
    }

  filter { "configurations:DebugDLL" }
    symbols "On"
    optimize "Off"
    kind "ConsoleApp"
    defines
    {
      "CONFIG_DEBUG",
      "YK_ENABLE_DEBUG_LOG"
    }

  filter { "configurations:Release" }
    symbols "Off"
    optimize "Full"
    kind "ConsoleApp"
    defines
    {
      "CONFIG_RELEASE",
      "YK_ENABLE_DEBUG_LOG"
    }

  filter { "configurations:ReleaseDLL" }
    symbols "Off"
    optimize "Full"
    kind "ConsoleApp"
    defines
    {
      "CONFIG_RELEASE",
      "YK_ENABLE_DEBUG_LOG"
    }

  filter { "configurations:Final" }
    symbols "Off"
    optimize "Full"
    kind "WindowedApp"
    entrypoint "mainCRTStartup"
    defines
    {
      "CONFIG_FINAL"
    }

  filter { "configurations:FinalDLL" }
    symbols "Off"
    optimize "Full"
    kind "WindowedApp"
    entrypoint "mainCRTStartup"
    defines
    {
      "CONFIG_FINAL"
    }