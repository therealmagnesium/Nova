#include "Editor.h"
#include <Nova.h>

using namespace Nova;

int main(int argc, char** argv)
{
    const AppConfig config = {
        .name = "Nova Editor",
        .callbacks = {
            .on_create = Editor::OnCreate,
            .on_event = Editor::OnEvent,
            .on_update = Editor::OnUpdate,
            .on_render = Editor::OnRender,
            .on_render_ui = Editor::OnRenderUI,
            .on_shutdown = Editor::OnShutdown,
        },
        .screen_width = 960,
        .screen_height = 540,
        .msaa = MSAASamples::Eight,
    };

    App* app = Application::Create(config);
    Application::Run(app);
    Application::Shutdown(app);
}
