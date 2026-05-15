---@diagnostic disable: undefined-global
-- Nova.lua
-- Games using Nova include this file to get all
-- necessary include paths, lib paths, and link flags.

framework_root = path.getabsolute(".") -- resolves to wherever framework.lua lives

group("Dependencies")
include("vendor/stb_image/premake5.lua")
group("")

function LinkNova()
	includedirs({
		framework_root .. "/Nova/source",
		framework_root .. "/vendor/SDL3/include",
		framework_root .. "/vendor/glm",
	})

	libdirs({
		framework_root .. "/bin/%{cfg.buildcfg}-windows/Nova",
		framework_root .. "/vendor/SDL3-build",
		framework_root .. "/vendor/stb_image/bin",
	})

	links({
		"Nova",
		"SDL3",
		"stb_image",
	})

	filter("system:windows")
	links({
		"setupapi",
		"winmm",
		"imm32",
		"version",
		"ole32",
		"oleaut32",
	})

	filter("system:linux")
	links({
		"dl",
		"pthread",
		"m",
	})

	filter({}) -- reset filters
end
