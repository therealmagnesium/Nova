#include "Game.h"
#include "Graphics/Renderer.h"

#include <Nova.h>
#include <SDL3/SDL_gpu.h>
#include <glm/gtc/matrix_transform.hpp>

using namespace Nova;

struct GameState
{
    Model model_xbot;
    Model model_ybot;
    Camera3D camera;
    bool is_wireframe = false;
};

static GameState state;

namespace Game
{
    void ResetEditorCamera();

    void OnCreate()
    {
        state.model_xbot = Models::Load("Assets/Models/X-Bot.fbx");
        state.model_ybot = Models::Load("Assets/Models/Y-Bot.fbx");

        ResetEditorCamera();
        Renderer::SetPrimaryCamera(&state.camera);
    }

    void OnEvent()
    {
        if (Input::IsKeyPressed(KEY_F1))
            state.is_wireframe = !state.is_wireframe;

        if (Input::IsKeyPressed(KEY_F2))
            ResetEditorCamera();
    }

    void OnUpdate() { Cameras::UpdateEditor(state.camera, 0.1f, 1.f); }

    void OnRender()
    {
        const auto swapchain_info = (ColorTargetInfo){
            .clear_color = glm::vec4(0.12, 0.12, 0.12, 1.f),
            .texture = &Renderer::GetTextureSwapchain(),
            .load_op = GPULoadOp::Clear,
            .store_op = GPUStoreOp::Store,
        };

        const auto ds_info = (DepthStencilTargetInfo){
            .texture = &Renderer::GetTextureDepthStencil(),
            .clear_depth = 1.f,
            .load_op = GPULoadOp::Clear,
            .store_op = GPUStoreOp::Discard,
        };

        const RenderPassHandle render_pass = RenderPasses::Begin(&swapchain_info, 1, ds_info);
        const PipelineType pipeline = !state.is_wireframe ? PipelineType::OutdoorMeshes : PipelineType::WireframeMeshes;
        Pipelines::Bind(pipeline, render_pass);

        Renderer::DrawModel(state.model_xbot, glm::vec3(-1.f, 0.f, 0.f));
        Renderer::DrawModel(state.model_ybot, glm::vec3(1.f, 0.f, 0.f));

        RenderPasses::End(render_pass);
    }

    void OnRenderUI() {}

    void OnShutdown()
    {
        Models::Unload(state.model_xbot);
        Models::Unload(state.model_ybot);
    }

    void ResetEditorCamera()
    {
        state.camera.position = glm::vec3(0.f, 0.f, 2.f);
        state.camera.up = glm::vec3(0.f, 1.f, 0.f);
        state.camera.yaw = -90.f;
        state.camera.pitch = 0.f;
        state.camera.fov = 75.f;
        state.camera.clip_near = 0.1f;
        state.camera.clip_far = 100.f;
    }
}
