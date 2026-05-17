#pragma once
#include "Core/KeyCodes.h"
#include <glm/vec2.hpp>

namespace Nova
{
    enum class InputAxis
    {
        Horizontal = 0,
        Vertical,
    };

    namespace Input
    {
        bool IsCapturing();

        bool IsMouseDown(MouseButton button);
        bool IsMouseClicked(MouseButton button);
        bool IsMouseReleased(MouseButton button);
        glm::vec2 GetMousePosition();
        glm::vec2 GetMouseRelative();
        glm::vec2 GetMouseScroll();

        bool IsKeyDown(KeyboardKey scancode);
        bool IsKeyPressed(KeyboardKey scancode);
        bool IsKeyReleased(KeyboardKey scancode);
        float GetAxis(InputAxis axis);
        float GetAxisAlt(InputAxis axis);

        void Reset();
        void Capture(bool shouldCapture);

        void Callback_OnKeyHeld(void* event);
        void Callback_OnKeyReleased(void* event);
        void Callback_OnMouseMove(void* event);
        void Callback_OnMouseButtonHeld(void* event);
        void Callback_OnMouseButtonReleased(void* event);
    }
}
