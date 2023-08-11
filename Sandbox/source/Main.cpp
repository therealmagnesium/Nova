#include <Nova/Window.hpp>

int main(int argc, char* argv[])
{
    Nova::Window window(1280, 720, "Nova Engine");

    while (!Nova::WindowShouldClose(&window))
    {
        window.HandleEvents();
    }

    window.Kill();
    return 0;
}
