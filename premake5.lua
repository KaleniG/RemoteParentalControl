outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["asio"]          = "Deps/asio/asio/include"
IncludeDir["stb"]           = "Deps/stb"
IncludeDir["GLFW"]          = "Deps/GLFW/include"
IncludeDir["GLAD"]          = "Deps/GLAD/include"
IncludeDir["libjpeg_turbo"] = "Deps/libjpeg-turbo/src"

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

  filter { "platforms:Windows" }
    system "Windows"
    systemversion "latest"
    architecture "x64"

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
    "%{IncludeDir.GLAD}",
    "%{IncludeDir.libjpeg_turbo}"
  }

  defines
  {
    "ASIO_STANDALONE",
    "STB_IMAGE_IMPLEMENTATION",
    "GLFW_INCLUDE_NONE",
    "TJ_STATIC"
  }

  links
  {
    "GLFW",
    "GLAD",
    "turbojpeg-static.lib"
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
    "%{IncludeDir.asio}",
    "%{IncludeDir.stb}",
    "%{IncludeDir.libjpeg_turbo}"
  }

  defines
  {
    "ASIO_STANDALONE",
    "TJ_STATIC"
  }

  links
  {
    "turbojpeg-static.lib",
    "obs.lib"
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
  