outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["asio"]          = "Deps/asio/asio/include"
IncludeDir["stb"]           = "Deps/stb"
IncludeDir["GLFW"]          = "Deps/GLFW/include"
IncludeDir["GLAD"]          = "Deps/GLAD/include"
IncludeDir["libjpeg_turbo"] = "Deps/libjpeg-turbo/src"
IncludeDir["NetCommon"]     = "NetCommon/Source"
IncludeDir["CoreCommon"]    = "CoreCommon/Source"

LibDir = {}
LibDir["libjpeg_turbo_debug"]   = "Deps/libjpeg-turbo/build/Debug"
LibDir["libjpeg_turbo_release"] = "Deps/libjpeg-turbo/build/Release"

workspace "RemoteParentalControl"
  startproject "ParentClient"

  configurations 
  { 
    "Debug",
    "Release",
    "Final"
  }

  platforms 
  { 
    "Windows"
  }

  flags
  {
    "MultiProcessorCompile"
  }

  defines
  {
    "_CRT_SECURE_NO_WARNINGS"
  }

  filter { "platforms:Windows" }
    system "Windows"
    systemversion "latest"
    architecture "x64"

group "Deps"
  include "Deps/GLFW"
  include "Deps/GLAD"
  
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
      "%{IncludeDir.asio}"
    }
    
    defines
    {
      "ASIO_STANDALONE"
    }

    filter { "platforms:Windows" }
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
    "%{IncludeDir.libjpeg_turbo}",
    "%{IncludeDir.NetCommon}",
    "%{IncludeDir.CoreCommon}"
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
    "CoreCommon"
  }

  filter { "platforms:Windows" }
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
      "CONFIG_DEBUG"
    }
    libdirs
    {
      "%{LibDir.libjpeg_turbo_debug}"
    }

  filter { "configurations:Release" }
    symbols "Off"
    optimize "Full"
    kind "ConsoleApp"
    defines
    {
      "CONFIG_RELEASE"
    }
    libdirs
    {
      "%{LibDir.libjpeg_turbo_release}"
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
    libdirs
    {
      "%{LibDir.libjpeg_turbo_release}"
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
    "%{IncludeDir.libjpeg_turbo}",
    "%{IncludeDir.NetCommon}",
    "%{IncludeDir.CoreCommon}"
  }

  defines
  {
    "TJ_STATIC"
  }

  links
  {
    "turbojpeg-static.lib",
    "NetCommon",
    "CoreCommon"
  }

  filter { "platforms:Windows" }
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
      "CONFIG_DEBUG"
    }
    libdirs
    {
      "%{LibDir.libjpeg_turbo_debug}"
    }

  filter { "configurations:Release" }
    symbols "Off"
    optimize "Full"
    kind "ConsoleApp"
    defines
    {
      "CONFIG_RELEASE"
    }
    libdirs
    {
      "%{LibDir.libjpeg_turbo_release}"
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
    libdirs
    {
      "%{LibDir.libjpeg_turbo_release}"
    }
  