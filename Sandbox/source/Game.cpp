#include "Game.h"
#include "Player.h"

#include <imgui.h>

using namespace Nova;

struct GameState
{
    Scene scene_editor;
    Scene scene_runtime;
    Scene* scene_active = NULL;
    Player player;
    Entity camera_game;
    Material material_grid;
    Material material_red;
    Material material_green;
    Material material_blue;

    EnvironmentMap environment_map;
    DirectionalLight sun;
    Camera3D camera_editor;
    Texture attachment_hdr;
    Texture attachment_resolve;
    Texture attachment_depth;

    AssetHandle assets[Assets::_Length];
};

static GameState state;

namespace Game
{
    void ResetCameraEditor();
    void ResetCameraGame();
    void RenderPass_SceneHDR();
    void RenderPass_PostProcessing();

    void OnCreate()
    {
        // Load all of the assets at the very start of the program
        state.assets[Assets::AnimationIdle] = AssetManager::ImportByPath("Assets/Animations/Idle.fbx", AssetType::AnimationClip);
        state.assets[Assets::AnimationRun] = AssetManager::ImportByPath("Assets/Animations/Run.fbx", AssetType::AnimationClip);
        state.assets[Assets::AnimationJump] = AssetManager::ImportByPath("Assets/Animations/Jump.fbx", AssetType::AnimationClip);
        state.assets[Assets::ModelRobot] = AssetManager::ImportByPath("Assets/Models/Robot.fbx", AssetType::ModelAnimated);
        state.assets[Assets::TextureGrid] = AssetManager::ImportByPath("Assets/Textures/Grid.png", AssetType::Texture);

        state.material_grid.texture_albedo = *AssetManager::GetAsset<Texture>(GetAsset(Assets::TextureGrid));
        state.material_red.albedo = glm::vec4(1.f, 0.f, 0.f, 1.f);
        state.material_red.metallic = 0.65f;
        state.material_red.roughness = 0.35f;
        state.material_green.albedo = glm::vec4(0.f, 1.f, 0.f, 1.f);
        state.material_green.metallic = 0.15f;
        state.material_green.roughness = 0.05f;
        state.material_blue.albedo = glm::vec4(0.f, 0.f, 1.f, 1.f);
        state.material_blue.metallic = 0.75f;
        state.material_blue.roughness = 0.25f;

        // Bake the environment and setup the sun
        state.environment_map = IBL::BakeFromHDRI("Assets/HDRIs/puresky_citrus.hdr");
        state.sun.direction = glm::vec3(-0.7f, -1.f, -0.25f);
        state.sun.color = glm::vec4(1.f, 0.92f, 0.86f, 1.f);
        state.sun.intensity = 2.f;
        Renderer::SetSun(state.sun);

        state.scene_active = &state.scene_editor;
        state.scene_editor = Scenes::Create(10);
        state.player = Player_Create(state.scene_editor);

        // Create the HDR framebuffer attachments
        const Window& window = Application::GetWindow();
        const MSAASamples msaa = Application::GetMSAASamples();
        state.attachment_hdr = Textures::CreateFramebufferAttachmentHDR(window.width, window.height, msaa);
        state.attachment_resolve = Textures::CreateFramebufferAttachmentHDR(window.width, window.height, MSAASamples::One);
        state.attachment_depth = Textures::CreateFramebufferAttachmentDepth(window.width, window.height, msaa);

        // Setup settings for the scene
        ResetCameraEditor();
        ResetCameraGame();
        Renderer::SetExposure(1.f);
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

        if (Input::IsKeyPressed(KEY_F5))
        {
            if (state.scene_active == &state.scene_editor)
            {
                Scenes::Copy(state.scene_editor, state.scene_runtime);
                Scenes::Play(state.scene_runtime);
                state.scene_active = &state.scene_runtime;
            }
            else
            {
                state.scene_active = &state.scene_editor;
                Scenes::Stop(state.scene_runtime);
                Scenes::Destroy(state.scene_runtime);
            }
        }

        if (Input::IsKeyPressed(KEY_F2))
            ResetCameraEditor();

        if (Input::IsKeyPressed(KEY_C))
        {
            WARN("Position: " V3_FMT, V3_OPEN(state.camera_editor.position));
            WARN("Target: " V3_FMT, V3_OPEN(state.camera_editor.target));
        }
    }

    void OnUpdate()
    {
        const Window& window = Application::GetWindow();
        if (Windows::IsMinimized(window))
            return;

        Player_Update(*state.scene_active, state.player);
        if (state.scene_active->state == SceneState::Runtime)
        {
            // Make the runtime camera follow the player
            const glm::vec3 player_position = state.player.entity.GetComponent<TransformComponent>(state.scene_runtime)->position;
            const glm::vec3 camera_offset = glm::vec3(0.f, 6.f, -5.f);

            auto transform = state.camera_game.GetComponent<TransformComponent>(state.scene_runtime);
            transform->position = player_position + camera_offset;
        }

        Scenes::UpdateSubsystems(*state.scene_active);
        if (state.scene_active->state == SceneState::Editor)
            Cameras::UpdateEditor(state.camera_editor, 1.f, 12.f);
    }

    void OnRender()
    {
        const Window& window = Application::GetWindow();
        if (Windows::IsMinimized(window))
            return;

        RenderPass_SceneHDR();
        RenderPass_PostProcessing();
    }

    void OnRenderUI()
    {
        if (state.scene_active->state == SceneState::Editor)
            ImGui::ShowDemoWindow();
    }

    void OnShutdown()
    {
        Scenes::Destroy(state.scene_editor);
        Scenes::Destroy(state.scene_runtime);
        IBL::Free(state.environment_map);

        Textures::Unload(state.attachment_hdr);
        Textures::Unload(state.attachment_resolve);
        Textures::Unload(state.attachment_depth);
    }

    AssetHandle GetAsset(AssetIndex index) { return state.assets[index]; }

    void ResetCameraEditor()
    {
        state.camera_editor.position = glm::vec3(0.625, 3.620, 6.042);
        state.camera_editor.target = glm::vec3(0.f);
        state.camera_editor.fov = 75.f;
        state.camera_editor.clip_near = 0.1f;
        state.camera_editor.clip_far = 50.f;

        if (state.scene_active->state == SceneState::Editor)
            Renderer::SetPrimaryCamera(&state.camera_editor);
    }

    void ResetCameraGame()
    {
        state.camera_game = Scenes::CreateEntity(*state.scene_active, "Main Camera");

        const auto transform = state.camera_game.GetComponent<TransformComponent>(*state.scene_active);
        const auto cc = state.camera_game.AddComponent<PerspectiveCameraComponent>(*state.scene_active, true, state.player.entity);

        transform->position = glm::vec3(0.064, 5.704, -8.300);
        cc->camera.clip_near = 0.1f;
        cc->camera.clip_far = 50.f;
        cc->camera.fov = 75.f;
    }

    void RenderPass_SceneHDR()
    {
        const ColorTargetInfo hdr_info = {
            .clear_color = glm::vec4(0.01f, 0.01f, 0.01f, 1.f), // Note: Colors are not in linear space after compositing pass
            .texture = Textures::GetHandle(state.attachment_hdr),
            .texture_msaa_resolve = Textures::GetHandle(state.attachment_resolve),
            .load_op = GPULoadOp::Clear,
            .store_op = GPUStoreOp::Resolve,
        };

        const DepthStencilTargetInfo ds_info = {
            .texture = Textures::GetHandle(state.attachment_depth),
            .clear_depth = 1.f,
            .load_op = GPULoadOp::Clear,
            .store_op = GPUStoreOp::Discard,
        };

        // Renders the scene to the HDR framebuffer
        const RenderPassHandle scene_pass = RenderPasses::Begin(&hdr_info, 1, ds_info);
        Scenes::RenderSubsystems(*state.scene_active);
        Renderer::DrawPrimitive(PrimitiveMesh::Plane, Meshes::CalculateTransform(glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f), glm::vec3(10.f, 1.f, 10.f)), state.material_grid);
        Renderer::DrawPrimitive(PrimitiveMesh::Cone, Meshes::CalculateTransform(glm::vec3(4.f, 0.f, 0.f)), state.material_red);
        Renderer::DrawPrimitive(PrimitiveMesh::Pyramid, Meshes::CalculateTransform(glm::vec3(-4.f, 0.f, 0.f)), state.material_green);
        Renderer::DrawPrimitive(PrimitiveMesh::Torus, Meshes::CalculateTransform(glm::vec3(0.f, -0.5f, -4.f)), state.material_blue);
        Renderer::DrawSkybox(state.environment_map);
        RenderPasses::End(scene_pass);
    }

    void RenderPass_PostProcessing()
    {
        const ColorTargetInfo swapchain_info = {
            .clear_color = glm::vec4(0.12, 0.12, 0.12, 1.f),
            .texture = NULL, // Resorts to using the renderer's swapchain texture
            .load_op = GPULoadOp::Clear,
            .store_op = GPUStoreOp::Store,
        };

        const DepthStencilTargetInfo ds_info = {
            .texture = NULL, // Resorts to using the renderer's default depth-stencil texture
            .clear_depth = 1.f,
            .load_op = GPULoadOp::Clear,
            .store_op = GPUStoreOp::Discard,
        };

        // Renders the HDR framebuffer onto a fullscreen quad and applies post-processing effects
        const RenderPassHandle post_processing_pass = RenderPasses::Begin(&swapchain_info, 1, ds_info);
        Renderer::DrawTextureCompositing(state.attachment_resolve);
        UI::Display(post_processing_pass);
        RenderPasses::End(post_processing_pass);
    }
}
