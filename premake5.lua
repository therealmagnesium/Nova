---@diagnostic disable: undefined-global
workspace("Nova")
configurations({ "Debug", "Release" })
platforms({ "x64" })

-- Output directories relative to workspace root
output_dir = "%{cfg.buildcfg}-%{cfg.system}"

include("Nova.lua")
include("Nova/premake5.lua")
include("Sandbox/premake5.lua")
