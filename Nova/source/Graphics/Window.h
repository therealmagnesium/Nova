#pragma once
#include "Core/Base.h"

namespace Nova
{
    struct Window
    {
        string title = "Untitled";
        void* handle = NULL;
        void* gpu_device = NULL;
        bool is_valid = false;
        u16 width = 0;
        u16 height = 0;
    };

    inline const Window Stub_Window;

    namespace Windows
    {
        Window Create(u16 width, u16 height, const string& title);
        void Destroy(Window& window);
        void HandleEvents(Window& window);
    }
}
