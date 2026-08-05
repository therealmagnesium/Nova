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

        void Callback_OnKeyHeld(const void* event);
        void Callback_OnKeyReleased(const void* event);
        void Callback_OnMouseMove(const void* event);
        void Callback_OnMouseButtonHeld(const void* event);
        void Callback_OnMouseButtonReleased(const void* event);
        void Callback_OnMouseScroll(const void* event);
    }
}
