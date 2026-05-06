---@diagnostic disable: undefined-global
-- Nova.lua
-- Games using Nova include this file to get all
-- necessary include paths, lib paths, and link flags.

framework_root = path.getabsolute(".") -- resolves to wherever framework.lua lives

function Test()
    print("What The Fuck")
end

function LinkNova()
    includedirs({
        framework_root .. "/Nova/source",
        framework_root .. "/vendor/SDL3/include",
    })

    links({ "Nova" })

    filter("system:windows")
    libdirs({
        framework_root .. "/bin/%{cfg.buildcfg}-windows/Nova",
        framework_root .. "/vendor/SDL3-build",
    })
    links({
        "SDL3",
        "setupapi",
        "winmm",
        "imm32",
        "version",
        "ole32",
        "oleaut32",
    })

    filter("system:linux")
    libdirs({
        framework_root .. "/bin/%{cfg.buildcfg}-linux/Nova",
        framework_root .. "/vendor/SDL3-build",
    })
    links({
        "SDL3",
        "dl",
        "pthread",
        "m",
    })

    filter({}) -- reset filters
end
