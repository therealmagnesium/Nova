#include "Graphics/Texture.h"
#include "Graphics/Renderer.h"
#include "Core/Application.h"
#include "Core/Log.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_filesystem.h>
#include <stb_image.h>
#include <algorithm>

namespace Nova::Textures
{
    struct CachedTexture
    {
        std::filesystem::path path = "";
        Texture metadata;
        TextureHandle handle = NULL;
        TextureSampler sampler = TextureSampler::LinearClamp;
    };

    static SDL_GPUSampler* samplers[6];
    static u32 next_id = 1;
    static std::unordered_map<u32, CachedTexture> cache;
    static std::unordered_map<std::filesystem::path, u32> path_to_id;
    static const std::filesystem::path path_empty;

    SDL_GPUTextureFormat TextureFormatToSDL(TextureFormat format);
    u32 BytesPerPixel(TextureFormat format);
    u16 CalculateMipLevels(u16 width, u16 height);
    Texture RegisterTexture(CachedTexture&& entry);
    void UploadTexture(const Texture& metadata, const void* image_data, SDL_GPUTexture* handle);

    void SetupSamplers()
    {
        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);
        u8 index;

        SDL_GPUSamplerCreateInfo linear_repeat_info = {};
        linear_repeat_info.min_filter = SDL_GPU_FILTER_LINEAR;
        linear_repeat_info.mag_filter = SDL_GPU_FILTER_LINEAR;
        linear_repeat_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        linear_repeat_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        linear_repeat_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        linear_repeat_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        linear_repeat_info.mip_lod_bias = 0.f;
        linear_repeat_info.max_anisotropy = 1.f;
        linear_repeat_info.compare_op = SDL_GPU_COMPAREOP_NEVER;
        linear_repeat_info.min_lod = 0.f;
        linear_repeat_info.max_lod = 16.f;
        linear_repeat_info.enable_anisotropy = false;
        linear_repeat_info.enable_compare = false;

        index = static_cast<u8>(TextureSampler::LinearRepeat);
        samplers[index] = SDL_CreateGPUSampler(device, &linear_repeat_info);

        SDL_GPUSamplerCreateInfo linear_clamp_info = {};
        linear_clamp_info.min_filter = SDL_GPU_FILTER_LINEAR;
        linear_clamp_info.mag_filter = SDL_GPU_FILTER_LINEAR;
        linear_clamp_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        linear_clamp_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        linear_clamp_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        linear_clamp_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        linear_clamp_info.mip_lod_bias = 0.f;
        linear_clamp_info.max_anisotropy = 1.f;
        linear_clamp_info.compare_op = SDL_GPU_COMPAREOP_NEVER;
        linear_clamp_info.min_lod = 0.f;
        linear_clamp_info.max_lod = 16.f;
        linear_clamp_info.enable_anisotropy = false;
        linear_clamp_info.enable_compare = false;

        index = static_cast<u8>(TextureSampler::LinearClamp);
        samplers[index] = SDL_CreateGPUSampler(device, &linear_clamp_info);

        SDL_GPUSamplerCreateInfo point_repeat_info = {};
        point_repeat_info.min_filter = SDL_GPU_FILTER_NEAREST;
        point_repeat_info.mag_filter = SDL_GPU_FILTER_NEAREST;
        point_repeat_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        point_repeat_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        point_repeat_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        point_repeat_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;

        index = static_cast<u8>(TextureSampler::PointRepeat);
        samplers[index] = SDL_CreateGPUSampler(device, &point_repeat_info);

        SDL_GPUSamplerCreateInfo point_clamp_info = {};
        point_clamp_info.min_filter = SDL_GPU_FILTER_NEAREST;
        point_clamp_info.mag_filter = SDL_GPU_FILTER_NEAREST;
        point_clamp_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        point_clamp_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        point_clamp_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        point_clamp_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

        index = static_cast<u8>(TextureSampler::PointClamp);
        samplers[index] = SDL_CreateGPUSampler(device, &point_clamp_info);

        SDL_GPUSamplerCreateInfo anisotropic_repeat_info = {};
        anisotropic_repeat_info.min_filter = SDL_GPU_FILTER_LINEAR;
        anisotropic_repeat_info.mag_filter = SDL_GPU_FILTER_LINEAR;
        anisotropic_repeat_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        anisotropic_repeat_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        anisotropic_repeat_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        anisotropic_repeat_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        anisotropic_repeat_info.enable_anisotropy = true;
        anisotropic_repeat_info.max_anisotropy = 16.0f; // 2, 4, 8, or 16x (higher = better quality, more cost)
        anisotropic_repeat_info.min_lod = 0.0f;
        anisotropic_repeat_info.max_lod = 16.f;

        index = static_cast<u8>(TextureSampler::AnisotropicRepeat);
        samplers[index] = SDL_CreateGPUSampler(device, &anisotropic_repeat_info);

        SDL_GPUSamplerCreateInfo anisotropic_clamp_info = {};
        anisotropic_clamp_info.min_filter = SDL_GPU_FILTER_LINEAR;
        anisotropic_clamp_info.mag_filter = SDL_GPU_FILTER_LINEAR;
        anisotropic_clamp_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        anisotropic_clamp_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        anisotropic_clamp_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        anisotropic_clamp_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        anisotropic_clamp_info.enable_anisotropy = true;
        anisotropic_clamp_info.max_anisotropy = 16.0f; // 2, 4, 8, or 16x (higher = better quality, more cost)
        anisotropic_clamp_info.min_lod = 0.0f;
        anisotropic_clamp_info.max_lod = 16.f;

        index = static_cast<u8>(TextureSampler::AnisotropicClamp);
        samplers[index] = SDL_CreateGPUSampler(device, &anisotropic_clamp_info);

        for (u8 i = 0; i < LEN(samplers); i++)
        {
            if (samplers[i] != NULL)
                INFO("Texture sampler at index %d was created successfully", i);
            else
                ERROR("Textures::CreateSamplers - Failed to initialize sampler at index %d!", i);
        }
    }

    void FreeSamplers()
    {
        const Window& window = Application::GetWindow();

        for (u8 i = 0; i < LEN(samplers); i++)
            if (samplers[i] != NULL)
                SDL_ReleaseGPUSampler(static_cast<SDL_GPUDevice*>(window.gpu_device), samplers[i]);
    }

    Texture LoadDefaultWhite()
    {
        CachedTexture entry;
        entry.metadata.width = 1;
        entry.metadata.height = 1;
        entry.metadata.mip_levels = 1;
        entry.metadata.channel_count = 4;
        entry.metadata.format = TextureFormat::RGBA8;
        entry.path = "__default_white__";

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);

        SDL_GPUTextureCreateInfo info = {};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = TextureFormatToSDL(entry.metadata.format);
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = entry.metadata.width;
        info.height = entry.metadata.height;
        info.layer_count_or_depth = 1;
        info.num_levels = entry.metadata.mip_levels;

        entry.handle = SDL_CreateGPUTexture(device, &info);
        if (entry.handle == NULL)
        {
            ERROR("Textures::LoadDefaultWhite - %s", "Failed to create GPU texture!");
            return Stub_Texture;
        }

        const u8 pixel_data[4] = {0xFF, 0xFF, 0xFF, 0xFF};
        UploadTexture(entry.metadata, pixel_data, static_cast<SDL_GPUTexture*>(entry.handle));

        return RegisterTexture(std::move(entry));
    }

    Texture LoadDefaultNormal()
    {
        CachedTexture entry;
        entry.metadata.width = 1;
        entry.metadata.height = 1;
        entry.metadata.mip_levels = 1;
        entry.metadata.channel_count = 4;
        entry.metadata.format = TextureFormat::RGBA8;
        entry.path = "__default_normal__";

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);

        SDL_GPUTextureCreateInfo info = {};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = TextureFormatToSDL(entry.metadata.format);
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = entry.metadata.width;
        info.height = entry.metadata.height;
        info.layer_count_or_depth = 1;
        info.num_levels = entry.metadata.mip_levels;

        entry.handle = SDL_CreateGPUTexture(device, &info);
        if (entry.handle == NULL)
        {
            ERROR("Textures::LoadDefaultNormal - %s", "Failed to create GPU texture!");
            return Stub_Texture;
        }

        const u8 pixel_data[4] = {128, 128, 255, 255};
        UploadTexture(entry.metadata, pixel_data, static_cast<SDL_GPUTexture*>(entry.handle));

        return RegisterTexture(std::move(entry));
    }

    Texture LoadFromMemory(const u8* data, u32 buffer_size, TextureFormat format, TextureSampler sampler, const std::filesystem::path& path)
    {
        if (!path.empty())
        {
            auto it = path_to_id.find(path);
            if (it != path_to_id.end())
                return cache[it->second].metadata;
        }

        stbi_set_flip_vertically_on_load(true);
        s32 width, height, channels = 0;
        u8* image_data = stbi_load_from_memory(data, (s32)buffer_size, &width, &height, &channels, STBI_rgb_alpha);
        if (image_data == NULL)
        {
            WARN("Textures::LoadFromMemory - Failed to load texture %s from memory!", path.c_str());
            return Stub_Texture;
        }

        CachedTexture entry;
        entry.metadata.width = (u16)width;
        entry.metadata.height = (u16)height;
        entry.metadata.channel_count = (u8)channels;
        entry.metadata.mip_levels = CalculateMipLevels(width, height);
        entry.metadata.format = format;
        entry.sampler = sampler;

        if (!path.empty())
            entry.path = path;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);

        SDL_GPUTextureCreateInfo info = {};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = TextureFormatToSDL(entry.metadata.format);
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        info.width = (u16)width;
        info.height = (u16)height;
        info.layer_count_or_depth = 1;
        info.num_levels = entry.metadata.mip_levels;

        entry.handle = SDL_CreateGPUTexture(device, &info);
        if (entry.handle == NULL)
        {
            ERROR("Textures::LoadFromMemory - %s", "Failed to create gpu texture!");
            stbi_image_free(image_data);
            return Stub_Texture;
        }

        UploadTexture(entry.metadata, image_data, static_cast<SDL_GPUTexture*>(entry.handle));
        stbi_image_free(image_data);

        return RegisterTexture(std::move(entry));
    }

    Texture Load(const std::filesystem::path& path, TextureFormat format, TextureSampler sampler)
    {
        auto it = path_to_id.find(path);
        if (it != path_to_id.end())
            return cache[it->second].metadata;

        const std::filesystem::path path_base = SDL_GetBasePath();
        const std::filesystem::path path_full = path_base / path;

        s32 width, height, channels = 0;
        u8* image_data = stbi_load(path_full.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (image_data == NULL)
        {
            WARN("Textures::Load - Failed to load texture \"%s\"!", path_full.c_str());
            return Stub_Texture;
        }

        CachedTexture entry;
        entry.metadata.width = (u16)width;
        entry.metadata.height = (u16)height;
        entry.metadata.channel_count = (u8)channels;
        entry.metadata.mip_levels = CalculateMipLevels(width, height);
        entry.metadata.format = format;
        entry.path = path; // store the relative path as the cache key
        entry.sampler = sampler;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);

        SDL_GPUTextureCreateInfo info = {};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = TextureFormatToSDL(entry.metadata.format);
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        info.width = (u16)width;
        info.height = (u16)height;
        info.layer_count_or_depth = 1;
        info.num_levels = entry.metadata.mip_levels;

        entry.handle = SDL_CreateGPUTexture(device, &info);
        if (entry.handle == NULL)
        {
            ERROR("Textures::Load - Failed to create gpu texture for \"%s\"!", path_full.c_str());
            stbi_image_free(image_data);
            return Stub_Texture;
        }

        UploadTexture(entry.metadata, image_data, static_cast<SDL_GPUTexture*>(entry.handle));
        stbi_image_free(image_data);

        return RegisterTexture(std::move(entry));
    }

    Texture LoadHDRI(const std::filesystem::path& path)
    {
        auto it = path_to_id.find(path);
        if (it != path_to_id.end())
            return cache[it->second].metadata;

        const std::filesystem::path path_base = SDL_GetBasePath();
        const std::filesystem::path path_full = path_base / path;

        s32 width, height, channels = 0;
        float* image_data = stbi_loadf(path_full.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (image_data == NULL)
        {
            WARN("Textures::LoadHDRI - Failed to load HDRI \"%s\"!", path_full.c_str());
            return Stub_Texture;
        }

        CachedTexture entry;
        entry.metadata.width = (u16)width;
        entry.metadata.height = (u16)height;
        entry.metadata.channel_count = (u8)channels;
        entry.metadata.mip_levels = 1;
        entry.metadata.format = TextureFormat::RGBA32F;
        entry.sampler = TextureSampler::LinearClamp;
        entry.path = path; // store the relative path as the cache key

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);

        SDL_GPUTextureCreateInfo info = {};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = TextureFormatToSDL(entry.metadata.format);
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        info.width = (u16)width;
        info.height = (u16)height;
        info.layer_count_or_depth = 1;
        info.num_levels = entry.metadata.mip_levels;

        entry.handle = SDL_CreateGPUTexture(device, &info);
        if (entry.handle == NULL)
        {
            ERROR("Textures::LoadHDRI - Failed to create GPU texture for HDRI \"%s\"!", path_full.c_str());
            stbi_image_free(image_data);
            return Stub_Texture;
        }

        UploadTexture(entry.metadata, image_data, static_cast<SDL_GPUTexture*>(entry.handle));
        stbi_image_free(image_data);

        return RegisterTexture(std::move(entry));
    }

    Texture CreateFramebufferAttachmentHDR(u16 framebuffer_width, u16 framebuffer_height)
    {
        CachedTexture entry;
        entry.metadata.width = framebuffer_width;
        entry.metadata.height = framebuffer_height;
        entry.metadata.mip_levels = 1;
        entry.metadata.channel_count = 4;
        entry.metadata.format = TextureFormat::RGBA16F;
        // No path — framebuffer attachments are never deduplicated by path

        SDL_GPUTextureCreateInfo info = {};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = TextureFormatToSDL(entry.metadata.format);
        info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = framebuffer_width;
        info.height = framebuffer_height;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        info.sample_count = SDL_GPU_SAMPLECOUNT_1;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);
        entry.handle = SDL_CreateGPUTexture(device, &info);

        if (entry.handle == NULL)
        {
            ERROR("Textures::CreateFramebufferAttachmentHDR - %s", "Failed to create GPU texture for HDR texture!");
            return Stub_Texture;
        }

        return RegisterTexture(std::move(entry));
    }

    Texture CreateFramebufferAttachmentDepth(u16 framebuffer_width, u16 framebuffer_height)
    {
        CachedTexture entry;
        entry.metadata.width = framebuffer_width;
        entry.metadata.height = framebuffer_height;
        entry.metadata.mip_levels = 1;
        entry.metadata.channel_count = 1;
        entry.metadata.format = TextureFormat::Depth32F;

        SDL_GPUTextureCreateInfo info = {};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = TextureFormatToSDL(entry.metadata.format);
        info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        info.width = framebuffer_width;
        info.height = framebuffer_height;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);

        entry.handle = SDL_CreateGPUTexture(device, &info);
        if (entry.handle == NULL)
        {
            ERROR("Textures::CreateFramebufferAttachmentDepth - %s", "Failed to create GPU texture for depth texture!");
            return Stub_Texture;
        }

        return RegisterTexture(std::move(entry));
    }

    Texture CreateCubemap(u16 width, u16 height, u16 mip_levels, TextureFormat format)
    {
        constexpr u8 face_count = 6;

        CachedTexture entry;
        entry.metadata.width = width;
        entry.metadata.height = height;
        entry.metadata.format = format;
        entry.metadata.mip_levels = mip_levels;
        entry.metadata.channel_count = 4;

        SDL_GPUTextureCreateInfo info = {};
        info.type = SDL_GPU_TEXTURETYPE_CUBE;
        info.format = TextureFormatToSDL(entry.metadata.format);
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        info.width = entry.metadata.width;
        info.height = entry.metadata.height;
        info.layer_count_or_depth = face_count;
        info.num_levels = entry.metadata.mip_levels;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);

        entry.handle = SDL_CreateGPUTexture(device, &info);
        if (entry.handle == NULL)
        {
            ERROR("Textures::CreateCubemap - %s", "Failed to create GPU texture for cubemap texture!");
            return Stub_Texture;
        }

        return RegisterTexture(std::move(entry));
    }

    void Unload(Texture& texture)
    {
        if (texture.id == TEXTURE_ID_NULL)
            return;

        auto it = cache.find(texture.id);
        if (it != cache.end())
        {
            const Window& window = Application::GetWindow();
            SDL_ReleaseGPUTexture(
                static_cast<SDL_GPUDevice*>(window.gpu_device),
                static_cast<SDL_GPUTexture*>(it->second.handle)
            );

            if (!it->second.path.empty())
                path_to_id.erase(it->second.path);

            cache.erase(it);
        }

        texture.id = TEXTURE_ID_NULL;
        texture.width = 0;
        texture.height = 0;
        texture.mip_levels = 0;
        texture.channel_count = 0;
    }

    void Bind(const Texture& texture, u8 slot)
    {
        auto it = cache.find(texture.id);
        if (it == cache.end())
            return;

        SDL_GPUTextureSamplerBinding binding = {};
        binding.texture = static_cast<SDL_GPUTexture*>(it->second.handle);
        binding.sampler = samplers[static_cast<u8>(it->second.sampler)];

        SDL_GPURenderPass* render_pass = static_cast<SDL_GPURenderPass*>(Renderer::GetActiveRenderPass());
        SDL_BindGPUFragmentSamplers(render_pass, slot, &binding, 1);
    }

    TextureHandle GetHandle(const Texture& texture)
    {
        auto it = cache.find(texture.id);
        return (it != cache.end()) ? it->second.handle : NULL;
    }

    const std::filesystem::path& GetPath(const Texture& texture)
    {
        const auto it = cache.find(texture.id);
        return (it != cache.end()) ? it->second.path : path_empty;
    }

    SDL_GPUTextureFormat TextureFormatToSDL(TextureFormat format)
    {
        SDL_GPUTextureFormat format_sdl = (SDL_GPUTextureFormat)0;

        switch (format)
        {
            case TextureFormat::RGBA8:
                format_sdl = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
                break;
            case TextureFormat::RGBA8_SRGB:
                format_sdl = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB;
                break;
            case TextureFormat::RGBA16F:
                format_sdl = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
                break;
            case TextureFormat::RGBA32F:
                format_sdl = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
                break;
            case TextureFormat::RG16F:
                format_sdl = SDL_GPU_TEXTUREFORMAT_R16G16_FLOAT;
                break;
            case TextureFormat::Depth32F:
                format_sdl = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
                break;
            default:
                break;
        }

        return format_sdl;
    }

    u32 BytesPerPixel(TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::RGBA8:
            case TextureFormat::RGBA8_SRGB:
                return 4; // 1 byte × 4 channels
            case TextureFormat::RGBA16F:
                return 8; // 2 bytes × 4 channels
            case TextureFormat::RGBA32F:
                return 16; // 4 bytes × 4 channels
            case TextureFormat::RG16F:
                return 4; // 2 bytes × 2 channels
            default:
                return 4;
        }
    }

    u16 CalculateMipLevels(u16 width, u16 height)
    {
        u16 levels = 1;
        while (width > 1 || height > 1)
        {
            width = std::max(1u, width / 2u);
            height = std::max(1u, height / 2u);
            levels++;
        }

        return levels;
    }

    Texture RegisterTexture(CachedTexture&& entry)
    {
        u32 id = next_id++;
        entry.metadata.id = id;

        if (!entry.path.empty())
            path_to_id[entry.path] = id;

        cache.emplace(id, std::move(entry));
        return cache.at(id).metadata;
    }

    void UploadTexture(const Texture& metadata, const void* image_data, SDL_GPUTexture* handle)
    {
        const Window& window = Application::GetWindow();
        const u32 data_size = metadata.width * metadata.height * BytesPerPixel(metadata.format);
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);

        SDL_GPUTransferBufferCreateInfo transfer_info = {};
        transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transfer_info.size = data_size;
        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_info);

        void* mapped = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
        memcpy(mapped, image_data, data_size);
        SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

        SDL_GPUTextureTransferInfo source = {};
        source.transfer_buffer = transfer_buffer;
        source.offset = 0;
        source.pixels_per_row = metadata.width;
        source.rows_per_layer = metadata.height;

        SDL_GPUTextureRegion dest = {};
        dest.texture = handle;
        dest.mip_level = 0;
        dest.layer = 0;
        dest.x = dest.y = dest.z = 0;
        dest.w = metadata.width;
        dest.h = metadata.height;
        dest.d = 1;

        SDL_UploadToGPUTexture(copy_pass, &source, &dest, false);
        SDL_EndGPUCopyPass(copy_pass);

        if (metadata.mip_levels > 1)
            SDL_GenerateMipmapsForGPUTexture(command_buffer, handle);

        SDL_SubmitGPUCommandBuffer(command_buffer);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    }
}
