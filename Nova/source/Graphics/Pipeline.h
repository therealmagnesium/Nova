#pragma once
#include "Core/Base.h"
#include "Graphics/RenderPass.h"

namespace Nova
{
    struct Shader;

    enum class PipelineType : u8
    {
        OutdoorMeshes = 0,
        OutdoorMeshesSkinned,
        IndoorMeshes,
        WireframeMeshes,
        _Length
    };

    struct PipelineShaderInfo
    {
        Shader* outdoor_meshes = NULL;
        Shader* outdoor_meshes_skinned = NULL;
        Shader* indoor_meshes = NULL;
        Shader* wireframe_meshes = NULL;
    };

    namespace Pipelines
    {
        void Init(const PipelineShaderInfo& shader_info);
        void Bind(PipelineType type, const RenderPassHandle render_pass);
        void Shutdown();
    }
}
