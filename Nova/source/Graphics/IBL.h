#pragma once
#include "Graphics/Texture.h"

namespace Nova
{
    struct EnvironmentMap
    {
        Texture environment;
        Texture irradiance;
        Texture prefilter;
        Texture brdf_lut;

        inline bool IsValid() const { return environment.IsValid(); }
    };

    inline const EnvironmentMap Stub_EnvironmentMap;

    namespace IBL
    {
        /**
         * @brief Loads an .hdr equirectangular image and bakes all three cubemaps + LUT
         * @param path The path of the HDRI to bake onto the cubemaps and lookup table
         * @return The baked environment map that can drawn onto a skybox with Renderer::DrawSkybox*/
        EnvironmentMap BakeFromHDRI(const std::filesystem::path& path);
        void Free(EnvironmentMap& map);
    }
}
