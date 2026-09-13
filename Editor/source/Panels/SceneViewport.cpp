#include "Panels/SceneViewport.h"

#include <imgui.h>
#include <SDL3/SDL_gpu.h>

using namespace Nova;

struct SceneViewportState
{
    glm::vec2 position;
    glm::vec2 size;
    float target_aspect = 16.f / 9.f;
    bool is_hovered = false;
    bool is_focused = false;
    bool should_display = true;
};

static SceneViewportState state;
static constexpr u32 k_ViewportFlags = ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_NoDecoration |
                                       ImGuiWindowFlags_NoScrollbar |
                                       ImGuiWindowFlags_NoScrollWithMouse;

namespace SceneViewportPanel
{
    ImVec2 GetLargestViewportSize();
    ImVec2 GetCenteredViewportPosition(const ImVec2& aspect_size);

    void Display(const Texture& framebuffer_attachment)
    {
        if (!state.should_display)
            return;

        ImGuiWindowClass window_class;
        window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_AutoHideTabBar;
        ImGui::SetNextWindowClass(&window_class);

        ImGui::Begin("Scene Viewport", &state.should_display, k_ViewportFlags);
        state.is_hovered = ImGui::IsWindowHovered();
        state.is_focused = ImGui::IsWindowFocused();

        const ImVec2 aspect_size = GetLargestViewportSize();
        const ImVec2 window_position = GetCenteredViewportPosition(aspect_size);

        state.position = glm::vec2(ImGui::GetWindowPos().x + window_position.x, ImGui::GetWindowPos().y + window_position.y);
        state.size = *(glm::vec2*)&aspect_size;

        const TextureHandle attachment_handle = Textures::GetHandle(framebuffer_attachment);
        ImGui::SetCursorPos(window_position);
        ImGui::Image((ImTextureID)attachment_handle, aspect_size);
        ImGui::End();
    }

    bool IsHovered() { return state.is_hovered; }
    bool IsFocused() { return state.is_focused; }
    const glm::vec2& GetPosition() { return state.position; }
    const glm::vec2& GetSize() { return state.size; }

    ImVec2 GetLargestViewportSize()
    {
        ImVec2 window_size = ImGui::GetContentRegionAvail();
        ImVec2 aspect_size = ImVec2(window_size.x, window_size.x / state.target_aspect);

        if (aspect_size.y > window_size.y)
        {
            aspect_size.y = window_size.y;
            aspect_size.x = window_size.y * state.target_aspect;
        }

        return aspect_size;
    }

    ImVec2 GetCenteredViewportPosition(const ImVec2& aspect_size)
    {
        const ImVec2 window_size = ImGui::GetContentRegionAvail();

        ImVec2 viewport;
        viewport.x = (window_size.x / 2.f) - (aspect_size.x / 2.f) + ImGui::GetCursorPosX();
        viewport.y = (window_size.y / 2.f) - (aspect_size.y / 2.f) + ImGui::GetCursorPosY();

        return viewport;
    }
}
