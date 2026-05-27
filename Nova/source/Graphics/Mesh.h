#pragma once
#include "Core/Base.h"
#include "Graphics/Buffers.h"

#include <vector>
#include <glm/glm.hpp>

namespace Nova
{
    struct Texture;

    struct Vertex
    {
        glm::vec3 position; // Attribute 0
        glm::vec3 normal;   // Attribute 1
        glm::vec2 uv;       // Attribute 2
    };

    struct Material
    {
        glm::vec4 albedo = glm::vec4(1.f);
        Texture* albedo_texture = NULL;
    };

    struct Mesh
    {
        std::vector<Vertex> vertices;
        std::vector<u32> indices;
        GPUBuffer buffer_vertex;
        GPUBuffer buffer_index;
        u32 material_index = 0;

        /*
        Mesh() = default;
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&&) = default;
        Mesh& operator=(Mesh&&) = default;*/
    };

    inline const Mesh Stub_Mesh;
    inline const Material Stub_Material;

    namespace Meshes
    {
        Mesh Create(const Vertex* vertices, u32 vertex_count, const u32* indices, u32 index_count);
        Mesh GenerateQuad();
        void Destroy(Mesh& mesh);
    }
}
