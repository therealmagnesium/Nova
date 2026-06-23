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
        IBL_EquirectangularToCubemap,
        IBL_Irradiance,
        IBL_Prefilter,
        IBL_BRDF_Integration,
        IBL_Skybox,
        _Length
    };

    struct PipelineShaderInfo
    {
        Shader* outdoor_meshes = NULL;
        Shader* outdoor_meshes_skinned = NULL;
        Shader* indoor_meshes = NULL;
        Shader* wireframe_meshes = NULL;
        Shader* post_processing = NULL;
        Shader* ibl_equirectangular_to_cubemap = NULL;
        Shader* ibl_irradiance = NULL;
        Shader* ibl_prefilter = NULL;
        Shader* ibl_brdf = NULL;
        Shader* ibl_skybox = NULL;
    };

    namespace Pipelines
    {
        void Init(const PipelineShaderInfo& shader_info);
        void Shutdown();
        void Bind(GPUPipeline pipeline, const RenderPassHandle render_pass);
        void ResetBindingCache();
    }
}
