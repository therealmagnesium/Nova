#include "Editor.h"
#include "Panels/AssetRegistry.h"
#include "Panels/ContentBrowser.h"
#include "Panels/Inspector.h"
#include "Panels/SceneHierarchy.h"
#include "Panels/SceneViewport.h"
#include <imgui.h>
#include <imgui_internal.h>

using namespace Nova;

struct EditorState
{
    Framebuffer framebuffer_hdr;
    Framebuffer framebuffer_viewport_composite;
    EnvironmentMap environment_map;
    Scene scene_editor;
    Scene scene_runtime;
    Camera3D camera_editor;
    bool show_demo_window = false;
};

static EditorState state;
static constexpr u64 k_ExpectedEntityCount = 100;
static constexpr float k_GammaCorrection = 2.2f;
static constexpr glm::vec4 k_ClearColorLinear = glm::vec4(0.12f, 0.12f, 0.12f, 1.f);
static const glm::vec4 k_ClearColorNonLinear = glm::pow(k_ClearColorLinear, glm::vec4(glm::vec3(1.f / k_GammaCorrection), 1.f));

namespace Editor
{
    void ResetCameraEditor();
    void RenderPass_SceneHDR();
    void RenderPass_Compositing();
    void RenderPass_UI();

    void OnCreate()
    {
        state.environment_map = IBL::BakeFromHDRI("Assets/HDRIs/puresky_citrus.hdr");
        UI::AddFont("Assets/Fonts/Quicksand.ttf", 20.f, true);

        const Window& window = Application::GetWindow();
        const MSAASamples msaa = Application::GetMSAASamples();

        const FramebufferSpecification framebuffer_spec = {
            .width = window.width,
            .height = window.height,
            .attachments = {
                FramebufferAttachmentSpecification{
                    .format = TextureFormat::RGBA16F,
                    .msaa = msaa,
                    .type = FramebufferAttachmentType::Color,
                    .requires_resolve = true,
                },
                FramebufferAttachmentSpecification{
                    .format = TextureFormat::Depth32F,
                    .msaa = msaa,
                    .type = FramebufferAttachmentType::DepthStencil,
                },
            }
        };
        const FramebufferSpecification framebuffer_composite_spec = {
            .width = window.width,
            .height = window.height,
            .attachments = {
                FramebufferAttachmentSpecification{
                    .format = TextureFormat::RGBA16F,
                    .msaa = MSAASamples::One,
                    .type = FramebufferAttachmentType::Color,
                },
            }
        };

        state.framebuffer_hdr = Framebuffers::Create(framebuffer_spec);
        state.framebuffer_viewport_composite = Framebuffers::Create(framebuffer_composite_spec);

        state.scene_editor = Scenes::Create(k_ExpectedEntityCount);
        state.scene_runtime = Scenes::Create(k_ExpectedEntityCount);
        Scenes::SetActive(state.scene_editor);

        ResetCameraEditor();
        ContentBrowserPanel::Init();
    }

    void OnShutdown()
    {
        IBL::Free(state.environment_map);
        Scenes::Destroy(state.scene_editor);
        Scenes::Destroy(state.scene_runtime);
        Framebuffers::Destroy(state.framebuffer_hdr);
        Framebuffers::Destroy(state.framebuffer_viewport_composite);
    }

    void OnEvent()
    {
        const Window& window = Application::GetWindow();
        const Project& project = Projects::GetContext();

        if (Windows::IsResizing(window))
        {
            Framebuffers::Resize(state.framebuffer_hdr, window.width, window.height);
            Framebuffers::Resize(state.framebuffer_viewport_composite, window.width, window.height);
        }

        if (Input::IsKeyPressed(KEY_F1))
            state.show_demo_window = !state.show_demo_window;

        if (Input::IsKeyPressed(KEY_F2) || Input::IsGamepadButtonPressed(GamepadButton::Back))
            ResetCameraEditor();

        if (Input::IsKeyDown(KEY_LEFT_CTRL))
        {
            if (Input::IsKeyPressed(KEY_U))
                UI::ExportLayout(project);

            if (Input::IsKeyPressed(KEY_S))
                Projects::Export(project.path_config, project);
        }
    }

    void OnUpdate()
    {
        Scene* active_scene = Scenes::GetActive();
        Scenes::UpdateSubsystems(*active_scene);

        if (active_scene->state == SceneState::Editor && SceneViewportPanel::IsHovered())
            Cameras::UpdateEditor(state.camera_editor, 1.f, 12.f);

        if (Projects::ClearPendingLayoutLoad())
        {
            const Project& project = Projects::GetContext();
            UI::ImportLayout(project);
        }
    }

    void OnRender()
    {
        RenderPass_SceneHDR();
        RenderPass_Compositing();
        RenderPass_UI();
    }

    void OnRenderUI()
    {
        const Texture& viewport_texture = Framebuffers::GetColorAttachment(state.framebuffer_viewport_composite, 0);

        ImGui::DockSpaceOverViewport();
        AssetRegistryPanel::Display();
        ContentBrowserPanel::Display();
        SceneHierarchyPanel::Display();
        InspectorPanel::Display();
        SceneViewportPanel::Display(viewport_texture);

        if (state.show_demo_window)
            ImGui::ShowDemoWindow(&state.show_demo_window);
    }

    void RenderPass_SceneHDR()
    {
        Scene* active_scene = Scenes::GetActive();
        const RenderPassSpecification render_pass_spec = {
            .clear_color = glm::vec4(k_ClearColorLinear), // Colors are in linear space before the compositing pass
            .clear_depth = 1.f,
            .color_load_op = GPULoadOp::Clear,
            .color_store_op = GPUStoreOp::Resolve,
            .depth_load_op = GPULoadOp::Clear,
            .depth_store_op = GPUStoreOp::Discard,
        };

        RenderPassHandle render_pass = RenderPasses::Begin(&state.framebuffer_hdr, render_pass_spec);
        Scenes::RenderSubsystems(*active_scene);
        Renderer::DrawSkybox(state.environment_map);
        RenderPasses::End(render_pass);
    }

    void RenderPass_Compositing()
    {
        const RenderPassSpecification render_pass_spec = {
            .clear_color = glm::vec4(k_ClearColorLinear),
            .clear_depth = 1.f,
            .color_load_op = GPULoadOp::Clear,
            .color_store_op = GPUStoreOp::Store,
            .depth_load_op = GPULoadOp::Clear,
            .depth_store_op = GPUStoreOp::Discard,
        };

        const Texture& attachment_resolve = Framebuffers::GetResolveAttachment(state.framebuffer_hdr, 0);
        RenderPassHandle render_pass = RenderPasses::Begin(&state.framebuffer_viewport_composite, render_pass_spec);
        Renderer::DrawTextureCompositing(attachment_resolve, false);
        RenderPasses::End(render_pass);
    }

    void RenderPass_UI()
    {
        const RenderPassSpecification render_pass_spec = {
            .clear_color = glm::vec4(k_ClearColorNonLinear),
            .clear_depth = 1.f,
            .color_load_op = GPULoadOp::Clear,
            .color_store_op = GPUStoreOp::Store,
            .depth_load_op = GPULoadOp::Clear,
            .depth_store_op = GPUStoreOp::Discard,
        };

        RenderPassHandle render_pass = RenderPasses::Begin(NULL, render_pass_spec);
        UI::Display(render_pass);
        RenderPasses::End(render_pass);
    }

    void ResetCameraEditor()
    {
        state.camera_editor.position = glm::vec3(2.f, 5.f, 4.f);
        state.camera_editor.target = glm::vec3(0.f, 0.f, 0.f);
        state.camera_editor.fov = 75.f;
        state.camera_editor.clip_near = 0.1f;
        state.camera_editor.clip_far = 500.f;
        Renderer::SetPrimaryCamera(&state.camera_editor);
    }
}
