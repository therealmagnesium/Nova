#!/usr/bin/env bash

clean() {
    echo "Cleaning..."
    rm -r bin/ build/
    rm Makefile Nova/Makefile Sandbox/Makefile
}

run-sandbox() {
    local app_path="bin/${config_type^}-${platform}/Sandbox/Sandbox"
    ./"${app_path}"
}

setup-config() {
    clear

    read -p "Select desired platform (windows | linux): " platform

    if [[ $platform != "windows" && $platform != "linux" ]]; then
        echo "Incorrect platform entered, exiting safely..."
        return 1
    fi

    read -p "Select configuration type (debug | release): " config_type

    if [[ $config_type != "debug" && $config_type != "release" ]]; then
        echo "Incorrect configuration type entered, exiting safely..."
        return 2
    fi

    read -p "Select project files to generate (gmake | vs2026): " premake_config

    if [[ $premake_config != "gmake" && $premake_config != "vs2026" ]]; then
        echo "Incorrect configuration for premake, exiting safely..."
        return 3
    fi
}

build-cmake-dependencies() {
    local build_dir_sdl="vendor/SDL3/build"
    if [[ ! -d "$build_dir_sdl" ]]; then
        echo "Building SDL3..."
        cmake -G "Unix Makefiles" -S vendor/SDL3 -B $build_dir_sdl \
            -DCMAKE_BUILD_TYPE=Release \
            -DSDL_SHARED=OFF \
            -DSDL_STATIC=ON \
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        #cmake --build $build_dir_sdl --config Release
        cd $build_dir_sdl
        make -j4
        cd ../../..
    fi

    local build_dir_assimp="vendor/assimp/build"
    if [[ ! -d "$build_dir_assimp" ]]; then
        echo "Building Assimp..."
        cmake -G "Ninja" -DASSIMP_BUILD_TESTS=OFF \
            -DASSIMP_INSTALL=OFF \
            -DASSIMP_BUILD_ZLIB=ON \
            -DBUILD_SHARED_LIBS=OFF \
            -DASSIMP_WARNINGS_AS_ERRORS=OFF -S vendor/assimp -B $build_dir_assimp
        cd $build_dir_assimp
        ninja
        cd ../../..
    fi
}

build-nova() {
    echo "Building Nova-${config_type^}-${platform^}..."
    premake5 export-compile-commands
    premake5 $premake_config

    if [[ $premake_config != "gmake" ]]; then
        return 0
    fi

    cp compile_commands/debug_x64.json compile_commands.json

    local make_config=$config_type
    make_config+="_x64"
    make all -s -j4 config=$make_config

    echo "Compiling shaders..."
    glslc -fshader-stage="vertex" Sandbox/Assets/Shaders/Source/Diffuse_vs.glsl -o Sandbox/Assets/Shaders/Compiled/Diffuse_vs.spv
    glslc -fshader-stage="fragment" Sandbox/Assets/Shaders/Source/Diffuse_fs.glsl -o Sandbox/Assets/Shaders/Compiled/Diffuse_fs.spv
    glslc -fshader-stage="vertex" Sandbox/Assets/Shaders/Source/PBR_vs.glsl -o Sandbox/Assets/Shaders/Compiled/PBR_vs.spv
    glslc -fshader-stage="fragment" Sandbox/Assets/Shaders/Source/PBR_fs.glsl -o Sandbox/Assets/Shaders/Compiled/PBR_fs.spv
    glslc -fshader-stage="vertex" Sandbox/Assets/Shaders/Source/Compositing_vs.glsl -o Sandbox/Assets/Shaders/Compiled/Compositing_vs.spv
    glslc -fshader-stage="fragment" Sandbox/Assets/Shaders/Source/Compositing_fs.glsl -o Sandbox/Assets/Shaders/Compiled/Compositing_fs.spv

    cp -r Sandbox/Assets/ bin/${config_type^}-${platform}/Sandbox/

    local should_run="n"
    read -p "Would you like to run the sandbox project? (y | n) > " should_run

    if [[ $should_run == "y" || $should_run == "yes" ]]; then
        run-sandbox
    fi
}

if [[ "$1" == "clean" ]]; then
    clean
    exit
fi

setup-config
if [[ $? != 0 ]]; then
    exit
fi

build-cmake-dependencies
if [[ $? != 0 ]]; then
    exit
fi

build-nova
if [[ $? != 0 ]]; then
    exit
fi
