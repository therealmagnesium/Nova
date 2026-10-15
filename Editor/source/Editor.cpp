#include "Editor.h"
#include "Panels/AssetRegistry.h"
#include "Panels/Console.h"
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
    bool EnsureStartupScene(Project& project, Scene& scene);
    bool SaveSceneToPath(const std::filesystem::path& path_absolute, Scene& scene, Project& project);
    bool SaveScene(Project& project, Scene& scene);

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
        ContentBrowserPanel::Shutdown();
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
            if (Input::IsKeyPressed(KEY_S))
            {
                Project& project = Projects::GetContext();
                if (!project.IsValid())
                    return;

                if (!EnsureStartupScene(project, state.scene_editor))
                    return;

                SaveScene(project, state.scene_editor);
                Projects::Export(project.path_config, project);
            }
        }
    }

    void OnUpdate()
    {
        Scene* const active_scene = Scenes::GetActive();
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
        ConsolePanel::Display();
        ContentBrowserPanel::Display();
        SceneHierarchyPanel::Display();
        InspectorPanel::Display();
        SceneViewportPanel::Display(viewport_texture);

        if (state.show_demo_window)
            ImGui::ShowDemoWindow(&state.show_demo_window);
    }

    void Callback_ExportScene()
    {
        Project& project = Projects::GetContext();

        const std::filesystem::path path_scenes = Projects::GetAssetPath(project) / "Scenes";
        std::error_code e;
        std::filesystem::create_directories(path_scenes, e);
        if (e)
        {
            ERROR("Editor::Callback_ExportScene - Cannot create directory %s!", path_scenes.string().c_str());
            return;
        }

        const std::filesystem::path path_scene = path_scenes / "Playground.nscene";
        SaveSceneToPath(path_scene, state.scene_editor, project);
        project.path_scene_start = "Scenes/Playground.nscene";
        Projects::Export(project.path_config, project);
    }

    void Callback_ImportStartupScene()
    {
        const Project& project = Projects::GetContext();
        if (!Projects::HasStartupScene(project))
        {
            WARN("Editor::Callback_LoadStartupScene - %s", "Project has no startup scene on disk, keeping the current scene");
            return;
        }

        const std::filesystem::path path_relative = Projects::GetAssetPathRelative(Projects::GetStartScenePath(project), project);
        const AssetHandle handle = AssetManager::FindAssetHandleByPath(path_relative);
        const Scene* loaded = AssetManager::GetAsset<Scene>(handle);
        if (loaded == NULL)
        {
            ERROR("Editor::Callback_LoadStartupScene - Startup scene \"%s\" is not in the asset registry!", path_relative.string().c_str());
            return;
        }

        // The renderer holds a pointer into the old registry's light component,
        // and on_render still runs this frame, so drop it before replacing the registry.
        Renderer::SetSun(Stub_DirectionalLight);
        SceneHierarchyPanel::SetSelectionContext(Stub_Entity);

        state.scene_editor = *loaded;
        state.scene_editor.state = SceneState::Editor;
        state.scene_editor.handle = handle;
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

    bool EnsureStartupScene(Project& project, Scene& scene)
    {
        if (Projects::HasStartupScene(project))
            return true;

        /*
        if (const auto handles = AssetManager::GetAllHandlesOfType(AssetType::Scene); !handles.empty() && project.path_scene_start.empty())
        {
            // optionally auto-assign first, but per spec still prompt
        }*/

        const FileDialogFilter filter = {
            .name = "Scene",
            .specification = "nscene",
        };

        const std::filesystem::path path_scenes = Projects::GetAssetPath(project) / "Scenes";
        std::error_code e;
        if (!std::filesystem::exists(path_scenes))
            std::filesystem::create_directories(path_scenes, e);

        if (e)
        {
            ERROR("Editor::EnsureStartupScene - Cannot create directory %s!", path_scenes.string().c_str());
            return false;
        }

        FileDialogs::SetDefaultPath(path_scenes); // ensure Scenes/ exists first
        if (!FileDialogs::AskSave(&filter, 1))
            return false; // user cancelled -> abort project save

        const std::filesystem::path path_selected = FileDialogs::GetPathSelected(); // already EnsureExtension(.nscene)
        std::filesystem::create_directories(path_selected.parent_path(), e);
        if (e)
        {
            ERROR("Editor::EnsureStartupScene - Cannot create directory %s!", path_selected.parent_path().string().c_str());
            return false;
        }

        SaveSceneToPath(path_selected, scene, project);
        project.path_scene_start = std::filesystem::proximate(path_selected, Projects::GetAssetPath(project)); // Scenes/Playground.nscene
        return true;
    }

    bool SaveSceneToPath(const std::filesystem::path& path_absolute, Scene& scene, Project& project)
    {
        const std::filesystem::path path_relative = Projects::GetAssetPathRelative(path_absolute, project);

        AssetHandle handle = AssetManager::FindAssetHandleByPath(path_relative);
        const bool was_registered = AssetManager::IsHandleValid(handle);
        if (!was_registered)
            handle = Random::GenerateUUID();

        scene.handle = handle; // must happen BEFORE Export
        Scenes::Export(path_absolute, scene);

        if (!was_registered)
            AssetManager::ImportByPath(path_absolute, AssetType::Scene, handle);

        return true;
    }

    bool SaveScene(Project& project, Scene& scene)
    {
        std::filesystem::path path;
        if (AssetManager::IsHandleValid(scene.handle))
            path = Projects::GetAssetPathAbsolute(AssetManager::GetAssetPath(scene.handle), project);
        else
            path = Projects::GetStartScenePath(project);

        return SaveSceneToPath(path, scene, project);
    }
}
