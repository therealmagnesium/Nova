#pragma once
#include "Core/Base.h"
#include <filesystem>

namespace Nova
{
    struct ShaderStorageInfo
    {
        u32 sampler_count = 0;
        u32 uniform_buffer_count = 0;
        u32 storage_buffer_count = 0;
        u32 storage_texture_count = 0;
    };

    struct Shader
    {
        ShaderStorageInfo info_vertex;
        ShaderStorageInfo info_fragment;
        void* handle_vertex = NULL;
        void* handle_fragment = NULL;
    };

    inline const ShaderStorageInfo Stub_ShaderStorageInfo;

    namespace Shaders
    {
        Shader Load(const std::filesystem::path& path_vertex, const std::filesystem::path& path_fragment, const ShaderStorageInfo& info_vertex = Stub_ShaderStorageInfo, const ShaderStorageInfo& info_fragment = Stub_ShaderStorageInfo);
        void Unload(Shader& shader);
    }
}
