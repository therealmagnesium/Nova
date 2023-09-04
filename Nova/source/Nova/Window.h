#pragma once
#include <stdint.h>

namespace Nova
{
    struct WindowArgs
    {
        uint32_t width;
        uint32_t height;
        const char* title;

        WindowArgs(const char* titleIn = "Nova Window", uint32_t widthIn = 1280, uint32_t heightIn = 720)
            : width(widthIn), height(heightIn), title(titleIn)
        {
        }
    };

    struct Window
    {
        bool closed;

        virtual ~Window()
        {
        }

        virtual void* GetHandle() = 0;

        virtual void HandleEvents() = 0;
        virtual void Clear(float r, float g, float b) = 0;
        virtual void Display() = 0;
        virtual void SetVSync(bool enable) = 0;
    };

    Window* CreateWindow(const WindowArgs& args = WindowArgs());
    bool WindowShouldClose(Window* window);
}
