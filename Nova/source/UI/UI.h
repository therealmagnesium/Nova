#pragma once
#include "Graphics/RenderPass.h"
#include "Core/Project.h"

namespace Nova
{
    using FontHandle = void*;

    enum class UITheme : u8
    {
        ComfyDarkCyan = 0,
        DarkPastel,
        DeepDark,
        Moonlight
    };

    namespace UI
    {
        void Init();
        void Shutdown();
        void ProcessEvent(const void* event);
        void BeginFrame();
        void EndFrame();
        void Display(RenderPassHandle render_pass);

        FontHandle AddFont(const std::filesystem::path& path, float font_size, bool bind_on_load = false);
        void BindFont(FontHandle font);
        void SetTheme(UITheme theme);

        void ImportLayout(const Project& project);
        void ExportLayout(const Project& project);
    }
}
