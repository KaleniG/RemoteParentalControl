outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["asio"]   = "Deps/asio/asio/include"
IncludeDir["stb"]    = "Deps/stb"
IncludeDir["GLFW"]   = "Deps/GLFW/include"
IncludeDir["GLAD"]   = "Deps/GLAD/include"

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

  filter { "platforms:Windows" }
    system "Windows"
    systemversion "latest"
    architecture "x86"

group "Deps"
  include "Deps/GLFW"
  include "Deps/GLAD"
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
    "%{IncludeDir.asio}",
    "%{IncludeDir.stb}",
    "%{IncludeDir.GLFW}",
    "%{IncludeDir.GLAD}"
  }

  defines
  {
    "ASIO_STANDALONE",
    "STB_IMAGE_IMPLEMENTATION",
    "GLFW_INCLUDE_NONE"
  }

  links
  {
    "GLFW",
    "GLAD"
  }

  filter { "platforms:Windows" }
    defines
    {
      "WIN32_LEAN_AND_MEAN"
    }

    links
    {
      "Ws2_32.lib",
      "iphlpapi.lib"
    }

  filter { "configurations:Debug" }
    symbols "On"
    optimize "Off"
    kind "ConsoleApp"
    defines
    {
      "CONFIG_DEBUG"
    }

  filter { "configurations:Release" }
    symbols "Off"
    optimize "Full"
    kind "ConsoleApp"
    defines
    {
      "CONFIG_RELEASE"
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
    "%{IncludeDir.asio}",
    "%{IncludeDir.stb}"
  }

  defines
  {
    "ASIO_STANDALONE"
  }

  filter { "platforms:Windows" }
    defines
    {
      "WIN32_LEAN_AND_MEAN"
    }

    links
    {
      "Ws2_32.lib",
      "dxgi.lib",
      "d3d11.lib",
      "mfplat.lib",
      "mf.lib",
      "mfreadwrite.lib",
      "mfuuid.lib"
    }

  filter { "configurations:Debug" }
    symbols "On"
    optimize "Off"
    kind "ConsoleApp"
    defines
    {
      "CONFIG_DEBUG"
    }

  filter { "configurations:Release" }
    symbols "Off"
    optimize "Full"
    kind "ConsoleApp"
    defines
    {
      "CONFIG_RELEASE"
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
  