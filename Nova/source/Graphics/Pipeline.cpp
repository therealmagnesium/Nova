#include "Graphics/Pipeline.h"
#include "Graphics/Mesh.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture.h"
#include "Graphics/Window.h"

#include "Core/Application.h"
#include "Core/Log.h"

#include <SDL3/SDL_gpu.h>

namespace Nova::Pipelines
{
    static constexpr u8 pipeline_count = static_cast<u8>(GPUPipeline::_Length);
    static SDL_GPUGraphicsPipeline* prev_bound_pipeline = NULL;
    static SDL_GPUGraphicsPipeline* pipelines[pipeline_count];

    SDL_GPUSampleCount MSAASamplesToSDL(MSAASamples msaa); // TODO: Remove this, already defined in Texture.cpp, just not exposed in a clean way yet
    void InitOutdoorMeshes(const Shader* shader, MSAASamples msaa = MSAASamples::One);
    void InitOutdoorMeshesSkinned(const Shader* shader, MSAASamples msaa = MSAASamples::One);
    void InitIndoorMeshes(const Shader* shader, MSAASamples msaa = MSAASamples::One);
    void InitWireframeMeshes(const Shader* shader, MSAASamples msaa = MSAASamples::One);
    void InitPostProcessing(const Shader* shader, MSAASamples msaa = MSAASamples::One);
    void InitEquirectangularToCubemap(const Shader* shader, MSAASamples msaa = MSAASamples::One);
    void InitIrradiance(const Shader* shader, MSAASamples msaa = MSAASamples::One);
    void InitPrefilter(const Shader* shader, MSAASamples msaa = MSAASamples::One);
    void InitBRDF(const Shader* shader, MSAASamples msaa = MSAASamples::One);
    void InitSkybox(const Shader* shader, MSAASamples msaa = MSAASamples::One);

    SDL_GPUGraphicsPipeline* CreateGraphicsPipeline(
        const Shader* shader,
        MSAASamples msaa,
        SDL_GPUTextureFormat color_target_format,
        bool enable_depth_test,
        bool enable_depth_write,
        SDL_GPUCompareOp depth_compare_op,
        SDL_GPUCullMode cull_mode,
        SDL_GPUFillMode fill_mode
    );

    void Init(const PipelineShaderInfo& shader_info, MSAASamples msaa)
    {
        InitOutdoorMeshes(shader_info.outdoor_meshes, msaa);
        InitOutdoorMeshesSkinned(shader_info.outdoor_meshes_skinned, msaa);
        InitIndoorMeshes(shader_info.indoor_meshes, msaa);
        InitWireframeMeshes(shader_info.wireframe_meshes, msaa);
        InitPostProcessing(shader_info.post_processing);
        InitEquirectangularToCubemap(shader_info.ibl_equirectangular_to_cubemap);
        InitIrradiance(shader_info.ibl_irradiance);
        InitPrefilter(shader_info.ibl_prefilter);
        InitBRDF(shader_info.ibl_brdf);
        InitSkybox(shader_info.ibl_skybox, msaa);
    }

    void Shutdown()
    {
        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);

        for (u8 i = 0; i < static_cast<u8>(GPUPipeline::_Length); i++)
        {
            if (pipelines[i] != NULL)
                SDL_ReleaseGPUGraphicsPipeline(device, pipelines[i]);
        }
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

    void ResetBindingCache()
    {
        prev_bound_pipeline = NULL;
    }

    SDL_GPUGraphicsPipeline* CreateGraphicsPipeline(
        const Shader* shader,
        MSAASamples msaa,
        SDL_GPUTextureFormat color_target_format,
        bool enable_depth_test,
        bool enable_depth_write,
        SDL_GPUCompareOp depth_compare_op,
        SDL_GPUCullMode cull_mode,
        SDL_GPUFillMode fill_mode
    )
    {
        if (shader == NULL || shader->handle_vertex == NULL || shader->handle_fragment == NULL)
            return NULL;

        SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};
        pipeline_info.vertex_shader = static_cast<SDL_GPUShader*>(shader->handle_vertex);
        pipeline_info.fragment_shader = static_cast<SDL_GPUShader*>(shader->handle_fragment);

        // --- Vertex Input Layout ---
        SDL_GPUVertexBufferDescription vertex_buffer_desc = {};
        vertex_buffer_desc.slot = 0;
        vertex_buffer_desc.pitch = sizeof(Vertex);
        vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

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

        // --- Primitive & Rasterizer ---
        pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        pipeline_info.rasterizer_state.fill_mode = fill_mode;
        pipeline_info.rasterizer_state.cull_mode = cull_mode;
        pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

        // --- Depth/Stencil ---
        pipeline_info.depth_stencil_state.enable_depth_test = enable_depth_test;
        pipeline_info.depth_stencil_state.enable_depth_write = enable_depth_write;
        pipeline_info.depth_stencil_state.compare_op = depth_compare_op;

        // -- Multisampling --
        pipeline_info.multisample_state.sample_count = MSAASamplesToSDL(msaa);

        // --- Color Blend ---
        SDL_GPUColorTargetBlendState blend = {};
        blend.enable_blend = false;
        blend.color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;

        SDL_GPUColorTargetDescription color_target_desc = {};
        color_target_desc.format = color_target_format;
        color_target_desc.blend_state = blend;

        pipeline_info.target_info.color_target_descriptions = &color_target_desc;
        pipeline_info.target_info.num_color_targets = 1;
        pipeline_info.target_info.has_depth_stencil_target = enable_depth_test;

        if (enable_depth_test)
            pipeline_info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);

        return SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
    }

    SDL_GPUSampleCount MSAASamplesToSDL(MSAASamples msaa)
    {
        switch (msaa)
        {
            case MSAASamples::One:
                return SDL_GPU_SAMPLECOUNT_1;
            case MSAASamples::Two:
                return SDL_GPU_SAMPLECOUNT_2;
            case MSAASamples::Four:
                return SDL_GPU_SAMPLECOUNT_4;
            case MSAASamples::Eight:
                return SDL_GPU_SAMPLECOUNT_8;
        }
        return SDL_GPU_SAMPLECOUNT_1;
    }

    // --- Pipeline Initialization Functions ---

    void InitOutdoorMeshes(const Shader* shader, MSAASamples msaa)
    {
        const Window& window = Application::GetWindow();
        const u8 index = static_cast<u8>(GPUPipeline::OutdoorMeshes);
        pipelines[index] = CreateGraphicsPipeline(shader, msaa, SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT, true, true, SDL_GPU_COMPAREOP_LESS, SDL_GPU_CULLMODE_BACK, SDL_GPU_FILLMODE_FILL);
        if (pipelines[index] == NULL) FATAL("Pipelines::Init - %s", "Failed to create Outdoor Meshes pipeline!");
    }

    void InitOutdoorMeshesSkinned(const Shader* shader, MSAASamples msaa)
    {
        const Window& window = Application::GetWindow();
        const u8 index = static_cast<u8>(GPUPipeline::OutdoorMeshesSkinned);
        pipelines[index] = CreateGraphicsPipeline(shader, msaa, SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT, true, true, SDL_GPU_COMPAREOP_LESS, SDL_GPU_CULLMODE_BACK, SDL_GPU_FILLMODE_FILL);
        if (pipelines[index] == NULL) FATAL("Pipelines::Init - %s", "Failed to create Outdoor Skinned Meshes pipeline!");
    }

    void InitIndoorMeshes(const Shader* shader, MSAASamples msaa)
    {
        const Window& window = Application::GetWindow();
        const u8 index = static_cast<u8>(GPUPipeline::IndoorMeshes);
        pipelines[index] = CreateGraphicsPipeline(shader, msaa, SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT, true, true, SDL_GPU_COMPAREOP_LESS, SDL_GPU_CULLMODE_FRONT, SDL_GPU_FILLMODE_FILL);
        if (pipelines[index] == NULL) FATAL("Pipelines::Init - %s", "Failed to create Indoor Meshes pipeline!");
    }

    void InitWireframeMeshes(const Shader* shader, MSAASamples msaa)
    {
        const Window& window = Application::GetWindow();
        const u8 index = static_cast<u8>(GPUPipeline::WireframeMeshes);
        pipelines[index] = CreateGraphicsPipeline(shader, msaa, SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT, true, true, SDL_GPU_COMPAREOP_LESS, SDL_GPU_CULLMODE_NONE, SDL_GPU_FILLMODE_LINE);
        if (pipelines[index] == NULL) FATAL("Pipelines::Init - %s", "Failed to create Wireframe Meshes pipeline!");
    }

    void InitPostProcessing(const Shader* shader, MSAASamples msaa)
    {
        const Window& window = Application::GetWindow();
        SDL_GPUTextureFormat format = SDL_GetGPUSwapchainTextureFormat(static_cast<SDL_GPUDevice*>(window.gpu_device), static_cast<SDL_Window*>(window.handle));

        const u8 index = static_cast<u8>(GPUPipeline::PostProcessing);
        pipelines[index] = CreateGraphicsPipeline(shader, msaa, format, false, false, SDL_GPU_COMPAREOP_NEVER, SDL_GPU_CULLMODE_BACK, SDL_GPU_FILLMODE_FILL);
        if (pipelines[index] == NULL) FATAL("Pipelines::Init - %s", "Failed to create Post Processing pipeline!");
    }

    void InitEquirectangularToCubemap(const Shader* shader, MSAASamples msaa)
    {
        const u8 index = static_cast<u8>(GPUPipeline::IBL_EquirectangularToCubemap);
        pipelines[index] = CreateGraphicsPipeline(shader, msaa, SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT, false, false, SDL_GPU_COMPAREOP_NEVER, SDL_GPU_CULLMODE_NONE, SDL_GPU_FILLMODE_FILL);
        if (pipelines[index] == NULL) FATAL("Pipelines::Init - %s", "Failed to create Equirectangular to Cubemap pipeline!");
    }

    void InitIrradiance(const Shader* shader, MSAASamples msaa)
    {
        const u8 index = static_cast<u8>(GPUPipeline::IBL_Irradiance);
        pipelines[index] = CreateGraphicsPipeline(shader, msaa, SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT, false, false, SDL_GPU_COMPAREOP_NEVER, SDL_GPU_CULLMODE_NONE, SDL_GPU_FILLMODE_FILL);
        if (pipelines[index] == NULL) FATAL("Pipelines::Init - %s", "Failed to create Irradiance pipeline!");
    }

    void InitPrefilter(const Shader* shader, MSAASamples msaa)
    {
        const u8 index = static_cast<u8>(GPUPipeline::IBL_Prefilter);
        pipelines[index] = CreateGraphicsPipeline(shader, msaa, SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT, false, false, SDL_GPU_COMPAREOP_NEVER, SDL_GPU_CULLMODE_NONE, SDL_GPU_FILLMODE_FILL);
        if (pipelines[index] == NULL) FATAL("Pipelines::Init - %s", "Failed to create Prefilter pipeline!");
    }

    void InitBRDF(const Shader* shader, MSAASamples msaa)
    {
        const u8 index = static_cast<u8>(GPUPipeline::IBL_BRDF_Integration);
        // Note: Targeted to RGBA16F because Textures::CreateFramebufferAttachmentHDR defaults to 4 channels
        pipelines[index] = CreateGraphicsPipeline(shader, msaa, SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT, false, false, SDL_GPU_COMPAREOP_NEVER, SDL_GPU_CULLMODE_NONE, SDL_GPU_FILLMODE_FILL);
        if (pipelines[index] == NULL) FATAL("Pipelines::Init - %s", "Failed to create BRDF pipeline!");
    }

    void InitSkybox(const Shader* shader, MSAASamples msaa)
    {
        const u8 index = static_cast<u8>(GPUPipeline::IBL_Skybox);
        // Rendered to the HDR scene buffer (RGBA16F). Depth testing enabled with LESS_OR_EQUAL for the z=w trick. Depth writes disabled.
        pipelines[index] = CreateGraphicsPipeline(shader, msaa, SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT, true, false, SDL_GPU_COMPAREOP_LESS_OR_EQUAL, SDL_GPU_CULLMODE_FRONT, SDL_GPU_FILLMODE_FILL);
        if (pipelines[index] == NULL) FATAL("Pipelines::Init - %s", "Failed to create Skybox pipeline!");
    }
}
