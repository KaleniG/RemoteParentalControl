OUTPUTDIR = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

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

  filter { "platforms:Windows" }
    system "Windows"
    systemversion "latest"
    architecture "x86"

project "ParentClient"
  location "ParentClient"
  language "C++"
  cppdialect "C++latest"
  staticruntime "On"
  
  targetdir ("Bin/" .. OUTPUTDIR .. "/%{prj.name}")
  objdir ("Bin-Int/" .. OUTPUTDIR .. "/%{prj.name}")
  
  files
  {
    "%{prj.name}/Source/**.cpp",
    "%{prj.name}/Source/**.h"
  }
  
  includedirs
  {
    "%{prj.name}/Source"
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
  
  targetdir ("Bin/" .. OUTPUTDIR .. "/%{prj.name}")
  objdir ("Bin-Int/" .. OUTPUTDIR .. "/%{prj.name}")
  
  files
  {
    "%{prj.name}/Source/**.cpp",
    "%{prj.name}/Source/**.h"
  }
  
  includedirs
  {
    "%{prj.name}/Source"
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
  