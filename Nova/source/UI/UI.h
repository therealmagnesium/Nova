#pragma once
#include "Graphics/RenderPass.h"

namespace Nova::UI
{
    void Init();
    void Shutdown();
    void ProcessEvent(const void* event);
    void BeginFrame();
    void EndFrame();
    void Display(RenderPassHandle render_pass);
}
