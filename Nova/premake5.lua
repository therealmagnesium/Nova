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
    "%{wks.location}/vendor/imgui/include",
    "%{wks.location}/vendor/assimp/include",
    "%{wks.location}/vendor/assimp/build/include", -- assimp/config.h
    "%{wks.location}/vendor/yaml-cpp/include",
    "%{wks.location}/vendor/nfd/src/include",
})

libdirs({
    "%{wks.location}/vendor/SDL3/build",
    "%{wks.location}/vendor/yaml-cpp/build",
    "%{wks.location}/vendor/stb_image/bin",
    "%{wks.location}/vendor/imgui/bin",
    "%{wks.location}/vendor/nfd/build/src",
    "%{wks.location}/vendor/assimp/build/lib",
})

linkoptions({ "-Wl,--start-group" })
links({
    "SDL3",
    "nfd",
    "stb_image",
    "assimp",
    "imgui",
    "yaml-cpp",
    "z",
})
linkoptions({ "-Wl,--end-group" })

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
    "freetype",
})

-- Pass the correct include paths to the compiler
buildoptions({ "`pkg-config --cflags gtk+-3.0 wayland-client`" })

-- Pass the correct library flags (-lgtk-3, -lwayland-client, etc.) to the linker
linkoptions({ "`pkg-config --libs gtk+-3.0 wayland-client`" })

filter("configurations:Debug")
defines({ "DEBUG" })
symbols("on")

filter("configurations:Release")
defines({ "RELEASE" })
optimize("on")

filter("action:export-compile-commands")
buildoptions({ "-std=c++23" })
filter({})

filter({})
