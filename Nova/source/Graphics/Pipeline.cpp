#include "Graphics/Pipeline.h"
#include "Graphics/Mesh.h"
#include "Graphics/Shader.h"
#include "Graphics/Window.h"

#include "Core/Application.h"
#include "Core/Log.h"

#include <SDL3/SDL_gpu.h>

namespace Nova::Pipelines
{
    static constexpr u8 pipeline_count = static_cast<u8>(GPUPipeline::_Length);
    static SDL_GPUGraphicsPipeline* prev_bound_pipeline = NULL;
    static SDL_GPUGraphicsPipeline* pipelines[pipeline_count];

    void InitOutdoorMeshes(const Shader* shader);
    void InitOutdoorMeshesSkinned(const Shader* shader);
    void InitIndoorMeshes(const Shader* shader);
    void InitWireframeMeshes(const Shader* shader);
    void InitPostProcessing(const Shader* shader);

    void Init(const PipelineShaderInfo& shader_info)
    {
        InitOutdoorMeshes(shader_info.outdoor_meshes);
        InitOutdoorMeshesSkinned(shader_info.outdoor_meshes_skinned);
        InitIndoorMeshes(shader_info.indoor_meshes);
        InitWireframeMeshes(shader_info.wireframe_meshes);
        InitPostProcessing(shader_info.post_processing);
    }

    void Shutdown()
    {
        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);

        for (u8 i = 0; i < static_cast<u8>(GPUPipeline::_Length); i++)
            if (pipelines[i] != NULL)
                SDL_ReleaseGPUGraphicsPipeline(device, pipelines[i]);
    }

    void Bind(GPUPipeline type, const RenderPassHandle render_pass)
    {
        const u8 index = static_cast<u8>(type);
        SDL_GPURenderPass* render_pass_handle = static_cast<SDL_GPURenderPass*>(render_pass);

        if (pipelines[index] != NULL && prev_bound_pipeline != pipelines[index])
        {
            SDL_BindGPUGraphicsPipeline(render_pass_handle, pipelines[index]);
            prev_bound_pipeline = pipelines[index];
        }
    }

    void ResetBindingCache() { prev_bound_pipeline = NULL; }

    void InitOutdoorMeshes(const Shader* shader)
    {
        if (shader == NULL)
            return;

        SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};

        // --- Shaders ---
        pipeline_info.vertex_shader = static_cast<SDL_GPUShader*>(shader->handle_vertex);
        pipeline_info.fragment_shader = static_cast<SDL_GPUShader*>(shader->handle_fragment);

        if (pipeline_info.vertex_shader == NULL || pipeline_info.fragment_shader == NULL)
        {
            FATAL("Pipelines::Init - %s", "Cannot create the graphics pipeline since its shaders are invalid!");
            return;
        }

        // --- Vertex Input Layout ---
        // Describes the memory layout of each vertex in the vertex buffer
        SDL_GPUVertexBufferDescription vertex_buffer_desc = {};
        vertex_buffer_desc.slot = 0;
        vertex_buffer_desc.pitch = sizeof(Vertex); // bytes per vertex
        vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

        // Define each attribute (element within a vertex)
        SDL_GPUVertexAttribute vertex_attributes[4] = {};

        // Attribute 0: position
        vertex_attributes[0].location = 0;
        vertex_attributes[0].buffer_slot = 0;
        vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertex_attributes[0].offset = offsetof(Vertex, position);

        // Attribute 1: normal
        vertex_attributes[1].location = 1;
        vertex_attributes[1].buffer_slot = 0;
        vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertex_attributes[1].offset = offsetof(Vertex, normal);

        // Attribute 2: uv
        vertex_attributes[2].location = 2;
        vertex_attributes[2].buffer_slot = 0;
        vertex_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        vertex_attributes[2].offset = offsetof(Vertex, uv);

        // Attribute 3: tangent
        vertex_attributes[3].location = 3;
        vertex_attributes[3].buffer_slot = 0;
        vertex_attributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertex_attributes[3].offset = offsetof(Vertex, tangent);

        pipeline_info.vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_desc;
        pipeline_info.vertex_input_state.num_vertex_buffers = 1;
        pipeline_info.vertex_input_state.vertex_attributes = vertex_attributes;
        pipeline_info.vertex_input_state.num_vertex_attributes = LEN(vertex_attributes);

        // --- Primitive Type ---
        pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        // --- Rasterizer ---
        pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
        pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

        // --- Depth/Stencil  ---
        pipeline_info.depth_stencil_state.enable_depth_test = true;
        pipeline_info.depth_stencil_state.enable_depth_write = true;
        pipeline_info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;

        // --- Color Blend ---
        SDL_GPUColorTargetBlendState blend = {};
        blend.enable_blend = false; // opaque rendering
        blend.color_write_mask = SDL_GPU_COLORCOMPONENT_R |
                                 SDL_GPU_COLORCOMPONENT_G |
                                 SDL_GPU_COLORCOMPONENT_B |
                                 SDL_GPU_COLORCOMPONENT_A;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);
        SDL_Window* window_handle = static_cast<SDL_Window*>(window.handle);

        // --- Color Target Format (must match swapchain format) ---
        SDL_GPUColorTargetDescription color_target_desc = {};
        color_target_desc.format = SDL_GetGPUSwapchainTextureFormat(device, window_handle);
        color_target_desc.blend_state = blend;

        pipeline_info.target_info.color_target_descriptions = &color_target_desc;
        pipeline_info.target_info.num_color_targets = 1;
        pipeline_info.target_info.has_depth_stencil_target = true;
        pipeline_info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

        // --- Create the pipeline ---
        const u8 index = static_cast<u8>(GPUPipeline::OutdoorMeshes);
        pipelines[index] = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
        if (pipelines[index] == NULL)
        {
            FATAL("Renderer::Init - %s", "Failed to create the graphics pipeline!");
            return;
        }
    }

    void InitOutdoorMeshesSkinned(const Shader* shader)
    {
        if (shader == NULL)
            return;

        SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};

        // --- Shaders ---
        pipeline_info.vertex_shader = static_cast<SDL_GPUShader*>(shader->handle_vertex);
        pipeline_info.fragment_shader = static_cast<SDL_GPUShader*>(shader->handle_fragment);

        if (pipeline_info.vertex_shader == NULL || pipeline_info.fragment_shader == NULL)
        {
            FATAL("Pipelines::Init - %s", "Cannot create the graphics pipeline since its shaders are invalid!");
            return;
        }

        // --- Vertex Input Layout ---
        // Describes the memory layout of each vertex in the vertex buffer
        SDL_GPUVertexBufferDescription vertex_buffer_desc = {};
        vertex_buffer_desc.slot = 0;
        vertex_buffer_desc.pitch = sizeof(Vertex); // bytes per vertex
        vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

        // Define each attribute (element within a vertex)
        SDL_GPUVertexAttribute vertex_attributes[4] = {};

        // Attribute 0: position (vec3 = 3 floats)
        vertex_attributes[0].location = 0;
        vertex_attributes[0].buffer_slot = 0;
        vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertex_attributes[0].offset = offsetof(Vertex, position);

        // Attribute 1: normal (vec3 = 3 floats)
        vertex_attributes[1].location = 1;
        vertex_attributes[1].buffer_slot = 0;
        vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertex_attributes[1].offset = offsetof(Vertex, normal);

        // Attribute 2: uv (vec2 = 2 floats)
        vertex_attributes[2].location = 2;
        vertex_attributes[2].buffer_slot = 0;
        vertex_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        vertex_attributes[2].offset = offsetof(Vertex, uv);

        // Attribute 3: tangent
        vertex_attributes[3].location = 3;
        vertex_attributes[3].buffer_slot = 0;
        vertex_attributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertex_attributes[3].offset = offsetof(Vertex, tangent);

        pipeline_info.vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_desc;
        pipeline_info.vertex_input_state.num_vertex_buffers = 1;
        pipeline_info.vertex_input_state.vertex_attributes = vertex_attributes;
        pipeline_info.vertex_input_state.num_vertex_attributes = LEN(vertex_attributes);

        // --- Primitive Type ---
        pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        // --- Rasterizer ---
        pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
        pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

        // --- Depth/Stencil  ---
        pipeline_info.depth_stencil_state.enable_depth_test = true;
        pipeline_info.depth_stencil_state.enable_depth_write = true;
        pipeline_info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;

        // --- Color Blend ---
        SDL_GPUColorTargetBlendState blend = {};
        blend.enable_blend = false; // opaque rendering
        blend.color_write_mask = SDL_GPU_COLORCOMPONENT_R |
                                 SDL_GPU_COLORCOMPONENT_G |
                                 SDL_GPU_COLORCOMPONENT_B |
                                 SDL_GPU_COLORCOMPONENT_A;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);
        SDL_Window* window_handle = static_cast<SDL_Window*>(window.handle);

        // --- Color Target Format (must match swapchain format) ---
        SDL_GPUColorTargetDescription color_target_desc = {};
        color_target_desc.format = SDL_GetGPUSwapchainTextureFormat(device, window_handle);
        color_target_desc.blend_state = blend;

        pipeline_info.target_info.color_target_descriptions = &color_target_desc;
        pipeline_info.target_info.num_color_targets = 1;
        pipeline_info.target_info.has_depth_stencil_target = true;
        pipeline_info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

        // --- Create the pipeline ---
        const u8 index = static_cast<u8>(GPUPipeline::OutdoorMeshesSkinned);
        pipelines[index] = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
        if (pipelines[index] == NULL)
        {
            FATAL("Renderer::Init - %s", "Failed to create the graphics pipeline for outdoor skinned meshes!");
            return;
        }
    }

    void InitIndoorMeshes(const Shader* shader)
    {
        if (shader == NULL)
            return;

        SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};

        // --- Shaders ---
        pipeline_info.vertex_shader = static_cast<SDL_GPUShader*>(shader->handle_vertex);
        pipeline_info.fragment_shader = static_cast<SDL_GPUShader*>(shader->handle_fragment);

        if (pipeline_info.vertex_shader == NULL || pipeline_info.fragment_shader == NULL)
        {
            FATAL("Pipelines::Init - %s", "Cannot create the graphics pipeline since its shaders are invalid!");
            return;
        }

        // --- Vertex Input Layout ---
        // Describes the memory layout of each vertex in the vertex buffer
        SDL_GPUVertexBufferDescription vertex_buffer_desc = {};
        vertex_buffer_desc.slot = 0;
        vertex_buffer_desc.pitch = sizeof(Vertex); // bytes per vertex
        vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

        // Define each attribute (element within a vertex)
        SDL_GPUVertexAttribute vertex_attributes[4] = {};

        // Attribute 0: position (vec3 = 3 floats)
        vertex_attributes[0].location = 0;
        vertex_attributes[0].buffer_slot = 0;
        vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertex_attributes[0].offset = offsetof(Vertex, position);

        // Attribute 1: normal (vec3 = 3 floats)
        vertex_attributes[1].location = 1;
        vertex_attributes[1].buffer_slot = 0;
        vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertex_attributes[1].offset = offsetof(Vertex, normal);

        // Attribute 2: uv (vec2 = 2 floats)
        vertex_attributes[2].location = 2;
        vertex_attributes[2].buffer_slot = 0;
        vertex_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        vertex_attributes[2].offset = offsetof(Vertex, uv);

        // Attribute 3: tangent
        vertex_attributes[3].location = 3;
        vertex_attributes[3].buffer_slot = 0;
        vertex_attributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertex_attributes[3].offset = offsetof(Vertex, tangent);

        pipeline_info.vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_desc;
        pipeline_info.vertex_input_state.num_vertex_buffers = 1;
        pipeline_info.vertex_input_state.vertex_attributes = vertex_attributes;
        pipeline_info.vertex_input_state.num_vertex_attributes = LEN(vertex_attributes);

        // --- Primitive Type ---
        pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        // --- Rasterizer ---
        pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_FRONT;
        pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

        // --- Depth/Stencil  ---
        pipeline_info.depth_stencil_state.enable_depth_test = true;
        pipeline_info.depth_stencil_state.enable_depth_write = true;
        pipeline_info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;

        // --- Color Blend ---
        SDL_GPUColorTargetBlendState blend = {};
        blend.enable_blend = false; // opaque rendering
        blend.color_write_mask = SDL_GPU_COLORCOMPONENT_R |
                                 SDL_GPU_COLORCOMPONENT_G |
                                 SDL_GPU_COLORCOMPONENT_B |
                                 SDL_GPU_COLORCOMPONENT_A;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);
        SDL_Window* window_handle = static_cast<SDL_Window*>(window.handle);

        // --- Color Target Format (must match swapchain format) ---
        SDL_GPUColorTargetDescription color_target_desc = {};
        color_target_desc.format = SDL_GetGPUSwapchainTextureFormat(device, window_handle);
        color_target_desc.blend_state = blend;

        pipeline_info.target_info.color_target_descriptions = &color_target_desc;
        pipeline_info.target_info.num_color_targets = 1;
        pipeline_info.target_info.has_depth_stencil_target = true;
        pipeline_info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

        // --- Create the pipeline ---
        const u8 index = static_cast<u8>(GPUPipeline::IndoorMeshes);
        pipelines[index] = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
        if (pipelines[index] == NULL)
        {
            FATAL("Renderer::Init - %s", "Failed to create the graphics pipeline for indoor meshes!");
            return;
        }
    }

    void InitWireframeMeshes(const Shader* shader)
    {
        if (shader == NULL)
            return;

        SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};

        // --- Shaders ---
        pipeline_info.vertex_shader = static_cast<SDL_GPUShader*>(shader->handle_vertex);
        pipeline_info.fragment_shader = static_cast<SDL_GPUShader*>(shader->handle_fragment);

        if (pipeline_info.vertex_shader == NULL || pipeline_info.fragment_shader == NULL)
        {
            FATAL("Pipelines::Init - %s", "Cannot create the graphics pipeline since its shaders are invalid!");
            return;
        }

        // --- Vertex Input Layout ---
        // Describes the memory layout of each vertex in the vertex buffer
        SDL_GPUVertexBufferDescription vertex_buffer_desc = {};
        vertex_buffer_desc.slot = 0;
        vertex_buffer_desc.pitch = sizeof(Vertex); // bytes per vertex
        vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

        // Define each attribute (element within a vertex)
        SDL_GPUVertexAttribute vertex_attributes[4] = {};

        // Attribute 0: position (vec3 = 3 floats)
        vertex_attributes[0].location = 0;
        vertex_attributes[0].buffer_slot = 0;
        vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertex_attributes[0].offset = offsetof(Vertex, position);

        // Attribute 1: normal (vec3 = 3 floats)
        vertex_attributes[1].location = 1;
        vertex_attributes[1].buffer_slot = 0;
        vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertex_attributes[1].offset = offsetof(Vertex, normal);

        // Attribute 2: uv (vec2 = 2 floats)
        vertex_attributes[2].location = 2;
        vertex_attributes[2].buffer_slot = 0;
        vertex_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        vertex_attributes[2].offset = offsetof(Vertex, uv);

        // Attribute 3: tangent
        vertex_attributes[3].location = 3;
        vertex_attributes[3].buffer_slot = 0;
        vertex_attributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertex_attributes[3].offset = offsetof(Vertex, tangent);

        pipeline_info.vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_desc;
        pipeline_info.vertex_input_state.num_vertex_buffers = 1;
        pipeline_info.vertex_input_state.vertex_attributes = vertex_attributes;
        pipeline_info.vertex_input_state.num_vertex_attributes = LEN(vertex_attributes);

        // --- Primitive Type ---
        pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        // --- Rasterizer ---
        pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
        pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

        // --- Depth/Stencil  ---
        pipeline_info.depth_stencil_state.enable_depth_test = true;
        pipeline_info.depth_stencil_state.enable_depth_write = true;
        pipeline_info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;

        // --- Color Blend ---
        SDL_GPUColorTargetBlendState blend = {};
        blend.enable_blend = false; // opaque rendering
        blend.color_write_mask = SDL_GPU_COLORCOMPONENT_R |
                                 SDL_GPU_COLORCOMPONENT_G |
                                 SDL_GPU_COLORCOMPONENT_B |
                                 SDL_GPU_COLORCOMPONENT_A;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);
        SDL_Window* window_handle = static_cast<SDL_Window*>(window.handle);

        // --- Color Target Format (must match swapchain format) ---
        SDL_GPUColorTargetDescription color_target_desc = {};
        color_target_desc.format = SDL_GetGPUSwapchainTextureFormat(device, window_handle);
        color_target_desc.blend_state = blend;

        pipeline_info.target_info.color_target_descriptions = &color_target_desc;
        pipeline_info.target_info.num_color_targets = 1;
        pipeline_info.target_info.has_depth_stencil_target = true;
        pipeline_info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

        // --- Create the pipeline ---
        const u8 index = static_cast<u8>(GPUPipeline::WireframeMeshes);
        pipelines[index] = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
        if (pipelines[index] == NULL)
        {
            FATAL("Renderer::Init - %s", "Failed to create the graphics pipeline for wireframe meshes!");
            return;
        }
    }

    void InitPostProcessing(const Shader* shader)
    {
        if (shader == NULL)
            return;

        SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};

        // --- Shaders ---
        pipeline_info.vertex_shader = static_cast<SDL_GPUShader*>(shader->handle_vertex);
        pipeline_info.fragment_shader = static_cast<SDL_GPUShader*>(shader->handle_fragment);

        if (pipeline_info.vertex_shader == NULL || pipeline_info.fragment_shader == NULL)
        {
            FATAL("Pipelines::Init - %s", "Cannot create the graphics pipeline since its shaders are invalid!");
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

        // Attribute 1: normal (vec3 = 3 floats)
        vertex_attributes[1].location = 1;
        vertex_attributes[1].buffer_slot = 0;
        vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertex_attributes[1].offset = offsetof(Vertex, normal);

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
        pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
        pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

        // --- Depth/Stencil  ---
        pipeline_info.depth_stencil_state.enable_depth_test = false;
        pipeline_info.depth_stencil_state.enable_depth_write = false;

        // --- Color Blend ---
        SDL_GPUColorTargetBlendState blend = {};
        blend.enable_blend = false; // opaque rendering
        blend.color_write_mask = SDL_GPU_COLORCOMPONENT_R |
                                 SDL_GPU_COLORCOMPONENT_G |
                                 SDL_GPU_COLORCOMPONENT_B |
                                 SDL_GPU_COLORCOMPONENT_A;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);
        SDL_Window* window_handle = static_cast<SDL_Window*>(window.handle);

        // --- Color Target Format (must match swapchain format) ---
        SDL_GPUColorTargetDescription color_target_desc = {};
        color_target_desc.format = SDL_GetGPUSwapchainTextureFormat(device, window_handle);
        color_target_desc.blend_state = blend;

        pipeline_info.target_info.color_target_descriptions = &color_target_desc;
        pipeline_info.target_info.num_color_targets = 1;
        pipeline_info.target_info.has_depth_stencil_target = false;

        // --- Create the pipeline ---
        const u8 index = static_cast<u8>(GPUPipeline::PostProcessing);
        pipelines[index] = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
        if (pipelines[index] == NULL)
        {
            FATAL("Renderer::Init - %s", "Failed to create the graphics pipeline for post processing!");
            return;
        }
    }
}
