#include "Graphics/Renderer.h"
#include "Graphics/Camera.h"
#include "Graphics/Mesh.h"
#include "Graphics/Model.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture.h"

#include "Core/Application.h"
#include "Core/Base.h"
#include "Core/Log.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_filesystem.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Nova::Renderer
{
    struct RenderState
    {
        glm::mat4 matrix_view;
        glm::mat4 matrix_projection;
        Shader shader_diffuse;
        Texture texture_default_white;
        Texture texture_depth_stencil;
        Texture texture_swapchain;
        RenderPassHandle active_render_pass = NULL;
        Camera3D* primary_camera = NULL;
        SDL_GPUCommandBuffer* command_buffer = NULL;
        u32 vertex_buffer_count = 0;
        u32 index_buffer_count = 0;
        u32 storage_buffer_count = 0;
        PipelineType pipeline_index = PipelineType::OutdoorMeshes;
    };

    struct MVPData
    {
        glm::mat4 matrix_model;
        glm::mat4 matrix_view_projection;
        glm::mat4 matrix_normal;
    };

    static RenderState state;

    void Init()
    {
        const std::filesystem::path path_vertex = "Assets/Shaders/Compiled/Diffuse_vs.spv";
        const std::filesystem::path path_fragment = "Assets/Shaders/Compiled/Diffuse_fs.spv";

        ShaderStorageInfo diffuse_vertex_info = {};
        diffuse_vertex_info.uniform_buffer_count = 1;

        ShaderStorageInfo diffuse_fragment_info = {};
        diffuse_fragment_info.uniform_buffer_count = 1;
        diffuse_fragment_info.sampler_count = 1;
        state.shader_diffuse = Shaders::Load(path_vertex, path_fragment, diffuse_vertex_info, diffuse_fragment_info);

        // Initialize all of the graphics pipelines
        PipelineShaderInfo shader_info = {};
        shader_info.outdoor_meshes = &state.shader_diffuse;
        shader_info.outdoor_meshes_skinned = &state.shader_diffuse;
        shader_info.indoor_meshes = &state.shader_diffuse;
        shader_info.wireframe_meshes = &state.shader_diffuse;
        Pipelines::Init(shader_info);

        // Shader resources not needed after the pipelines are initialized
        Shaders::Unload(state.shader_diffuse);

        // Setup texture samplers and default textures
        const Window& window = Application::GetWindow();
        Textures::SetupSamplers();
        state.texture_default_white = Textures::LoadDefaultWhite();
        state.texture_depth_stencil = Textures::LoadDepthTexture(window.width, window.height);
        state.texture_swapchain = Stub_Texture; // Gets written in "BeginFrame"

        INFO("The renderer initialized successfully with %d graphics pipelines", PipelineType::_Length);
    }

    void Shutdown()
    {
        INFO("%s", "Shutting down the renderer...");
        Textures::Unload(state.texture_default_white);
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
        state.texture_swapchain.width = swapchain_width;
        state.texture_swapchain.height = swapchain_height;
        state.texture_swapchain.channel_count = 4;

        return true;
    }

    void EndFrame()
    {
        state.matrix_view = glm::mat4(1.f);
        state.matrix_projection = glm::mat4(1.f);
        SDL_SubmitGPUCommandBuffer(state.command_buffer);
    }

    void DrawMesh(const Mesh& mesh, const glm::mat4& transform, const Material& material)
    {
        if (mesh.buffer_vertex.handle == NULL || mesh.buffer_index.handle == NULL)
            return;

        Buffers::Bind(mesh.buffer_vertex);
        Buffers::Bind(mesh.buffer_index);
        Textures::Bind(material.albedo_texture != NULL ? *material.albedo_texture : state.texture_default_white, TextureSampler::PointClamp);

        // Albedo, metallic, roughness
        const float material_data[6] = {material.albedo.r, material.albedo.g, material.albedo.b, material.albedo.a, 0.f, 0.f};

        const MVPData mvp_data = (MVPData){
            .matrix_model = transform,
            .matrix_view_projection = state.matrix_projection * state.matrix_view,
            .matrix_normal = glm::transpose(glm::inverse(transform))
        };
        SDL_PushGPUVertexUniformData(state.command_buffer, 0, &mvp_data, sizeof(MVPData));
        SDL_PushGPUFragmentUniformData(state.command_buffer, 0, material_data, sizeof(float) * LEN(material_data));
        SDL_DrawGPUIndexedPrimitives(static_cast<SDL_GPURenderPass*>(state.active_render_pass), mesh.indices.size(), 1, 0, 0, 0);
    }

    void DrawModel(const Model& model, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
    {
        for (const Mesh& mesh : model.meshes)
        {
            glm::mat4 transform = glm::mat4(1.f);
            transform = glm::translate(transform, position);
            transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1.f, 0.f, 0.f));
            transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0.f, 1.f, 0.f));
            transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0.f, 0.f, 1.f));
            transform = glm::scale(transform, scale);

            Renderer::DrawMesh(mesh, transform, model.materials[mesh.material_index]);
        }
    }

    void* GetCommandBuffer() { return state.command_buffer; }
    Camera3D* GetPrimaryCamera() { return state.primary_camera; }
    RenderPassHandle GetActiveRenderPass() { return state.active_render_pass; }
    const Texture& GetTextureSwapchain() { return state.texture_swapchain; }
    const Texture& GetTextureDepthStencil() { return state.texture_depth_stencil; }
    const glm::mat4& GetMatrixView() { return state.matrix_view; }
    const glm::mat4& GetMatrixProjection() { return state.matrix_projection; }

    void SetActiveRenderPass(RenderPassHandle render_pass) { state.active_render_pass = render_pass; }
    void SetPrimaryCamera(Camera3D* camera) { state.primary_camera = camera; }
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
        state.texture_depth_stencil = Textures::LoadDepthTexture(window.width, window.height);
    }
}
