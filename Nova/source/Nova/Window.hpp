#pragma once

#include <glad/gl.h> 
#include <GLFW/glfw3.h>

namespace Nova
{
    struct Window
    {
        int width, height;
        const char* title;
        GLFWwindow* handle;
        float clearColor[3];

        Window(int widthIn, int heightIn, const char* titleIn);
        ~Window();

        bool Init();
        void HandleEvents(); 
        void Kill();
    };

    bool WindowShouldClose(Window* window); 
} 
