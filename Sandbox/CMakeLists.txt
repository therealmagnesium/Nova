add_executable("SandboxApp" "source/Main.cpp")

add_compile_definitions("GLFW_INCLUDE_NONE")

target_include_directories("Nova" PUBLIC "${CMAKE_SOURCE_DIR}/Nova/source")
target_include_directories("Nova" PUBLIC "${CMAKE_SOURCE_DIR}/vendor/glm/glm")

target_link_directories("SandboxApp" PRIVATE "${CMAKE_SOURCE_DIR}/build/Nova")
target_link_directories("SandboxApp" PRIVATE "${CMAKE_SOURCE_DIR}/build/vendor/glad")
target_link_libraries("SandboxApp" PRIVATE "Nova")
target_link_libraries("SandboxApp" PRIVATE "glad")
