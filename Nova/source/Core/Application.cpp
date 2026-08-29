#include "Core/Application.h"
#include "Core/Log.h"
#include "Core/Random.h"
#include "Graphics/Renderer.h"
#include "UI/UI.h"

#include <SDL3/SDL.h>

namespace Nova::Application
{
    static App* context = NULL;

    App* Create(const AppConfig& config)
    {
        App* app = new App();
        app->config = config;

        if (config.callbacks.on_create == NULL || config.callbacks.on_event == NULL ||
            config.callbacks.on_update == NULL || config.callbacks.on_render == NULL ||
            config.callbacks.on_render_ui == NULL || config.callbacks.on_shutdown == NULL)
        {
            FATAL("%s", "Application::Create - The event callbacks are not setup properly!");
            return NULL;
        }

        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        {
            FATAL("%s", "Application::Create - SDL failed to initialize properly!");
            return NULL;
        }

        context = app;
        app->window = Windows::Create(config.screen_width, config.screen_height, config.name);
        Random::Init();
        AssetManager::Init(&app->assets);
        Renderer::Init();
        UI::Init();

        app->is_valid = true;
        INFO("Application \"%s\" initialized successfully!", app->config.name.c_str());
        return app;
    }

    void Shutdown(App* app)
    {
        INFO("Shutting down application \"%s\"...", app->config.name.c_str());
        app->is_valid = false;

        UI::Shutdown();
        AssetManager::Clean();
        Renderer::Shutdown();
        Windows::Destroy(app->window);
        SDL_Quit();

        delete app;
    }

    void Run(App* app)
    {
        if (app == NULL || !app->is_valid)
            return;

        context = app;
        app->is_running = true;
        app->config.callbacks.on_create();

        u64 ticks_previous = SDL_GetTicksNS();
        while (app->is_running)
        {
            const u64 ticks_current = SDL_GetTicksNS();
            app->delta_time = static_cast<float>(ticks_current - ticks_previous) / 1e9f;
            ticks_previous = ticks_current;

            Windows::HandleEvents(app->window);
            app->config.callbacks.on_event();
            app->config.callbacks.on_update();

            if (Renderer::BeginFrame())
            {
                UI::BeginFrame();
                app->config.callbacks.on_render_ui();
                UI::EndFrame();

                app->config.callbacks.on_render();

                Renderer::EndFrame();
            }
        }
        app->config.callbacks.on_shutdown();
    }

    float GetDeltaTime() { return context->delta_time; }
    u16 GetScreenWidth() { return context->config.screen_width; }
    u16 GetScreenHeight() { return context->config.screen_height; }
    MSAASamples GetMSAASamples() { return context->config.msaa; }
    const Window& GetWindow() { return context->window; }
    void Quit() { context->is_running = false; }
}
