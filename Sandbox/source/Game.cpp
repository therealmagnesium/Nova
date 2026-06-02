#include "Game.h"

#include <Nova.h>
#include <SDL3/SDL_gpu.h>
#include <glm/gtc/matrix_transform.hpp>

using namespace Nova;

struct GameState
{
    Model model_xbot;
    Model model_ybot;
    Model model_cube;
    Camera3D camera;
    Texture attachment_hdr;
    Texture attachment_depth;
};

static GameState state;

namespace Game
{
    void ResetEditorCamera();
    void RenderPass_SceneHDR();
    void RenderPass_PostProcessing();

    void OnCreate()
    {
        // Load the models
        state.model_xbot = Models::Load("Assets/Models/X-Bot.fbx");
        state.model_ybot = Models::Load("Assets/Models/Y-Bot.fbx");
        state.model_cube = Models::Load("Assets/Models/Cube.fbx");

        // Create the HDR framebuffer attachments
        const Window& window = Application::GetWindow();
        state.attachment_hdr = Textures::CreateFramebufferAttachmentHDR(window.width, window.height);
        state.attachment_depth = Textures::CreateFramebufferAttachmentDepth(window.width, window.height);

        // Setup settings for the scene
        ResetEditorCamera();
        Renderer::SetPrimaryCamera(&state.camera);
        Renderer::SetExposure(1.f);
    }

    void OnEvent()
    {
        // If the window is resized, recreate the the HDR framebuffer attachments
        const Window& window = Application::GetWindow();
        if (Windows::IsResizing(window))
        {
            Textures::Unload(state.attachment_hdr);
            Textures::Unload(state.attachment_depth);
            state.attachment_hdr = Textures::CreateFramebufferAttachmentHDR(window.width, window.height);
            state.attachment_depth = Textures::CreateFramebufferAttachmentDepth(window.width, window.height);
        }

        if (Input::IsKeyPressed(KEY_F1))
            Renderer::ToggleWireframeMode();

        if (Input::IsKeyPressed(KEY_F2))
            ResetEditorCamera();
    }

    void OnUpdate()
    {
        const Window& window = Application::GetWindow();
        if (Windows::IsMinimized(window))
            return;

        Cameras::UpdateEditor(state.camera, 0.1f, 1.f); // TODO: Add a scene camera to enable switching between editing and runtime
    }

    void OnRender()
    {
        const Window& window = Application::GetWindow();
        if (Windows::IsMinimized(window))
            return;

        RenderPass_SceneHDR();
        RenderPass_PostProcessing();
    }

    void OnRenderUI() {}

    void OnShutdown()
    {
        // Unload the models
        Models::Unload(state.model_xbot);
        Models::Unload(state.model_ybot);
        Models::Unload(state.model_cube);
    }

    void ResetEditorCamera()
    {
        state.camera.position = glm::vec3(0.f, 2.f, 4.f);
        state.camera.up = glm::vec3(0.f, 1.f, 0.f);
        state.camera.yaw = -90.f;
        state.camera.pitch = -25.f;
        state.camera.fov = 75.f;
        state.camera.clip_near = 0.1f;
        state.camera.clip_far = 100.f;
    }

    void RenderPass_SceneHDR()
    {
        const auto hdr_info = (ColorTargetInfo){
            .clear_color = glm::vec4(0.01f, 0.01f, 0.01f, 1.f), // Note: Colors are in linear space
            .texture = &state.attachment_hdr,
            .load_op = GPULoadOp::Clear,
            .store_op = GPUStoreOp::Store,
        };

        const auto ds_info = (DepthStencilTargetInfo){
            .texture = &state.attachment_depth,
            .clear_depth = 1.f,
            .load_op = GPULoadOp::Clear,
            .store_op = GPUStoreOp::Discard,
        };

        // Renders the scene to the HDR framebuffer
        const RenderPassHandle scene_pass = RenderPasses::Begin(&hdr_info, 1, ds_info);
        Renderer::DrawModel(state.model_xbot, glm::vec3(-1.f, 1.2f, 0.f));
        Renderer::DrawModel(state.model_ybot, glm::vec3(1.f, 1.2f, 0.f));
        Renderer::DrawModel(state.model_cube, glm::vec3(0.f), glm::vec3(0.f), glm::vec3(5.f, 0.1f, 5.f));
        RenderPasses::End(scene_pass);
    }

    void RenderPass_PostProcessing()
    {
        const auto swapchain_info = (ColorTargetInfo){
            .clear_color = glm::vec4(0.12, 0.12, 0.12, 1.f),
            .texture = NULL, // Resorts to using the renderer's swapchain texture
            .load_op = GPULoadOp::Clear,
            .store_op = GPUStoreOp::Store,
        };

        const auto ds_info = (DepthStencilTargetInfo){
            .texture = NULL, // Resorts to using the renderer's default depth-stencil texture
            .clear_depth = 1.f,
            .load_op = GPULoadOp::Clear,
            .store_op = GPUStoreOp::Discard,
        };

        // Renders the HDR framebuffer onto a fullscreen quad and applies post-processing effects
        const RenderPassHandle post_processing_pass = RenderPasses::Begin(&swapchain_info, 1, ds_info);
        Renderer::DrawTextureCompositing(state.attachment_hdr);
        RenderPasses::End(post_processing_pass);
    }
}
