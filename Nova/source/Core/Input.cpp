#include "Core/Input.h"
#include "Core/KeyCodes.h"

#include <SDL3/SDL_events.h>
#include <string.h>

namespace Nova::Input
{
    struct InputMouseState
    {
        glm::vec2 position;
        glm::vec2 relative;
        glm::vec2 scroll;
        bool buttonsHeld[MOUSE_BUTTON_COUNT];
        bool buttonsClicked[MOUSE_BUTTON_COUNT];
        bool buttonsReleased[MOUSE_BUTTON_COUNT];
    };

    struct InputKeyboardState
    {
        bool keysHeld[KEY_COUNT];
        bool keysPressed[KEY_COUNT];
        bool keysReleased[KEY_COUNT];
    };

    struct InputState
    {
        bool shouldCapture = true;
        InputMouseState mouse;
        InputKeyboardState keyboard;
    };

    static InputState state;

    bool IsCapturing() { return state.shouldCapture; }

    bool IsMouseDown(MouseButton button) { return state.shouldCapture ? state.mouse.buttonsHeld[button] : false; }
    bool IsMouseClicked(MouseButton button) { return state.shouldCapture ? state.mouse.buttonsClicked[button] : false; }
    bool IsMouseReleased(MouseButton button) { return state.shouldCapture ? state.mouse.buttonsReleased[button] : false; }
    glm::vec2 GetMousePosition() { return state.shouldCapture ? state.mouse.position : glm::vec2(0.f); }
    glm::vec2 GetMouseRelative() { return state.shouldCapture ? state.mouse.relative : glm::vec2(0.f); }
    glm::vec2 GetMouseScroll() { return state.shouldCapture ? state.mouse.scroll : glm::vec2(0.f); }

    bool IsKeyDown(KeyboardKey scancode) { return state.shouldCapture ? state.keyboard.keysHeld[scancode] : false; }
    bool IsKeyPressed(KeyboardKey scancode) { return state.shouldCapture ? state.keyboard.keysPressed[scancode] : false; }
    bool IsKeyReleased(KeyboardKey scancode) { return state.shouldCapture ? state.keyboard.keysReleased[scancode] : false; }

    float GetAxis(InputAxis axis)
    {
        float value = 0.f;
        switch (axis)
        {
            case InputAxis::Horizontal:
                value = IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT);
                break;

            case InputAxis::Vertical:
                value = IsKeyDown(KEY_UP) - IsKeyDown(KEY_DOWN);
                break;
        }

        return value;
    }

    float GetAxisAlt(InputAxis axis)
    {
        float value = 0.f;
        switch (axis)
        {
            case InputAxis::Horizontal:
                value = IsKeyDown(KEY_D) - IsKeyDown(KEY_A);
                break;

            case InputAxis::Vertical:
                value = IsKeyDown(KEY_S) - IsKeyDown(KEY_W);
                break;
        }

        return value;
    }

    void Reset()
    {
        state.mouse.relative = glm::vec2(0.f);
        state.mouse.scroll = glm::vec2(0.f);

        memset(state.keyboard.keysPressed, 0, sizeof(bool) * KEY_COUNT);
        memset(state.keyboard.keysReleased, 0, sizeof(bool) * KEY_COUNT);
        memset(state.mouse.buttonsClicked, 0, sizeof(bool) * MOUSE_BUTTON_COUNT);
        memset(state.mouse.buttonsReleased, 0, sizeof(bool) * MOUSE_BUTTON_COUNT);

        /*
        for (u32 i = 0; i < KEY_COUNT; i++)
        {
            state.keyboard.keysPressed[i] = false;
            state.keyboard.keysReleased[i] = false;
        }

        for (u32 i = 0; i < MOUSE_BUTTON_COUNT; i++)
        {
            state.mouse.buttonsClicked[i] = false;
            state.mouse.buttonsReleased[i] = false;
        }*/
    }

    void Capture(bool shouldCapture) { state.shouldCapture = shouldCapture; }

    void Callback_OnKeyHeld(void* event)
    {
        SDL_Event* sdl_event = (SDL_Event*)event;
        state.keyboard.keysPressed[sdl_event->key.scancode] = !state.keyboard.keysHeld[sdl_event->key.scancode];
        state.keyboard.keysHeld[sdl_event->key.scancode] = true;
        state.keyboard.keysReleased[sdl_event->key.scancode] = false;
    }

    void Callback_OnKeyReleased(void* event)
    {
        SDL_Event* sdl_event = (SDL_Event*)event;
        state.keyboard.keysPressed[sdl_event->key.scancode] = false;
        state.keyboard.keysHeld[sdl_event->key.scancode] = false;
        state.keyboard.keysReleased[sdl_event->key.scancode] = true;
    }

    void Callback_OnMouseMove(void* event)
    {
        SDL_Event* sdl_event = (SDL_Event*)event;
        state.mouse.position.x = sdl_event->motion.x;
        state.mouse.position.y = sdl_event->motion.y;
        state.mouse.relative.x = sdl_event->motion.xrel;
        state.mouse.relative.y = sdl_event->motion.yrel;
    }

    void Callback_OnMouseButtonHeld(void* event)
    {
        SDL_Event* sdl_event = (SDL_Event*)event;
        state.mouse.buttonsClicked[sdl_event->button.button] = !state.mouse.buttonsHeld[sdl_event->button.button];
        state.mouse.buttonsHeld[sdl_event->button.button] = true;
        state.mouse.buttonsReleased[sdl_event->button.button] = false;
    }

    void Callback_OnMouseButtonReleased(void* event)
    {
        SDL_Event* sdl_event = (SDL_Event*)event;
        state.mouse.buttonsClicked[sdl_event->button.button] = false;
        state.mouse.buttonsHeld[sdl_event->button.button] = false;
        state.mouse.buttonsReleased[sdl_event->button.button] = true;
    }
}
