#include "Graphics/Shader.h"
#include "Core/Application.h"
#include "Core/Log.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_filesystem.h>

namespace Nova::Shaders
{
    void* CreateShader(SDL_GPUDevice* gpu_device, const std::filesystem::path& path, const ShaderStorageInfo& info);

    Shader Load(const std::filesystem::path& path_vertex, const std::filesystem::path& path_fragment, const ShaderStorageInfo& info_vertex, const ShaderStorageInfo& info_fragment)
    {
        Shader shader;

        const std::filesystem::path path_base = SDL_GetBasePath();
        const std::filesystem::path path_vertex_full = path_base / path_vertex;
        const std::filesystem::path path_fragment_full = path_base / path_fragment;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = (SDL_GPUDevice*)window.gpu_device;

        shader.handle_vertex = CreateShader(device, path_vertex_full, info_vertex);
        shader.handle_fragment = CreateShader(device, path_fragment_full, info_fragment);
        shader.info_vertex = info_vertex;
        shader.info_fragment = info_fragment;

        return shader;
    }

    void* CreateShader(SDL_GPUDevice* gpu_device, const std::filesystem::path& path, const ShaderStorageInfo& info)
    {
        SDL_GPUShaderStage stage = SDL_GPU_SHADERSTAGE_VERTEX;
        if (strstr(path.c_str(), "_vs"))
            stage = SDL_GPU_SHADERSTAGE_VERTEX;
        else if (strstr(path.c_str(), "_fs"))
            stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        else
        {
            ERROR("Shaders::Load - Shader %s has an invalid stage, ensure '_vs' or '_fs' is somwhere in the filename!", path.c_str());
            return NULL;
        }

        SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(gpu_device);
        SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
        const char* entrypoint;

        if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV)
        {
            format = SDL_GPU_SHADERFORMAT_SPIRV;
            entrypoint = "main";
        }
        else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL)
        {
            format = SDL_GPU_SHADERFORMAT_MSL;
            entrypoint = "main0";
        }
        else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL)
        {
            format = SDL_GPU_SHADERFORMAT_DXIL;
            entrypoint = "main";
        }
        else
        {
            ERROR("Shaders::Load - %s", "Unrecognized backend shader format!");
            return NULL;
        }

        size_t code_size;
        u8* code = (u8*)SDL_LoadFile(path.c_str(), &code_size);
        if (code == NULL)
        {
            ERROR("Shaders::Load - Failed to load shader %s from disk!", path.c_str());
            return NULL;
        }

        SDL_GPUShaderCreateInfo shader_info = {};
        shader_info.code_size = code_size;
        shader_info.code = code;
        shader_info.entrypoint = entrypoint;
        shader_info.format = format;
        shader_info.stage = stage;
        shader_info.num_samplers = info.sampler_count;
        shader_info.num_storage_textures = info.storage_texture_count;
        shader_info.num_storage_buffers = info.storage_buffer_count;
        shader_info.num_uniform_buffers = info.uniform_buffer_count;

        SDL_GPUShader* shader = SDL_CreateGPUShader(gpu_device, &shader_info);
        if (shader == NULL)
        {
            ERROR("Shaders::Load - Failed to create shader %s!", path.c_str());
            SDL_free(code);
            return NULL;
        }

        SDL_free(code);
        INFO("%s shader %s was created successfully", stage == SDL_GPU_SHADERSTAGE_VERTEX ? "Vertex" : "Fragment", path.c_str());
        return (void*)shader;
    }

    void Unload(Shader& shader)
    {
        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = (SDL_GPUDevice*)window.gpu_device;

        if (shader.handle_vertex != NULL)
            SDL_ReleaseGPUShader(device, (SDL_GPUShader*)shader.handle_vertex);

        if (shader.handle_fragment != NULL)
            SDL_ReleaseGPUShader(device, (SDL_GPUShader*)shader.handle_fragment);

        shader.info_vertex.sampler_count = 0;
        shader.info_vertex.storage_texture_count = 0;
        shader.info_vertex.storage_buffer_count = 0;
        shader.info_vertex.uniform_buffer_count = 0;

        shader.info_fragment.sampler_count = 0;
        shader.info_fragment.storage_texture_count = 0;
        shader.info_fragment.storage_buffer_count = 0;
        shader.info_fragment.uniform_buffer_count = 0;
    }
}
