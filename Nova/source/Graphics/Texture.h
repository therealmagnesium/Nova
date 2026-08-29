#pragma once
#include "Core/Asset.h"
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
        RGBA32F,
        RG16F,
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

    enum class MSAASamples
    {
        One = 0,
        Two,
        Four,
        Eight,
    };

    struct Image
    {
        u16 width = 0;
        u16 height = 0;
        u8* pixel_data = NULL;
    };

    struct Texture : public Asset
    {
        u32 id = 0;
        u16 width = 0;
        u16 height = 0;
        u16 mip_levels = 0;
        u8 channel_count = 0;
        TextureFormat format = TextureFormat::RGBA8;
        bool has_transparency = false;

        inline bool IsValid() const { return id != 0; }
        inline bool operator==(const Texture& other) const { return id == other.id; }
        inline bool operator!=(const Texture& other) const { return !(*this == other); }
    };

    inline const Texture Stub_Texture;
    inline constexpr u32 TEXTURE_ID_NULL = 0;

    namespace Textures
    {
        void SetupSamplers();
        void FreeSamplers();

        Texture LoadDefaultWhite();
        Texture LoadDefaultNormal();
        Texture LoadFromMemory(const u8* data, u32 buffer_size, TextureFormat format = TextureFormat::RGBA8_SRGB, TextureSampler sampler = TextureSampler::LinearClamp, const std::filesystem::path& path = "");
        Texture Load(const std::filesystem::path& path, TextureFormat format = TextureFormat::RGBA8_SRGB, TextureSampler sampler = TextureSampler::LinearClamp);
        Texture LoadHDRI(const std::filesystem::path& path);
        Texture CreateFramebufferAttachmentHDR(u16 framebuffer_width, u16 framebuffer_height, MSAASamples msaa = MSAASamples::One);
        Texture CreateFramebufferAttachmentDepth(u16 framebuffer_width, u16 framebuffer_height, MSAASamples msaa = MSAASamples::One);
        Texture CreateCubemap(u16 width, u16 height, u16 mip_levels, TextureFormat format = TextureFormat::RGBA16F);
        void Unload(Texture& texture);
        void Bind(const Texture& texture, u8 slot = 0);

        u32 GetDefaultWhiteID();
        TextureHandle GetHandle(const Texture& texture);
        const std::filesystem::path& GetPath(const Texture& texture);
    }
}
