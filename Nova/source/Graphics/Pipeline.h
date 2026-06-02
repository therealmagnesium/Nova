#pragma once
#include "Core/Base.h"
#include "Graphics/RenderPass.h"

namespace Nova
{
    struct Shader;

    enum class GPUPipeline : u8
    {
        OutdoorMeshes = 0,
        OutdoorMeshesSkinned,
        IndoorMeshes,
        WireframeMeshes,
        PostProcessing,
        _Length
    };

    struct PipelineShaderInfo
    {
        Shader* outdoor_meshes = NULL;
        Shader* outdoor_meshes_skinned = NULL;
        Shader* indoor_meshes = NULL;
        Shader* wireframe_meshes = NULL;
        Shader* post_processing = NULL;
    };

    namespace Pipelines
    {
        void Init(const PipelineShaderInfo& shader_info);
        void Shutdown();
        void Bind(GPUPipeline pipeline, const RenderPassHandle render_pass);
        void ResetBindingCache();
    }
}
