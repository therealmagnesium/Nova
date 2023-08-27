#pragma once

#include <GLFW/glfw3.h>
#include <glad/gl.h>

namespace Nova
{
    struct Window
    {
        int width, height;
        float clearColor[3];
        const char* title;
        GLFWwindow* handle;

        Window(int widthIn, int heightIn, const char* titleIn);
        ~Window();

        bool Init();
        void Clear();
        void Display();
        void HandleEvents();
        void Kill();

        static void ResizeCallback(GLFWwindow* window, int width, int height);
    };

    bool WindowShouldClose(Window* window);
}
