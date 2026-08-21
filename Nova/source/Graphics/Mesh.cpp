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

    void Destroy(Mesh& mesh)
    {
        Buffers::Destroy(mesh.buffer_vertex);
        Buffers::Destroy(mesh.buffer_index);
        mesh.vertex_count = 0;
        mesh.index_count = 0;
    }

    Mesh GenerateQuad()
    {
        const Vertex vertices[4] = {
            (Vertex){ .position = glm::vec3(-1.f, 1.f, 0.f), .normal = glm::vec3(0.f, 0.f, 1.f), .uv = glm::vec2(0.f, 1.f), .tangent = glm::vec3(1.f, 0.f, 0.f) },
            (Vertex){ .position = glm::vec3(1.f, 1.f, 0.f), .normal = glm::vec3(0.f, 0.f, 1.f), .uv = glm::vec2(1.f, 1.f), .tangent = glm::vec3(1.f, 0.f, 0.f) },
            (Vertex){ .position = glm::vec3(-1.f, -1.f, 0.f), .normal = glm::vec3(0.f, 0.f, 1.f), .uv = glm::vec2(0.f, 0.f), .tangent = glm::vec3(1.f, 0.f, 0.f) },
            (Vertex){ .position = glm::vec3(1.f, -1.f, 0.f), .normal = glm::vec3(0.f, 0.f, 1.f), .uv = glm::vec2(1.f, 0.f), .tangent = glm::vec3(1.f, 0.f, 0.f) },
        };
        const u32 indices[6] = {
            0, 2, 1, // Left side
            1, 2, 3  // Right side
        };

        return Create(vertices, LEN(vertices), indices, LEN(indices));
    }

    Mesh GenerateCube()
    {
        // 24 vertices (4 per face for 6 faces) to ensure unique normals, UVs, and tangents per face.
        // Each face's tangent is constant across its 4 vertices - it's simply the world-space
        // direction of increasing U for that face's planar UV mapping (same convention documented
        // on GenerateSphere's tangent below), and must stay perpendicular to that face's normal or
        // the TBN basis PBR_fs.glsl builds from it (T, cross(N,T), N) is invalid.
        const Vertex vertices[24] = {
            // Front Face (Normal: 0, 0, 1) - U increases with +X
            { .position = glm::vec3(-1.f, 1.f, 1.f), .normal = glm::vec3(0.f, 0.f, 1.f), .uv = glm::vec2(0.f, 1.f), .tangent = glm::vec3(1.f, 0.f, 0.f) },  // Top-Left
            { .position = glm::vec3(1.f, 1.f, 1.f), .normal = glm::vec3(0.f, 0.f, 1.f), .uv = glm::vec2(1.f, 1.f), .tangent = glm::vec3(1.f, 0.f, 0.f) },   // Top-Right
            { .position = glm::vec3(-1.f, -1.f, 1.f), .normal = glm::vec3(0.f, 0.f, 1.f), .uv = glm::vec2(0.f, 0.f), .tangent = glm::vec3(1.f, 0.f, 0.f) }, // Bottom-Left
            { .position = glm::vec3(1.f, -1.f, 1.f), .normal = glm::vec3(0.f, 0.f, 1.f), .uv = glm::vec2(1.f, 0.f), .tangent = glm::vec3(1.f, 0.f, 0.f) },  // Bottom-Right

            // Back Face (Normal: 0, 0, -1) - U increases with -X
            { .position = glm::vec3(1.f, 1.f, -1.f), .normal = glm::vec3(0.f, 0.f, -1.f), .uv = glm::vec2(0.f, 1.f), .tangent = glm::vec3(-1.f, 0.f, 0.f) },
            { .position = glm::vec3(-1.f, 1.f, -1.f), .normal = glm::vec3(0.f, 0.f, -1.f), .uv = glm::vec2(1.f, 1.f), .tangent = glm::vec3(-1.f, 0.f, 0.f) },
            { .position = glm::vec3(1.f, -1.f, -1.f), .normal = glm::vec3(0.f, 0.f, -1.f), .uv = glm::vec2(0.f, 0.f), .tangent = glm::vec3(-1.f, 0.f, 0.f) },
            { .position = glm::vec3(-1.f, -1.f, -1.f), .normal = glm::vec3(0.f, 0.f, -1.f), .uv = glm::vec2(1.f, 0.f), .tangent = glm::vec3(-1.f, 0.f, 0.f) },

            // Left Face (Normal: -1, 0, 0) - U increases with +Z
            { .position = glm::vec3(-1.f, 1.f, -1.f), .normal = glm::vec3(-1.f, 0.f, 0.f), .uv = glm::vec2(0.f, 1.f), .tangent = glm::vec3(0.f, 0.f, 1.f) },
            { .position = glm::vec3(-1.f, 1.f, 1.f), .normal = glm::vec3(-1.f, 0.f, 0.f), .uv = glm::vec2(1.f, 1.f), .tangent = glm::vec3(0.f, 0.f, 1.f) },
            { .position = glm::vec3(-1.f, -1.f, -1.f), .normal = glm::vec3(-1.f, 0.f, 0.f), .uv = glm::vec2(0.f, 0.f), .tangent = glm::vec3(0.f, 0.f, 1.f) },
            { .position = glm::vec3(-1.f, -1.f, 1.f), .normal = glm::vec3(-1.f, 0.f, 0.f), .uv = glm::vec2(1.f, 0.f), .tangent = glm::vec3(0.f, 0.f, 1.f) },

            // Right Face (Normal: 1, 0, 0) - U increases with -Z
            { .position = glm::vec3(1.f, 1.f, 1.f), .normal = glm::vec3(1.f, 0.f, 0.f), .uv = glm::vec2(0.f, 1.f), .tangent = glm::vec3(0.f, 0.f, -1.f) },
            { .position = glm::vec3(1.f, 1.f, -1.f), .normal = glm::vec3(1.f, 0.f, 0.f), .uv = glm::vec2(1.f, 1.f), .tangent = glm::vec3(0.f, 0.f, -1.f) },
            { .position = glm::vec3(1.f, -1.f, 1.f), .normal = glm::vec3(1.f, 0.f, 0.f), .uv = glm::vec2(0.f, 0.f), .tangent = glm::vec3(0.f, 0.f, -1.f) },
            { .position = glm::vec3(1.f, -1.f, -1.f), .normal = glm::vec3(1.f, 0.f, 0.f), .uv = glm::vec2(1.f, 0.f), .tangent = glm::vec3(0.f, 0.f, -1.f) },

            // Top Face (Normal: 0, 1, 0) - U increases with +X
            { .position = glm::vec3(-1.f, 1.f, -1.f), .normal = glm::vec3(0.f, 1.f, 0.f), .uv = glm::vec2(0.f, 1.f), .tangent = glm::vec3(1.f, 0.f, 0.f) },
            { .position = glm::vec3(1.f, 1.f, -1.f), .normal = glm::vec3(0.f, 1.f, 0.f), .uv = glm::vec2(1.f, 1.f), .tangent = glm::vec3(1.f, 0.f, 0.f) },
            { .position = glm::vec3(-1.f, 1.f, 1.f), .normal = glm::vec3(0.f, 1.f, 0.f), .uv = glm::vec2(0.f, 0.f), .tangent = glm::vec3(1.f, 0.f, 0.f) },
            { .position = glm::vec3(1.f, 1.f, 1.f), .normal = glm::vec3(0.f, 1.f, 0.f), .uv = glm::vec2(1.f, 0.f), .tangent = glm::vec3(1.f, 0.f, 0.f) },

            // Bottom Face (Normal: 0, -1, 0) - U increases with +X
            { .position = glm::vec3(-1.f, -1.f, 1.f), .normal = glm::vec3(0.f, -1.f, 0.f), .uv = glm::vec2(0.f, 1.f), .tangent = glm::vec3(1.f, 0.f, 0.f) },
            { .position = glm::vec3(1.f, -1.f, 1.f), .normal = glm::vec3(0.f, -1.f, 0.f), .uv = glm::vec2(1.f, 1.f), .tangent = glm::vec3(1.f, 0.f, 0.f) },
            { .position = glm::vec3(-1.f, -1.f, -1.f), .normal = glm::vec3(0.f, -1.f, 0.f), .uv = glm::vec2(0.f, 0.f), .tangent = glm::vec3(1.f, 0.f, 0.f) },
            { .position = glm::vec3(1.f, -1.f, -1.f), .normal = glm::vec3(0.f, -1.f, 0.f), .uv = glm::vec2(1.f, 0.f), .tangent = glm::vec3(1.f, 0.f, 0.f) }
        };

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
                    sin_phi * sin_theta);

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
                    cos_theta);

                vertices.emplace_back((Vertex){ position, normal, uv, tangent });
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

    Mesh GeneratePlane()
    {
        // Facing up (+Y), U increases with +X, V increases with -Z
        const Vertex vertices[4] = {
            (Vertex){ .position = glm::vec3(-1.f, 0.f, -1.f), .normal = glm::vec3(0.f, 1.f, 0.f), .uv = glm::vec2(0.f, 1.f), .tangent = glm::vec3(1.f, 0.f, 0.f) }, // Top-Left
            (Vertex){ .position = glm::vec3(1.f, 0.f, -1.f), .normal = glm::vec3(0.f, 1.f, 0.f), .uv = glm::vec2(1.f, 1.f), .tangent = glm::vec3(1.f, 0.f, 0.f) },  // Top-Right
            (Vertex){ .position = glm::vec3(-1.f, 0.f, 1.f), .normal = glm::vec3(0.f, 1.f, 0.f), .uv = glm::vec2(0.f, 0.f), .tangent = glm::vec3(1.f, 0.f, 0.f) },  // Bottom-Left
            (Vertex){ .position = glm::vec3(1.f, 0.f, 1.f), .normal = glm::vec3(0.f, 1.f, 0.f), .uv = glm::vec2(1.f, 0.f), .tangent = glm::vec3(1.f, 0.f, 0.f) }    // Bottom-Right
        };

        const u32 indices[6] = {
            0, 2, 1,
            1, 2, 3
        };

        return Create(vertices, LEN(vertices), indices, LEN(indices));
    }

    Mesh GeneratePyramid()
    {
        // 16 vertices total: 4 for the base, 3 per triangular side (4 sides) to ensure distinct normals
        std::vector<Vertex> vertices;
        std::vector<u32> indices;
        vertices.reserve(16);
        indices.reserve(18);

        // Base (Normal: 0, -1, 0)
        glm::vec3 n_base = glm::vec3(0.f, -1.f, 0.f);
        glm::vec3 t_base = glm::vec3(1.f, 0.f, 0.f);
        vertices.emplace_back((Vertex){ .position = glm::vec3(-1.f, -1.f, 1.f), .normal = n_base, .uv = glm::vec2(0.f, 1.f), .tangent = t_base });
        vertices.emplace_back((Vertex){ .position = glm::vec3(1.f, -1.f, 1.f), .normal = n_base, .uv = glm::vec2(1.f, 1.f), .tangent = t_base });
        vertices.emplace_back((Vertex){ .position = glm::vec3(-1.f, -1.f, -1.f), .normal = n_base, .uv = glm::vec2(0.f, 0.f), .tangent = t_base });
        vertices.emplace_back((Vertex){ .position = glm::vec3(1.f, -1.f, -1.f), .normal = n_base, .uv = glm::vec2(1.f, 0.f), .tangent = t_base });

        indices.insert(indices.end(), { 0, 2, 1, 1, 2, 3 });

        // Sides
        const glm::vec3 tip = glm::vec3(0.f, 1.f, 0.f);
        const glm::vec3 corners[4] = {
            glm::vec3(-1.f, -1.f, 1.f), // Front-Left
            glm::vec3(1.f, -1.f, 1.f),  // Front-Right
            glm::vec3(1.f, -1.f, -1.f), // Back-Right
            glm::vec3(-1.f, -1.f, -1.f) // Back-Left
        };

        for (u32 i = 0; i < 4; i++)
        {
            glm::vec3 c1 = corners[i];
            glm::vec3 c2 = corners[(i + 1) % 4];

            glm::vec3 normal = glm::normalize(glm::cross(c2 - c1, tip - c1));
            glm::vec3 tangent = glm::normalize(c2 - c1);

            u32 idx = static_cast<u32>(vertices.size());
            vertices.emplace_back((Vertex){ .position = c1, .normal = normal, .uv = glm::vec2(0.f, 0.f), .tangent = tangent });
            vertices.emplace_back((Vertex){ .position = c2, .normal = normal, .uv = glm::vec2(1.f, 0.f), .tangent = tangent });
            vertices.emplace_back((Vertex){ .position = tip, .normal = normal, .uv = glm::vec2(0.5f, 1.f), .tangent = tangent });

            indices.insert(indices.end(), { idx, idx + 1, idx + 2 });
        }

        return Create(vertices.data(), static_cast<u32>(vertices.size()), indices.data(), static_cast<u32>(indices.size()));
    }

    Mesh GenerateCone()
    {
        std::vector<Vertex> vertices;
        std::vector<u32> indices;

        const u32 segments = 32;
        vertices.reserve((segments + 1) * 3 + 1);
        indices.reserve(segments * 6);

        const float radius = 1.0f;
        const float height = 2.0f;
        const float half_height = height * 0.5f;

        // Base center
        glm::vec3 n_base = glm::vec3(0.f, -1.f, 0.f);
        glm::vec3 t_base = glm::vec3(1.f, 0.f, 0.f);
        vertices.emplace_back((Vertex){ .position = glm::vec3(0.f, -half_height, 0.f), .normal = n_base, .uv = glm::vec2(0.5f, 0.5f), .tangent = t_base });
        const u32 base_center_idx = 0;

        for (u32 i = 0; i <= segments; i++)
        {
            float theta = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
            float cos_theta = std::cos(theta);
            float sin_theta = std::sin(theta);

            float x = cos_theta * radius;
            float z = sin_theta * radius;

            // Base perimeter
            glm::vec2 uv_base = glm::vec2((cos_theta + 1.f) * 0.5f, (sin_theta + 1.f) * 0.5f);
            vertices.emplace_back((Vertex){ .position = glm::vec3(x, -half_height, z), .normal = n_base, .uv = uv_base, .tangent = t_base });

            if (i > 0)
            {
                // Fixed base winding order: size - 2, size - 1
                indices.insert(indices.end(), { base_center_idx, static_cast<u32>(vertices.size() - 2), static_cast<u32>(vertices.size() - 1) });
            }
        }

        // Side geometry
        u32 side_start_idx = static_cast<u32>(vertices.size());
        for (u32 i = 0; i <= segments; i++)
        {
            float theta = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
            float cos_theta = std::cos(theta);
            float sin_theta = std::sin(theta);

            float x = cos_theta * radius;
            float z = sin_theta * radius;

            // Separate the normals: bottom points outward, top points straight up
            glm::vec3 normal_bottom = glm::normalize(glm::vec3(x, radius * (radius / height), z));
            glm::vec3 normal_top = glm::vec3(0.f, 1.f, 0.f);

            glm::vec3 tangent = glm::normalize(glm::vec3(-sin_theta, 0.f, cos_theta));

            float u = static_cast<float>(i) / static_cast<float>(segments);

            // Apply the distinct normals to their respective vertices
            vertices.emplace_back((Vertex){ .position = glm::vec3(x, -half_height, z), .normal = normal_bottom, .uv = glm::vec2(u, 0.f), .tangent = tangent });
            vertices.emplace_back((Vertex){ .position = glm::vec3(0.f, half_height, 0.f), .normal = normal_top, .uv = glm::vec2(u, 1.f), .tangent = tangent });
        }

        for (u32 i = 0; i < segments; i++)
        {
            u32 bottom_left = side_start_idx + (i * 2);
            u32 top = side_start_idx + (i * 2) + 1;
            u32 bottom_right = side_start_idx + ((i + 1) * 2);

            // Fixed side winding order: bottom_left, top, bottom_right
            indices.insert(indices.end(), { bottom_left, top, bottom_right });
        }

        return Create(vertices.data(), static_cast<u32>(vertices.size()), indices.data(), static_cast<u32>(indices.size()));
    }

    Mesh GenerateTorus()
    {
        std::vector<Vertex> vertices;
        std::vector<u32> indices;

        const u32 main_segments = 32;
        const u32 tube_segments = 16;
        const float main_radius = 1.0f;
        const float tube_radius = 0.25f;

        vertices.reserve((main_segments + 1) * (tube_segments + 1));
        indices.reserve(main_segments * tube_segments * 6);

        for (u32 i = 0; i <= main_segments; i++)
        {
            float u = static_cast<float>(i) / static_cast<float>(main_segments);
            float theta = u * glm::two_pi<float>();
            float cos_theta = std::cos(theta);
            float sin_theta = std::sin(theta);

            for (u32 j = 0; j <= tube_segments; j++)
            {
                float v = static_cast<float>(j) / static_cast<float>(tube_segments);
                float phi = v * glm::two_pi<float>();
                float cos_phi = std::cos(phi);
                float sin_phi = std::sin(phi);

                glm::vec3 position = glm::vec3(
                    (main_radius + tube_radius * cos_phi) * cos_theta,
                    tube_radius * sin_phi,
                    (main_radius + tube_radius * cos_phi) * sin_theta);

                glm::vec3 center = glm::vec3(main_radius * cos_theta, 0.f, main_radius * sin_theta);
                glm::vec3 normal = glm::normalize(position - center);
                glm::vec3 tangent = glm::normalize(glm::vec3(-sin_theta, 0.f, cos_theta));

                vertices.emplace_back((Vertex){ .position = position, .normal = normal, .uv = glm::vec2(u, v), .tangent = tangent });
            }
        }

        for (u32 i = 0; i < main_segments; i++)
        {
            for (u32 j = 0; j < tube_segments; j++)
            {
                u32 next_i = i + 1;
                u32 next_j = j + 1;

                u32 top_left = (i * (tube_segments + 1)) + j;
                u32 bot_left = (next_i * (tube_segments + 1)) + j;
                u32 bot_right = (next_i * (tube_segments + 1)) + next_j;
                u32 top_right = (i * (tube_segments + 1)) + next_j;

                // Fixed quad winding order: top_left, top_right, bot_left & bot_left, top_right, bot_right
                indices.insert(indices.end(), { top_left, top_right, bot_left, bot_left, top_right, bot_right });
            }
        }

        return Create(vertices.data(), static_cast<u32>(vertices.size()), indices.data(), static_cast<u32>(indices.size()));
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
}
