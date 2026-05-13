#include "Core/Application.h"
#include "Core/Log.h"
#include "Graphics/Renderer.h"

#include <SDL3/SDL.h>

using namespace Nova::Graphics;

namespace Nova::Core::Application
{
    static App* context = NULL;

    App Create(const AppConfig& config)
    {
        App app;

        if (config.callbacks.on_create == NULL || config.callbacks.on_event == NULL ||
            config.callbacks.on_update == NULL || config.callbacks.on_render == NULL ||
            config.callbacks.on_render_ui == NULL || config.callbacks.on_shutdown == NULL)
        {
            FATAL("%s", "Application::Create - The event callbacks are not setup properly!");
            return app;
        }

        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        {
            FATAL("%s", "Application::Create - SDL failed to initialize properly!");
            return app;
        }
        context = &app;

        app.window = Windows::Create(config.screen_width, config.screen_height, config.name);
        Renderer::Init();

        app.config = config;
        app.is_valid = true;
        return app;
    }

    void Shutdown(App& app)
    {
        app.is_valid = false;

        Renderer::Shutdown();
        Windows::Destroy(app.window);
        SDL_Quit();
    }

    void Run(App& app)
    {
        if (!app.is_valid)
            return;

        app.is_running = true;

        app.config.callbacks.on_create();
        while (app.is_running)
        {
            Windows::HandleEvents(app.window);
            app.config.callbacks.on_event();
            app.config.callbacks.on_update();

            if (Renderer::BeginFrame())
            {
                app.config.callbacks.on_render();
                app.config.callbacks.on_render_ui();
                Renderer::EndFrame();
            }
        }
        app.config.callbacks.on_shutdown();
    }

    u16 GetScreenWidth() { return context->config.screen_width; }
    u16 GetScreenHeight() { return context->config.screen_height; }
    Window& GetWindow() { return context->window; }
    void Quit() { context->is_running = false; }
}
