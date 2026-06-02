#pragma once
#include "Core/Base.h"
#include "Graphics/Buffers.h"
#include "Graphics/Pipeline.h"

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
        float metallic = 0.f;
        float roughness = 0.f;
    };

    struct Mesh
    {
        GPUBuffer buffer_vertex;
        GPUBuffer buffer_index;
        u64 vertex_count = 0;
        u64 index_count = 0;
        u32 material_index = 0;
        GPUPipeline pipeline = GPUPipeline::OutdoorMeshes;
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
