#include "Panels/ContentBrowser.h"
#include "Panels/SceneHierarchy.h"
#include "MenuAction.h"

#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <string>
#include <system_error>
#include <vector>

using namespace Nova;

local constexpr u8 k_MaxNameCharacters = 32;
local constexpr u32 k_AssetTypeCount = static_cast<u32>(AssetType::_Length);

// Grid layout
local constexpr float k_ThumbnailMin = 64.f;
local constexpr float k_ThumbnailMax = 192.f;
local constexpr float k_ThumbnailZoomStep = 8.f;
local constexpr float k_SpacingMin = 0.f;
local constexpr float k_SpacingMax = 128.f;
local constexpr float k_LabelGap = 4.f;
local constexpr float k_CellRounding = 4.f;

// Flip to false once every asset type has its own icon: files then show their stem only.
local constexpr bool k_ShowFileExtensions = true;

struct ContentBrowserState
{
    std::filesystem::path path_selection;
    Texture texture_file = Stub_Texture;
    Texture texture_folder = Stub_Texture;
    Texture icons[k_AssetTypeCount] = {}; // indexed by AssetType; invalid entries fall back to texture_file
    float thumbnail_size = 96.f;
    float padding = 8.f;
    float spacing = 32.f;
    char buffer_input[k_MaxNameCharacters + 1] = {};
    bool show_folder_popup = false;
    bool show_rename_popup = false;
    bool show_project_create_modal = false;
    bool should_display = true;
};

struct BrowserEntry
{
    std::filesystem::directory_entry entry;
    std::string name;  // filename with extension, used for sorting
    std::string label; // what the grid displays
    bool is_directory = false;
};

local ContentBrowserState state;

local constexpr ImGuiWindowFlags k_FlagsWindow = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
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
    void DisplayToolbar(const Project& project, bool is_at_root);
    void DisplayEntryContextMenu(const std::filesystem::directory_entry& entry, const Project& project);
    void DisplayEntryGrid(const Project& project);
    void DisplayFooter();
    void DisplayContentBrowser();
    void DisplayPopupRename();

    const char* GetIconPath(AssetType type);
    const Texture& GetIcon(bool is_directory, AssetType type);
    std::vector<BrowserEntry> GatherEntries(const std::filesystem::path& directory);
    void DrawLabel(ImDrawList* draw_list, const ImVec2& position, float max_width, const std::string& label);

    void Init()
    {
        state.texture_file = Textures::Load("Assets/Textures/Icon_File.png");
        state.texture_folder = Textures::Load("Assets/Textures/Icon_Folder.png");

        for (u32 i = 0; i < k_AssetTypeCount; i++)
        {
            const char* const path = GetIconPath(static_cast<AssetType>(i));
            if (path != NULL && std::filesystem::exists(path))
                state.icons[i] = Textures::Load(path);
        }
    }

    void Shutdown()
    {
        Textures::Unload(state.texture_file);
        Textures::Unload(state.texture_folder);

        for (Texture& icon : state.icons)
        {
            if (icon.IsValid())
                Textures::Unload(icon);
        }
    }

    void Display()
    {
        ImGui::Begin("Content Browser", &state.should_display, k_FlagsWindow);

        DisplayProjectSetupModal();
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

    void DisplayToolbar(const Project& project, bool is_at_root)
    {
        std::string path_text = "Assets";
        if (!is_at_root)
            path_text = Projects::GetAssetPathRelative(std::filesystem::current_path(), project);
        ImGui::TextUnformatted(path_text.c_str());

        if (is_at_root)
            return;

        const float width_back = ImGui::CalcTextSize("<-").x + ImGui::GetStyle().FramePadding.x * 2.f;
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - width_back + ImGui::GetCursorPosX());
        if (ImGui::Button("<-"))
            std::filesystem::current_path(std::filesystem::current_path().parent_path());
    }

    void DisplayFooter()
    {
        const float width_region = ImGui::GetContentRegionAvail().x;
        const float width_slider = width_region * 0.5f - ImGui::GetStyle().ItemSpacing.x * 2.f;

        ImGui::SetNextItemWidth(width_slider);
        ImGui::SliderFloat("##Thumbnail Size", &state.thumbnail_size, k_ThumbnailMin, k_ThumbnailMax, "Size  %.0f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(width_slider);
        ImGui::SliderFloat("##Cell Spacing", &state.spacing, k_SpacingMin, k_SpacingMax, "Spacing  %.0f", ImGuiSliderFlags_AlwaysClamp);
    }

    void DisplayEntryContextMenu(const std::filesystem::directory_entry& entry, const Project& project)
    {
        // Must be called right after the entry's InvisibleButton: BeginPopupContextItem attaches to the last item.
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
    }

    void DisplayEntryGrid(const Project& project)
    {
        const float thumbnail = state.thumbnail_size;
        const float padding = state.padding;
        const float height_label = ImGui::GetTextLineHeight();

        // Cell = icon + label + padding on every side; it is the hit area and the highlight rect.
        // Spacing is the gap *between* cells, so each table slot (the "pitch") is the cell plus one spacing.
        // The table uses zero cell padding and gets its gaps from the slot sizes instead, which keeps the math exact.
        const ImVec2 size_cell = ImVec2(thumbnail + padding * 2.f, padding + thumbnail + k_LabelGap + height_label + padding);
        const float pitch_x = size_cell.x + state.spacing;
        const float pitch_y = size_cell.y + state.spacing;
        const int column_count = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / pitch_x));

        const std::vector<BrowserEntry> entries = GatherEntries(std::filesystem::current_path());
        std::filesystem::path path_navigate;

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.f, 0.f));
        if (ImGui::BeginTable("##Content Grid", column_count, ImGuiTableFlags_NoSavedSettings))
        {
            for (int i = 0; i < column_count; i++)
                ImGui::TableSetupColumn(NULL, ImGuiTableColumnFlags_WidthFixed, pitch_x);

            ImDrawList* const draw_list = ImGui::GetWindowDrawList();

            u32 entry_index = 0;
            for (const BrowserEntry& item : entries)
            {
                const std::filesystem::directory_entry& entry = item.entry;

                // Rows are started by hand so each one can be given the full pitch (cell + vertical spacing).
                const int column = static_cast<int>(entry_index % column_count);
                if (column == 0)
                    ImGui::TableNextRow(ImGuiTableRowFlags_None, pitch_y);
                ImGui::TableSetColumnIndex(column);

                ImGui::PushID(entry_index++);

                const ImVec2 cell_min = ImGui::GetCursorScreenPos();
                const ImVec2 cell_max = ImVec2(cell_min.x + size_cell.x, cell_min.y + size_cell.y);

                ImGui::InvisibleButton("##Entry", size_cell);
                const bool is_hovered = ImGui::IsItemHovered();
                const bool is_selected = state.path_selection == entry.path();

                if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                {
                    state.path_selection = entry.path();
                    SceneHierarchyPanel::SetSelectionContext(Stub_Entity);
                }

                // Defer navigation: changing the working directory mid-iteration would desync the table with 'entries'
                if (is_hovered && item.is_directory && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    path_navigate = entry.path();

                // Background (hover / selection), inset by 1px so neighbouring cells don't touch
                if (is_selected || is_hovered)
                {
                    const ImU32 color = ImGui::GetColorU32(is_selected ? ImGuiCol_Header : ImGuiCol_HeaderHovered);
                    draw_list->AddRectFilled(ImVec2(cell_min.x + 1.f, cell_min.y + 1.f), ImVec2(cell_max.x - 1.f, cell_max.y - 1.f), color, k_CellRounding);
                }

                // Icon
                const AssetType type = item.is_directory ? AssetType::Invalid : PathToAssetType(Projects::GetAssetPathRelative(entry.path(), project));
                const Texture& icon = GetIcon(item.is_directory, type);
                if (icon.IsValid())
                {
                    const ImVec2 icon_min = ImVec2(cell_min.x + padding, cell_min.y + padding);
                    const ImVec2 icon_max = ImVec2(icon_min.x + thumbnail, icon_min.y + thumbnail);
                    draw_list->AddImage((ImTextureID)Textures::GetHandle(icon), icon_min, icon_max);
                }

                // Label
                const ImVec2 label_min = ImVec2(cell_min.x + padding, cell_min.y + padding + thumbnail + k_LabelGap);
                DrawLabel(draw_list, label_min, thumbnail, item.label);

                // Right-click menu: must come right after the InvisibleButton (the draw list calls above create no items)
                DisplayEntryContextMenu(entry, project);

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        if (!path_navigate.empty())
            std::filesystem::current_path(path_navigate);
    }

    void DisplayContentBrowser()
    {
        const Project& project = Projects::GetContext();
        const std::filesystem::path path_assets = Projects::GetAssetPath(project);
        const bool is_at_root = std::filesystem::current_path() == path_assets;

        DisplayToolbar(project, is_at_root);
        ImGui::Separator();

        // Reserve room for the separator + one row of sliders; the grid child scrolls on its own above them.
        const ImGuiStyle& style = ImGui::GetStyle();
        const float height_footer = style.ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

        // AlwaysUseWindowPadding keeps the grid inset like the old direct-in-window layout (ImGui 1.90.9+).
        if (ImGui::BeginChild("##Content Scroll", ImVec2(0.f, -height_footer), ImGuiChildFlags_AlwaysUseWindowPadding))
        {
            // Everything that used to hang off the whole panel now lives in the child, because
            // IsWindowHovered() / BeginPopupContextWindow() only see the window they are called in.
            // Order matters: the empty-space popup is opened *before* the entries so a right-click on an entry wins.
            DisplayCreateAssetPopup();

            // Ctrl + Mouse Wheel zooms the thumbnails, like Unity
            const ImGuiIO& io = ImGui::GetIO();
            if (ImGui::IsWindowHovered() && io.KeyCtrl && io.MouseWheel != 0.f)
                state.thumbnail_size = std::clamp(state.thumbnail_size + io.MouseWheel * k_ThumbnailZoomStep, k_ThumbnailMin, k_ThumbnailMax);

            DisplayEntryGrid(project);

            // Click on empty space to deselect
            const bool should_deselect = ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
            if (should_deselect)
                state.path_selection.clear();
        }
        ImGui::EndChild();

        ImGui::Separator();
        DisplayFooter();
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

    const char* GetIconPath(AssetType type)
    {
        switch (type)
        {
            case AssetType::AnimationClip:
                return "Assets/Textures/Icon_AnimationClip.png";
            case AssetType::AudioClip:
                return "Assets/Textures/Icon_AudioClip.png";
            case AssetType::Material:
                return "Assets/Textures/Icon_Material.png";
            case AssetType::Model:
                return "Assets/Textures/Icon_Model.png";
            case AssetType::ModelAnimated:
                return "Assets/Textures/Icon_ModelAnimated.png";
            case AssetType::Texture:
                return "Assets/Textures/Icon_Texture.png";
            default:
                return NULL;
        }
    }

    const Texture& GetIcon(bool is_directory, AssetType type)
    {
        if (is_directory)
            return state.texture_folder;

        const Texture& icon = state.icons[static_cast<u32>(type)];
        return icon.IsValid() ? icon : state.texture_file;
    }

    std::vector<BrowserEntry> GatherEntries(const std::filesystem::path& directory)
    {
        std::vector<BrowserEntry> entries;

        std::error_code error;
        for (const auto& entry : std::filesystem::directory_iterator(directory, error))
        {
            const bool is_directory = entry.is_directory(error);
            if (!is_directory && !entry.is_regular_file(error))
                continue;

            BrowserEntry& item = entries.emplace_back();
            item.entry = entry;
            item.is_directory = is_directory;
            item.name = entry.path().filename().string();
            item.label = (k_ShowFileExtensions || is_directory) ? item.name : entry.path().stem().string();
        }

        std::sort(entries.begin(), entries.end(), [](const BrowserEntry& a, const BrowserEntry& b)
                  {
                      if (a.is_directory != b.is_directory)
                          return a.is_directory;

                      return std::lexicographical_compare(a.name.begin(), a.name.end(), b.name.begin(), b.name.end(),
                                                          [](unsigned char l, unsigned char r)
                                                          {
                                                              return std::tolower(l) < std::tolower(r);
                                                          });
                  });

        return entries;
    }

    void DrawLabel(ImDrawList* draw_list, const ImVec2& position, float max_width, const std::string& label)
    {
        const char* const begin = label.c_str();
        const char* end = begin + label.size();
        const ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);

        const float width_full = ImGui::CalcTextSize(begin, end).x;
        if (width_full <= max_width)
        {
            draw_list->AddText(ImVec2(position.x + (max_width - width_full) * 0.5f, position.y), color, begin, end);
            return;
        }

        const float width_ellipsis = ImGui::CalcTextSize("...").x;
        while (end > begin)
        {
            // Step back one whole UTF-8 codepoint
            do
            {
                --end;
            } while (end > begin && (static_cast<u8>(*end) & 0xC0) == 0x80);

            if (ImGui::CalcTextSize(begin, end).x + width_ellipsis <= max_width)
                break;
        }

        const float width_text = ImGui::CalcTextSize(begin, end).x;
        draw_list->AddText(position, color, begin, end);
        draw_list->AddText(ImVec2(position.x + width_text, position.y), color, "...");
    }
}
