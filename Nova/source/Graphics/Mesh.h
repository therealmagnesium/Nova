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
        glm::vec3 position;
        glm::vec4 color;
        glm::vec2 uv;
    };

    struct Material
    {
        glm::vec4 albedo = glm::vec4(1.f);
        const Texture* albedo_texture = NULL;
    };

    struct Mesh
    {
        std::vector<Vertex> vertices;
        std::vector<u16> indices;
        GPUBuffer buffer_vertex;
        GPUBuffer buffer_index;
    };

    inline const Mesh Stub_Mesh;
    inline const Material Stub_Material;

    namespace Meshes
    {
        Mesh Create(const Vertex* vertices, u32 vertex_count, const u16* indices, u32 index_count);
        Mesh GenerateQuad();
        void Destroy(Mesh& mesh);
    }
}
