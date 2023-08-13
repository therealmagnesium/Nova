#include <Nova/Window.hpp>

int main(int argc, char* argv[])
{
    Nova::Window window(1280, 720, "Nova Engine");
    window.clearColor[0] = 0.79f;
    window.clearColor[1] = 0.67f;
    window.clearColor[2] = 0.47f;

    while (!Nova::WindowShouldClose(&window))
    {
        window.HandleEvents();
    }

    window.Kill();
    return 0;
}
