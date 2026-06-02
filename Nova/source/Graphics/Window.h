#pragma once
#include "Core/Base.h"

namespace Nova
{
    enum WindowState : u8
    {
        NOVA_WINDOWSTATE_INVALID = 0,
        NOVA_WINDOWSTATE_VALID = 1 << 0,
        NOVA_WINDOWSTATE_RESIZED = 1 << 1,
        NOVA_WINDOWSTATE_MAXIMIZED = 1 << 2,
        NOVA_WINDOWSTATE_MINIMIZED = 1 << 3,
    };

    struct Window
    {
        string title = "Untitled";
        void* handle = NULL;
        void* gpu_device = NULL;
        u16 width = 0;
        u16 height = 0;
        u8 state = NOVA_WINDOWSTATE_INVALID;
    };

    inline const Window Stub_Window;

    namespace Windows
    {
        Window Create(u16 width, u16 height, const string& title);
        void Destroy(Window& window);
        void HandleEvents(Window& window);

        bool IsResizing(const Window& window);
        bool IsMinimized(const Window& window);
        bool IsMaximized(const Window& window);
    }
}
