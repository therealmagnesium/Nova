#include "Graphics/Mesh.h"

namespace Nova::Meshes
{
    Mesh Create(const Vertex* vertices, u32 vertex_count, const u32* indices, u32 index_count)
    {
        if (vertices == NULL || indices == NULL)
            return Stub_Mesh;

        const u64 size_vertex_buffer = sizeof(Vertex) * vertex_count;
        const u64 size_index_buffer = sizeof(u32) * index_count;

        Mesh mesh;

        mesh.buffer_vertex = Buffers::Create(GPUBufferType::Vertex, size_vertex_buffer);
        mesh.buffer_index = Buffers::Create(GPUBufferType::Index, size_index_buffer);

        Buffers::Upload(mesh.buffer_vertex, vertices, size_vertex_buffer);
        Buffers::Upload(mesh.buffer_index, indices, size_index_buffer);

        mesh.vertex_count = vertex_count;
        mesh.index_count = index_count;

        return mesh;
    }

    Mesh GenerateQuad()
    {
        const Vertex vertices[4] = {
            (Vertex){.position = glm::vec3(-1.f, 1.f, 0.f), .normal = glm::vec3(0.f, 0.f, 1.f), .uv = glm::vec2(0.f, 1.f)},
            (Vertex){.position = glm::vec3(1.f, 1.f, 0.f), .normal = glm::vec3(0.f, 0.f, 1.f), .uv = glm::vec2(1.f, 1.f)},
            (Vertex){.position = glm::vec3(-1.f, -1.f, 0.f), .normal = glm::vec3(0.f, 0.f, 1.f), .uv = glm::vec2(0.f, 0.f)},
            (Vertex){.position = glm::vec3(1.f, -1.f, 0.f), .normal = glm::vec3(0.f, 0.f, 1.f), .uv = glm::vec2(1.f, 0.f)},
        };

        const u32 indices[6] = {
            0, 2, 1, // Left side
            1, 2, 3  // Right side
        };

        return Create(vertices, LEN(vertices), indices, LEN(indices));
    }

    Mesh GenerateCube()
    {
        // 24 vertices (4 per face for 6 faces) to ensure unique normals and UVs per face
        const Vertex vertices[24] = {
            // Front Face (Normal: 0, 0, 1)
            {.position = glm::vec3(-1.f, 1.f, 1.f), .normal = glm::vec3(0.f, 0.f, 1.f), .uv = glm::vec2(0.f, 1.f)},  // Top-Left
            {.position = glm::vec3(1.f, 1.f, 1.f), .normal = glm::vec3(0.f, 0.f, 1.f), .uv = glm::vec2(1.f, 1.f)},   // Top-Right
            {.position = glm::vec3(-1.f, -1.f, 1.f), .normal = glm::vec3(0.f, 0.f, 1.f), .uv = glm::vec2(0.f, 0.f)}, // Bottom-Left
            {.position = glm::vec3(1.f, -1.f, 1.f), .normal = glm::vec3(0.f, 0.f, 1.f), .uv = glm::vec2(1.f, 0.f)},  // Bottom-Right

            // Back Face (Normal: 0, 0, -1)
            {.position = glm::vec3(1.f, 1.f, -1.f), .normal = glm::vec3(0.f, 0.f, -1.f), .uv = glm::vec2(0.f, 1.f)},
            {.position = glm::vec3(-1.f, 1.f, -1.f), .normal = glm::vec3(0.f, 0.f, -1.f), .uv = glm::vec2(1.f, 1.f)},
            {.position = glm::vec3(1.f, -1.f, -1.f), .normal = glm::vec3(0.f, 0.f, -1.f), .uv = glm::vec2(0.f, 0.f)},
            {.position = glm::vec3(-1.f, -1.f, -1.f), .normal = glm::vec3(0.f, 0.f, -1.f), .uv = glm::vec2(1.f, 0.f)},

            // Left Face (Normal: -1, 0, 0)
            {.position = glm::vec3(-1.f, 1.f, -1.f), .normal = glm::vec3(-1.f, 0.f, 0.f), .uv = glm::vec2(0.f, 1.f)},
            {.position = glm::vec3(-1.f, 1.f, 1.f), .normal = glm::vec3(-1.f, 0.f, 0.f), .uv = glm::vec2(1.f, 1.f)},
            {.position = glm::vec3(-1.f, -1.f, -1.f), .normal = glm::vec3(-1.f, 0.f, 0.f), .uv = glm::vec2(0.f, 0.f)},
            {.position = glm::vec3(-1.f, -1.f, 1.f), .normal = glm::vec3(-1.f, 0.f, 0.f), .uv = glm::vec2(1.f, 0.f)},

            // Right Face (Normal: 1, 0, 0)
            {.position = glm::vec3(1.f, 1.f, 1.f), .normal = glm::vec3(1.f, 0.f, 0.f), .uv = glm::vec2(0.f, 1.f)},
            {.position = glm::vec3(1.f, 1.f, -1.f), .normal = glm::vec3(1.f, 0.f, 0.f), .uv = glm::vec2(1.f, 1.f)},
            {.position = glm::vec3(1.f, -1.f, 1.f), .normal = glm::vec3(1.f, 0.f, 0.f), .uv = glm::vec2(0.f, 0.f)},
            {.position = glm::vec3(1.f, -1.f, -1.f), .normal = glm::vec3(1.f, 0.f, 0.f), .uv = glm::vec2(1.f, 0.f)},

            // Top Face (Normal: 0, 1, 0)
            {.position = glm::vec3(-1.f, 1.f, -1.f), .normal = glm::vec3(0.f, 1.f, 0.f), .uv = glm::vec2(0.f, 1.f)},
            {.position = glm::vec3(1.f, 1.f, -1.f), .normal = glm::vec3(0.f, 1.f, 0.f), .uv = glm::vec2(1.f, 1.f)},
            {.position = glm::vec3(-1.f, 1.f, 1.f), .normal = glm::vec3(0.f, 1.f, 0.f), .uv = glm::vec2(0.f, 0.f)},
            {.position = glm::vec3(1.f, 1.f, 1.f), .normal = glm::vec3(0.f, 1.f, 0.f), .uv = glm::vec2(1.f, 0.f)},

            // Bottom Face (Normal: 0, -1, 0)
            {.position = glm::vec3(-1.f, -1.f, 1.f), .normal = glm::vec3(0.f, -1.f, 0.f), .uv = glm::vec2(0.f, 1.f)},
            {.position = glm::vec3(1.f, -1.f, 1.f), .normal = glm::vec3(0.f, -1.f, 0.f), .uv = glm::vec2(1.f, 1.f)},
            {.position = glm::vec3(-1.f, -1.f, -1.f), .normal = glm::vec3(0.f, -1.f, 0.f), .uv = glm::vec2(0.f, 0.f)},
            {.position = glm::vec3(1.f, -1.f, -1.f), .normal = glm::vec3(0.f, -1.f, 0.f), .uv = glm::vec2(1.f, 0.f)}
        };

        // 36 indices mapping out the 12 triangles (2 per face)
        const u32 indices[36] = {
            0, 2, 1, 1, 2, 3,       // Front
            4, 6, 5, 5, 6, 7,       // Back
            8, 10, 9, 9, 10, 11,    // Left
            12, 14, 13, 13, 14, 15, // Right
            16, 18, 17, 17, 18, 19, // Top
            20, 22, 21, 21, 22, 23  // Bottom
        };

        Mesh mesh = Create(vertices, LEN(vertices), indices, LEN(indices));
        mesh.pipeline = GPUPipeline::OutdoorMeshes;
        return mesh;
    }

    void Destroy(Mesh& mesh)
    {
        Buffers::Destroy(mesh.buffer_vertex);
        Buffers::Destroy(mesh.buffer_index);
        mesh.vertex_count = 0;
        mesh.index_count = 0;
    }

}
