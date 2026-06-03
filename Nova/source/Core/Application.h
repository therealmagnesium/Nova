#pragma once
#include "Core/Base.h"
#include "Graphics/Window.h"

namespace Nova
{
    using EventCallback = void (*)(void);
    struct EventCallbacks
    {
        EventCallback on_create = NULL;
        EventCallback on_event = NULL;
        EventCallback on_update = NULL;
        EventCallback on_render = NULL;
        EventCallback on_render_ui = NULL;
        EventCallback on_shutdown = NULL;
    };

    struct AppConfig
    {
        string name;
        EventCallbacks callbacks;
        u16 screen_width = 0;
        u16 screen_height = 0;
    };

    struct App
    {
        Window window;
        AppConfig config;
        bool is_running = false;
        bool is_valid = false;
    };

    namespace Application
    {
        App Create(const AppConfig& config);
        void Run(App& app);
        void Shutdown(App& app);

        u16 GetScreenWidth();
        u16 GetScreenHeight();
        const Window& GetWindow();

        void Quit();
    }
}
