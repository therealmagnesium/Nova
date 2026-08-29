#include "Graphics/Renderer.h"
#include "Graphics/Animator.h"
#include "Graphics/Camera.h"
#include "Graphics/IBL.h"
#include "Graphics/Lights.h"
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
    static constexpr u32 MAX_ANIMATED_DRAWS_PER_FRAME = 64;

    struct SwapchainTexture
    {
        TextureHandle handle = NULL;
        Texture metadata;
    };

    // A fixed pool of pre-allocated SSBOs for skinning matrices, one slot per animated draw
    // call in a frame. Pre-allocating avoids creating/destroying a GPU buffer on every single
    // DrawAnimatedModel call; BeginFrame just rewinds next_slot back to 0. If a scene needs
    // more than MAX_ANIMATED_DRAWS_PER_FRAME simultaneous animated draws, bump the constant -
    // it's a small, fixed amount of VRAM (MAX_ANIMATED_DRAWS_PER_FRAME * MAX_BONES * 64 bytes).
    struct BoneMatrixPool
    {
        GPUBuffer buffers[MAX_ANIMATED_DRAWS_PER_FRAME];
        Buffers::UploadStream* upload_stream = NULL;
        u32 next_slot = 0;
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
        glm::vec4 camera_position;
    };

    struct RenderCommand
    {
        glm::mat4 transform = glm::mat4(1.f);
        const GPUBuffer* buffer_vertex = NULL;
        const GPUBuffer* buffer_index = NULL;
        const GPUBuffer* buffer_ssbo_bones = NULL;
        const Material* material = NULL;
        u64 index_count = 0;
        float sort_depth = 0.f;
        GPUPipeline pipeline = GPUPipeline::OutdoorMeshes;
    };

    struct RenderQueue
    {
        std::vector<RenderCommand> opaque;
        std::vector<RenderCommand> translucent;
    };

    struct RenderState
    {
        BoneMatrixPool bone_matrix_pool;
        glm::mat4 matrix_view;
        glm::mat4 matrix_projection;
        Mesh primitives[static_cast<u8>(PrimitiveMesh::_Length)];
        SwapchainTexture texture_swapchain;
        Texture texture_default_white;
        Texture texture_default_normal;
        Texture texture_depth_stencil;
        const DirectionalLight* active_sun = NULL;
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

    static RenderState state;
    static RenderQueue queue;

    void SubmitCommand(const RenderCommand& command);
    void RegisterCommand(const RenderCommand& command);
    void DrawSkinnedMesh(const Mesh& mesh, const glm::mat4& transform, const Material& material, const GPUBuffer& bone_matrix_buffer);

    void Init()
    {
        const ShaderStorageInfo info_scene_vertex = {
            .sampler_count = 0,
            .uniform_buffer_count = 1,
            .storage_buffer_count = 0,
            .storage_texture_count = 0
        };
        const ShaderStorageInfo info_skinned_scene_vertex = {
            .sampler_count = 0,
            .uniform_buffer_count = 1,
            .storage_buffer_count = 1, // Bone matrices SSBO
            .storage_texture_count = 0
        };
        const ShaderStorageInfo info_scene_fragment = {
            .sampler_count = 7,
            .uniform_buffer_count = 1,
            .storage_buffer_count = 0,
            .storage_texture_count = 0
        };
        const ShaderStorageInfo info_compositing_vertex = {
            .sampler_count = 0,
            .uniform_buffer_count = 0,
            .storage_buffer_count = 0,
            .storage_texture_count = 0
        };
        const ShaderStorageInfo info_compositing_fragment = {
            .sampler_count = 1,
            .uniform_buffer_count = 1,
            .storage_buffer_count = 0,
            .storage_texture_count = 0
        };
        const ShaderStorageInfo info_ibl_vertex = {
            .sampler_count = 0,
            .uniform_buffer_count = 1,
            .storage_buffer_count = 0,
            .storage_texture_count = 0
        };
        const ShaderStorageInfo info_ibl_fragment = {
            .sampler_count = 1,
            .uniform_buffer_count = 0,
            .storage_buffer_count = 0,
            .storage_texture_count = 0
        };
        const ShaderStorageInfo info_prefilter_fragment = {
            .sampler_count = 1,
            .uniform_buffer_count = 1,
            .storage_buffer_count = 0,
            .storage_texture_count = 0
        };

        Shader shader_pbr = Shaders::Load("Assets/Shaders/Compiled/PBR_vs.spv", "Assets/Shaders/Compiled/PBR_fs.spv", info_scene_vertex, info_scene_fragment);
        Shader shader_pbr_skinned = Shaders::Load("Assets/Shaders/Compiled/SkinnedPBR_vs.spv", "Assets/Shaders/Compiled/PBR_fs.spv", info_skinned_scene_vertex, info_scene_fragment);
        Shader shader_compositing = Shaders::Load("Assets/Shaders/Compiled/Compositing_vs.spv", "Assets/Shaders/Compiled/Compositing_fs.spv", info_compositing_vertex, info_compositing_fragment);
        Shader shader_hdri_to_cubemap = Shaders::Load("Assets/Shaders/Compiled/Cubemap_vs.spv", "Assets/Shaders/Compiled/EquirectangularToCubemap_fs.spv", info_ibl_vertex, info_ibl_fragment);
        Shader shader_irradiance = Shaders::Load("Assets/Shaders/Compiled/Cubemap_vs.spv", "Assets/Shaders/Compiled/Irradiance_fs.spv", info_ibl_vertex, info_ibl_fragment);
        Shader shader_prefilter = Shaders::Load("Assets/Shaders/Compiled/Cubemap_vs.spv", "Assets/Shaders/Compiled/Prefilter_fs.spv", info_ibl_vertex, info_prefilter_fragment);
        Shader shader_brdf = Shaders::Load("Assets/Shaders/Compiled/BRDF_vs.spv", "Assets/Shaders/Compiled/BRDF_fs.spv");
        Shader shader_skybox = Shaders::Load("Assets/Shaders/Compiled/Skybox_vs.spv", "Assets/Shaders/Compiled/Skybox_fs.spv", info_ibl_vertex, info_ibl_fragment);

        // Initialize all of the graphics pipelines
        const auto shader_info = (PipelineShaderInfo){
            .outdoor_meshes = &shader_pbr,
            .outdoor_meshes_skinned = &shader_pbr_skinned,
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
        Shaders::Unload(shader_pbr_skinned);
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

        state.primitives[static_cast<u8>(PrimitiveMesh::Quad)] = Meshes::GenerateQuad();
        state.primitives[static_cast<u8>(PrimitiveMesh::Cube)] = Meshes::GenerateCube();
        state.primitives[static_cast<u8>(PrimitiveMesh::Sphere)] = Meshes::GenerateSphere(32, 16);
        state.primitives[static_cast<u8>(PrimitiveMesh::Plane)] = Meshes::GeneratePlane();
        state.primitives[static_cast<u8>(PrimitiveMesh::Cone)] = Meshes::GenerateCone();
        state.primitives[static_cast<u8>(PrimitiveMesh::Pyramid)] = Meshes::GeneratePyramid();
        state.primitives[static_cast<u8>(PrimitiveMesh::Torus)] = Meshes::GenerateTorus();

        // Pre-allocate every bone matrix pool slot up front - see BoneMatrixPool's comment above.
        const u32 bone_buffer_size = static_cast<u32>(sizeof(glm::mat4) * MAX_BONES);
        for (u32 i = 0; i < MAX_ANIMATED_DRAWS_PER_FRAME; i++)
            state.bone_matrix_pool.buffers[i] = Buffers::Create(GPUBufferType::Storage, bone_buffer_size);

        // One persistent staging buffer sized for the whole pool, created once and reused every frame.
        state.bone_matrix_pool.upload_stream = Buffers::CreateUploadStream(bone_buffer_size * MAX_ANIMATED_DRAWS_PER_FRAME);

        Renderer::SetSun(Stub_DirectionalLight);

        INFO("The renderer initialized successfully with %d graphics pipelines", GPUPipeline::_Length);
    }

    void Shutdown()
    {
        INFO("%s", "Shutting down the renderer...");

        for (Mesh& primitive : state.primitives)
            Meshes::Destroy(primitive);

        Textures::Unload(state.texture_default_white);
        Textures::Unload(state.texture_default_normal);
        Textures::Unload(state.texture_depth_stencil);
        Textures::FreeSamplers();

        Buffers::DestroyUploadStream(state.bone_matrix_pool.upload_stream);
        for (u32 i = 0; i < MAX_ANIMATED_DRAWS_PER_FRAME; i++)
            Buffers::Destroy(state.bone_matrix_pool.buffers[i]);

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

        u32 swapchain_width = 0;
        u32 swapchain_height = 0;
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
        SDL_SubmitGPUCommandBuffer(state.command_buffer);
        state.command_buffer = NULL;

        Pipelines::ResetBindingCache();
        Buffers::ResetBindingCache();

        state.matrix_view = glm::mat4(1.f);
        state.matrix_projection = glm::mat4(1.f);
        state.bone_matrix_pool.next_slot = 0;

        // Flush every bone-matrix write queued this frame as ONE copy pass on its own command
        // buffer, submitted before the render command buffer.
        Buffers::StreamFlush(state.bone_matrix_pool.upload_stream);
    }

    void Flush()
    {
        const auto SortDepthNearest = [](const RenderCommand& a, const RenderCommand& b)
        {
            return a.sort_depth < b.sort_depth;
        };

        const auto SortDepthFurthest = [](const RenderCommand& a, const RenderCommand& b)
        {
            return a.sort_depth > b.sort_depth;
        };

        std::sort(queue.opaque.begin(), queue.opaque.end(), SortDepthNearest);            // near → far
        std::sort(queue.translucent.begin(), queue.translucent.end(), SortDepthFurthest); // far → near

        for (const RenderCommand& command : queue.opaque)
            SubmitCommand(command);

        for (const RenderCommand& command : queue.translucent)
            SubmitCommand(command);

        queue.opaque.clear();
        queue.translucent.clear();
    }

    void DrawPrimitive(PrimitiveMesh primitive, const glm::mat4& transform, const Material& material)
    {
        const Mesh& mesh = state.primitives[static_cast<u8>(primitive)];
        if (mesh.index_count < 1)
            return;

        DrawMesh(mesh, transform, material);
    }

    void DrawMesh(const Mesh& mesh, const glm::mat4& transform, const Material& material)
    {
        if (mesh.buffer_vertex.handle == NULL || mesh.buffer_index.handle == NULL)
            return;
        if (state.active_render_pass == NULL || state.primary_camera == NULL)
            return;

        const glm::vec3 world_pos = glm::vec3(transform[3]);
        const glm::vec4 view_pos = state.matrix_view * glm::vec4(world_pos, 1.f);
        const float view_depth = view_pos.z;

        const RenderCommand command = {
            .transform = transform,
            .buffer_vertex = &mesh.buffer_vertex,
            .buffer_index = &mesh.buffer_index,
            .material = &material,
            .index_count = mesh.index_count,
            .sort_depth = 0.f,
            .pipeline = mesh.pipeline,
        };

        RegisterCommand(command);
    }

    void DrawModel(const Model& model, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
    {
        const glm::mat4 transform = Meshes::CalculateTransform(position, rotation, scale);

        for (const Mesh& mesh : model.meshes)
            Renderer::DrawMesh(mesh, transform, model.materials[mesh.material_index]);
    }

    void DrawAnimatedModel(const AnimatedModel& model, const Animator& animator, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
    {
        if (state.active_render_pass == NULL || state.primary_camera == NULL || !animator.IsValid())
            return;

        if (state.bone_matrix_pool.next_slot >= MAX_ANIMATED_DRAWS_PER_FRAME)
        {
            WARN("%s", "Renderer::DrawAnimatedModel - Exceeded MAX_ANIMATED_DRAWS_PER_FRAME, dropping draw call! Consider raising the pool size.");
            return;
        }

        GPUBuffer& bone_matrix_buffer = state.bone_matrix_pool.buffers[state.bone_matrix_pool.next_slot++];
        const u32 bone_count = static_cast<u32>(animator.skeleton->bones.size());
        const u32 size = static_cast<u32>(sizeof(glm::mat4)) * bone_count;
        const u32 offset = Buffers::StreamWrite(state.bone_matrix_pool.upload_stream, animator.bone_matrices, size);
        Buffers::StreamQueueCopy(state.bone_matrix_pool.upload_stream, offset, bone_matrix_buffer, size);

        // The actual GPU-side copy is deferred to EndFrame's StreamFlush call - see Buffers::UploadStream.
        // Binding bone_matrix_buffer below is safe: submission order governs execution order, and
        // StreamFlush's upload command buffer is submitted before state.command_buffer.
        const glm::mat4 transform = Meshes::CalculateTransform(position, rotation, scale);
        for (const Mesh& mesh : model.meshes)
            DrawSkinnedMesh(mesh, transform, model.materials[mesh.material_index], bone_matrix_buffer);
    }

    void DrawSkinnedMesh(const Mesh& mesh, const glm::mat4& transform, const Material& material, const GPUBuffer& bone_matrix_buffer)
    {
        if (mesh.buffer_vertex.handle == NULL || mesh.buffer_index.handle == NULL)
            return;
        if (state.active_render_pass == NULL || state.primary_camera == NULL)
            return;

        const glm::vec3 world_pos = glm::vec3(transform[3]);
        const glm::vec4 view_pos = state.matrix_view * glm::vec4(world_pos, 1.f);
        const float view_depth = view_pos.z;

        const RenderCommand command = {
            .transform = transform,
            .buffer_vertex = &mesh.buffer_vertex,
            .buffer_index = &mesh.buffer_index,
            .buffer_ssbo_bones = &bone_matrix_buffer,
            .material = &material,
            .index_count = mesh.index_count,
            .sort_depth = 0.f,
            .pipeline = mesh.pipeline,
        };

        RegisterCommand(command);
    }

    void DrawTextureCompositing(const Texture& screen_texture)
    {
        const RenderPassHandle render_pass = Renderer::GetActiveRenderPass();
        const Mesh& mesh_compositing = GetPrimitiveMesh(PrimitiveMesh::Quad);

        Pipelines::Bind(GPUPipeline::PostProcessing, render_pass);
        Buffers::Bind(mesh_compositing.buffer_index);
        Buffers::Bind(mesh_compositing.buffer_vertex);
        Textures::Bind(screen_texture);

        const float exposure = state.exposure;
        SDL_PushGPUFragmentUniformData(state.command_buffer, 0, &exposure, sizeof(float));
        SDL_DrawGPUIndexedPrimitives(static_cast<SDL_GPURenderPass*>(render_pass), mesh_compositing.index_count, 1, 0, 0, 0);
    }

    void DrawSkybox(const EnvironmentMap& environment_map)
    {
        if (state.active_render_pass == NULL || state.primary_camera == NULL)
            return;

        const Mesh& mesh_skybox = GetPrimitiveMesh(PrimitiveMesh::Cube);
        state.active_environment_map = &environment_map;

        Pipelines::Bind(GPUPipeline::IBL_Skybox, state.active_render_pass);
        Buffers::Bind(mesh_skybox.buffer_vertex);
        Buffers::Bind(mesh_skybox.buffer_index);
        Textures::Bind(environment_map.environment);

        const glm::mat4 mvp_data[2] = { state.matrix_view, state.matrix_projection };
        SDL_PushGPUVertexUniformData(state.command_buffer, 0, mvp_data, sizeof(glm::mat4) * LEN(mvp_data));
        SDL_DrawGPUIndexedPrimitives(static_cast<SDL_GPURenderPass*>(state.active_render_pass), mesh_skybox.index_count, 1, 0, 0, 0);
    }

    void SubmitCommand(const RenderCommand& command)
    {
        if (command.buffer_vertex == NULL || command.buffer_index == NULL)
            return;

        if (command.buffer_vertex->handle == NULL || command.buffer_index->handle == NULL)
            return;

        if (command.material == NULL)
            return;

        if (state.active_render_pass == NULL || state.primary_camera == NULL)
            return;

        Pipelines::Bind(command.pipeline, state.active_render_pass);
        Buffers::Bind(*command.buffer_vertex);
        Buffers::Bind(*command.buffer_index);

        if (command.buffer_ssbo_bones != NULL)
            Buffers::Bind(*command.buffer_ssbo_bones);

        Textures::Bind(command.material->texture_albedo.IsValid() ? command.material->texture_albedo : state.texture_default_white, 0);
        Textures::Bind(command.material->texture_normal.IsValid() ? command.material->texture_normal : state.texture_default_normal, 1);
        Textures::Bind(command.material->texture_metallic.IsValid() ? command.material->texture_metallic : state.texture_default_white, 2);
        Textures::Bind(command.material->texture_roughness.IsValid() ? command.material->texture_roughness : state.texture_default_white, 3);

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

        const MVPData mvp_data = {
            .matrix_model = command.transform,
            .matrix_view_projection = state.matrix_projection * state.matrix_view,
            .matrix_normal = glm::transpose(glm::inverse(command.transform))
        };

        const glm::vec4 albedo_linear = glm::vec4(powf(command.material->albedo.r, 2.2f), powf(command.material->albedo.g, 2.2f), powf(command.material->albedo.b, 2.2f), powf(command.material->albedo.a, 2.2f));
        const FragmentData frag_data = {
            .data_light = {
                .direction_intensity = glm::vec4(state.active_sun->direction, state.active_sun->intensity),
                .color = state.active_sun->color,
            },
            .data_material = {
                .albedo = albedo_linear,
                .pbr = glm::vec4(command.material->texture_metallic.IsValid() ? 1.f : command.material->metallic, command.material->texture_roughness.IsValid() ? 1.f : command.material->roughness, 0.f, 0.f),
            },
            .camera_position = glm::vec4(state.primary_camera->position, 0.f),
        };

        SDL_PushGPUVertexUniformData(state.command_buffer, 0, &mvp_data, sizeof(MVPData));
        SDL_PushGPUFragmentUniformData(state.command_buffer, 0, &frag_data, sizeof(FragmentData));
        SDL_DrawGPUIndexedPrimitives(static_cast<SDL_GPURenderPass*>(state.active_render_pass), command.index_count, 1, 0, 0, 0);
    }

    void RegisterCommand(const RenderCommand& command)
    {
        const bool is_transparent = command.material->albedo.a < 1.f || command.material->texture_albedo.has_transparency;
        (!is_transparent ? queue.opaque : queue.translucent).emplace_back(command);
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
    const Mesh& GetPrimitiveMesh(PrimitiveMesh primitive) { return state.primitives[static_cast<u8>(primitive)]; }

    void SetSun(const DirectionalLight& sun) { state.active_sun = &sun; }
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
