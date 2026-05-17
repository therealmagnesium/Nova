#include "Graphics/Renderer.h"
#include "Graphics/Camera.h"
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
        Camera3D* primary_camera = NULL;
        SDL_GPUGraphicsPipeline* pipeline = NULL;
        SDL_GPUCommandBuffer* command_buffer = NULL;
        SDL_GPURenderPass* render_pass = NULL;
        SDL_GPUTexture* swapchain_texture = NULL;
        u32 vertex_buffer_count = 0;
        u32 index_buffer_count = 0;
        u16 framebuffer_width = 0;
        u16 framebuffer_height = 0;
    };

    static RenderState state;

    void Init()
    {
        const Window& window = Application::GetWindow();

        const std::filesystem::path path_base = SDL_GetBasePath();
        const std::filesystem::path path_vertex = path_base / "Assets/Shaders/Compiled/Diffuse_vs.spv";
        const std::filesystem::path path_fragment = path_base / "Assets/Shaders/Compiled/Diffuse_fs.spv";

        ShaderStorageInfo diffuse_vertex_info = {};
        diffuse_vertex_info.uniform_buffer_count = 1;

        ShaderStorageInfo diffuse_fragment_info = {};
        diffuse_fragment_info.sampler_count = 1;
        state.shader_diffuse = Shaders::Load(path_vertex, path_fragment, diffuse_vertex_info, diffuse_fragment_info);

        SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};

        // --- Shaders ---
        pipeline_info.vertex_shader = (SDL_GPUShader*)state.shader_diffuse.handle_vertex;
        pipeline_info.fragment_shader = (SDL_GPUShader*)state.shader_diffuse.handle_fragment;

        if (pipeline_info.vertex_shader == NULL || pipeline_info.fragment_shader == NULL)
        {
            FATAL("Renderer::Init - %s", "Cannot create the graphics pipeline since its shaders are invalid!");
            return;
        }

        // --- Vertex Input Layout ---
        // Describes the memory layout of each vertex in the vertex buffer
        SDL_GPUVertexBufferDescription vertex_buffer_desc = {};
        vertex_buffer_desc.slot = 0;
        vertex_buffer_desc.pitch = sizeof(Vertex); // bytes per vertex
        vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

        // Define each attribute (element within a vertex)
        SDL_GPUVertexAttribute vertex_attributes[3] = {};

        // Attribute 0: position (vec3 = 3 floats)
        vertex_attributes[0].location = 0;
        vertex_attributes[0].buffer_slot = 0;
        vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertex_attributes[0].offset = offsetof(Vertex, position);

        // Attribute 1: color (vec4 = 4 floats)
        vertex_attributes[1].location = 1;
        vertex_attributes[1].buffer_slot = 0;
        vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
        vertex_attributes[1].offset = offsetof(Vertex, color);

        // Attribute 2: uv (vec2 = 2 floats)
        vertex_attributes[2].location = 2;
        vertex_attributes[2].buffer_slot = 0;
        vertex_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        vertex_attributes[2].offset = offsetof(Vertex, uv);

        pipeline_info.vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_desc;
        pipeline_info.vertex_input_state.num_vertex_buffers = 1;
        pipeline_info.vertex_input_state.vertex_attributes = vertex_attributes;
        pipeline_info.vertex_input_state.num_vertex_attributes = 3;

        // --- Primitive Type ---
        pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        // --- Rasterizer ---
        pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

        // --- Depth/Stencil (disabled for now) ---
        pipeline_info.depth_stencil_state.enable_depth_test = false;
        pipeline_info.depth_stencil_state.enable_depth_write = false;

        // --- Color Blend ---
        SDL_GPUColorTargetBlendState blend = {};
        blend.enable_blend = false; // opaque rendering
        blend.color_write_mask = SDL_GPU_COLORCOMPONENT_R |
                                 SDL_GPU_COLORCOMPONENT_G |
                                 SDL_GPU_COLORCOMPONENT_B |
                                 SDL_GPU_COLORCOMPONENT_A;

        // --- Color Target Format (must match swapchain format) ---
        SDL_GPUColorTargetDescription color_target_desc = {};
        color_target_desc.format = SDL_GetGPUSwapchainTextureFormat((SDL_GPUDevice*)window.gpu_device, (SDL_Window*)window.handle);
        color_target_desc.blend_state = blend;

        pipeline_info.target_info.color_target_descriptions = &color_target_desc;
        pipeline_info.target_info.num_color_targets = 1;

        // --- Create the pipeline ---
        state.pipeline = SDL_CreateGPUGraphicsPipeline((SDL_GPUDevice*)window.gpu_device, &pipeline_info);
        if (state.pipeline == NULL)
        {
            FATAL("Renderer::Init - %s", "Failed to create the graphics pipeline!");
            return;
        }

        Shaders::Unload(state.shader_diffuse); // Shader resources not needed after creating the pipeline

        Textures::SetupSamplers();
    }

    void Shutdown()
    {
        const Window& window = Application::GetWindow();
        Textures::FreeSamplers();
        SDL_ReleaseGPUGraphicsPipeline((SDL_GPUDevice*)window.gpu_device, state.pipeline);
    }

    bool BeginFrame()
    {
        const Window& window = Application::GetWindow();
        SDL_Window* window_handle = (SDL_Window*)window.handle;
        SDL_GPUDevice* gpu_device = (SDL_GPUDevice*)window.gpu_device;

        state.render_pass = NULL;

        state.command_buffer = SDL_AcquireGPUCommandBuffer(gpu_device);
        if (state.command_buffer == NULL)
            return false;

        if (!SDL_AcquireGPUSwapchainTexture(state.command_buffer, window_handle, &state.swapchain_texture,
                                            (u32*)&state.framebuffer_width, (u32*)&state.framebuffer_height))
        {
            SDL_CancelGPUCommandBuffer(state.command_buffer);
            state.command_buffer = NULL;
            return false;
        }

        if (state.swapchain_texture == NULL)
        {
            SDL_CancelGPUCommandBuffer(state.command_buffer);
            state.command_buffer = NULL;
            return false;
        }

        if (state.primary_camera != NULL)
        {
            state.matrix_view = Cameras::GetMatrixView3D(*state.primary_camera);
            state.matrix_projection = Cameras::GetMatrixProjection3D(*state.primary_camera);
        }

        SDL_GPUColorTargetInfo target_info = {};
        target_info.texture = state.swapchain_texture;
        target_info.clear_color = SDL_FColor{0.12f, 0.12f, 0.12f, 1.f};
        target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        target_info.store_op = SDL_GPU_STOREOP_STORE;
        target_info.mip_level = 0;
        target_info.layer_or_depth_plane = 0;
        target_info.cycle = false;

        state.render_pass = SDL_BeginGPURenderPass(state.command_buffer, &target_info, 1, NULL);
        SDL_BindGPUGraphicsPipeline(state.render_pass, state.pipeline);
        return true;
    }

    void EndFrame()
    {
        state.matrix_view = glm::mat4(1.f);
        state.matrix_projection = glm::mat4(1.f);
        SDL_EndGPURenderPass(state.render_pass);
        SDL_SubmitGPUCommandBuffer(state.command_buffer);
    }

    void* GetRenderPass() { return state.render_pass; }
    void* GetCommandBuffer() { return state.command_buffer; }
    Camera3D* GetPrimaryCamera() { return state.primary_camera; }
    const glm::mat4& GetMatrixView() { return state.matrix_view; }
    const glm::mat4& GetMatrixProjection() { return state.matrix_projection; }

    void SetPrimaryCamera(Camera3D* camera) { state.primary_camera = camera; }
    u32 IncrementVertexBuffers() { return state.vertex_buffer_count++; }
    u32 IncrementIndexBuffers() { return state.index_buffer_count++; }
    u32 DecrementVertexBuffers() { return state.vertex_buffer_count--; }
    u32 DecrementIndexBuffers() { return state.index_buffer_count--; }
}
