#pragma once
#include "Graphics/Animation.h"
#include "Graphics/Mesh.h"

#include <filesystem>
#include <vector>

namespace Nova
{
    struct Model
    {
        std::vector<Mesh> meshes;
        std::vector<Material> materials;
    };

    struct AnimatedModel
    {
        Skeleton skeleton;
        std::vector<Mesh> meshes;
        std::vector<Material> materials;
    };

    inline const Model Stub_Model;
    inline const AnimatedModel Stub_AnimatedModel;

    namespace Models
    {
        Model Load(const std::filesystem::path& path);
        void Unload(Model& model);

        AnimatedModel LoadAnimated(const std::filesystem::path& path);
        void UnloadAnimated(AnimatedModel& model);
    }
}
