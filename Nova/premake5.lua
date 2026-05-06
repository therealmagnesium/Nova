---@diagnostic disable: undefined-global
project("Nova")
kind("StaticLib")
language("C++")
cppdialect("C++23")
staticruntime("on")

targetdir("%{wks.location}/bin/" .. output_dir .. "/%{prj.name}")
objdir("%{wks.location}/build/" .. output_dir .. "/%{prj.name}")

files({
    "source/**.h",
    "source/**.cpp",
})

includedirs({
    "source",
    "%{wks.location}/vendor/SDL3/include",
})

filter("system:windows")
defines({ "PLATFORM_WINDOWS" })

filter("system:linux")
defines({ "PLATFORM_LINUX" })

filter("configurations:Debug")
defines({ "DEBUG" })
symbols("on")

filter("configurations:Release")
defines({ "RELEASE" })
optimize("on")
