#include "Game.h"

#include <Nova.h>
#include <SDL3/SDL_gpu.h>

using namespace Nova::Core;
using namespace Nova::Graphics;

struct GameState
{
    GPUBuffer vertex_buffer;
    GPUBuffer index_buffer;
};

static GameState state;

namespace Game
{
    void OnCreate()
    {
        Vertex vertices[4] = {
            (Vertex){.position = glm::vec3(-0.5f, 0.5f, 0.f), .color = glm::vec4(1.f, 0.f, 0.f, 1.f)},
            (Vertex){.position = glm::vec3(0.5f, 0.5f, 0.f), .color = glm::vec4(1.f, 1.f, 0.f, 1.f)},
            (Vertex){.position = glm::vec3(-0.5f, -0.5f, 0.f), .color = glm::vec4(0.f, 0.f, 1.f, 1.f)},
            (Vertex){.position = glm::vec3(0.5f, -0.5f, 0.f), .color = glm::vec4(0.f, 1.f, 0.f, 1.f)},
        };

        u16 indices[6] = {
            0, 2, 1, // Left side
            1, 2, 3  // Right side
        };

        state.vertex_buffer = Buffers::Create(GPUBufferType::Vertex, sizeof(Vertex) * LEN(vertices));
        state.index_buffer = Buffers::Create(GPUBufferType::Index, sizeof(u16) * LEN(indices));

        Buffers::Upload(state.vertex_buffer, vertices, sizeof(Vertex) * LEN(vertices));
        Buffers::Upload(state.index_buffer, indices, sizeof(u16) * LEN(indices));
    }

    void OnEvent() {}
    void OnUpdate() {}

    void OnRender()
    {
        SDL_GPURenderPass* render_pass = (SDL_GPURenderPass*)Renderer::GetRenderPass();
        Buffers::Bind(state.vertex_buffer);
        Buffers::Bind(state.index_buffer);
        SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
    }

    void OnRenderUI() {}
    void OnShutdown()
    {
        Buffers::Destroy(state.vertex_buffer);
        Buffers::Destroy(state.index_buffer);
    }
}
