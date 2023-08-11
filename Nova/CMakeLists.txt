file(GLOB_RECURSE SRC_FILES "source/Nova/*.cpp")
add_library("Nova" "${SRC_FILES}")

target_include_directories("Nova" PUBLIC "${CMAKE_SOURCE_DIR}/vendor/glfw/include")
target_include_directories("Nova" PUBLIC "${CMAKE_SOURCE_DIR}/vendor/glad/include")
target_include_directories("Nova" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/source/Nova")

target_link_directories("Nova" PUBLIC "${CMAKE_SOURCE_DIR}/build/vendor/glfw/src")
target_link_directories("Nova" PUBLIC "${CMAKE_SOURCE_DIR}/build/vendor/glad")
target_link_libraries("Nova" "glfw3")
target_link_libraries("Nova" "glad")
