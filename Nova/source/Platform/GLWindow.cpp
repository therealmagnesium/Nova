#include "GLWindow.h"
#include <glad/gl.h>
#include <stdio.h>

namespace Nova
{
    void WindowResizeCallback(GLFWwindow* window, int widthIn, int heightIn)
    {
        glViewport(0, 0, widthIn, heightIn);
    }

    Window* CreateWindow(const WindowArgs& args)
    {
        return new GLWindow(args);
    }

    GLWindow::GLWindow(const WindowArgs& args)
    {
        Init(args);
    }

    GLWindow::~GLWindow()
    {
        Shutdown();
    }

    void GLWindow::Init(const WindowArgs& args)
    {
        width = args.width;
        height = args.height;
        title = args.title;
        vsync = false;

        glfwInit();

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        handle = glfwCreateWindow(width, height, title, NULL, NULL);
        if (!handle)
            printf("Failed to create window handle!\n");

        glfwSetFramebufferSizeCallback(handle, WindowResizeCallback);

        glfwMakeContextCurrent(handle);
        glfwSwapInterval(1);

        int gladVersion = gladLoadGL(glfwGetProcAddress);
        if (gladVersion == 0)
            printf("Failed to load opengl!\n");
        printf("Loaded opengl version %d.%d\n", GLAD_VERSION_MAJOR(gladVersion), GLAD_VERSION_MINOR(gladVersion));

        glViewport(0, 0, width, height);
    }

    void GLWindow::Shutdown()
    {
        glfwDestroyWindow(handle);
        glfwTerminate();
    }

    void GLWindow::HandleEvents()
    {
        glfwPollEvents();
        closed = glfwWindowShouldClose(handle);
    }

    void GLWindow::Clear(float r, float g, float b)
    {
        glClearColor(r, g, b, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GLWindow::Display()
    {
        glfwSwapBuffers(handle);
    }

    void GLWindow::SetVSync(bool enable)
    {
        if (enable)
            glfwSwapInterval(1);
        else
            glfwSwapInterval(0);

        vsync = enable;
    }

    bool WindowShouldClose(Window* window)
    {
        return window->closed;
    }
}
