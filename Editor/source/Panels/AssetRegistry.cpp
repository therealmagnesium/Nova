#include "Panels/AssetRegistry.h"
#include <Nova.h>
#include <imgui.h>

using namespace Nova;

struct AssetRegistryState
{
    ImGuiTextFilter filter;
    AssetHandle asset_to_delete = AssetHandle_Invalid;
    bool should_display = true;
};

struct RegistryRow
{
    AssetHandle handle = AssetHandle_Invalid;
    const AssetMetadata* metadata = NULL;
};

local AssetRegistryState state;
local constexpr ImGuiWindowFlags k_FlagsWindow = ImGuiWindowFlags_None;
local constexpr ImGuiTableFlags k_FlagsTable = ImGuiTableFlags_BordersV |
                                               ImGuiTableFlags_BordersOuterH |
                                               ImGuiTableFlags_Resizable |
                                               ImGuiTableFlags_RowBg |
                                               ImGuiTableFlags_NoBordersInBody;
local constexpr u8 k_ColumnCount = 4;

namespace AssetRegistryPanel
{
    void DisplayAssetRegistry();
    void DisplayNode(const AssetHandle handle, const AssetMetadata& metadata);

    void Display()
    {
        ImGui::Begin("Asset Registry", &state.should_display, k_FlagsWindow);
        if (AssetManager::IsHandleValid(state.asset_to_delete))
        {
            AssetManager::Remove(state.asset_to_delete);
            state.asset_to_delete = AssetHandle_Invalid;
        }

        DisplayAssetRegistry();
        ImGui::End();
    }

    void DisplayAssetRegistry()
    {
        state.filter.Draw("##Asset Filter", -1.f);

        std::vector<RegistryRow> rows;
        for (const auto& [handle, metadata] : AssetManager::GetRegistry())
        {
            const std::string_view type = AssetTypeToStringView(metadata.type);
            const std::string path = metadata.path.string();
            const bool matches = !state.filter.IsActive() ||
                                 state.filter.PassFilter(metadata.name.c_str()) ||
                                 state.filter.PassFilter(path.c_str()) ||
                                 state.filter.PassFilter(type.data(), type.data() + type.size());
            if (matches)
                rows.emplace_back(handle, &metadata);
        }

        const auto SortAssets = [](const RegistryRow& a, const RegistryRow& b)
        {
            if (a.metadata->type != b.metadata->type)
                return a.metadata->type < b.metadata->type;
            return strcmp(a.metadata->name.c_str(), b.metadata->name.c_str()) < 0;
        };

        std::sort(rows.begin(), rows.end(), SortAssets);

        ImGui::TextDisabled("%zu / %u assets", rows.size(), AssetManager::GetTotalAssetCount());

        if (ImGui::BeginTable("Table Asset Registry", k_ColumnCount, k_FlagsTable))
        {
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Path");
            ImGui::TableSetupColumn("Handle");
            ImGui::TableHeadersRow();

            for (const RegistryRow& row : rows)
                DisplayNode(row.handle, *row.metadata);

            ImGui::EndTable();
        }
    }

    void DisplayNode(const AssetHandle handle, const AssetMetadata& metadata)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        const std::string_view type_as_string = AssetTypeToStringView(metadata.type);
        const float line_height = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.f;

        ImGui::PushID(handle);
        ImGui::SameLine(line_height * 0.75f);
        if (ImGui::SmallButton("x"))
            state.asset_to_delete = handle;
        ImGui::PopID();

        ImGui::SameLine(0.f, line_height * 0.5f);
        ImGui::TextUnformatted(type_as_string.data(), type_as_string.data() + type_as_string.size());
        ImGui::TableNextColumn();

        ImGui::TextUnformatted(metadata.name.c_str());
        ImGui::TableNextColumn();

        ImGui::TextUnformatted(metadata.path.string().c_str());
        ImGui::TableNextColumn();

        ImGui::Text("0x%lX", handle);
    }
}
