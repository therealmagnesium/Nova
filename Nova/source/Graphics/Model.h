#pragma once
#include <filesystem>
#include <vector>

namespace Nova
{
    struct Material;
    struct Mesh;

    struct Model
    {
        std::vector<Material> materials;
        std::vector<Mesh> meshes;

        /*
        Model() = default;
        Model(const Model&) = delete;
        Model& operator=(const Model&) = delete;
        Model(Model&&) = default;
        Model& operator=(Model&&) = default;*/
    };

    inline const Model Stub_Model;

    namespace Models
    {
        Model Load(const std::filesystem::path& path);
        void Unload(Model& model);
    }
}
