#include "Game.h"

#include <Nova.h>
#include <SDL3/SDL_gpu.h>
#include <glm/gtc/matrix_transform.hpp>

using namespace Nova;

namespace PlayerAnimations
{
    enum : u8
    {
        Idle = 0,
        Walking,
        Running,
        _Length
    };
}

struct GameState
{
    AnimatedModel model_xbot;
    AnimationClip clips[PlayerAnimations::_Length];
    Animator animator;
    u8 clip_index = PlayerAnimations::Idle;

    Material material;
    Mesh mesh_sphere;
    Model model_mario;

    EnvironmentMap environment_map;
    DirectionalLight sun;
    Camera3D camera;
    Texture attachment_hdr;
    Texture attachment_resolve;
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
        // Bake the environment and generate meshes
        state.environment_map = IBL::BakeFromHDRI("Assets/HDRIs/puresky_citrus.hdr");
        state.model_mario = Models::Load("Assets/Models/Mario.fbx");
        state.mesh_sphere = Meshes::GenerateSphere(64, 32);

        state.model_xbot = Models::LoadAnimated("Assets/Models/X-Bot.fbx");
        state.clips[PlayerAnimations::Idle] = Animations::Load("Assets/Animations/Idle.fbx");
        state.clips[PlayerAnimations::Walking] = Animations::Load("Assets/Animations/Walking.fbx");
        state.clips[PlayerAnimations::Running] = Animations::Load("Assets/Animations/Running.fbx");

        state.animator = Animators::Create(state.model_xbot.skeleton);

        for (const AnimationClip& clip : state.clips)
            Animations::Bind(clip, state.model_xbot.skeleton);

        Animators::Play(state.animator, state.clips[state.clip_index]);

        state.sun.direction = glm::vec3(-0.7f, -1.f, -0.25f);
        state.sun.color = glm::vec4(1.f, 0.92f, 0.86f, 1.f);
        state.sun.intensity = 2.f;
        Renderer::SetSun(state.sun);

        // Create the HDR framebuffer attachments
        const Window& window = Application::GetWindow();
        const MSAASamples msaa = Application::GetMSAASamples();
        state.attachment_hdr = Textures::CreateFramebufferAttachmentHDR(window.width, window.height, msaa);
        state.attachment_resolve = Textures::CreateFramebufferAttachmentHDR(window.width, window.height, MSAASamples::One);
        state.attachment_depth = Textures::CreateFramebufferAttachmentDepth(window.width, window.height, msaa);

        // Setup settings for the scene
        ResetEditorCamera();
        Renderer::SetPrimaryCamera(&state.camera);
        Renderer::SetExposure(1.5f);
    }

    void OnEvent()
    {
        // If the window is resized, recreate the the HDR framebuffer attachments
        const Window& window = Application::GetWindow();
        const MSAASamples msaa = Application::GetMSAASamples();
        if (Windows::IsResizing(window))
        {
            Textures::Unload(state.attachment_hdr);
            Textures::Unload(state.attachment_resolve);
            Textures::Unload(state.attachment_depth);
            state.attachment_hdr = Textures::CreateFramebufferAttachmentHDR(window.width, window.height, msaa);
            state.attachment_resolve = Textures::CreateFramebufferAttachmentHDR(window.width, window.height, MSAASamples::One);
            state.attachment_depth = Textures::CreateFramebufferAttachmentDepth(window.width, window.height, msaa);
        }

        if (Input::IsKeyPressed(KEY_F1))
            Renderer::ToggleWireframeMode();

        if (Input::IsKeyPressed(KEY_F2))
            ResetEditorCamera();

        if (Input::IsKeyPressed(KEY_C))
        {
            WARN("Position: " V3_FMT, V3_OPEN(state.camera.position));
            WARN("Target: " V3_FMT, V3_OPEN(state.camera.target));
        }

        if (Input::IsKeyPressed(KEY_N))
        {
            const float transition_duration = 0.25f;
            state.clip_index = (state.clip_index + 1) % PlayerAnimations::_Length;
            Animators::CrossfadeTo(state.animator, state.clips[state.clip_index], transition_duration);
        }
    }

    void OnUpdate()
    {
        const Window& window = Application::GetWindow();
        if (Windows::IsMinimized(window))
            return;

        Cameras::UpdateEditor(state.camera, 2.f, 12.f); // TODO: Add a scene camera to enable switching between editing and runtime
        Animators::Update(state.animator, Application::GetDeltaTime());
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
        for (AnimationClip& clip : state.clips)
            Animations::Unload(clip);

        Models::UnloadAnimated(state.model_xbot);

        Models::Unload(state.model_mario);
        Meshes::Destroy(state.mesh_sphere);
        IBL::Free(state.environment_map);

        Textures::Unload(state.attachment_hdr);
        Textures::Unload(state.attachment_resolve);
        Textures::Unload(state.attachment_depth);
    }

    void ResetEditorCamera()
    {
        state.camera.position = glm::vec3(-9.769f, 1.474f, 15.054f);
        state.camera.target = glm::vec3(-5.383f, 0.223f, 6.155f);
        state.camera.fov = 75.f;
        state.camera.clip_near = 0.1f;
        state.camera.clip_far = 50.f;
    }

    void RenderPass_SceneHDR()
    {
        const auto hdr_info = (ColorTargetInfo){
            .clear_color = glm::vec4(0.01f, 0.01f, 0.01f, 1.f), // Note: Colors are not in linear space after compositing pass
            .texture = Textures::GetHandle(state.attachment_hdr),
            .texture_msaa_resolve = Textures::GetHandle(state.attachment_resolve),
            .load_op = GPULoadOp::Clear,
            .store_op = GPUStoreOp::Resolve,
        };

        const auto ds_info = (DepthStencilTargetInfo){
            .texture = Textures::GetHandle(state.attachment_depth),
            .clear_depth = 1.f,
            .load_op = GPULoadOp::Clear,
            .store_op = GPUStoreOp::Discard,
        };

        // Renders the scene to the HDR framebuffer
        const RenderPassHandle scene_pass = RenderPasses::Begin(&hdr_info, 1, ds_info);

        // Render rows * column number of spheres with varying metallic/roughness values scaled by rows and columns respectively
        const u8 row_count = 7;
        const u8 column_count = 7;
        const float spacing = 2.5f;
        glm::mat4 transform = glm::mat4(1.0f);
        for (u8 i = 0; i < row_count; i++)
        {
            state.material.metallic = static_cast<float>(i) / static_cast<float>(row_count);
            for (u8 j = 0; j < column_count; j++)
            {
                // Clamp the roughness to 0.025 - 1.0 as perfectly smooth surfaces (roughness of 0.0) tend to look a bit off with direct lighting
                state.material.roughness = glm::clamp(static_cast<float>(j) / static_cast<float>(column_count), 0.025f, 1.f);

                const float position_x = (float)(j - (column_count * 0.5f)) * spacing;
                const float position_y = (float)(i - (row_count * 0.5f)) * spacing;
                transform = glm::mat4(1.0f);
                transform = glm::translate(transform, glm::vec3(position_x, position_y, -4.f));
                Renderer::DrawMesh(state.mesh_sphere, transform, state.material);
            }
        }

        Renderer::DrawModel(state.model_mario, glm::vec3(-2.f, 0.f, 4.f));
        Renderer::DrawAnimatedModel(state.model_xbot, state.animator, glm::vec3(2.f, 0.f, 4.f));

        Renderer::DrawSkybox(state.environment_map);
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
        Renderer::DrawTextureCompositing(state.attachment_resolve);
        RenderPasses::End(post_processing_pass);
    }
}
