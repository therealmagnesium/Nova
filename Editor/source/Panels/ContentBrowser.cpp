#include "Panels/ContentBrowser.h"
#include "Panels/SceneHierarchy.h"
#include "MenuAction.h"

#include <imgui.h>
#include <system_error>

using namespace Nova;

local constexpr u8 k_MaxNameCharacters = 32;

struct ContentBrowserState
{
    std::filesystem::path path_selection;
    Texture texture_file = Stub_Texture;
    Texture texture_folder = Stub_Texture;
    char buffer_input[k_MaxNameCharacters + 1] = {};
    bool show_folder_popup = false;
    bool show_rename_popup = false;
    bool show_project_create_modal = false;
    bool should_display = true;
};

local ContentBrowserState state;

local constexpr ImGuiWindowFlags k_FlagsWindow = ImGuiWindowFlags_None;
local constexpr ImGuiWindowFlags k_FlagsModal = ImGuiWindowFlags_AlwaysAutoResize;
local constexpr auto Callback_CreateFolder = [](Scene& scene)
{
    state.show_folder_popup = true;
};
local constexpr auto Callback_CreateMaterial = [](Scene& scene)
{
    const std::filesystem::path path = std::filesystem::current_path() / "Material.mat";
    if (!std::filesystem::exists(path))
        Materials::Export(path);

    const AssetHandle asset_material = AssetManager::ImportByPath(path, AssetType::Material);
    Material* const material = AssetManager::GetAsset<Material>(asset_material);
    Materials::Export(path, *material);

    state.path_selection = path;
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
    void DisplayProjectSetupModal();
    void DisplayCreateAssetPopup();
    void DisplayCreateFolderPopup();
    void DisplayContentBrowser();
    void DisplayPopupRename();

    void Init()
    {
    }

    void Display()
    {
        ImGui::Begin("Content Browser", &state.should_display, k_FlagsWindow);

        DisplayProjectSetupModal();
        DisplayCreateAssetPopup();
        DisplayCreateFolderPopup();
        DisplayContentBrowser();
        DisplayPopupRename();

        ImGui::End();
    }

    std::filesystem::path GetSelectionContext() { return state.path_selection; }
    void SetSelectionContext(const std::filesystem::path& path) { state.path_selection = path; }

    void DisplayProjectSetupModal()
    {
        Project& project = Projects::GetContext();

        const char* modal_project_select = "Select A Project";
        const char* modal_project_create = "Create A New Project";

        // Trigger the secondary modal next frame if flagged
        if (state.show_project_create_modal)
        {
            ImGui::OpenPopup(modal_project_create);
            state.show_project_create_modal = false;
        }
        else if (!project.IsValid() && !ImGui::IsPopupOpen(modal_project_create))
            ImGui::OpenPopup(modal_project_select);

        if (ImGui::BeginPopupModal(modal_project_select, NULL, k_FlagsModal))
        {
            ImGui::TextUnformatted("No valid project selected, please create or load an existing project.");
            if (ImGui::Button("Create"))
            {
                const FileDialogFilter filter = {
                    .name = "Project",
                    .specification = "nproj",
                };

                if (FileDialogs::AskSave(&filter, 1))
                {
                    const std::filesystem::path& path_selected = FileDialogs::GetPathSelected();
                    project.path_config = path_selected;
                    project.path_directory = path_selected.parent_path();

                    ImGui::CloseCurrentPopup();
                    state.show_project_create_modal = true;

                    const std::string stem = path_selected.stem();
                    stem.copy(state.buffer_input, sizeof(state.buffer_input) - 1);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Load"))
            {
                const FileDialogFilter filter = {
                    .name = "Project",
                    .specification = "nproj",
                };

                if (FileDialogs::AskOpen(&filter, 1))
                {
                    const std::filesystem::path& path_selected = FileDialogs::GetPathSelected();
                    project = Projects::Import(path_selected);
                    std::filesystem::current_path(Projects::GetAssetPath(project));

                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal(modal_project_create, NULL, k_FlagsModal))
        {
            if (ImGui::InputText("##Name", state.buffer_input, sizeof(char) * k_MaxNameCharacters, ImGuiInputTextFlags_EnterReturnsTrue))
                project.name = std::string_view(state.buffer_input);

            if (ImGui::Button("Create"))
            {
                if (state.buffer_input[0] != '\0')
                {
                    project.name = std::string(state.buffer_input);
                    std::fill(std::begin(state.buffer_input), std::end(state.buffer_input), '\0');
                }

                const std::filesystem::path& path_selected = FileDialogs::GetPathSelected();
                Projects::Export(path_selected, project);

                try
                {
                    std::filesystem::current_path(project.path_directory);
                    if (std::filesystem::create_directory("Assets"))
                        INFO("Created Assets folder for project \"%s\" successfully", project.name.c_str());
                    else
                        ERROR("ContentBrowserPanel::Display - Failed to create Assets folder for project \"%s\"", project.name.c_str());
                }
                catch (const std::filesystem::filesystem_error& e)
                {
                    ERROR("ContentBrowserPanel::Display - Failed to create assets directory for project \"%s\" (std::filesystem): %s", project.name.c_str(), e.what());
                }

                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }

        const bool should_deselect = ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        if (should_deselect)
            state.path_selection.clear();
    }

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

        if (!ImGui::BeginPopup("Popup Create Folder"))
            return;

        ImGui::Text("Folder Name");
        if (ImGui::InputText("##Input Folder Name", state.buffer_input, sizeof(char) * LEN(state.buffer_input), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            const std::string folder_name = std::string(state.buffer_input);
            std::fill(std::begin(state.buffer_input), std::end(state.buffer_input), '\0');

            try
            {
                if (std::filesystem::create_directory(folder_name))
                    INFO("Created folder %s successfully", (std::filesystem::current_path() / folder_name).string().c_str());
                else
                    WARN("Something went wrong with creating folder %s", (std::filesystem::current_path() / folder_name).string().c_str());

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

    void DisplayContentBrowser()
    {
        const Project& project = Projects::GetContext();
        const std::filesystem::path path_assets = Projects::GetAssetPath(project);

        if (std::filesystem::current_path() != path_assets)
        {
            const float text_width = ImGui::CalcTextSize("<-").x;
            const std::string path_text = Projects::GetAssetPathRelative(std::filesystem::current_path(), project);
            ImGui::TextUnformatted(path_text.c_str());
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - text_width);

            if (ImGui::Button("<-"))
            {
                const std::filesystem::path path_parent = std::filesystem::current_path().parent_path();
                std::filesystem::current_path(path_parent);
            }
        }

        u32 entry_index = 0;
        for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path()))
        {
            ImGui::PushID(entry_index++);
            const bool is_directory = entry.is_directory();
            const bool is_file = entry.is_regular_file();

            if (is_directory)
            {
                const std::string directory_name = entry.path().filename().string();

                ImGui::Button(directory_name.c_str());
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    std::filesystem::path path_working = std::filesystem::current_path();
                    std::filesystem::current_path(path_working / directory_name);
                }
            }
            else if (is_file)
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
                state.path_selection = entry.path();

                if (ImGui::MenuItem("Open in File Explorer"))
                    FileDialogs::OpenExplorer(state.path_selection.parent_path());

                if (ImGui::MenuItem("Rename"))
                {
                    state.show_rename_popup = true;

                    const std::string filename = entry.path().filename();
                    filename.copy(state.buffer_input, sizeof(state.buffer_input) - 1);
                }

                if (ImGui::MenuItem("Delete"))
                {
                    try
                    {
                        const std::filesystem::path& path_absolute = entry.path();
                        const std::filesystem::path& path_relative = Projects::GetAssetPathRelative(path_absolute, project);

                        if (AssetManager::IsAssetRegisteredByPath(path_relative))
                        {
                            const AssetHandle handle = AssetManager::FindAssetHandleByPath(path_relative);
                            AssetManager::Remove(handle);
                        }

                        const bool removed = entry.is_directory()
                                                 ? std::filesystem::remove_all(path_absolute) > 0
                                                 : std::filesystem::remove(path_absolute);
                        if (removed)
                        {
                            INFO("Deleted %s from disk successfully", path_absolute.string().c_str());
                            state.path_selection.clear();
                        }
                        else
                            WARN("Something went wrong deleting %s", path_absolute.string().c_str());
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

    void DisplayPopupRename()
    {
        if (state.show_rename_popup)
        {
            ImGui::OpenPopup("Popup Rename");
            state.show_rename_popup = false;
        }

        if (!ImGui::BeginPopup("Popup Rename"))
            return;

        if (ImGui::InputText("##Input Folder Name", state.buffer_input, sizeof(char) * LEN(state.buffer_input), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            const std::string new_name = std::string(state.buffer_input);
            const std::filesystem::path new_path = state.path_selection.parent_path() / new_name;
            std::fill(std::begin(state.buffer_input), std::end(state.buffer_input), '\0');

            std::error_code e;
            std::filesystem::rename(state.path_selection, new_path, e);

            if (e)
            {
                ERROR("Failed to rename %s -> %s", state.path_selection.string().c_str(), new_path.string().c_str());
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            }

            const Project& project = Projects::GetContext();
            const std::filesystem::path path_relative = Projects::GetAssetPathRelative(state.path_selection, project);
            if (AssetManager::IsAssetRegisteredByPath(path_relative))
            {
                const AssetHandle handle = AssetManager::FindAssetHandleByPath(path_relative);
                const AssetType type = AssetManager::GetAssetType(handle);
                AssetManager::Remove(handle);
                AssetManager::ImportByPath(new_path, type, handle);
            }

            state.path_selection = new_path;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
