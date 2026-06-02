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

    void Destroy(Mesh& mesh)
    {
        Buffers::Destroy(mesh.buffer_vertex);
        Buffers::Destroy(mesh.buffer_index);
        mesh.vertex_count = 0;
        mesh.index_count = 0;
    }

}
