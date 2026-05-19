#include "Game.h"
#include "Graphics/Renderer.h"

#include <Nova.h>
#include <SDL3/SDL_gpu.h>
#include <glm/gtc/matrix_transform.hpp>

using namespace Nova;

struct GameState
{
    Mesh mesh;
    Material material;
    Camera3D camera;
    Texture texture;
};

static GameState state;

namespace Game
{
    void OnCreate()
    {
        state.texture = Textures::Load("Assets/Textures/Grid.png");

        state.mesh = Meshes::GenerateQuad();
        state.material.albedo = glm::vec4(1.f);
        state.material.albedo_texture = &state.texture;

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

    void OnUpdate() { Cameras::UpdateEditor(state.camera, 0.05f, 1.f); }

    void OnRender()
    {
        glm::mat4 transform = glm::mat4(1.f);
        transform = glm::translate(transform, glm::vec3(0.f));
        transform = glm::rotate(transform, glm::radians(0.f), glm::vec3(1.f, 0.f, 0.f));
        transform = glm::rotate(transform, glm::radians(0.f), glm::vec3(0.f, 1.f, 0.f));
        transform = glm::rotate(transform, glm::radians(0.f), glm::vec3(0.f, 0.f, 1.f));
        transform = glm::scale(transform, glm::vec3(1.f));

        Renderer::DrawMesh(state.mesh, transform, state.material);
    }

    void OnRenderUI() {}
    void OnShutdown()
    {
        Meshes::Destroy(state.mesh);
        Textures::Unload(state.texture);
    }
}
