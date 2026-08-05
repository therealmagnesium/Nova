#pragma once
#include "Core/Asset.h"
#include "Graphics/Animation.h"
#include "Graphics/Mesh.h"

#include <filesystem>
#include <vector>

namespace Nova
{
    struct Model : public Asset
    {
        std::vector<Mesh> meshes;
        std::vector<Material> materials;
    };

    struct AnimatedModel : public Asset
    {
        Skeleton skeleton;
        std::vector<Mesh> meshes;
        std::vector<Material> materials;
    };

    namespace Models
    {
        Model Load(const std::filesystem::path& path);
        void Unload(Model& model);

        AnimatedModel LoadAnimated(const std::filesystem::path& path);
        void UnloadAnimated(AnimatedModel& model);
    }
}
