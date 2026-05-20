---@diagnostic disable: undefined-global
project("Nova")
kind("SharedLib")
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
    "%{wks.location}/vendor/glm",
    "%{wks.location}/vendor/stb_image/include",
    "%{wks.location}/vendor/assimp/include",
})

libdirs({
    "%{wks.location}/vendor/SDL3/build",
    "%{wks.location}/vendor/stb_image/bin",
    "%{wks.location}/vendor/assimp/build/lib",
})

links({
    "SDL3",
    "stb_image",
    "assimp",
})

filter("system:windows")
defines({ "PLATFORM_WINDOWS" })
links({
    "setupapi",
    "winmm",
    "imm32",
    "version",
    "ole32",
    "oleaut32",
})

filter("system:linux")
defines({ "PLATFORM_LINUX" })
links({
    "dl",
    "pthread",
    "m",
})

filter("configurations:Debug")
defines({ "DEBUG" })
symbols("on")

filter("configurations:Release")
defines({ "RELEASE" })
optimize("on")

filter({})
