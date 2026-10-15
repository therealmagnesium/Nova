#include "Panels/Console.h"
#include <Nova.h>
#include <imgui.h>

using namespace Nova;

struct ConsoleState
{
    bool show_level[4] = { true, true, true, true }; // Info, Warn, Error, Fatal
    bool auto_scroll = true;
    bool should_display = true;
};

local ConsoleState state;
local const ImVec4 k_LevelColors[4] = {
    ImVec4(0.55f, 0.85f, 0.55f, 1.f), // Info
    ImVec4(0.95f, 0.85f, 0.40f, 1.f), // Warn
    ImVec4(1.00f, 0.45f, 0.45f, 1.f), // Error
    ImVec4(1.00f, 0.20f, 0.20f, 1.f), // Fatal
};

namespace ConsolePanel
{
    void Display()
    {
        if (!state.should_display)
            return;

        ImGui::Begin("Console", &state.should_display);

        if (ImGui::Button("Clear"))
            Logs::Clear();
        ImGui::SameLine();
        ImGui::Checkbox("Info", &state.show_level[0]);
        ImGui::SameLine();
        ImGui::Checkbox("Warn", &state.show_level[1]);
        ImGui::SameLine();
        ImGui::Checkbox("Error", &state.show_level[2]);
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &state.auto_scroll);
        ImGui::Separator();

        if (ImGui::BeginChild("##Console Scroll", ImVec2(0.f, 0.f), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
        {
            for (const LogEntry& entry : Logs::GetEntries())
            {
                const u8 level = static_cast<u8>(entry.level);
                const bool visible = level == 3 ? state.show_level[2] : state.show_level[level]; // Fatal follows Error
                if (!visible)
                    continue;

                ImGui::PushStyleColor(ImGuiCol_Text, k_LevelColors[level]);
                ImGui::TextUnformatted(entry.text.c_str());
                ImGui::PopStyleColor();
            }

            if (state.auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.f);
        }
        ImGui::EndChild();
        ImGui::End();
    }
}
