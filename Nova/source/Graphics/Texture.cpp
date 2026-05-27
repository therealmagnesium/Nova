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
    static SDL_GPUSampler* samplers[6];

    void UploadTexture(SDL_GPUTexture* texture_handle, const u8* image_data, u16 width, u16 height);
    SDL_GPUTextureFormat TextureFormatToSDL(TextureFormat format);
    u16 CalculateMipLevels(u16 width, u16 height);

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
        linear_repeat_info.max_lod = 1000.f;
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
        linear_clamp_info.max_lod = 1000.f;
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
        anisotropic_repeat_info.max_lod = 1000.f;

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
        anisotropic_clamp_info.max_lod = 1000.f;

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
        Texture texture;
        texture.width = 1;
        texture.height = 1;
        texture.mip_levels = 1;
        texture.channel_count = 4;

        SDL_GPUTextureCreateInfo info = {};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = TextureFormatToSDL(texture.format);
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = texture.width;
        info.height = texture.height;
        info.layer_count_or_depth = 1;
        info.num_levels = texture.mip_levels;

        const u8 image_data[4] = {255, 255, 255, 255};
        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);
        texture.handle = SDL_CreateGPUTexture(device, &info);

        if (texture.handle == NULL)
        {
            ERROR("Textures::LoadDefaultWhite - %s", "Failed to create gpu texture for default white texture!");
            return Stub_Texture;
        }

        UploadTexture(static_cast<SDL_GPUTexture*>(texture.handle), image_data, texture.width, texture.height);
        return texture;
    }

    Texture LoadDepthTexture(u16 framebuffer_width, u16 framebuffer_height)
    {
        Texture texture;
        texture.width = framebuffer_width;
        texture.height = framebuffer_height;
        texture.mip_levels = 1;
        texture.channel_count = 1;
        texture.format = TextureFormat::Depth32F;

        SDL_GPUTextureCreateInfo info = {};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = TextureFormatToSDL(texture.format);
        info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        info.width = texture.width;
        info.height = texture.height;
        info.layer_count_or_depth = 1;
        info.num_levels = texture.mip_levels;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);
        texture.handle = SDL_CreateGPUTexture(device, &info);
        if (texture.handle == NULL)
        {
            ERROR("Textures::LoadDepthTexture - %s", "Failed to create gpu texture for depth texture!");
            return Stub_Texture;
        }

        return texture;
    }

    Texture Load(const std::filesystem::path& path)
    {
        Texture texture;

        const std::filesystem::path path_base = SDL_GetBasePath();
        const std::filesystem::path path_full = path_base / path;

        stbi_set_flip_vertically_on_load(true);
        s32 width, height, channel_count = 0;
        u8* image_data = stbi_load(path_full.c_str(), &width, &height, &channel_count, 0);
        if (image_data == NULL)
        {
            WARN("Textures::Load - Failed to load texture, ensure %s is a valid path!", path_full.c_str());
            return Stub_Texture;
        }

        texture.width = width;
        texture.height = height;
        texture.channel_count = channel_count;
        texture.mip_levels = CalculateMipLevels(texture.width, texture.height);

        SDL_GPUTextureCreateInfo info = {};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = TextureFormatToSDL(texture.format);
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = texture.width;
        info.height = texture.height;
        info.layer_count_or_depth = 1;
        info.num_levels = texture.mip_levels;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);
        texture.handle = SDL_CreateGPUTexture(device, &info);

        if (texture.handle == NULL)
        {
            ERROR("Textures::Load - Failed to create gpu texture for \"%s\"!", path_full.c_str());
            return Stub_Texture;
        }

        UploadTexture(static_cast<SDL_GPUTexture*>(texture.handle), image_data, texture.width, texture.height);
        stbi_image_free(image_data);

        return texture;
    }

    void Unload(Texture& texture)
    {
        if (texture.handle != NULL)
        {
            const Window& window = Application::GetWindow();
            SDL_ReleaseGPUTexture(static_cast<SDL_GPUDevice*>(window.gpu_device), static_cast<SDL_GPUTexture*>(texture.handle));
        }

        texture.handle = NULL;
        texture.width = 0;
        texture.height = 0;
        texture.mip_levels = 0;
        texture.channel_count = 0;
    }

    void Bind(const Texture& texture, TextureSampler sampler_index, u8 slot)
    {
        if (texture.handle == NULL)
            return;

        SDL_GPUTextureSamplerBinding binding = {};
        binding.texture = static_cast<SDL_GPUTexture*>(texture.handle);
        binding.sampler = samplers[(u8)sampler_index];

        SDL_GPURenderPass* render_pass = (SDL_GPURenderPass*)Renderer::GetRenderPass();
        SDL_BindGPUFragmentSamplers(render_pass, slot, &binding, 1);
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
            case TextureFormat::Depth32F:
                format_sdl = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
                break;
            default:
                break;
        }

        return format_sdl;
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

    void UploadTexture(SDL_GPUTexture* texture_handle, const u8* image_data, u16 width, u16 height)
    {
        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);
        const u32 data_size = width * height * 4;

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
        source.pixels_per_row = width;
        source.rows_per_layer = height;

        SDL_GPUTextureRegion dest = {};
        dest.texture = texture_handle;
        dest.mip_level = 0;
        dest.layer = 0;
        dest.x = dest.y = dest.z = 0;
        dest.w = width;
        dest.h = height;
        dest.d = 1;

        SDL_UploadToGPUTexture(copy_pass, &source, &dest, false);
        SDL_EndGPUCopyPass(copy_pass);

        SDL_SubmitGPUCommandBuffer(command_buffer);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    }
}
