---@diagnostic disable: undefined-global
project("imgui")
kind("StaticLib")
language("C++")
cppdialect("C++23")
systemversion("latest")
staticruntime("off")
pic("on")

targetdir("bin/")
objdir("build/")

files({
	"include/*.h",
	"source/*.cpp",
})

includedirs({ "include" })

filter("configurations:Debug")
runtime("Debug")
symbols("on")

filter("configurations:Release")
runtime("Release")
optimize("on")
