#pragma once
#include <Nova.h>

namespace SceneViewportPanel
{
    void Display(const Nova::Texture& framebuffer_attachment);

    bool IsHovered();
    bool IsFocused();
    const glm::vec2& GetPosition();
    const glm::vec2& GetSize();
}
