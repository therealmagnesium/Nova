#!/usr/bin/env bash

clean() {
    echo "Cleaning..."
    rm -r bin/ build/
    rm Makefile Nova/Makefile Sandbox/Makefile
}

copy-assets() {
    compile-shaders
    echo "Copying assets..."
    cp -r Sandbox/Assets/ bin/${config_type^}-${platform}/Sandbox/
}

run-sandbox() {
    local app_path="bin/${config_type^}-${platform}/Sandbox/Sandbox"
    ./"${app_path}"
}

run-example() {
    local name="$1"
    local app_path="bin/${config_type^}-${platform}/Examples/${name}/${name}"
    ./"${app_path}"
}

ask-platform() {
    read -p "Select desired platform (windows | linux): " platform

    if [[ $platform != "windows" && $platform != "linux" ]]; then
        echo "Invalid platform entered, exiting safely..."
        exit
    fi
}

ask-config-type() {
    read -p "Select configuration type (debug | release): " config_type

    if [[ $config_type != "debug" && $config_type != "release" ]]; then
        echo "Invalid configuration type entered, exiting safely..."
        exit
    fi
}

ask-premake-config() {
    read -p "Select project files to generate (gmake | vs2026): " premake_config

    if [[ $premake_config != "gmake" && $premake_config != "vs2026" ]]; then
        echo "Invalid configuration for premake, exiting safely..."
        exit
    fi
}

ask-example() {
    read -p "Enter the example you would like to run (1 | 2 | ...): " example_to_run

    ask-platform
    ask-config-type

    case "$example_to_run" in
    "1")
        run-example "Asset_Management"
        ;;
    "2")
        run-example "ECS_Lifetimes"
        ;;
    "3")
        run-example "PBR"
        ;;
    *)
        echo "Invalid example entered, exiting safely..."
        ;;
    esac
}

setup-config() {
    clear

    ask-platform
    ask-config-type
    ask-premake-config
}

compile-shaders() {
    echo "Compiling shaders..."
    glslc -fshader-stage="vertex" Sandbox/Assets/Shaders/Source/PBR_vs.glsl -o Sandbox/Assets/Shaders/Compiled/PBR_vs.spv
    glslc -fshader-stage="vertex" Sandbox/Assets/Shaders/Source/SkinnedPBR_vs.glsl -o Sandbox/Assets/Shaders/Compiled/SkinnedPBR_vs.spv
    glslc -fshader-stage="fragment" Sandbox/Assets/Shaders/Source/PBR_fs.glsl -o Sandbox/Assets/Shaders/Compiled/PBR_fs.spv

    glslc -fshader-stage="vertex" Sandbox/Assets/Shaders/Source/Compositing_vs.glsl -o Sandbox/Assets/Shaders/Compiled/Compositing_vs.spv
    glslc -fshader-stage="fragment" Sandbox/Assets/Shaders/Source/Compositing_fs.glsl -o Sandbox/Assets/Shaders/Compiled/Compositing_fs.spv

    glslc -fshader-stage="vertex" Sandbox/Assets/Shaders/Source/Cubemap_vs.glsl -o Sandbox/Assets/Shaders/Compiled/Cubemap_vs.spv
    glslc -fshader-stage="fragment" Sandbox/Assets/Shaders/Source/EquirectangularToCubemap_fs.glsl -o Sandbox/Assets/Shaders/Compiled/EquirectangularToCubemap_fs.spv
    glslc -fshader-stage="fragment" Sandbox/Assets/Shaders/Source/Irradiance_fs.glsl -o Sandbox/Assets/Shaders/Compiled/Irradiance_fs.spv
    glslc -fshader-stage="fragment" Sandbox/Assets/Shaders/Source/Prefilter_fs.glsl -o Sandbox/Assets/Shaders/Compiled/Prefilter_fs.spv

    glslc -fshader-stage="vertex" Sandbox/Assets/Shaders/Source/BRDF_vs.glsl -o Sandbox/Assets/Shaders/Compiled/BRDF_vs.spv
    glslc -fshader-stage="fragment" Sandbox/Assets/Shaders/Source/BRDF_fs.glsl -o Sandbox/Assets/Shaders/Compiled/BRDF_fs.spv

    glslc -fshader-stage="vertex" Sandbox/Assets/Shaders/Source/Skybox_vs.glsl -o Sandbox/Assets/Shaders/Compiled/Skybox_vs.spv
    glslc -fshader-stage="fragment" Sandbox/Assets/Shaders/Source/Skybox_fs.glsl -o Sandbox/Assets/Shaders/Compiled/Skybox_fs.spv
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
        make -j$(nproc)
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
    premake5 $premake_config

    if [[ $premake_config != "gmake" ]]; then
        return 0
    fi

    local cores=$(($(nproc) / 2))
    local make_config=$config_type
    make_config+="_x64"

    echo "Start building with $cores cores..."
    bear -- make all -s -j$cores config=$make_config

    copy-assets

    local should_run="n"
    read -p "Would you like to run the sandbox project? (y | n) > " should_run

    if [[ $should_run == "y" || $should_run == "yes" ]]; then
        run-sandbox
    fi
}

display-examples() {
    echo "Nova Examples:"
    echo "1) Asset Management"
    echo "2) ECS Lifetimes"
    echo "3) PBR"
}

if [[ "$1" == "clean" ]]; then
    clean
    exit
elif [[ "$1" == "copy-assets" ]]; then
    config_type="$2"
    platform="$3"
    copy-assets
    exit
elif [[ "$1" == "run-sandbox" ]]; then
    ask-platform
    ask-config-type
    run-sandbox
    exit
elif [[ "$1" == "run-example" ]]; then
    display-examples
    ask-example
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
