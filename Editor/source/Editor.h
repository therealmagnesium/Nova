#pragma once
#include <Nova.h>

namespace Editor
{
    void OnCreate();
    void OnShutdown();
    void OnEvent();
    void OnUpdate();
    void OnRender();
    void OnRenderUI();

    void Callback_ExportScene();
    void Callback_ImportStartupScene();
}
