#pragma once
#include "Core/Base.h"
#include <filesystem>

namespace Nova::Graphics
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
        ShaderStorageInfo info;
        void* handle_vertex = NULL;
        void* handle_fragment = NULL;
    };

    inline const ShaderStorageInfo Stub_ShaderStorageInfo;

    namespace Shaders
    {
        Shader Load(const std::filesystem::path& path_vertex, const std::filesystem::path& path_fragment, const ShaderStorageInfo& info = Stub_ShaderStorageInfo);
        void Unload(Shader& shader);
    }
}
