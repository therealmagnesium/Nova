#include "Panels/ContentBrowser.h"
#include "Panels/SceneHierarchy.h"
#include "MenuAction.h"

#include <imgui.h>

using namespace Nova;

local constexpr u8 k_MaxNameCharacters = 32;

struct ContentBrowserState
{
    std::filesystem::path path_assets;
    std::filesystem::path path_working;
    std::filesystem::path path_selection;
    char input_folder_name[k_MaxNameCharacters + 1] = {};
    bool show_folder_popup = false;
    bool should_display = true;
};

local ContentBrowserState state;

local constexpr ImGuiWindowFlags k_Flags = ImGuiWindowFlags_None;
local constexpr auto Callback_CreateFolder = [](Scene& scene)
{
    state.show_folder_popup = true;
};
local constexpr auto Callback_CreateMaterial = [](Scene& scene)
{
    const std::filesystem::path path = state.path_working / "Material.mat";
    Materials::Export(path);

    const AssetHandle asset_material = AssetManager::ImportByPath(path, AssetType::Material);
    Material* const material = AssetManager::GetAsset<Material>(asset_material);
    Materials::Export(path, *material);
};

local const MenuAction k_CreateActions[] = {
    MenuAction{
        .label = "Folder",
        .execute = Callback_CreateFolder,
    },
    MenuAction{
        .label = "Material (PBR)",
        .separator_text = "Assets",
        .execute = Callback_CreateMaterial,
    },
    MenuAction{
        .label = "Shader",
        .execute = NULL,
    }
};

namespace ContentBrowserPanel
{
    void DisplayCreateAssetPopup();
    void DisplayCreateFolderPopup();
    void DisplayContentBrowser();

    void Init()
    {
        state.path_assets = std::filesystem::current_path() / "Assets";
        state.path_working = state.path_assets;
    }

    void Display()
    {
        ImGui::Begin("Content Browser", &state.should_display, k_Flags);
        const bool should_deselect = ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        if (should_deselect)
            state.path_selection.clear();

        DisplayCreateAssetPopup();
        DisplayCreateFolderPopup();
        DisplayContentBrowser();

        ImGui::End();
    }

    std::filesystem::path GetSelectionContext() { return state.path_selection; }
    void SetSelectionContext(const std::filesystem::path& path) { state.path_selection = path; }

    void DisplayCreateAssetPopup()
    {
        if (!ImGui::BeginPopupContextWindow("##Popup Create Asset"))
            return;

        Scene* const scene_active = Scenes::GetActive();
        if (ImGui::BeginMenu("Create"))
        {
            for (const auto& action : k_CreateActions)
            {
                if (!action.separator_text.empty())
                    ImGui::SeparatorText(action.separator_text.data());

                if (ImGui::MenuItem(action.label.data()) && action.execute != NULL)
                    action.execute(*scene_active);
            }

            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }

    void DisplayCreateFolderPopup()
    {
        if (state.show_folder_popup)
        {
            ImGui::OpenPopup("Popup Create Folder");
            state.show_folder_popup = false;
        }

        if (ImGui::BeginPopup("Popup Create Folder"))
        {
            ImGui::Text("Folder Name");
            if (ImGui::InputText("##Input Folder Name", state.input_folder_name, sizeof(char) * LEN(state.input_folder_name), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                const std::string folder_name = std::string(state.input_folder_name);
                std::fill(std::begin(state.input_folder_name), std::end(state.input_folder_name), '\0');

                try
                {
                    if (std::filesystem::create_directory(state.path_working / folder_name))
                        INFO("Created folder %s successfully", (state.path_working / folder_name).string().c_str());
                    else
                        WARN("Something went wrong with creating folder %s", (state.path_working / folder_name).string().c_str());

                    ImGui::CloseCurrentPopup();
                }
                catch (const std::filesystem::filesystem_error& e)
                {
                    WARN("ContentBrowserPanel::Callback_CreateFolder (std::filesystem) - %s", e.what());
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }
    }

    void DisplayContentBrowser()
    {
        if (state.path_working != state.path_assets)
        {
            const float text_width = ImGui::CalcTextSize("<-").x;
            const std::string path_text = std::filesystem::relative(state.path_working, state.path_assets.parent_path());
            ImGui::TextUnformatted(path_text.c_str());
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - text_width);

            if (ImGui::Button("<-"))
            {
                const std::filesystem::path path_parent = state.path_working.parent_path();
                state.path_working = path_parent;
            }
        }

        u32 entry_index = 0;
        for (const auto& entry : std::filesystem::directory_iterator(state.path_working))
        {
            ImGui::PushID(entry_index++);

            if (entry.is_directory())
            {
                const std::string directory_name = entry.path().filename().string();

                ImGui::Button(directory_name.c_str());
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    state.path_working /= directory_name;
            }
            else if (entry.is_regular_file())
            {
                const std::string file_name = entry.path().filename();
                if (ImGui::Button(file_name.c_str()))
                {
                    state.path_selection = entry.path();
                    SceneHierarchyPanel::SetSelectionContext(Stub_Entity);
                }
            }

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete"))
                {
                    try
                    {
                        const std::filesystem::path& path = entry.path();

                        if (AssetManager::IsAssetRegisteredByPath(path))
                        {
                            const AssetHandle handle = AssetManager::FindAssetHandleByPath(path);
                            AssetManager::Remove(handle);
                        }

                        const bool removed = entry.is_directory()
                                                 ? std::filesystem::remove_all(path) > 0
                                                 : std::filesystem::remove(path);
                        if (removed)
                        {
                            INFO("Deleted %s from disk successfully", path.string().c_str());
                            state.path_selection.clear();
                        }
                        else
                            WARN("Something went wrong deleting %s", path.string().c_str());
                    }
                    catch (const std::filesystem::filesystem_error& e)
                    {
                        const std::filesystem::path& path = entry.path();
                        WARN("Failed to delete %s from disk (std::filesystem) - %s", path.string().c_str(), e.what());
                    }
                }

                ImGui::EndPopup();
            }

            ImGui::PopID();
        }
    }
}
