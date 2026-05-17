#include "Graphics/Window.h"

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
        Window window;

        SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        window.handle = (void*)SDL_CreateWindow(title.c_str(), width, height, flags);

        if (window.handle == NULL)
        {
            FATAL("Windows::Create - %s", "SDL failed to create the window handle!");
            return window;
        }

        SDL_SetWindowPosition((SDL_Window*)window.handle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

        SDL_GPUShaderFormat formats = SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL;
        window.gpu_device = (void*)SDL_CreateGPUDevice(formats, true, NULL);

        if (window.gpu_device == NULL)
        {
            FATAL("Windows::Create - %s", "SDL failed to create the GPU device!");
            return window;
        }

        if (!SDL_ClaimWindowForGPUDevice((SDL_GPUDevice*)window.gpu_device, (SDL_Window*)window.handle))
        {
            FATAL("Windows::Create - %s", "The window failed to claim it's GPU device!");
            return window;
        }

        SDL_SetGPUSwapchainParameters((SDL_GPUDevice*)window.gpu_device, (SDL_Window*)window.handle,
                                      SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

        window.is_valid = true;
        return window;
    }

    void Destroy(Window& window)
    {
        SDL_WaitForGPUIdle((SDL_GPUDevice*)window.gpu_device);
        SDL_ReleaseWindowFromGPUDevice((SDL_GPUDevice*)window.gpu_device, (SDL_Window*)window.handle);
        SDL_DestroyGPUDevice((SDL_GPUDevice*)window.gpu_device);
        SDL_DestroyWindow((SDL_Window*)window.handle);
        window.is_valid = false;
    }

    void HandleEvents(Window& window)
    {
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

                default:
                    break;
            }
        }
    }
}
