---@diagnostic disable: undefined-global

project("Sandbox")
kind("ConsoleApp") -- use WindowedApp to suppress the terminal on Windows
language("C++")
cppdialect("C++23")

targetdir("%{wks.location}/bin/" .. output_dir .. "/%{prj.name}")
objdir("%{wks.location}/build/" .. output_dir .. "/%{prj.name}")

files({ "source/**.h", "source/**.cpp" })

-- This one call handles everything: Nova headers, SDL3 headers,
-- lib paths, and all required link flags on both Windows and Linux.
dependson({ "Nova" })
LinkNova()

filter("configurations:Debug")
defines({ "SANDBOX_DEBUG" })
symbols("on")

filter("configurations:Release")
defines({ "SANDBOX_RELEASE" })
optimize("on")
