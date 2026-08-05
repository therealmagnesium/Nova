#pragma once
#include "Core/AssetManager.h"
#include "Core/Base.h"
#include "Graphics/Window.h"
#include "Graphics/Texture.h"

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
        MSAASamples msaa = MSAASamples::One;
    };

    struct App
    {
        AppConfig config;
        AssetCollection assets;
        Window window;
        float delta_time = 0.f;
        bool is_running = false;
        bool is_valid = false;
    };

    inline const App Stub_App;
    inline const EventCallbacks Stub_EventCallbacks;

    namespace Application
    {
        App Create(const AppConfig& config);
        void Run(App& app);
        void Shutdown(App& app);

        float GetDeltaTime();
        u16 GetScreenWidth();
        u16 GetScreenHeight();
        MSAASamples GetMSAASamples();
        const Window& GetWindow();

        void Quit();
    }
}
