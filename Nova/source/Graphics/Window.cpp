#include "Graphics/Window.h"
#include "Graphics/Renderer.h"

#include "Core/Application.h"
#include "Core/Input.h"
#include "Core/Log.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

namespace Nova::Windows
{
    Window Create(u16 width, u16 height, const string& title)
    {
        SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        SDL_Window* handle = SDL_CreateWindow(title.c_str(), width, height, flags);

        if (handle == NULL)
        {
            FATAL("Windows::Create - %s", "SDL failed to create the window handle!");
            return Stub_Window;
        }

        SDL_SetWindowPosition(handle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

        SDL_GPUShaderFormat formats = SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL;
        SDL_GPUDevice* device = SDL_CreateGPUDevice(formats, true, NULL);

        if (device == NULL)
        {
            FATAL("Windows::Create - %s", "SDL failed to create the GPU device!");
            return Stub_Window;
        }

        if (!SDL_ClaimWindowForGPUDevice(device, handle))
        {
            FATAL("Windows::Create - %s", "The window failed to claim it's GPU device!");
            return Stub_Window;
        }

        SDL_SetGPUSwapchainParameters(device, handle, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

        Window window;
        window.width = width;
        window.height = height;
        window.title = title;
        window.handle = handle;
        window.gpu_device = device;
        window.state ^= NOVA_WINDOWSTATE_VALID;

        return window;
    }

    void Destroy(Window& window)
    {
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);
        SDL_Window* handle = static_cast<SDL_Window*>(window.handle);

        SDL_WaitForGPUIdle(device);
        SDL_ReleaseWindowFromGPUDevice(device, handle);
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(handle);
        window.state ^= NOVA_WINDOWSTATE_VALID;
    }

    void HandleEvents(Window& window)
    {
        window.state &= ~NOVA_WINDOWSTATE_RESIZED;
        Input::Reset();

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    Application::Quit();
                    break;

                case SDL_EVENT_KEY_DOWN:
                    Input::Callback_OnKeyHeld(&event);
                    break;

                case SDL_EVENT_KEY_UP:
                    Input::Callback_OnKeyReleased(&event);
                    break;

                case SDL_EVENT_MOUSE_MOTION:
                    Input::Callback_OnMouseMove(&event);
                    break;

                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    Input::Callback_OnMouseButtonHeld(&event);
                    break;

                case SDL_EVENT_MOUSE_BUTTON_UP:
                    Input::Callback_OnMouseButtonReleased(&event);
                    break;

                case SDL_EVENT_WINDOW_RESIZED:
                    window.width = event.window.data1;
                    window.height = event.window.data2;
                    window.state |= NOVA_WINDOWSTATE_RESIZED;
                    window.state &= ~NOVA_WINDOWSTATE_MAXIMIZED;
                    window.state &= ~NOVA_WINDOWSTATE_MINIMIZED;
                    Renderer::Callback_OnResize();
                    break;

                case SDL_EVENT_WINDOW_MAXIMIZED:
                    window.state |= NOVA_WINDOWSTATE_MAXIMIZED;
                    window.state &= ~NOVA_WINDOWSTATE_MINIMIZED;
                    break;

                case SDL_EVENT_WINDOW_OCCLUDED:
                case SDL_EVENT_WINDOW_MINIMIZED:
                    window.state |= NOVA_WINDOWSTATE_MINIMIZED;
                    window.state &= ~NOVA_WINDOWSTATE_MAXIMIZED;
                    break;

                case SDL_EVENT_WINDOW_EXPOSED:
                case SDL_EVENT_WINDOW_RESTORED:
                    window.state |= NOVA_WINDOWSTATE_RESIZED;
                    window.state &= ~NOVA_WINDOWSTATE_MAXIMIZED;
                    window.state &= ~NOVA_WINDOWSTATE_MINIMIZED;
                    break;

                default:
                    break;
            }
        }
    }

    bool IsResizing(const Window& window) { return window.state & NOVA_WINDOWSTATE_RESIZED; }
    bool IsMinimized(const Window& window) { return window.state & NOVA_WINDOWSTATE_MINIMIZED; }
    bool IsMaximized(const Window& window) { return window.state & NOVA_WINDOWSTATE_MAXIMIZED; }
}
