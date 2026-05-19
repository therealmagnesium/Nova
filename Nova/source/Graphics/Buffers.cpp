#include "Graphics/Buffers.h"
#include "Graphics/Renderer.h"
#include "Core/Application.h"
#include "Core/Log.h"

#include <SDL3/SDL_gpu.h>

namespace Nova::Buffers
{
    GPUBuffer Create(GPUBufferType type, u32 size)
    {
        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = (SDL_GPUDevice*)window.gpu_device;

        SDL_GPUBufferCreateInfo buffer_info = {};
        buffer_info.usage = type == GPUBufferType::Vertex ? SDL_GPU_BUFFERUSAGE_VERTEX : SDL_GPU_BUFFERUSAGE_INDEX;
        buffer_info.size = size;

        GPUBuffer* buffer_handle = (GPUBuffer*)SDL_CreateGPUBuffer(device, &buffer_info);
        if (buffer_handle == NULL)
        {
            ERROR("Buffers::Create - Failed to create %s buffer!", type == GPUBufferType::Vertex ? "vertex" : "index");
            return Stub_GPUBuffer;
        }

        GPUBuffer buffer;
        buffer.type = type;
        buffer.id = type == GPUBufferType::Vertex ? Renderer::IncrementVertexBuffers() : Renderer::IncrementIndexBuffers();
        buffer.handle = buffer_handle;
        return buffer;
    }

    void Destroy(GPUBuffer& buffer)
    {
        if (buffer.handle == NULL)
            return;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = (SDL_GPUDevice*)window.gpu_device;
        SDL_ReleaseGPUBuffer(device, (SDL_GPUBuffer*)buffer.handle);

        buffer.handle = NULL;
        buffer.id = 0;
    }

    void Bind(const GPUBuffer& buffer, u32 slot)
    {
        SDL_GPURenderPass* render_pass = (SDL_GPURenderPass*)Renderer::GetRenderPass();
        SDL_GPUBufferBinding binding = {};
        binding.buffer = (SDL_GPUBuffer*)buffer.handle;
        binding.offset = 0;

        if (buffer.type == GPUBufferType::Vertex)
            SDL_BindGPUVertexBuffers(render_pass, slot, &binding, 1);
        else
            SDL_BindGPUIndexBuffer(render_pass, &binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    }

    void Upload(GPUBuffer& buffer, const void* data, u32 size)
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
        dest.buffer = (SDL_GPUBuffer*)buffer.handle;
        dest.offset = 0;
        dest.size = size;

        SDL_UploadToGPUBuffer(copy_pass, &source, &dest, false);
        SDL_EndGPUCopyPass(copy_pass);

        // Submit the command buffer and release the transfer buffer
        SDL_SubmitGPUCommandBuffer(command_buffer);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    }
}
