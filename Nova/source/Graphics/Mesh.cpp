#include "Graphics/Mesh.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

namespace Nova::Meshes
{
    Mesh Create(const Vertex* vertices, u32 vertex_count, const u32* indices, u32 index_count)
    {
        if (vertices == NULL || indices == NULL)
            return {};

        const u64 size_vertex_buffer = sizeof(Vertex) * vertex_count;
        const u64 size_index_buffer = sizeof(u32) * index_count;

        Mesh mesh;

        mesh.buffer_vertex = Buffers::Create(GPUBufferType::Vertex, size_vertex_buffer);
        mesh.buffer_index = Buffers::Create(GPUBufferType::Index, size_index_buffer);

        Buffers::Upload(mesh.buffer_vertex, vertices, size_vertex_buffer);
        Buffers::Upload(mesh.buffer_index, indices, size_index_buffer);

        mesh.vertex_count = vertex_count;
        mesh.index_count = index_count;
        mesh.pipeline = GPUPipeline::OutdoorMeshes;

        return mesh;
    }

    Mesh CreateSkinned(const VertexSkinned* vertices, u32 vertex_count, const u32* indices, u32 index_count)
    {
        if (vertices == NULL || indices == NULL)
            return {};

        const u64 size_vertex_buffer = sizeof(VertexSkinned) * vertex_count;
        const u64 size_index_buffer = sizeof(u32) * index_count;

        Mesh mesh;

        mesh.buffer_vertex = Buffers::Create(GPUBufferType::Vertex, size_vertex_buffer);
        mesh.buffer_index = Buffers::Create(GPUBufferType::Index, size_index_buffer);

        Buffers::Upload(mesh.buffer_vertex, vertices, size_vertex_buffer);
        Buffers::Upload(mesh.buffer_index, indices, size_index_buffer);

        mesh.vertex_count = vertex_count;
        mesh.index_count = index_count;
        mesh.pipeline = GPUPipeline::OutdoorMeshesSkinned;

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

        return Create(vertices, LEN(vertices), indices, LEN(indices));
    }

    Mesh GenerateSphere(u32 segment_count, u32 ring_count)
    {
        // Sanity bounds check for topology generation
        if (segment_count < 3 || ring_count < 2)
            return {};

        std::vector<Vertex> vertices;
        std::vector<u32> indices;

        // Reserve memory upfront to eliminate runtime reallocations
        const u32 vertex_count = (ring_count + 1) * (segment_count + 1);
        const u32 index_count = ring_count * segment_count * 6;
        vertices.reserve(vertex_count);
        indices.reserve(index_count);

        // 1. Vertex Generation
        for (u32 y = 0; y <= ring_count; ++y)
        {
            // Polar angle (phi) goes from 0 (Top Pole) to PI (Bottom Pole)
            const float phi = glm::pi<float>() * static_cast<float>(y) / static_cast<float>(ring_count);
            const float sin_phi = std::sin(phi);
            const float cos_phi = std::cos(phi);

            for (u32 x = 0; x <= segment_count; ++x)
            {
                // Azimuthal angle (theta) goes from 0 to 2*PI
                const float theta = glm::two_pi<float>() * static_cast<float>(x) / static_cast<float>(segment_count);
                const float sin_theta = std::sin(theta);
                const float cos_theta = std::cos(theta);

                // Compute Unit Sphere Coordinates
                const glm::vec3 position = glm::vec3(
                    sin_phi * cos_theta,
                    cos_phi, // Y-axis is Up
                    sin_phi * sin_theta
                );

                // For a unit sphere centered at origin, the normal vector equals the position vector
                const glm::vec3 normal = position;

                // Map texture coordinates linearly based on angles
                const glm::vec2 uv = glm::vec2(
                    static_cast<float>(x) / static_cast<float>(segment_count),
                    1.f - (static_cast<float>(y) / static_cast<float>(ring_count)) // Flip Y for typical API layouts
                );

                // Tangent vector represents the direction of increasing U parameter (parallel to latitude ring_count)
                const glm::vec3 tangent = glm::vec3(
                    -sin_theta,
                    0.f,
                    cos_theta
                );

                vertices.emplace_back((Vertex){position, normal, uv, tangent});
            }
        }

        // 2. Index / Topology Generation
        for (u32 y = 0; y < ring_count; ++y)
        {
            for (u32 x = 0; x < segment_count; ++x)
            {
                // Row stride multiplier matches our vertex iteration boundaries
                const u32 current_row_start = y * (segment_count + 1);
                const u32 next_row_start = (y + 1) * (segment_count + 1);

                const u32 top_left = current_row_start + x;
                const u32 top_right = top_left + 1;
                const u32 bot_left = next_row_start + x;
                const u32 bot_right = bot_left + 1;

                // Triangle 1: Top Left -> Top Right -> Bottom Left
                indices.emplace_back(top_left);
                indices.emplace_back(top_right);
                indices.emplace_back(bot_left);

                // Triangle 2: Top Right -> Bottom Right -> Bottom Left
                indices.emplace_back(top_right);
                indices.emplace_back(bot_right);
                indices.emplace_back(bot_left);
            }
        }

        // 3. Dispatch to GPU Buffers via existing internal framework
        Mesh mesh = Create(vertices.data(), static_cast<u32>(vertices.size()), indices.data(), static_cast<u32>(indices.size()));
        return mesh;
    }

    glm::mat4 CalculateTransform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
    {
        glm::mat4 transform = glm::mat4(1.f);
        transform = glm::translate(transform, position);
        transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1.f, 0.f, 0.f));
        transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0.f, 1.f, 0.f));
        transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0.f, 0.f, 1.f));
        transform = glm::scale(transform, scale);
        return transform;
    }

    void Destroy(Mesh& mesh)
    {
        Buffers::Destroy(mesh.buffer_vertex);
        Buffers::Destroy(mesh.buffer_index);
        mesh.vertex_count = 0;
        mesh.index_count = 0;
    }

}
