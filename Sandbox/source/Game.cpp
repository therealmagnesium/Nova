#include "Game.h"

#include <Nova.h>
#include <SDL3/SDL_gpu.h>
#include <glm/gtc/matrix_transform.hpp>

using namespace Nova;

struct MVPData
{
    glm::mat4 matrix_model;
    glm::mat4 matrix_view_projection;
};

struct GameState
{
    Camera3D camera;
    Texture texture;
    GPUBuffer vertex_buffer;
    GPUBuffer index_buffer;
};

static GameState state;

namespace Game
{
    void OnCreate()
    {
        state.texture = Textures::Load("Assets/Textures/Grid.png");

        Vertex vertices[4] = {
            (Vertex){.position = glm::vec3(-0.5f, 0.5f, 0.f), .color = glm::vec4(1.f, 0.f, 0.f, 1.f), .uv = glm::vec2(0.f, 1.f)},
            (Vertex){.position = glm::vec3(0.5f, 0.5f, 0.f), .color = glm::vec4(1.f, 1.f, 0.f, 1.f), .uv = glm::vec2(1.f, 1.f)},
            (Vertex){.position = glm::vec3(-0.5f, -0.5f, 0.f), .color = glm::vec4(0.f, 0.f, 1.f, 1.f), .uv = glm::vec2(0.f, 0.f)},
            (Vertex){.position = glm::vec3(0.5f, -0.5f, 0.f), .color = glm::vec4(0.f, 1.f, 0.f, 1.f), .uv = glm::vec2(1.f, 0.f)},
        };

        u16 indices[6] = {
            0, 2, 1, // Left side
            1, 2, 3  // Right side
        };

        state.vertex_buffer = Buffers::Create(GPUBufferType::Vertex, sizeof(Vertex) * LEN(vertices));
        state.index_buffer = Buffers::Create(GPUBufferType::Index, sizeof(u16) * LEN(indices));

        Buffers::Upload(state.vertex_buffer, vertices, sizeof(Vertex) * LEN(vertices));
        Buffers::Upload(state.index_buffer, indices, sizeof(u16) * LEN(indices));

        state.camera.position = glm::vec3(0.f, 0.f, 2.f);
        state.camera.up = glm::vec3(0.f, 1.f, 0.f);
        state.camera.yaw = -90.f;
        state.camera.pitch = 0.f;
        state.camera.fov = 75.f;
        state.camera.clip_near = 0.1f;
        state.camera.clip_far = 100.f;
        Renderer::SetPrimaryCamera(&state.camera);
    }

    void OnEvent()
    {
    }

    void OnUpdate() { Cameras::UpdateEditor(state.camera, 0.1f, 1.f); }

    void OnRender()
    {
        SDL_GPURenderPass* render_pass = (SDL_GPURenderPass*)Renderer::GetRenderPass();
        SDL_GPUCommandBuffer* command_buffer = (SDL_GPUCommandBuffer*)Renderer::GetCommandBuffer();

        Buffers::Bind(state.vertex_buffer);
        Buffers::Bind(state.index_buffer);
        Textures::Bind(state.texture, TextureSampler::PointClamp);

        MVPData mvp_data;
        mvp_data.matrix_model = glm::mat4(1.f);
        mvp_data.matrix_model = glm::translate(mvp_data.matrix_model, glm::vec3(0.f));
        mvp_data.matrix_model = glm::rotate(mvp_data.matrix_model, glm::radians(0.f), glm::vec3(1.f, 0.f, 0.f));
        mvp_data.matrix_model = glm::rotate(mvp_data.matrix_model, glm::radians(0.f), glm::vec3(0.f, 1.f, 0.f));
        mvp_data.matrix_model = glm::rotate(mvp_data.matrix_model, glm::radians(0.f), glm::vec3(0.f, 0.f, 1.f));
        mvp_data.matrix_model = glm::scale(mvp_data.matrix_model, glm::vec3(1.f));
        mvp_data.matrix_view_projection = Renderer::GetMatrixProjection() * Renderer::GetMatrixView();
        SDL_PushGPUVertexUniformData(command_buffer, 0, &mvp_data, sizeof(MVPData));
        SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
    }

    void OnRenderUI() {}
    void OnShutdown()
    {
        Textures::Unload(state.texture);
        Buffers::Destroy(state.vertex_buffer);
        Buffers::Destroy(state.index_buffer);
    }
}
