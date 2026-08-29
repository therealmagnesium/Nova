#include "UI/UI.h"
#include "Core/Application.h"
#include "Graphics/Renderer.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>
#include <SDL3/SDL_events.h>

namespace Nova::UI
{
    void Init()
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

        const Window& window = Application::GetWindow();
        SDL_Window* sdl_window = static_cast<SDL_Window*>(window.handle);
        SDL_GPUDevice* sdl_device = static_cast<SDL_GPUDevice*>(window.gpu_device);

        // Setup Platform/Renderer backends
        ImGui_ImplSDL3_InitForSDLGPU(sdl_window);
        ImGui_ImplSDLGPU3_InitInfo init_info = {
            .Device = sdl_device,
            .ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(sdl_device, sdl_window),
            .MSAASamples = SDL_GPU_SAMPLECOUNT_1,                     // Only used in multi-viewports mode
            .SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR, // Only used in multi-viewports mode
            .PresentMode = SDL_GPU_PRESENTMODE_VSYNC,
        };
        ImGui_ImplSDLGPU3_Init(&init_info);
    }

    void Shutdown()
    {
        ImGui_ImplSDL3_Shutdown();
        ImGui_ImplSDLGPU3_Shutdown();
        ImGui::DestroyContext();
    }

    void ProcessEvent(const void* event)
    {
        const SDL_Event* sdl_event = static_cast<const SDL_Event*>(event);
        ImGui_ImplSDL3_ProcessEvent(sdl_event);
    }

    void BeginFrame()
    {
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void EndFrame()
    {
        ImGui::Render();

        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data == NULL)
            return;

        SDL_GPUCommandBuffer* command_buffer = static_cast<SDL_GPUCommandBuffer*>(Renderer::GetCommandBuffer());
        ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer); // Prepare vertex/index data using the engine's current command buffer
    }

    void Display(RenderPassHandle render_pass)
    {
        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data == NULL)
            return;

        SDL_GPUCommandBuffer* command_buffer = static_cast<SDL_GPUCommandBuffer*>(Renderer::GetCommandBuffer());
        SDL_GPURenderPass* sdl_render_pass = static_cast<SDL_GPURenderPass*>(render_pass);
        ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, sdl_render_pass);
    }
}
