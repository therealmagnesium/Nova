---@diagnostic disable: undefined-global
-- Nova.lua
-- Games using Nova include this file to get all
-- necessary include paths, lib paths, and link flags.

framework_root = path.getabsolute(".") -- resolves to wherever Nova.lua lives

function LinkNova()
    local nova_build_path = framework_root .. "/bin/%{cfg.buildcfg}-%{cfg.system}/Nova"
    local nova_build = ""

    filter("system:windows")
    nova_build = nova_build_path .. "/libNova.dll"

    filter("system:linux")
    nova_build = nova_build_path .. "/libNova.so"

    postbuildcommands({
        "echo Copying Nova.[dll/so] to %{cfg.buildtarget.abspath}...",
        "{COPYFILE} " .. nova_build .. " %{cfg.buildtarget.directory}",
    })

    includedirs({
        framework_root .. "/Nova/source",
        framework_root .. "/vendor/SDL3/include",
        framework_root .. "/vendor/glm",
    })

    links({
        "Nova",
    })

    filter("system:windows")
    filter("system:linux")
    filter({}) -- reset filters
end
