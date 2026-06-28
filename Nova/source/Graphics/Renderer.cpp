#include "Graphics/Renderer.h"
#include "Graphics/Camera.h"
#include "Graphics/IBL.h"
#include "Graphics/Mesh.h"
#include "Graphics/Model.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Shader.h"

#include "Core/Application.h"
#include "Core/Base.h"
#include "Core/Log.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_filesystem.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Nova::Renderer
{
    struct SwapchainTexture
    {
        TextureHandle handle = NULL;
        Texture metadata;
    };

    struct RenderState
    {
        glm::mat4 matrix_view;
        glm::mat4 matrix_projection;
        Mesh mesh_screen;
        Mesh mesh_skybox;
        SwapchainTexture texture_swapchain;
        Texture texture_default_white;
        Texture texture_default_normal;
        Texture texture_depth_stencil;
        const EnvironmentMap* active_environment_map = NULL;
        RenderPassHandle active_render_pass = NULL;
        Camera3D* primary_camera = NULL;
        SDL_GPUCommandBuffer* command_buffer = NULL;
        u32 vertex_buffer_count = 0;
        u32 index_buffer_count = 0;
        u32 storage_buffer_count = 0;
        float exposure = 1.f;
        GPUPipeline pipeline_index = GPUPipeline::OutdoorMeshes;
        bool wireframe_enabled = false;
    };

    struct alignas(16) MVPData
    {
        glm::mat4 matrix_model;
        glm::mat4 matrix_view_projection;
        glm::mat4 matrix_normal;
    };

    struct alignas(16) MaterialData
    {
        glm::vec4 albedo = glm::vec4(0.f, 0.f, 0.f, 1.f);
        glm::vec4 pbr = glm::vec4(0.f);
    };

    struct alignas(16) LightData
    {
        glm::vec4 direction_intensity = glm::vec4(0.f, -1.f, 0.f, 1.f);
        glm::vec4 color = glm::vec4(1.f);
    };

    struct alignas(16) FragmentData
    {
        LightData data_light;
        MaterialData data_material;
        glm::vec3 camera_position;
    };

    static RenderState state;

    void Init()
    {
        const auto info_scene_vertex = (ShaderStorageInfo){
            .sampler_count = 0,
            .uniform_buffer_count = 1,
            .storage_buffer_count = 0,
            .storage_texture_count = 0
        };
        const auto info_scene_fragment = (ShaderStorageInfo){
            .sampler_count = 7,
            .uniform_buffer_count = 1,
            .storage_buffer_count = 0,
            .storage_texture_count = 0

        };
        const auto info_compositing_vertex = (ShaderStorageInfo){
            .sampler_count = 0,
            .uniform_buffer_count = 0,
            .storage_buffer_count = 0,
            .storage_texture_count = 0
        };
        const auto info_compositing_fragment = (ShaderStorageInfo){
            .sampler_count = 1,
            .uniform_buffer_count = 1,
            .storage_buffer_count = 0,
            .storage_texture_count = 0
        };
        const auto info_ibl_vertex = (ShaderStorageInfo){
            .sampler_count = 0,
            .uniform_buffer_count = 1,
            .storage_buffer_count = 0,
            .storage_texture_count = 0
        };
        const auto info_ibl_fragment = (ShaderStorageInfo){
            .sampler_count = 1,
            .uniform_buffer_count = 0,
            .storage_buffer_count = 0,
            .storage_texture_count = 0
        };
        const auto info_prefilter_fragment = (ShaderStorageInfo){
            .sampler_count = 1,
            .uniform_buffer_count = 1,
            .storage_buffer_count = 0,
            .storage_texture_count = 0
        };

        Shader shader_pbr = Shaders::Load("Assets/Shaders/Compiled/PBR_vs.spv", "Assets/Shaders/Compiled/PBR_fs.spv", info_scene_vertex, info_scene_fragment);
        Shader shader_compositing = Shaders::Load("Assets/Shaders/Compiled/Compositing_vs.spv", "Assets/Shaders/Compiled/Compositing_fs.spv", info_compositing_vertex, info_compositing_fragment);
        Shader shader_hdri_to_cubemap = Shaders::Load("Assets/Shaders/Compiled/Cubemap_vs.spv", "Assets/Shaders/Compiled/EquirectangularToCubemap_fs.spv", info_ibl_vertex, info_ibl_fragment);
        Shader shader_irradiance = Shaders::Load("Assets/Shaders/Compiled/Cubemap_vs.spv", "Assets/Shaders/Compiled/Irradiance_fs.spv", info_ibl_vertex, info_ibl_fragment);
        Shader shader_prefilter = Shaders::Load("Assets/Shaders/Compiled/Cubemap_vs.spv", "Assets/Shaders/Compiled/Prefilter_fs.spv", info_ibl_vertex, info_prefilter_fragment);
        Shader shader_brdf = Shaders::Load("Assets/Shaders/Compiled/BRDF_vs.spv", "Assets/Shaders/Compiled/BRDF_fs.spv");
        Shader shader_skybox = Shaders::Load("Assets/Shaders/Compiled/Skybox_vs.spv", "Assets/Shaders/Compiled/Skybox_fs.spv", info_ibl_vertex, info_ibl_fragment);

        // Initialize all of the graphics pipelines
        const auto shader_info = (PipelineShaderInfo){
            .outdoor_meshes = &shader_pbr,
            .outdoor_meshes_skinned = &shader_pbr,
            .indoor_meshes = &shader_pbr,
            .wireframe_meshes = &shader_pbr,
            .post_processing = &shader_compositing,
            .ibl_equirectangular_to_cubemap = &shader_hdri_to_cubemap,
            .ibl_irradiance = &shader_irradiance,
            .ibl_prefilter = &shader_prefilter,
            .ibl_brdf = &shader_brdf,
            .ibl_skybox = &shader_skybox
        };

        const MSAASamples msaa = Application::GetMSAASamples();
        Pipelines::Init(shader_info, msaa);

        // Shader resources not needed after the pipelines are initialized
        Shaders::Unload(shader_pbr);
        Shaders::Unload(shader_compositing);
        Shaders::Unload(shader_hdri_to_cubemap);
        Shaders::Unload(shader_irradiance);
        Shaders::Unload(shader_prefilter);
        Shaders::Unload(shader_brdf);
        Shaders::Unload(shader_skybox);

        // Setup texture samplers and default textures
        const Window& window = Application::GetWindow();
        Textures::SetupSamplers();
        state.texture_default_white = Textures::LoadDefaultWhite();
        state.texture_default_normal = Textures::LoadDefaultNormal();
        state.texture_depth_stencil = Textures::CreateFramebufferAttachmentDepth(window.width, window.height);
        state.texture_swapchain.metadata = Stub_Texture; // Gets written in "BeginFrame"

        state.mesh_screen = Meshes::GenerateQuad();
        state.mesh_skybox = Meshes::GenerateCube();
        INFO("The renderer initialized successfully with %d graphics pipelines", GPUPipeline::_Length);
    }

    void Shutdown()
    {
        INFO("%s", "Shutting down the renderer...");
        Meshes::Destroy(state.mesh_screen);
        Meshes::Destroy(state.mesh_skybox);
        Textures::Unload(state.texture_default_white);
        Textures::Unload(state.texture_default_normal);
        Textures::Unload(state.texture_depth_stencil);
        Textures::FreeSamplers();
        Pipelines::Shutdown();
    }

    bool BeginFrame()
    {
        const Window& window = Application::GetWindow();
        SDL_Window* window_handle = static_cast<SDL_Window*>(window.handle);
        SDL_GPUDevice* gpu_device = static_cast<SDL_GPUDevice*>(window.gpu_device);

        state.command_buffer = SDL_AcquireGPUCommandBuffer(gpu_device);
        if (state.command_buffer == NULL)
            return false;

        u32 swapchain_width, swapchain_height = 0;
        SDL_GPUTexture* swapchain_handle = NULL;
        if (!SDL_AcquireGPUSwapchainTexture(state.command_buffer, window_handle, &swapchain_handle, &swapchain_width, &swapchain_height))
        {
            SDL_SubmitGPUCommandBuffer(state.command_buffer);
            state.command_buffer = NULL;
            return false;
        }

        if (swapchain_handle == NULL)
        {
            SDL_SubmitGPUCommandBuffer(state.command_buffer);
            state.command_buffer = NULL;
            return false;
        }

        if (state.primary_camera != NULL)
        {
            state.matrix_view = Cameras::GetMatrixView3D(*state.primary_camera);
            state.matrix_projection = Cameras::GetMatrixProjection3D(*state.primary_camera);
        }

        state.texture_swapchain.handle = swapchain_handle;
        state.texture_swapchain.metadata.width = swapchain_width;
        state.texture_swapchain.metadata.height = swapchain_height;
        state.texture_swapchain.metadata.channel_count = 4;

        return true;
    }

    void EndFrame()
    {
        Pipelines::ResetBindingCache();
        state.matrix_view = glm::mat4(1.f);
        state.matrix_projection = glm::mat4(1.f);

        SDL_SubmitGPUCommandBuffer(state.command_buffer);
        state.command_buffer = NULL;
    }

    void DrawSkybox(const EnvironmentMap& environment_map)
    {
        if (state.active_render_pass == NULL || state.primary_camera == NULL)
            return;

        state.active_environment_map = &environment_map;

        Pipelines::Bind(GPUPipeline::IBL_Skybox, state.active_render_pass);
        Buffers::Bind(state.mesh_skybox.buffer_vertex);
        Buffers::Bind(state.mesh_skybox.buffer_index);
        Textures::Bind(environment_map.environment);

        const glm::mat4 mvp_data[2] = {state.matrix_view, state.matrix_projection};
        SDL_PushGPUVertexUniformData(state.command_buffer, 0, mvp_data, sizeof(glm::mat4) * LEN(mvp_data));
        SDL_DrawGPUIndexedPrimitives(static_cast<SDL_GPURenderPass*>(state.active_render_pass), state.mesh_skybox.index_count, 1, 0, 0, 0);
    }

    void DrawMesh(const Mesh& mesh, const glm::mat4& transform, const Material& material)
    {
        if (mesh.buffer_vertex.handle == NULL || mesh.buffer_index.handle == NULL)
            return;

        if (state.active_render_pass == NULL || state.primary_camera == NULL)
            return;

        Pipelines::Bind(!state.wireframe_enabled ? mesh.pipeline : GPUPipeline::WireframeMeshes, state.active_render_pass);
        Buffers::Bind(mesh.buffer_vertex);
        Buffers::Bind(mesh.buffer_index);
        Textures::Bind(material.texture_albedo.IsValid() ? material.texture_albedo : state.texture_default_white, 0);
        Textures::Bind(material.texture_normal.IsValid() ? material.texture_normal : state.texture_default_normal, 1);
        Textures::Bind(material.texture_metallic.IsValid() ? material.texture_metallic : state.texture_default_white, 2);
        Textures::Bind(material.texture_roughness.IsValid() ? material.texture_roughness : state.texture_default_white, 3);

        if (state.active_environment_map != NULL && state.active_environment_map->IsValid())
        {
            Textures::Bind(state.active_environment_map->irradiance, 4);
            Textures::Bind(state.active_environment_map->prefilter, 5);
            Textures::Bind(state.active_environment_map->brdf_lut, 6);
        }
        else
        {
            Textures::Bind(state.texture_default_white, 4);
            Textures::Bind(state.texture_default_white, 5);
            Textures::Bind(state.texture_default_white, 6);
        }

        const auto mvp_data = (MVPData){
            .matrix_model = transform,
            .matrix_view_projection = state.matrix_projection * state.matrix_view,
            .matrix_normal = glm::transpose(glm::inverse(transform))
        };

        const auto albedo_linear = glm::vec4(powf(material.albedo.r, 2.2f), powf(material.albedo.g, 2.2f), powf(material.albedo.b, 2.2f), powf(material.albedo.a, 2.2f));
        const auto frag_data = (FragmentData){
            .data_light = (LightData){
                .direction_intensity = glm::vec4(-0.4f, -1.f, -0.8f, 4.f),
                .color = glm::vec4(0.98f, 0.96f, 0.92f, 1.f),
            },
            .data_material = (MaterialData){
                .albedo = albedo_linear,
                .pbr = glm::vec4(material.texture_metallic.IsValid() ? 1.f : material.metallic, material.texture_roughness.IsValid() ? 1.f : material.roughness, 0.f, 0.f),
            },
            .camera_position = state.primary_camera->position,
        };

        SDL_PushGPUVertexUniformData(state.command_buffer, 0, &mvp_data, sizeof(MVPData));
        SDL_PushGPUFragmentUniformData(state.command_buffer, 0, &frag_data, sizeof(FragmentData));
        SDL_DrawGPUIndexedPrimitives(static_cast<SDL_GPURenderPass*>(state.active_render_pass), mesh.index_count, 1, 0, 0, 0);
    }

    void DrawModel(const Model& model, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
    {
        for (const Mesh& mesh : model.meshes)
        {
            const glm::mat4 transform = Meshes::CalculateTransform(position, rotation, scale);
            Renderer::DrawMesh(mesh, transform, model.materials[mesh.material_index]);
        }
    }

    void DrawTextureCompositing(const Texture& screen_texture)
    {
        const RenderPassHandle render_pass = Renderer::GetActiveRenderPass();
        Pipelines::Bind(GPUPipeline::PostProcessing, render_pass);
        Buffers::Bind(state.mesh_screen.buffer_vertex);
        Buffers::Bind(state.mesh_screen.buffer_index);
        Textures::Bind(screen_texture);

        const float exposure = state.exposure;
        SDL_PushGPUFragmentUniformData(state.command_buffer, 0, &exposure, sizeof(float));
        SDL_DrawGPUIndexedPrimitives(static_cast<SDL_GPURenderPass*>(render_pass), state.mesh_screen.index_count, 1, 0, 0, 0);
    }

    float GetExposure() { return state.exposure; }
    void* GetCommandBuffer() { return state.command_buffer; }
    Camera3D* GetPrimaryCamera() { return state.primary_camera; }
    RenderPassHandle GetActiveRenderPass() { return state.active_render_pass; }
    TextureHandle GetSwapchainHandle() { return state.texture_swapchain.handle; }
    const Texture& GetTextureSwapchain() { return state.texture_swapchain.metadata; }
    const Texture& GetTextureDepthStencil() { return state.texture_depth_stencil; }
    const glm::mat4& GetMatrixView() { return state.matrix_view; }
    const glm::mat4& GetMatrixProjection() { return state.matrix_projection; }
    const Mesh& GetMeshSkybox() { return state.mesh_skybox; }
    const Mesh& GetMeshScreenQuad() { return state.mesh_screen; }

    void SetExposure(float exposure) { state.exposure = exposure; }
    void SetActiveRenderPass(RenderPassHandle render_pass) { state.active_render_pass = render_pass; }
    void SetPrimaryCamera(Camera3D* camera) { state.primary_camera = camera; }
    void ToggleWireframeMode() { state.wireframe_enabled = !state.wireframe_enabled; }
    u32 IncrementVertexBuffers() { return ++state.vertex_buffer_count; }
    u32 IncrementIndexBuffers() { return ++state.index_buffer_count; }
    u32 IncrementStorageBuffers() { return ++state.storage_buffer_count; }
    u32 DecrementVertexBuffers() { return --state.vertex_buffer_count; }
    u32 DecrementIndexBuffers() { return --state.index_buffer_count; }
    u32 DecrementStorageBuffers() { return --state.storage_buffer_count; }

    void Callback_OnResize()
    {
        const Window& window = Application::GetWindow();
        Textures::Unload(state.texture_depth_stencil);
        state.texture_depth_stencil = Textures::CreateFramebufferAttachmentDepth(window.width, window.height);
    }
}
