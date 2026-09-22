#include "Panels/SceneHierarchy.h"
#include "MenuAction.h"

#include <imgui.h>

using namespace Nova;

struct SceneHierarchyState
{
    Scene* context = NULL;
    Entity selection_context = Stub_Entity;
    bool should_display = true;
};

local SceneHierarchyState state;

local constexpr auto Callback_CreateBlankEntity = [](Scene& scene)
{
    state.selection_context = Scenes::CreateEntity(scene);
};

local constexpr auto Callback_CreateMeshPrimitive = [](Scene& scene)
{
    Entity entity = Scenes::CreateEntity(scene, "Cube");
    entity.AddComponent<MeshFilterComponent>(PrimitiveMesh::Cube);
    entity.AddComponent<MeshRendererComponent>();

    state.selection_context = entity;
};

local constexpr auto Callback_CreateMeshStandard = [](Scene& scene)
{
    Entity entity = Scenes::CreateEntity(scene, "Standard Mesh");
    entity.AddComponent<MeshFilterComponent>(AssetHandle_Invalid);
    entity.AddComponent<MeshRendererComponent>();

    state.selection_context = entity;
};

local constexpr auto Callback_CreateMeshSkinned = [](Scene& scene)
{
    Entity entity = Scenes::CreateEntity(scene, "Skinned Mesh");
    entity.AddComponent<AnimatorComponent>();

    state.selection_context = entity;
};

local constexpr auto Callback_CreateLightDirectional = [](Scene& scene)
{
    Entity entity = Scenes::CreateEntity(scene, "Directional Light");
    entity.AddComponent<DirectionalLightComponent>();

    state.selection_context = entity;
};

local constexpr auto Callback_CreateCameraPerspective = [](Scene& scene)
{
    Entity entity = Scenes::CreateEntity(scene, "Perspective Camera");
    entity.AddComponent<PerspectiveCameraComponent>();

    state.selection_context = entity;
};

local const MenuAction k_CreateActions[] = {
    MenuAction{
        .label = "Blank Entity",
        .execute = Callback_CreateBlankEntity,
    },
    MenuAction{
        .label = "Primitive Mesh",
        .separator_text = "Meshes",
        .execute = Callback_CreateMeshPrimitive,
    },
    MenuAction{
        .label = "Standard Mesh",
        .execute = Callback_CreateMeshStandard,
    },
    MenuAction{
        .label = "Skinned Mesh",
        .execute = Callback_CreateMeshSkinned,
    },
    MenuAction{
        .label = "Directional Light",
        .separator_text = "Lights",
        .execute = Callback_CreateLightDirectional,
    },
    MenuAction{
        .label = "Perspective Camera",
        .separator_text = "Cameras",
        .execute = Callback_CreateCameraPerspective,
    },
};

namespace SceneHierarchyPanel
{
    void DisplayCreateEntityPopup();
    void DisplaySceneHierarchy();
    void DisplayNode(Entity entity);

    void Display()
    {
        state.context = Scenes::GetActive();
        if (!state.should_display || state.context == NULL)
            return;

        if (ImGui::Begin("Scene Hierarchy", &state.should_display))
        {

            const bool should_deselect = ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
            if (should_deselect)
                state.selection_context = Stub_Entity;

            DisplayCreateEntityPopup();
            DisplaySceneHierarchy();
        }
        ImGui::End();
    }

    void DisplayNode(Entity entity)
    {

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        const bool has_children = false;
        const ImGuiTreeNodeFlags flags_node = ImGuiTreeNodeFlags_DefaultOpen |
                                              ImGuiTreeNodeFlags_DrawLinesFull;
        const ImGuiTreeNodeFlags flags_child = ImGuiTreeNodeFlags_Leaf |
                                               ImGuiTreeNodeFlags_Bullet |
                                               ImGuiTreeNodeFlags_NoTreePushOnOpen;
        const ImGuiTreeNodeFlags flags_selected = state.selection_context == entity ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None;

        ImGui::PushID(entity.id);
        const std::string& tag = entity.GetComponent<InternalComponent>().tag;
        ImGui::TreeNodeEx(tag.c_str(), flags_node | flags_child | flags_selected);
        if (ImGui::IsItemClicked())
            state.selection_context = entity;
        ImGui::PopID();

        const std::string unique_popup_id = std::format("##Popup Destroy Entity_{}", entity.id);
        if (!ImGui::BeginPopupContextItem(unique_popup_id.c_str()))
            return;

        const auto& internal = entity.GetComponent<InternalComponent>();
        const std::string label = std::format("Destroy {}", internal.tag.c_str());
        if (ImGui::MenuItem(label.c_str()))
        {
            Scenes::DestroyEntity(*state.context, entity);
            state.selection_context = Stub_Entity;
        }

        ImGui::EndPopup();
    }

    void DisplaySceneHierarchy()
    {
        const ImGuiTableFlags table_flags = ImGuiTableFlags_BordersV |
                                            ImGuiTableFlags_BordersOuterH |
                                            ImGuiTableFlags_Resizable |
                                            ImGuiTableFlags_RowBg |
                                            ImGuiTableFlags_NoBordersInBody;

        if (ImGui::BeginTable("Scene Heirarchy Table", 1, table_flags))
        {
            for (Entity entity : Views::Create<InternalComponent>(*state.context))
                DisplayNode(entity);

            ImGui::EndTable();
        }
    }

    void DisplayCreateEntityPopup()
    {
        if (!ImGui::BeginPopupContextWindow("##Popup Create Entity"))
            return;

        if (ImGui::BeginMenu("Create"))
        {
            for (const auto& action : k_CreateActions)
            {
                if (!action.separator_text.empty())
                    ImGui::SeparatorText(action.separator_text.data());

                if (ImGui::MenuItem(action.label.data()))
                    action.execute(*state.context);
            }

            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }

    Entity GetSelectionContext() { return state.selection_context; }
    void SetSelectionContext(Entity entity) { state.selection_context = entity; }
}
