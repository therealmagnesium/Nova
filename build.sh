#!/usr/bin/env bash

clean() {
    echo "Cleaning..."
    rm -r bin/ build/
    rm Makefile Nova/Makefile Sandbox/Makefile
}

copy-assets() {
    compile-shaders
    echo "Copying assets..."
    cp -r "Sandbox/Assets/" "bin/${config_type^}-${platform}/Sandbox/"
    cp -r "Editor/Assets/" "bin/${config_type^}-${platform}/NovaEditor/"
    cp -r "bin/SPIR-V/" "bin/${config_type^}-${platform}/Sandbox/"
    cp -r "bin/SPIR-V/" "bin/${config_type^}-${platform}/NovaEditor/"
}

run-sandbox() {
    local app_name="Sandbox"
    local path="bin/${config_type^}-${platform}/Sandbox/"
    cd "${path}"
    ./"${app_name}"
}

run-editor() {
    local app_name="NovaEditor"
    local path="bin/${config_type^}-${platform}/NovaEditor/"
    cd "${path}"
    ./"${app_name}"
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

setup-config() {
    clear

    ask-platform
    ask-config-type
    ask-premake-config
}

compile-shaders() {
    echo "Compiling shaders..."

    local spv_bin="bin/SPIR-V"
    if [[ ! -d "${spv_bin}" ]]; then
        echo "Creating directory ${spv_bin}..."
        mkdir "${spv_bin}"
    fi

    glslc -fshader-stage="vertex" "Nova/source/Shaders/PBR_vs.glsl" -o "bin/SPIR-V/PBR_vs.spv"
    glslc -fshader-stage="vertex" "Nova/source/Shaders/SkinnedPBR_vs.glsl" -o "bin/SPIR-V/SkinnedPBR_vs.spv"
    glslc -fshader-stage="fragment" "Nova/source/Shaders/PBR_fs.glsl" -o "bin/SPIR-V/PBR_fs.spv"

    glslc -fshader-stage="vertex" "Nova/source/Shaders/Compositing_vs.glsl" -o "bin/SPIR-V/Compositing_vs.spv"
    glslc -fshader-stage="fragment" "Nova/source/Shaders/Compositing_fs.glsl" -o "bin/SPIR-V/Compositing_fs.spv"

    glslc -fshader-stage="vertex" "Nova/source/Shaders/Cubemap_vs.glsl" -o "bin/SPIR-V/Cubemap_vs.spv"
    glslc -fshader-stage="fragment" "Nova/source/Shaders/EquirectangularToCubemap_fs.glsl" -o "bin/SPIR-V/EquirectangularToCubemap_fs.spv"
    glslc -fshader-stage="fragment" "Nova/source/Shaders/Irradiance_fs.glsl" -o "bin/SPIR-V/Irradiance_fs.spv"
    glslc -fshader-stage="fragment" "Nova/source/Shaders/Prefilter_fs.glsl" -o "bin/SPIR-V/Prefilter_fs.spv"

    glslc -fshader-stage="vertex" "Nova/source/Shaders/BRDF_vs.glsl" -o "bin/SPIR-V/BRDF_vs.spv"
    glslc -fshader-stage="fragment" "Nova/source/Shaders/BRDF_fs.glsl" -o "bin/SPIR-V/BRDF_fs.spv"

    glslc -fshader-stage="vertex" "Nova/source/Shaders/Skybox_vs.glsl" -o "bin/SPIR-V/Skybox_vs.spv"
    glslc -fshader-stage="fragment" "Nova/source/Shaders/Skybox_fs.glsl" -o "bin/SPIR-V/Skybox_fs.spv"
}

build-cmake-dependencies() {
    local cores=$(($(nproc) / 2))

    local build_dir_sdl="vendor/SDL3/build"
    if [[ ! -d "${build_dir_sdl}" ]]; then
        echo "Building SDL3..."
        cmake -G "Unix Makefiles" -S vendor/SDL3 -B $build_dir_sdl \
            -DCMAKE_BUILD_TYPE=Release \
            -DSDL_SHARED=OFF \
            -DSDL_STATIC=ON \
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        #cmake --build $build_dir_sdl --config Release
        cd $build_dir_sdl
        make -j$cores
        cd ../../..
    fi

    local build_dir_assimp="vendor/assimp/build"
    if [[ ! -d "${build_dir_assimp}" ]]; then
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

    local build_dir_yaml="vendor/yaml-cpp/build"
    if [[ ! -d "${build_dir_yaml}" ]]; then
        echo "Building yaml-cpp..."
        cmake -G "Unix Makefiles" -DYAML_BUILD_SHARED_LIBS=OFF -S vendor/yaml-cpp -B $build_dir_yaml
        cd $build_dir_yaml
        make -j$cores
        cd ../../..
    fi
}

build-nova() {
    echo "Building Nova-${config_type^}-${platform^}..."
    premake5 export-compile-commands
    cp compile_commands/debug_x64.json ./compile_commands.json

    premake5 $premake_config

    if [[ $premake_config != "gmake" ]]; then
        return 0
    fi

    local cores=$(($(nproc) / 2))
    local make_config=$config_type
    make_config+="_x64"

    echo "Start building with $cores cores..."
    make all -s -j$cores config=$make_config

    copy-assets
    echo "Finished building Nova, Nova Editor, and the testbed application Sandbox..."

    local app_to_run=0
    echo "1) Nova Editor"
    echo "2) Sandbox"
    read -p "Please select which app you would like to run (Ctrl+C to quit): " app_to_run

    case "${app_to_run}" in
    1)
        run-editor
        ;;
    2)
        run-sandbox
        ;;
    *)
        ;;
    esac
}

case "$1" in
"clean")
    clean
    exit
    ;;
"copy-assets")
    config_type="$2"
    platform="$3"
    copy-assets
    exit
    ;;
"run-sandbox")
    ask-platform
    ask-config-type
    run-sandbox
    exit
    ;;
"run-editor")
    ask-platform
    ask-config-type
    run-editor
    exit
    ;;
*)
    ;;
esac

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
