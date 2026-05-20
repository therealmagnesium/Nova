---@diagnostic disable: undefined-global
workspace("Nova")
configurations({ "Debug", "Release" })
platforms({ "x64" })

-- Output directories relative to workspace root
output_dir = "%{cfg.buildcfg}-%{cfg.system}"

group("Dependencies")
include("vendor/stb_image/premake5.lua")
group("")

include("Nova.lua")
include("Nova/premake5.lua")
include("Sandbox/premake5.lua")
