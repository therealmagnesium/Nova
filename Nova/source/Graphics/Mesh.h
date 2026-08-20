#pragma once
#include "Core/Base.h"
#include "Graphics/Buffers.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Texture.h"

#include <glm/glm.hpp>

namespace Nova
{
    enum class PrimitiveMesh : u8
    {
        None = 0,
        Quad,
        Cube,
        Sphere,
        Plane,
        Cone,
        Pyramid,
        Torus,
        _Length,
    };

    struct Vertex
    {
        glm::vec3 position; // Attribute 0
        glm::vec3 normal;   // Attribute 1
        glm::vec2 uv;       // Attribute 2
        glm::vec3 tangent;  // Attribute 3
    };

    struct VertexSkinned
    {
        glm::vec3 position;                      // Attribute 0
        glm::vec3 normal;                        // Attribute 1
        glm::vec2 uv;                            // Attribute 2
        glm::vec3 tangent;                       // Attribute 3
        glm::uvec4 bone_ids = glm::uvec4(0);     // Attribute 4
        glm::vec4 bone_weights = glm::vec4(0.f); // Attribute 5
    };

    struct Material
    {
        glm::vec4 albedo = glm::vec4(1.f);
        Texture texture_albedo = Stub_Texture;
        Texture texture_normal = Stub_Texture;
        Texture texture_metallic = Stub_Texture;
        Texture texture_roughness = Stub_Texture;
        float metallic = 0.f;
        float roughness = 1.f;
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

    inline const Material Stub_Material;

    namespace Meshes
    {
        Mesh Create(const Vertex* vertices, u32 vertex_count, const u32* indices, u32 index_count);
        Mesh CreateSkinned(const VertexSkinned* vertices, u32 vertex_count, const u32* indices, u32 index_count);
        Mesh GenerateQuad();
        Mesh GenerateCube();
        Mesh GenerateSphere(u32 segment_count, u32 ring_count);
        Mesh GeneratePlane();
        Mesh GenerateCone();
        Mesh GeneratePyramid();
        Mesh GenerateTorus();
        glm::mat4 CalculateTransform(const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.f), const glm::vec3& scale = glm::vec3(1.f));
        void Destroy(Mesh& mesh);
    }
}
