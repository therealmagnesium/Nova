---@diagnostic disable: undefined-global

project("NovaEditor")
kind("ConsoleApp")
language("C++")
cppdialect("C++23")

targetdir("%{wks.location}/bin/" .. output_dir .. "/%{prj.name}")
objdir("%{wks.location}/build/" .. output_dir .. "/%{prj.name}")

files({ "source/**.h", "source/**.cpp" })
includedirs({ "source" })

-- This one call handles everything: Nova headers, SDL3 headers,
-- lib paths, and all required link flags on both Windows and Linux.
dependson({ "Nova", "imgui" })
LinkNova()

filter("configurations:Debug")
defines({ "SANDBOX_DEBUG" })
symbols("on")

filter("configurations:Release")
defines({ "SANDBOX_RELEASE" })
optimize("on")
