#include "Graphics/Renderer.h"
#include "Core/Application.h"
#include "Core/Base.h"

#include <SDL3/SDL_gpu.h>

using namespace Nova::Core;

namespace Nova::Graphics::Renderer
{
    struct RenderState
    {
        SDL_GPUCommandBuffer* command_buffer = NULL;
        SDL_GPURenderPass* render_pass = NULL;
        SDL_GPUTexture* swapchain_texture = NULL;
        bool should_display_frame = false;
        u16 framebuffer_width = 0;
        u16 framebuffer_height = 0;
    };

    static RenderState state;

    void Init() {}
    void Shutdown() {}

    void BeginFrame()
    {
        const Window& window = Application::GetWindow();
        SDL_Window* window_handle = (SDL_Window*)window.handle;
        SDL_GPUDevice* gpu_device = (SDL_GPUDevice*)window.gpu_device;

        state.command_buffer = SDL_AcquireGPUCommandBuffer(gpu_device);
        if (state.command_buffer == NULL)
        {
            state.should_display_frame = false;
            return;
        }

        if (!SDL_AcquireGPUSwapchainTexture(state.command_buffer, window_handle, &state.swapchain_texture,
                                            (u32*)&state.framebuffer_width, (u32*)&state.framebuffer_height))
        {
            SDL_CancelGPUCommandBuffer(state.command_buffer);
            state.should_display_frame = false;
            return;
        }

        if (state.swapchain_texture == NULL)
        {
            SDL_CancelGPUCommandBuffer(state.command_buffer);
            state.command_buffer = NULL;
            state.should_display_frame = false;
            return;
        }

        SDL_GPUColorTargetInfo target_info = {};
        target_info.texture = state.swapchain_texture;
        target_info.clear_color = SDL_FColor{0.2f, 0.2f, 0.2f, 1.f};
        target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        target_info.store_op = SDL_GPU_STOREOP_STORE;
        target_info.mip_level = 0;
        target_info.layer_or_depth_plane = 0;
        target_info.cycle = false;

        state.render_pass = SDL_BeginGPURenderPass(state.command_buffer, &target_info, 1, NULL);
        state.should_display_frame = true;
    }

    void EndFrame()
    {
        if (!state.should_display_frame)
            return;

        SDL_EndGPURenderPass(state.render_pass);
        SDL_SubmitGPUCommandBuffer(state.command_buffer);
    }
}
