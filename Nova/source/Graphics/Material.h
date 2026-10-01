#pragma once
#include "Graphics/Texture.h"

#include <glm/vec4.hpp>
#include <filesystem>

namespace Nova
{
    struct Material : public Asset
    {
        glm::vec4 albedo = glm::vec4(1.f);
        Texture texture_albedo = Stub_Texture;
        Texture texture_normal = Stub_Texture;
        Texture texture_metallic = Stub_Texture;
        Texture texture_roughness = Stub_Texture;
        float metallic = 0.f;
        float roughness = 1.f;

        inline AssetType GetType() const override { return AssetType::Material; }
    };

    inline const Material Stub_Material;

    namespace Materials
    {
        Material Import(const std::filesystem::path& path);
        void Export(const std::filesystem::path& path, const Material& material = Stub_Material);
    }
}
