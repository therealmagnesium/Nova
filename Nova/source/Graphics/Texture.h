#pragma once
#include "Core/Base.h"
#include <filesystem>

namespace Nova
{
    using TextureHandle = void*;

    enum class TextureFormat : u8
    {
        RGBA8 = 0,
        RGBA8_SRGB,
        RGBA16F,
        Depth32F
    };

    enum class TextureSampler : u8
    {
        LinearRepeat = 0,
        LinearClamp,
        PointRepeat,
        PointClamp,
        AnisotropicRepeat,
        AnisotropicClamp,
    };

    struct Texture
    {
        TextureHandle handle = NULL;
        u16 width = 0;
        u16 height = 0;
        u16 mip_levels = 0;
        u8 channel_count = 0;
        TextureFormat format = TextureFormat::RGBA8;
    };

    inline const Texture Stub_Texture;

    namespace Textures
    {
        void SetupSamplers();
        void FreeSamplers();

        Texture LoadDefaultWhite();
        Texture Load(const std::filesystem::path& path);
        void Unload(Texture& texture);
        void Bind(const Texture& texture, TextureSampler sampler_index, u8 slot = 0);
    }
}
