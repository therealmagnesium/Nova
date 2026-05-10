#include "Game.h"
#include <Nova.h>

using namespace Nova::Core;
using namespace Nova::Graphics;

int main(int argc, char** argv)
{
    AppConfig config;
    config.name = "Sandbox Project";
    config.screen_width = 960;
    config.screen_height = 540;
    config.callbacks.on_create = Game::OnCreate;
    config.callbacks.on_event = Game::OnEvent;
    config.callbacks.on_update = Game::OnUpdate;
    config.callbacks.on_render = Game::OnRender;
    config.callbacks.on_render_ui = Game::OnRenderUI;
    config.callbacks.on_shutdown = Game::OnShutdown;

    App app = Application::Create(config);
    Application::Run(app);
    Application::Shutdown(app);
}
