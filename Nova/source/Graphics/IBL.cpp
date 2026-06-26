#include "Graphics/IBL.h"
#include "Graphics/Mesh.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Renderer.h"
#include "Graphics/Window.h"

#include "Core/Application.h"
#include "Core/Log.h"

#include <SDL3/SDL_gpu.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Nova::IBL
{
    EnvironmentMap BakeFromHDRI(const std::filesystem::path& path)
    {
        Texture hdri = Textures::LoadHDRI(path);
        if (!hdri.IsValid())
        {
            ERROR("IBL::BakeFromHDRI - Cannot bake \"%s\" since it is not a valid HDRI path!", path.c_str());
            return Stub_EnvironmentMap;
        }

        EnvironmentMap map;
        map.environment = Textures::CreateCubemap(512, 512, 9, TextureFormat::RGBA32F);
        map.irradiance = Textures::CreateCubemap(32, 32, 1, TextureFormat::RGBA32F);
        map.prefilter = Textures::CreateCubemap(128, 128, 5, TextureFormat::RGBA32F);
        map.brdf_lut = Textures::CreateFramebufferAttachmentHDR(512, 512);

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);
        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);

        glm::mat4 captureProjection = glm::perspectiveRH_ZO(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        captureProjection[1][1] *= -1;
        glm::mat4 captureViews[] = {
            glm::lookAt(glm::vec3(0), glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)),  // +X
            glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)), // -X
            glm::lookAt(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),   // +Y
            glm::lookAt(glm::vec3(0), glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)), // -Y
            glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)),  // +Z
            glm::lookAt(glm::vec3(0), glm::vec3(0, 0, -1), glm::vec3(0, -1, 0))  // -Z
        };

        const u8 face_count = 6;
        const Mesh& quad = Renderer::GetMeshScreenQuad();
        const Mesh& cube = Renderer::GetMeshSkybox();

        SDL_GPUTexture* handle_environment = static_cast<SDL_GPUTexture*>(Textures::GetHandle(map.environment));
        SDL_GPUTexture* handle_irradiance = static_cast<SDL_GPUTexture*>(Textures::GetHandle(map.irradiance));
        SDL_GPUTexture* handle_prefilter = static_cast<SDL_GPUTexture*>(Textures::GetHandle(map.prefilter));
        SDL_GPUTexture* handle_brdf = static_cast<SDL_GPUTexture*>(Textures::GetHandle(map.brdf_lut));

        // --- 1. Equirectangular to Cubemap ---
        for (u8 i = 0; i < face_count; i++)
        {
            SDL_GPUColorTargetInfo color_target = {};
            color_target.texture = static_cast<SDL_GPUTexture*>(Textures::GetHandle(map.environment));
            color_target.layer_or_depth_plane = i;
            color_target.mip_level = 0;
            color_target.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(command_buffer, &color_target, 1, NULL);
            Renderer::SetActiveRenderPass(pass);

            const glm::mat4 view_projection = captureProjection * captureViews[i];
            Pipelines::Bind(GPUPipeline::IBL_EquirectangularToCubemap, pass);
            Buffers::Bind(cube.buffer_vertex);
            Buffers::Bind(cube.buffer_index);
            Textures::Bind(hdri);
            SDL_PushGPUVertexUniformData(command_buffer, 0, &view_projection, sizeof(view_projection));
            SDL_DrawGPUIndexedPrimitives(pass, cube.index_count, 1, 0, 0, 0);

            SDL_EndGPURenderPass(pass);
            Pipelines::ResetBindingCache();
        }
        SDL_GenerateMipmapsForGPUTexture(command_buffer, handle_environment);

        // --- 2. Irradiance Convolution ---
        for (u8 i = 0; i < face_count; i++)
        {
            SDL_GPUColorTargetInfo color_target = {};
            color_target.texture = static_cast<SDL_GPUTexture*>(Textures::GetHandle(map.irradiance));
            color_target.layer_or_depth_plane = i;
            color_target.mip_level = 0;
            color_target.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(command_buffer, &color_target, 1, NULL);
            Renderer::SetActiveRenderPass(pass);

            const glm::mat4 view_projection = captureProjection * captureViews[i];
            Pipelines::Bind(GPUPipeline::IBL_Irradiance, pass);
            Buffers::Bind(cube.buffer_vertex);
            Buffers::Bind(cube.buffer_index);
            Textures::Bind(map.environment);
            SDL_PushGPUVertexUniformData(command_buffer, 0, &view_projection, sizeof(view_projection));
            SDL_DrawGPUIndexedPrimitives(pass, cube.index_count, 1, 0, 0, 0);

            SDL_EndGPURenderPass(pass);
            Pipelines::ResetBindingCache();
        }

        // --- 3. Prefilter Cubemap ---
        for (u8 i = 0; i < face_count; i++)
        {
            for (u8 j = 0; j < map.prefilter.mip_levels; j++)
            {
                SDL_GPUColorTargetInfo color_target = {};
                color_target.texture = static_cast<SDL_GPUTexture*>(Textures::GetHandle(map.prefilter));
                color_target.layer_or_depth_plane = i;
                color_target.mip_level = j;
                color_target.load_op = SDL_GPU_LOADOP_CLEAR;
                color_target.store_op = SDL_GPU_STOREOP_STORE;

                const float roughness = (float)j / (float)(map.prefilter.mip_levels - 1.f);
                SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(command_buffer, &color_target, 1, NULL);
                Renderer::SetActiveRenderPass(pass);

                const glm::mat4 view_projection = captureProjection * captureViews[i];
                Pipelines::Bind(GPUPipeline::IBL_Prefilter, pass);
                Buffers::Bind(cube.buffer_vertex);
                Buffers::Bind(cube.buffer_index);
                Textures::Bind(map.environment);
                SDL_PushGPUVertexUniformData(command_buffer, 0, &view_projection, sizeof(view_projection));
                SDL_PushGPUFragmentUniformData(command_buffer, 0, &roughness, sizeof(float));
                SDL_DrawGPUIndexedPrimitives(pass, cube.index_count, 1, 0, 0, 0);

                SDL_EndGPURenderPass(pass);
                Pipelines::ResetBindingCache();
            }
        }
        // SDL_GenerateMipmapsForGPUTexture(command_buffer, handle_prefilter);

        // --- 4. BRDF LUT ---
        SDL_GPUColorTargetInfo color_target = {};
        color_target.texture = static_cast<SDL_GPUTexture*>(Textures::GetHandle(map.brdf_lut));
        color_target.layer_or_depth_plane = 0;
        color_target.mip_level = 0;
        color_target.load_op = SDL_GPU_LOADOP_CLEAR;
        color_target.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(command_buffer, &color_target, 1, NULL);
        Renderer::SetActiveRenderPass(pass);
        Pipelines::Bind(GPUPipeline::IBL_BRDF_Integration, pass);
        Buffers::Bind(quad.buffer_vertex);
        Buffers::Bind(quad.buffer_index);
        SDL_DrawGPUIndexedPrimitives(pass, quad.index_count, 1, 0, 0, 0);
        SDL_EndGPURenderPass(pass);
        Pipelines::ResetBindingCache();

        SDL_SubmitGPUCommandBuffer(command_buffer);
        Textures::Unload(hdri);

        INFO("HDRI \"%s\" successfully baked into an environment map!", path.c_str());
        return map;
    }

    void Free(EnvironmentMap& map)
    {
        Textures::Unload(map.environment);
        Textures::Unload(map.irradiance);
        Textures::Unload(map.prefilter);
        Textures::Unload(map.brdf_lut);
    }

}
