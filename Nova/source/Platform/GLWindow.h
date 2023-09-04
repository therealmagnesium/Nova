#pragma once

#include "Window.h"
#include <GLFW/glfw3.h>

namespace Nova
{
    struct GLWindow : Window
    {
        uint32_t width;
        uint32_t height;
        const char* title;
        bool vsync;
        GLFWwindow* handle;

        GLWindow(const WindowArgs& args);
        virtual ~GLWindow();

        virtual void Init(const WindowArgs& args);
        virtual void Shutdown();

        inline void* GetHandle() override
        {
            return handle;
        };

        void HandleEvents() override;
        void Clear(float r, float g, float b) override;
        void Display() override;
        void SetVSync(bool enable) override;
    };
}
