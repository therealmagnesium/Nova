add_executable("SandboxApp" "source/Main.cpp")
target_include_directories("Nova" PUBLIC "${CMAKE_SOURCE_DIR}/Nova/source")
target_link_libraries("SandboxApp" PRIVATE "Nova")
