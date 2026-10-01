#pragma once
#include "Core/Asset.h"
#include "Graphics/Animation.h"
#include "Graphics/Material.h"
#include "Graphics/Mesh.h"

#include <filesystem>
#include <vector>

namespace Nova
{
    struct Model : public Asset
    {
        std::vector<Mesh> meshes;
        std::vector<Material> materials;

        inline AssetType GetType() const override { return AssetType::Model; }
    };

    struct AnimatedModel : public Asset
    {
        Skeleton skeleton;
        std::vector<Mesh> meshes;
        std::vector<Material> materials;

        inline AssetType GetType() const override { return AssetType::ModelAnimated; }
    };

    namespace Models
    {
        Model Load(const std::filesystem::path& path);
        void Unload(Model& model);

        AnimatedModel LoadAnimated(const std::filesystem::path& path);
        void UnloadAnimated(AnimatedModel& model);
    }
}
