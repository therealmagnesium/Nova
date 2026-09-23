#include "Panels/AssetRegistry.h"
#include <Nova.h>
#include <imgui.h>

using namespace Nova;

struct AssetRegistryState
{
    AssetHandle selection_context = AssetHandle_Invalid;
    bool should_display = true;
};

local AssetRegistryState state;
local constexpr ImGuiWindowFlags k_WindowFlags = ImGuiWindowFlags_None;

namespace AssetRegistryPanel
{
    void DisplayAssetRegistry();
    void DisplayNode(const AssetHandle handle, const AssetMetadata& metadata);
    std::string_view AssetTypeToStringView(AssetType type);

    void Display()
    {
        ImGui::Begin("Asset Registry", &state.should_display, k_WindowFlags);
        DisplayAssetRegistry();
        ImGui::End();
    }

    void DisplayAssetRegistry()
    {
        const ImGuiTableFlags table_flags = ImGuiTableFlags_BordersV |
                                            ImGuiTableFlags_BordersOuterH |
                                            ImGuiTableFlags_Resizable |
                                            ImGuiTableFlags_RowBg |
                                            ImGuiTableFlags_NoBordersInBody;

        if (ImGui::BeginTable("Table Asset Registry", 3, table_flags))
        {
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Path");
            ImGui::TableSetupColumn("Handle");
            ImGui::TableHeadersRow();

            const AssetRegistry& registry = AssetManager::GetRegistry();
            for (const auto& [handle, metadata] : registry)
                DisplayNode(handle, metadata);

            ImGui::EndTable();
        }
    }

    void DisplayNode(const AssetHandle handle, const AssetMetadata& metadata)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        const std::string_view type_as_string = AssetTypeToStringView(metadata.type);
        ImGui::TextUnformatted(type_as_string.data(), type_as_string.data() + type_as_string.size());
        ImGui::TableNextColumn();

        ImGui::TextUnformatted(metadata.path.string().c_str());
        ImGui::TableNextColumn();

        ImGui::Text("0x%lX", handle);
    }

    std::string_view AssetTypeToStringView(AssetType type)
    {
        switch (type)
        {
            case AssetType::AnimationClip:
                return "Animation CLip";
            case AssetType::AudioClip:
                return "Audio Clip";
            case AssetType::Material:
                return "Material";
            case AssetType::Model:
                return "Standard Model";
            case AssetType::ModelAnimated:
                return "Animated Model";
            case AssetType::Texture:
                return "Texture";
            default:
                return "Invalid";
        }
    }
}
