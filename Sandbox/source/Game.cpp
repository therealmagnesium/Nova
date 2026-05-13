#include "Game.h"

#include <Nova.h>
#include <SDL3/SDL_gpu.h>

using namespace Nova::Core;
using namespace Nova::Graphics;

struct GameState
{
    GPUBuffer* vertex_buffer = NULL;
};

static GameState state;

namespace Game
{
    void OnCreate()
    {
        Vertex vertices[3] = {
            (Vertex){.position = glm::vec3(-0.5f, -0.5f, 0.f), .color = glm::vec4(1.f, 0.f, 0.f, 1.f)},
            (Vertex){.position = glm::vec3(0.f, 0.5f, 0.f), .color = glm::vec4(0.f, 1.f, 0.f, 1.f)},
            (Vertex){.position = glm::vec3(0.5f, -0.5f, 0.f), .color = glm::vec4(0.f, 0.f, 1.f, 1.f)},
        };

        state.vertex_buffer = Buffers::Create(GPUBufferType::Vertex, sizeof(Vertex) * LEN(vertices));
        Buffers::Upload(state.vertex_buffer, vertices, sizeof(Vertex) * LEN(vertices));
    }

    void OnEvent() {}
    void OnUpdate() {}

    void OnRender()
    {
        SDL_GPURenderPass* render_pass = (SDL_GPURenderPass*)Renderer::GetRenderPass();
        Buffers::Bind(state.vertex_buffer);
        SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
    }

    void OnRenderUI() {}
    void OnShutdown() { Buffers::Destroy(state.vertex_buffer); }
}
