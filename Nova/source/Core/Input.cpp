#include "Core/Input.h"
#include "Core/KeyCodes.h"
#include "Core/Log.h"

#include <SDL3/SDL_events.h>
#include <unordered_map>
#include <string.h>
#include <math.h>

namespace Nova::Input
{
    struct InputMouseState
    {
        glm::vec2 position;
        glm::vec2 relative;
        glm::vec2 scroll;
        bool buttons_held[MOUSE_BUTTON_COUNT] = {};
        bool buttons_clicked[MOUSE_BUTTON_COUNT] = {};
        bool buttons_released[MOUSE_BUTTON_COUNT] = {};
    };

    struct InputKeyboardState
    {
        bool keys_held[KEY_COUNT] = {};
        bool keys_pressed[KEY_COUNT] = {};
        bool keys_released[KEY_COUNT] = {};
    };

    struct InputGamepadState
    {
        bool buttons_held[SDL_GAMEPAD_BUTTON_COUNT] = {};
        bool buttons_pressed[SDL_GAMEPAD_BUTTON_COUNT] = {};
        bool buttons_released[SDL_GAMEPAD_BUTTON_COUNT] = {};
        float axes[SDL_GAMEPAD_AXIS_COUNT] = {};
    };

    struct InputState
    {
        bool should_capture = true;
        InputMouseState mouse;
        InputKeyboardState keyboard;
        InputGamepadState gamepad;
        std::unordered_map<GamepadID, SDL_Gamepad*> connected_gamepads;
    };

    static InputState state;
    static constexpr float k_Deadzone = 0.15f;

    float CalculateScaledAxis(float value);

    bool IsCapturing() { return state.should_capture; }

    bool IsMouseDown(MouseButton button) { return state.should_capture ? state.mouse.buttons_held[button] : false; }
    bool IsMouseClicked(MouseButton button) { return state.should_capture ? state.mouse.buttons_clicked[button] : false; }
    bool IsMouseReleased(MouseButton button) { return state.should_capture ? state.mouse.buttons_released[button] : false; }
    glm::vec2 GetMousePosition() { return state.should_capture ? state.mouse.position : glm::vec2(0.f); }
    glm::vec2 GetMouseRelative() { return state.should_capture ? state.mouse.relative : glm::vec2(0.f); }
    glm::vec2 GetMouseScroll() { return state.should_capture ? state.mouse.scroll : glm::vec2(0.f); }

    bool IsKeyDown(KeyboardKey scancode) { return state.should_capture ? state.keyboard.keys_held[scancode] : false; }
    bool IsKeyPressed(KeyboardKey scancode) { return state.should_capture ? state.keyboard.keys_pressed[scancode] : false; }
    bool IsKeyReleased(KeyboardKey scancode) { return state.should_capture ? state.keyboard.keys_released[scancode] : false; }

    bool IsGamepadLeftStickMoving()
    {
        const u8 index_x = static_cast<u8>(GamepadAxis::LeftX);
        const u8 index_y = static_cast<u8>(GamepadAxis::LeftY);
        const float left_x = CalculateScaledAxis(state.gamepad.axes[index_x]);
        const float left_y = CalculateScaledAxis(state.gamepad.axes[index_y]);
        const bool is_moving = fabsf(left_x) > 0.f || fabsf(left_y) > 0.f;

        return state.should_capture ? is_moving : false;
    }

    bool IsGamepadRightStickMoving()
    {
        const u8 index_x = static_cast<u8>(GamepadAxis::RightX);
        const u8 index_y = static_cast<u8>(GamepadAxis::RightY);
        const float right_x = CalculateScaledAxis(state.gamepad.axes[index_x]);
        const float right_y = CalculateScaledAxis(state.gamepad.axes[index_y]);
        const bool is_moving = fabsf(right_x) > 0.f || fabsf(right_y) > 0.f;

        return state.should_capture ? is_moving : false;
    }

    bool IsGamepadButtonDown(GamepadButton button) { return state.should_capture ? state.gamepad.buttons_held[static_cast<u8>(button)] : false; }
    bool IsGamepadButtonPressed(GamepadButton button) { return state.should_capture ? state.gamepad.buttons_pressed[static_cast<u8>(button)] : false; }
    bool IsGamepadButtonReleased(GamepadButton button) { return state.should_capture ? state.gamepad.buttons_released[static_cast<u8>(button)] : false; }

    float GetAxis(InputAxis axis)
    {
        float value = 0.f;
        switch (axis)
        {
            case InputAxis::Horizontal:
                value = (IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT)) + (IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
                break;

            case InputAxis::Vertical:
                value = (IsKeyDown(KEY_UP) - IsKeyDown(KEY_DOWN)) + (IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
                break;
        }

        value += GetAxisGamepad(axis);
        return value;
    }

    float GetAxisGamepad(InputAxis axis)
    {
        float value = 0.f;
        switch (axis)
        {
            case InputAxis::Horizontal:
                value = state.gamepad.axes[static_cast<u8>(GamepadAxis::LeftX)];
                break;

            case InputAxis::Vertical:
                // NOTE: SDL Y-axes are inverted.
                value = -state.gamepad.axes[static_cast<u8>(GamepadAxis::LeftY)];
                break;
        }

        return CalculateScaledAxis(value);
    }

    float GetAxisGamepad(GamepadAxis axis)
    {
        const u8 index = static_cast<u8>(axis);
        return CalculateScaledAxis(state.gamepad.axes[index]);
    }

    void Reset()
    {
        state.mouse.relative = glm::vec2(0.f);
        state.mouse.scroll = glm::vec2(0.f);

        memset(state.keyboard.keys_pressed, 0, sizeof(bool) * KEY_COUNT);
        memset(state.keyboard.keys_released, 0, sizeof(bool) * KEY_COUNT);
        memset(state.mouse.buttons_clicked, 0, sizeof(bool) * MOUSE_BUTTON_COUNT);
        memset(state.mouse.buttons_released, 0, sizeof(bool) * MOUSE_BUTTON_COUNT);
        memset(state.gamepad.buttons_pressed, 0, sizeof(bool) * SDL_GAMEPAD_BUTTON_COUNT);
        memset(state.gamepad.buttons_released, 0, sizeof(bool) * SDL_GAMEPAD_BUTTON_COUNT);
    }

    void Capture(bool should_capture) { state.should_capture = should_capture; }

    void Callback_OnKeyHeld(KeyboardKey key)
    {
        state.keyboard.keys_pressed[key] = !state.keyboard.keys_held[key];
        state.keyboard.keys_held[key] = true;
        state.keyboard.keys_released[key] = false;
    }

    void Callback_OnKeyReleased(KeyboardKey key)
    {
        state.keyboard.keys_pressed[key] = false;
        state.keyboard.keys_held[key] = false;
        state.keyboard.keys_released[key] = true;
    }

    void Callback_OnMouseMove(const glm::vec2& absolute, const glm::vec2& relative)
    {
        state.mouse.position.x = absolute.x;
        state.mouse.position.y = absolute.y;
        state.mouse.relative.x = relative.x;
        state.mouse.relative.y = relative.y;
    }

    void Callback_OnMouseButtonHeld(MouseButton button)
    {
        state.mouse.buttons_clicked[button] = !state.mouse.buttons_held[button];
        state.mouse.buttons_held[button] = true;
        state.mouse.buttons_released[button] = false;
    }

    void Callback_OnMouseButtonReleased(MouseButton button)
    {
        state.mouse.buttons_clicked[button] = false;
        state.mouse.buttons_held[button] = false;
        state.mouse.buttons_released[button] = true;
    }

    void Callback_OnMouseScroll(const glm::vec2& scroll)
    {
        state.mouse.scroll.x = scroll.x;
        state.mouse.scroll.y = scroll.y;
    }

    void Callback_OnGamepadButtonHeld(GamepadButton button)
    {
        const u8 button_id = static_cast<u8>(button);
        state.gamepad.buttons_pressed[button_id] = !state.gamepad.buttons_held[button_id];
        state.gamepad.buttons_held[button_id] = true;
        state.gamepad.buttons_released[button_id] = false;
    }

    void Callback_OnGamepadButtonReleased(GamepadButton button)
    {
        const u8 button_id = static_cast<u8>(button);
        state.gamepad.buttons_pressed[button_id] = false;
        state.gamepad.buttons_held[button_id] = false;
        state.gamepad.buttons_released[button_id] = true;
    }

    void Callback_OnGamepadAxisMotion(GamepadAxis axis, float value)
    {
        const u8 axis_id = static_cast<u8>(axis);
        state.gamepad.axes[axis_id] = value;
    }

    void Callback_OnGamepadConnected(GamepadID id)
    {
        SDL_Gamepad* gamepad = SDL_OpenGamepad(id);
        if (gamepad == NULL)
        {
            ERROR("Input::Callback_OnGamepadConnected - Failed to establish connection to gamepad #%d", id);
            return;
        }

        state.connected_gamepads[id] = gamepad;
        INFO("Gamepad connected successfully with and ID of %d", id);
    }

    void Callback_OnGamepadDisconnected(GamepadID id)
    {
        if (state.connected_gamepads.contains(id))
        {
            INFO("Disconnecting gamepad with and ID of %d...", id);
            const auto it = state.connected_gamepads.find(id);
            SDL_CloseGamepad(it->second);
            state.connected_gamepads.erase(it);
        }
    }

    void Callback_Quit()
    {
        INFO("%s", "Cleaning up all connected gamepads...");

        // Close the SDL handles safely without modifying the container during loop
        for (auto [id, sdl_gamepad] : state.connected_gamepads)
        {
            if (sdl_gamepad != NULL)
            {
                SDL_CloseGamepad(sdl_gamepad);
            }
        }

        // Wipe the map clear now that iteration is finished
        state.connected_gamepads.clear();
    }

    float CalculateScaledAxis(float value)
    {
        // Apply the deadzone filter
        if (fabsf(value) < k_Deadzone)
            return 0.f;

        // Rescale the remaining input range
        const float sign = (value > 0.f) ? 1.f : -1.f;
        return sign * ((fabsf(value) - k_Deadzone) / (1.f - k_Deadzone));
    }
}
