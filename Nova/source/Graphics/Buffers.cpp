#include "Graphics/Buffers.h"
#include "Graphics/Renderer.h"
#include "Core/Application.h"
#include "Core/Log.h"

#include <SDL3/SDL_gpu.h>

using namespace Nova::Core;

namespace Nova::Graphics::Buffers
{
    GPUBuffer* Create(GPUBufferType type, u32 size)
    {
        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = (SDL_GPUDevice*)window.gpu_device;

        SDL_GPUBufferCreateInfo buffer_info = {};
        buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        buffer_info.size = size;

        GPUBuffer* buffer = (GPUBuffer*)SDL_CreateGPUBuffer(device, &buffer_info);
        if (buffer == NULL)
        {
            ERROR("Buffers::Create - %s", "Failed to create vertex buffer!");
            return NULL;
        }

        return buffer;
    }

    void Destroy(GPUBuffer* buffer)
    {
        if (buffer == NULL)
            return;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = (SDL_GPUDevice*)window.gpu_device;
        SDL_ReleaseGPUBuffer(device, (SDL_GPUBuffer*)buffer);

        buffer = NULL;
    }

    void Bind(GPUBuffer* buffer, u32 slot)
    {
        SDL_GPURenderPass* render_pass = (SDL_GPURenderPass*)Renderer::GetRenderPass();
        SDL_GPUBufferBinding binding = {};
        binding.buffer = (SDL_GPUBuffer*)buffer;
        binding.offset = 0;
        SDL_BindGPUVertexBuffers(render_pass, slot, &binding, 1);
    }

    void Upload(GPUBuffer* buffer, void* data, u32 size)
    {
        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = (SDL_GPUDevice*)window.gpu_device;

        // Create a transfer buffer (CPU-accessible staging area)
        SDL_GPUTransferBufferCreateInfo transfer_buffer_info = {};
        transfer_buffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transfer_buffer_info.size = size;

        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_info);

        // Map the transfer buffer and write CPU data into it
        void* mapped = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
        memcpy(mapped, data, size);
        SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

        // Record a copy pass to upload from transfer → GPU buffer
        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

        SDL_GPUTransferBufferLocation source = {};
        source.transfer_buffer = transfer_buffer;
        source.offset = 0;

        SDL_GPUBufferRegion dest = {};
        dest.buffer = (SDL_GPUBuffer*)buffer;
        dest.offset = 0;
        dest.size = size;

        SDL_UploadToGPUBuffer(copy_pass, &source, &dest, false);
        SDL_EndGPUCopyPass(copy_pass);

        // Submit the command buffer and release the transfer buffer
        SDL_SubmitGPUCommandBuffer(command_buffer);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    }
}
