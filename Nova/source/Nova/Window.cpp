#include "Window.h"
#include "Util.h"

namespace Nova
{
    static void WindowResizeCallback(GLFWwindow* window, int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    Window::Window(int widthIn, int heightIn, const char* titleIn) : width(widthIn), height(heightIn), title(titleIn)
    {
        if (!Init())
            Kill();
    }

    Window::~Window()
    {
        Kill();
    }

    bool Window::Init()
    {
        if (!glfwInit())
        {
            ERROR_RETURN(false, "Failed to init glfw!\n");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        handle = glfwCreateWindow(width, height, title, NULL, NULL);
        if (!handle)
        {
            ERROR_RETURN(false, "Failed to create window handle!\n");
        }

        glfwSetFramebufferSizeCallback(handle, WindowResizeCallback);

        glfwMakeContextCurrent(handle);
        glfwSwapInterval(1);

        int gladVersion = gladLoadGL(glfwGetProcAddress);
        if (gladVersion == 0)
        {
            ERROR_RETURN(false, "Failed to load opengl!\n");
        }
        printf("Loaded opengl version %d.%d\n", GLAD_VERSION_MAJOR(gladVersion), GLAD_VERSION_MINOR(gladVersion));

        glViewport(0, 0, width, height);

        return true;
    }

    void Window::HandleEvents()
    {
        glfwPollEvents();
    }

    void Window::Clear()
    {
        glClearColor(clearColor[0], clearColor[1], clearColor[2], 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void Window::Display()
    {
        glfwSwapBuffers(handle);
    }

    void Window::Kill()
    {
        glfwTerminate();
    }

    bool WindowShouldClose(Window* window)
    {
        return glfwWindowShouldClose(window->handle);
    }
}
