cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

project("NovaEngine" VERSION 1.0)

add_subdirectory("vendor/glfw")
add_subdirectory("vendor/glad")

add_subdirectory("Nova")
add_subdirectory("Sandbox")
