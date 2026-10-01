#include "Panels/Inspector.h"
#include "Panels/ContentBrowser.h"
#include "Panels/SceneHierarchy.h"

#include <imgui.h>
#include <iterator>

using namespace Nova;

static constexpr u8 k_MaxTagCharacters = 64;

struct InspectorState
{
    char tag_input[k_MaxTagCharacters + 1];
    bool should_display = true;
};

local InspectorState state;

namespace InspectorPanel
{
    void DisplayTagInput(Entity selection_context);
    void DisplayComponents(Entity selection_context);
    void DisplayAddComponentButton(Entity selection_context);
    void DisplayAssetControls(AssetType asset_type, AssetHandle asset_handle, const std::filesystem::path& browser_selection);
    AssetType PathToAssetType(const std::filesystem::path& path);
    AssetType GuessAssetTypeFromPath(const std::filesystem::path& path);
    std::string PrimitiveToString(PrimitiveMesh primitive);

    template <typename T, typename UIFunction>
    void DrawComponent(const char* name, Entity entity, UIFunction callback);

    void Display()
    {
        if (!state.should_display)
            return;

        Entity selection_context = SceneHierarchyPanel::GetSelectionContext();

        if (ImGui::Begin("Inspector", &state.should_display))
        {
            DisplayTagInput(selection_context);
            DisplayComponents(selection_context);
            DisplayAddComponentButton(selection_context);

            const std::filesystem::path browser_selection = ContentBrowserPanel::GetSelectionContext();
            if (!selection_context.IsValid() && !browser_selection.empty() && std::filesystem::is_regular_file(browser_selection))
            {
                const Project& project = Projects::GetContext();
                const std::filesystem::path path_relative = Projects::GetAssetPathRelative(browser_selection, project);

                const AssetType asset_type = PathToAssetType(path_relative);
                const AssetHandle asset_handle = AssetManager::FindAssetHandleByPath(path_relative);
                DisplayAssetControls(asset_type, asset_handle, path_relative);
            }

            ImGui::End();
        }
    }

    void DisplayTagInput(Entity selection_context)
    {
        if (!selection_context.IsValid())
            return;

        auto& internal = selection_context.GetComponent<InternalComponent>();
        internal.tag.copy(state.tag_input, internal.tag.size());
        state.tag_input[internal.tag.size()] = '\0';

        const float font_size = ImGui::GetFontSize();
        const float available_width = ImGui::GetContentRegionAvail().x;
        const float checkbox_width = font_size * 2.f;
        ImGui::SetNextItemWidth(available_width - checkbox_width);
        if (ImGui::InputText("##Name Input", state.tag_input, sizeof(char) * LEN(state.tag_input), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            internal.tag = std::string(state.tag_input);
            std::fill(std::begin(state.tag_input), std::end(state.tag_input), '\0');
        }

        ImGui::SameLine();
        ImGui::Checkbox("##Is Selection Context  Active?", &internal.is_active);
    }

    void DisplayComponents(Entity selection_context)
    {
        if (!selection_context.IsValid())
            return;

        const auto DrawTransformComponent = [](TransformComponent& component)
        {
            const float column_width = 150.f;

            ImGui::Text("Position");
            ImGui::SameLine(column_width - ImGui::GetCursorPosX());
            ImGui::DragFloat3("##Position", &component.position.x, 0.1f);
            ImGui::Text("Rotation");
            ImGui::SameLine(column_width - ImGui::GetCursorPosX());
            ImGui::DragFloat3("##Rotation", &component.rotation.x, 0.1f);
            ImGui::Text("Scale");
            ImGui::SameLine(column_width - ImGui::GetCursorPosX());
            ImGui::DragFloat3("##Scale", &component.scale.x, 0.1f);
        };

        const auto DrawMeshFilterComponent = [](MeshFilterComponent& component)
        {
            const float column_width = 150.f;

            std::string preview_mesh_source = "None";
            if (component.source_type == MeshSource::Model)
                preview_mesh_source = "Standard";
            else if (component.source_type == MeshSource::Primitive)
                preview_mesh_source = "Primitive";

            ImGui::Text("Mesh Source");
            ImGui::SameLine(column_width - ImGui::GetCursorPosX());
            ImGui::SetNextItemWidth(-1.f);
            if (ImGui::BeginCombo("##Mesh Source", preview_mesh_source.c_str()))
            {
                if (ImGui::Selectable("None", component.source_type == MeshSource::None))
                    component.source_type = MeshSource::None;

                if (ImGui::Selectable("Primitive", component.source_type == MeshSource::Primitive))
                    component.source_type = MeshSource::Primitive;

                if (ImGui::Selectable("Standard", component.source_type == MeshSource::Model))
                    component.source_type = MeshSource::Model;

                ImGui::EndCombo();
            }

            switch (component.source_type)
            {
                case MeshSource::Primitive:
                {
                    const std::string preview = PrimitiveToString(component.primitive);

                    ImGui::Text("Primitive");
                    ImGui::SameLine(column_width - ImGui::GetCursorPosX());
                    ImGui::SetNextItemWidth(-1.f);
                    if (ImGui::BeginCombo("##Primitive", preview.c_str()))
                    {
                        const u8 primitive_count = static_cast<u8>(PrimitiveMesh::_Length);
                        const u8 cube_index = static_cast<u8>(PrimitiveMesh::Cube);
                        for (u8 i = cube_index; i < primitive_count; i++)
                        {
                            const PrimitiveMesh i_primitive = static_cast<PrimitiveMesh>(i);
                            if (ImGui::Selectable(PrimitiveToString(i_primitive).c_str(), i_primitive == component.primitive))
                                component.primitive = i_primitive;
                        }
                        ImGui::EndCombo();
                    }

                    break;
                }
                case MeshSource::Model:
                {
                    const std::string preview = component.asset_model != AssetHandle_Invalid ? AssetManager::GetAssetPath(component.asset_model) : "Select a mesh asset";

                    ImGui::Text("Mesh Asset");
                    ImGui::SameLine(column_width - ImGui::GetCursorPosX());
                    ImGui::SetNextItemWidth(-1.f);
                    if (ImGui::BeginCombo("##Mesh Asset", preview.c_str()))
                    {
                        const std::vector<AssetHandle> mesh_assets = AssetManager::GetAllHandlesOfType(AssetType::Model);
                        for (const AssetHandle handle : mesh_assets)
                        {
                            const std::filesystem::path path = AssetManager::GetAssetPath(handle);
                            if (ImGui::Selectable(path.string().c_str(), component.asset_model == handle))
                                component.asset_model = handle;
                        }

                        ImGui::EndCombo();
                    }

                    break;
                }
                default:
                    break;
            }
        };

        const auto DrawMeshRendererComponent = [selection_context](MeshRendererComponent& component)
        {
            if (!selection_context.HasComponent<MeshFilterComponent>())
                return;

            const auto& mesh_filter = selection_context.GetComponent<MeshFilterComponent>();
            const float column_width = 150.f;

            switch (mesh_filter.source_type)
            {
                case MeshSource::Primitive:
                {
                    const AssetHandle material_handle = !component.material_overrides.empty() ? component.material_overrides[0] : AssetHandle_Invalid;
                    const bool preview_check = component.material_overrides.empty() || !AssetManager::IsHandleValid(component.material_overrides[0]);
                    const std::string preview = preview_check ? "Select Material" : AssetManager::GetAssetPath(material_handle).stem();

                    ImGui::Text("Material");
                    ImGui::SameLine(column_width - ImGui::GetCursorPosX());
                    ImGui::SetNextItemWidth(-1.f);
                    if (ImGui::BeginCombo("##Material", preview.c_str()))
                    {
                        const bool none_selected = component.material_overrides.empty() ? true : component.material_overrides[0] == AssetHandle_Invalid;
                        if (ImGui::Selectable("None", none_selected))
                            if (!component.material_overrides.empty())
                                component.material_overrides[0] = AssetHandle_Invalid;

                        const std::vector<AssetHandle> materials = AssetManager::GetAllHandlesOfType(AssetType::Material);
                        for (const AssetHandle handle : materials)
                        {
                            const std::string name = AssetManager::GetAssetPath(handle).stem();
                            const bool is_selected = component.material_overrides.empty() ? false : component.material_overrides[0] == handle;
                            if (ImGui::Selectable(name.c_str(), is_selected))
                            {
                                if (!component.material_overrides.empty())
                                    component.material_overrides[0] = handle;
                                else
                                    component.material_overrides.push_back(handle);
                            }
                        }

                        ImGui::EndCombo();
                    }

                    break;
                }

                case MeshSource::Model:
                    break;

                default:
                    break;
            }
        };

        const auto DrawAnimatorComponent = [](AnimatorComponent& component)
        {
        };

        const auto DrawDirectionalLightComponent = [](DirectionalLightComponent& component)
        {
            const float column_width = 150.f;

            ImGui::Text("Intensity");
            ImGui::SameLine(column_width - ImGui::GetCursorPosX());
            ImGui::SetNextItemWidth(-1.f);
            ImGui::DragFloat("##Sun Intensity", &component.light.intensity, 0.1f, 0.1f, 100.f);

            ImGui::Text("Color");
            ImGui::SameLine(column_width - ImGui::GetCursorPosX());
            ImGui::ColorEdit3("##Sun Color", &component.light.color.x, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

            ImGui::Text("Is Primary?");
            ImGui::SameLine(column_width - ImGui::GetCursorPosX());
            ImGui::Checkbox("##Is Sun Primary?", &component.is_primary);
        };

        const auto DrawPerspectiveCameraComponent = [selection_context](PerspectiveCameraComponent& component)
        {
            const float column_width = 150.f;
            const std::string preview = component.target_entity.IsValid() ? component.target_entity.GetComponent<InternalComponent>().tag : "Select a target";

            ImGui::Text("Target");
            ImGui::SameLine(column_width - ImGui::GetCursorPosX());
            ImGui::SetNextItemWidth(-1.f);
            if (ImGui::BeginCombo("##Target", preview.c_str()))
            {
                Scene* const active_scene = Scenes::GetActive();
                for (const Entity entity : Views::Create<InternalComponent>(*active_scene))
                {
                    const auto& internal = entity.GetComponent<InternalComponent>();
                    const auto& internal_selection = selection_context.GetComponent<InternalComponent>();

                    if (internal.tag == internal_selection.tag) // Don't allow setting the same entity as it's target
                        continue;

                    if (ImGui::Selectable(internal.tag.c_str(), internal.tag == preview))
                        component.target_entity = entity;
                }

                ImGui::EndCombo();
            }

            ImGui::Text("FOV");
            ImGui::SameLine(column_width - ImGui::GetCursorPosX());
            ImGui::SetNextItemWidth(-1.f);
            ImGui::DragFloat("##FOV", &component.camera.fov);

            ImGui::Text("Near Clip");
            ImGui::SameLine(column_width - ImGui::GetCursorPosX());
            ImGui::SetNextItemWidth(-1.f);
            ImGui::DragFloat("##Near Clip", &component.camera.clip_near);

            ImGui::Text("Far Clip");
            ImGui::SameLine(column_width - ImGui::GetCursorPosX());
            ImGui::SetNextItemWidth(-1.f);
            ImGui::DragFloat("##Far Clip", &component.camera.clip_far);

            ImGui::Text("Is Primary?");
            ImGui::SameLine(column_width - ImGui::GetCursorPosX());
            ImGui::Checkbox("##Is Camera Primary?", &component.is_primary);
        };

        DrawComponent<TransformComponent>("Transform", selection_context, DrawTransformComponent);
        DrawComponent<MeshFilterComponent>("Mesh Filter", selection_context, DrawMeshFilterComponent);
        DrawComponent<MeshRendererComponent>("Mesh Renderer", selection_context, DrawMeshRendererComponent);
        DrawComponent<AnimatorComponent>("Animator", selection_context, DrawAnimatorComponent);
        DrawComponent<DirectionalLightComponent>("Directional Light", selection_context, DrawDirectionalLightComponent);
        DrawComponent<PerspectiveCameraComponent>("Perspective Camera", selection_context, DrawPerspectiveCameraComponent);
    }

    template <typename T, typename UIFunction>
    void DrawComponent(const char* name, Entity entity, UIFunction callback)
    {
        if (!entity.IsValid())
            return;

        const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen |
                                         ImGuiTreeNodeFlags_AllowOverlap;

        if (entity.HasComponent<T>())
        {
            ImGui::PushID(name);
            auto& component = entity.GetComponent<T>();

            const ImVec2 available_region = ImGui::GetContentRegionAvail();
            const float line_height = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.f;
            const bool is_open = ImGui::CollapsingHeader(name, flags);
            bool remove_component = false;

            if (strcmp(name, "Transform") != 0)
            {
                ImGui::SameLine(available_region.x - line_height * 0.5f);
                if (ImGui::Button("+", ImVec2(line_height, line_height)))
                    ImGui::OpenPopup("Component Settings");

                if (ImGui::BeginPopup("Component Settings"))
                {
                    if (ImGui::MenuItem("Remove component"))
                        remove_component = true;

                    ImGui::EndPopup();
                }
            }
            else
            {
                ImGui::SameLine(available_region.x - line_height * 0.5f);
                if (ImGui::Button("+", ImVec2(line_height, line_height)))
                    ImGui::OpenPopup("Component Settings");

                if (ImGui::BeginPopup("Component Settings"))
                {
                    if (ImGui::MenuItem("Reset"))
                        entity.GetComponent<TransformComponent>().Reset();

                    ImGui::EndPopup();
                }
            }

            ImGui::PopID();

            if (is_open)
                callback(component);

            if (remove_component)
                entity.RemoveComponent<T>();
        }
    }

    void DisplayAddComponentButton(Entity selection_context)
    {
        if (!selection_context.IsValid())
            return;

        const ImGuiStyle& style = ImGui::GetStyle();
        const float avail_width = ImGui::GetContentRegionAvail().x;
        const float line_height = ImGui::GetFontSize() + style.FramePadding.y * 2.f;
        const ImVec2 button_size = ImVec2(avail_width * 0.85f, line_height);
        const float button_offset = style.WindowPadding.x + ((avail_width - button_size.x) * 0.5f);

        ImGui::Separator();

        ImGui::SetCursorPosX(button_offset);
        if (ImGui::Button("Add Component", button_size))
            ImGui::OpenPopup("Popup Add Component");

        if (ImGui::BeginPopup("Popup Add Component"))
        {
            const bool add_mesh_filter = ImGui::MenuItem("Mesh Filter");
            const bool add_mesh_renderer = ImGui::MenuItem("Mesh Renderer");
            const bool add_animator = ImGui::MenuItem("Animator");
            const bool add_perspective = ImGui::MenuItem("Perspective Camera");
            const bool add_directional_light = ImGui::MenuItem("Directional Light");

            if (add_mesh_filter)
            {
                selection_context.AddComponent<MeshFilterComponent>(PrimitiveMesh::Cube);
                ImGui::CloseCurrentPopup();
            }

            if (add_mesh_renderer)
            {
                selection_context.AddComponent<MeshRendererComponent>();
                ImGui::CloseCurrentPopup();
            }

            if (add_animator)
            {
                selection_context.AddComponent<AnimatorComponent>();
                ImGui::CloseCurrentPopup();
            }

            if (add_perspective)
            {
                selection_context.AddComponent<PerspectiveCameraComponent>(false);
                ImGui::CloseCurrentPopup();
            }

            if (add_directional_light)
            {
                selection_context.AddComponent<DirectionalLightComponent>(glm::vec4(0.96f, 0.92f, 0.88f, 1.f), 1.f, false);
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void DisplayAssetControls(AssetType asset_type, AssetHandle asset_handle, const std::filesystem::path& browser_selection)
    {
        if (!AssetManager::IsHandleValid(asset_handle))
        {
            const ImGuiStyle& style = ImGui::GetStyle();
            const float avail_width = ImGui::GetContentRegionAvail().x;
            const float line_height = ImGui::GetFontSize() + style.FramePadding.y * 2.f;
            const ImVec2 button_size = ImVec2(avail_width * 0.85f, line_height);
            const float button_offset = style.WindowPadding.x + ((avail_width - button_size.x) * 0.5f);
            const std::string_view message = "This asset hasn't been imported to the registry yet,\n import it to start editing its properties.";

            ImGui::TextUnformatted(message.data(), message.data() + message.size());
            ImGui::SetCursorPosX(button_offset);
            if (ImGui::Button("Import", button_size))
            {
                const AssetType type = PathToAssetType(browser_selection);
                AssetManager::ImportByPath(browser_selection, type);
            }

            return;
        }

        switch (asset_type)
        {
            case AssetType::AudioClip:
            {
                ImGui::TextUnformatted("[AUDIO CLIP CONTROLS PLACEHOLDER]");
                break;
            }
            case AssetType::AnimationClip:
            {
                ImGui::TextUnformatted("[ANIMATION CLIP CONTROLS PLACEHOLDER]");
                break;
            }
            case AssetType::Material:
            {
                const float column_width = 150.f;
                Material* const material = AssetManager::GetAsset<Material>(asset_handle);

                ImGui::TextUnformatted(browser_selection.string().c_str());

                ImGui::TextUnformatted("Albedo");
                ImGui::SameLine(column_width - ImGui::GetCursorPosX());
                ImGui::ColorEdit3("##Material Albedo", &material->albedo.r, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

                ImGui::TextUnformatted("Metallic");
                ImGui::SameLine(column_width - ImGui::GetCursorPosX());
                ImGui::SetNextItemWidth(-1.f);
                ImGui::DragFloat("##Material Metallic", &material->metallic, 0.01f, 0.f, 1.f);

                ImGui::TextUnformatted("Roughness");
                ImGui::SameLine(column_width - ImGui::GetCursorPosX());
                ImGui::SetNextItemWidth(-1.f);
                ImGui::DragFloat("##Material Roughness", &material->roughness, 0.01f, 0.f, 1.f);

                ImGui::Separator();

                const ImGuiStyle& style = ImGui::GetStyle();
                const float avail_width = ImGui::GetContentRegionAvail().x;
                const float line_height = ImGui::GetFontSize() + style.FramePadding.y * 2.f;
                const ImVec2 button_size = ImVec2(avail_width * 0.85f, line_height);
                const float button_offset = style.WindowPadding.x + ((avail_width - button_size.x) * 0.5f);
                ImGui::SetCursorPosX(button_offset);
                if (ImGui::Button("Save", button_size))
                    Materials::Export(browser_selection, *material);

                break;
            }
            case AssetType::Model:
            {
                ImGui::TextUnformatted("[STANDARD MODEL CONTROLS PLACEHOLDER]");
                break;
            }
            case AssetType::ModelAnimated:
            {
                ImGui::TextUnformatted("[ANIMATED MODEL CONTROLS PLACEHOLDER]");
                break;
            }
            case AssetType::Texture:
            {
                ImGui::TextUnformatted("[TEXTURE CONTROLS PLACEHOLDER]");
                break;
            }

            default:
                ImGui::TextUnformatted("[UNSUPPORTED ASSET TYPE PLACEHOLDER]");
                break;
        }
    }

    AssetType PathToAssetType(const std::filesystem::path& path)
    {
        local_persist std::unordered_map<std::filesystem::path, AssetType> path_cache;

        const auto cached = path_cache.find(path);
        if (cached != path_cache.end())
            return cached->second;

        // Trust the registry first - it's authoritative, and it already
        // disambiguates cases (like .fbx) that extension alone can't.
        AssetType type = AssetType::Invalid;
        if (AssetManager::IsAssetRegisteredByPath(path))
        {
            const AssetHandle handle = AssetManager::FindAssetHandleByPath(path);
            type = AssetManager::GetAssetType(handle);
        }

        // Not imported yet - fall back to a best-effort guess.
        if (type == AssetType::Invalid)
            type = GuessAssetTypeFromPath(path);

        path_cache.emplace(path, type);
        return type;
    }

    AssetType GuessAssetTypeFromPath(const std::filesystem::path& path)
    {
        local_persist const std::unordered_map<std::filesystem::path, AssetType> k_ExtensionMap = {
            { ".png", AssetType::Texture },
            { ".jpg", AssetType::Texture },
            { ".jpeg", AssetType::Texture },
            { ".hdr", AssetType::Texture },
            { ".wav", AssetType::AudioClip },
            { ".mp3", AssetType::AudioClip },
            { ".ogg", AssetType::AudioClip },
            { ".mat", AssetType::Material },
        };

        const std::filesystem::path extension = path.extension();

        if (extension == ".fbx")
        {
            // Folder convention resolves what the extension alone can't.
            // NOTE: still can't tell Model apart from ModelAnimated this way -
            // that needs to actually inspect the file for a skeleton. Defaulting
            // to Model until that exists.
            for (const auto& segment : path)
            {
                if (segment == "Animations")
                    return AssetType::AnimationClip;
            }
            return AssetType::Model;
        }

        const auto it = k_ExtensionMap.find(extension);
        return it != k_ExtensionMap.end() ? it->second : AssetType::Invalid;
    }

    std::string PrimitiveToString(PrimitiveMesh primitive)
    {
        switch (primitive)
        {
            case PrimitiveMesh::None:
                return "None";
            case PrimitiveMesh::Quad:
                return "Quad";
            case PrimitiveMesh::Cube:
                return "Cube";
            case PrimitiveMesh::Cone:
                return "Cone";
            case PrimitiveMesh::Plane:
                return "Plane";
            case PrimitiveMesh::Pyramid:
                return "Pyramid";
            case PrimitiveMesh::Sphere:
                return "Sphere";
            case PrimitiveMesh::Torus:
                return "Torus";
            default:
                return "Invalid";
        }
    }
}
