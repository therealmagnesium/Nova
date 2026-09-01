#pragma once
#include "Core/Base.h"
#include "Core/KeyCodes.h"
#include <glm/vec2.hpp>

namespace Nova
{
    enum class InputAxis : u8
    {
        Horizontal = 0,
        Vertical,
    };

    enum class GamepadAxis : s8
    {
        Invalid = -1,
        LeftX,
        LeftY,
        RightX,
        RightY,
        LeftTrigger,
        RightTrigger,
    };

    enum class GamepadButton : s8
    {
        Invalid = -1,
        South, /**< Bottom face button (e.g. Xbox A button) */
        East,  /**< Right face button (e.g. Xbox B button) */
        West,  /**< Left face button (e.g. Xbox X button) */
        North, /**< Top face button (e.g. Xbox Y button) */
        Back,
        Guide,
        Start,
        LeftStick,
        RightStick,
        LeftShoulder,
        RightShoulder,
        DPAD_Up,
        DPAD_Down,
        DPAD_Left,
        DPAD_Right,
        Misc1,        /**< Additional button (e.g. Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon Luna microphone button, Google Stadia capture button) */
        RightPaddle1, /**< Upper or primary paddle, under your right hand (e.g. Xbox Elite paddle P1, DualSense Edge RB button, Right Joy-Con SR button) */
        LeftPaddle1,  /**< Upper or primary paddle, under your left hand (e.g. Xbox Elite paddle P3, DualSense Edge LB button, Left Joy-Con SL button) */
        RightPaddle2, /**< Lower or secondary paddle, under your right hand (e.g. Xbox Elite paddle P2, DualSense Edge right Fn button, Right Joy-Con SL button) */
        LeftPaddle2,  /**< Lower or secondary paddle, under your left hand (e.g. Xbox Elite paddle P4, DualSense Edge left Fn button, Left Joy-Con SR button) */
        Touchpad,     /**< PS4/PS5 touchpad button */
        Misc2,        /**< Additional button */
        Misc3,        /**< Additional button (e.g. Nintendo GameCube left trigger click) */
        Misc4,        /**< Additional button (e.g. Nintendo GameCube right trigger click) */
        Misc5,        /**< Additional button */
        Misc6,        /**< Additional button */
    };

    using GamepadID = u32;

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

        bool IsGamepadLeftStickMoving();
        bool IsGamepadRightStickMoving();
        bool IsGamepadButtonDown(GamepadButton button);
        bool IsGamepadButtonPressed(GamepadButton button);
        bool IsGamepadButtonReleased(GamepadButton button);

        float GetAxis(InputAxis axis);
        float GetAxisAlt(InputAxis axis);
        float GetAxisGamepad(InputAxis axis);
        float GetAxisGamepad(GamepadAxis axis);

        void Reset();
        void Capture(bool shouldCapture);

        void Callback_OnKeyHeld(KeyboardKey key);
        void Callback_OnKeyReleased(KeyboardKey key);
        void Callback_OnMouseMove(const glm::vec2& absolute, const glm::vec2& relative);
        void Callback_OnMouseButtonHeld(MouseButton button);
        void Callback_OnMouseButtonReleased(MouseButton button);
        void Callback_OnMouseScroll(const glm::vec2& scroll);
        void Callback_OnGamepadButtonHeld(GamepadButton button);
        void Callback_OnGamepadButtonReleased(GamepadButton button);
        void Callback_OnGamepadAxisMotion(GamepadAxis axis, float value);
        void Callback_OnGamepadConnected(GamepadID id);
        void Callback_OnGamepadDisconnected(GamepadID id);
        void Callback_Quit();
    }
}
