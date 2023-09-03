file(GLOB_RECURSE SRC_FILES "source/Nova/*.cpp" "source/Platform/*.cpp")
add_library("Nova" "${SRC_FILES}")

add_compile_definitions("GLFW_INCLUDE_NONE")

target_include_directories("Nova" PUBLIC "${CMAKE_SOURCE_DIR}/vendor/glfw/include")
target_include_directories("Nova" PUBLIC "${CMAKE_SOURCE_DIR}/vendor/glad/include")
target_include_directories("Nova" PUBLIC "${CMAKE_SOURCE_DIR}/vendor/glm/glm")
target_include_directories("Nova" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/source/Nova")
target_include_directories("Nova" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/source/Platform")

target_link_directories("Nova" PUBLIC "${CMAKE_SOURCE_DIR}/build/vendor/glfw/src")
target_link_directories("Nova" PUBLIC "${CMAKE_SOURCE_DIR}/build/vendor/glad")
target_link_libraries("Nova" PRIVATE "glfw3")
target_link_directories("Nova" PRIVATE "glad")
