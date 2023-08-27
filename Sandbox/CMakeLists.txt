add_executable("SandboxApp" "source/Main.cpp")

add_compile_definitions("GLFW_INCLUDE_NONE")

target_include_directories("Nova" PUBLIC "${CMAKE_SOURCE_DIR}/Nova/source")
target_link_libraries("SandboxApp" PRIVATE "Nova")
