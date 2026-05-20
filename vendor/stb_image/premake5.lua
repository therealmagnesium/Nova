---@diagnostic disable: undefined-global
project("stb_image")
kind("StaticLib")
language("C")
cdialect("C23")
systemversion("latest")
staticruntime("off")
pic("on")

targetdir("bin/")
objdir("build/")

files({
	"include/stb_image.h",
	"source/stb_image.c",
})

includedirs({ "include" })

filter("configurations:Debug")
runtime("Debug")
symbols("on")

filter("configurations:Release")
runtime("Release")
optimize("on")
