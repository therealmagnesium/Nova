#include "Graphics/Buffers.h"
#include "Graphics/Renderer.h"
#include "Core/Application.h"
#include "Core/Log.h"

#include <SDL3/SDL_gpu.h>

namespace Nova::Buffers
{
    u32 BufferTypeToSDLBufferUsage(GPUBufferType type);
    string BufferTypeToString(GPUBufferType type);
    u32 CalculateBufferID(GPUBufferType type);

    GPUBuffer Create(GPUBufferType type, u32 size)
    {
        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);

        SDL_GPUBufferCreateInfo buffer_info = {};
        buffer_info.usage = BufferTypeToSDLBufferUsage(type);
        buffer_info.size = size;

        SDL_GPUBuffer* buffer_handle = SDL_CreateGPUBuffer(device, &buffer_info);
        if (buffer_handle == NULL)
        {
            ERROR("Buffers::Create - Failed to create %s!", BufferTypeToString(type).c_str());
            return Stub_GPUBuffer;
        }

        GPUBuffer buffer;
        buffer.type = type;
        buffer.id = CalculateBufferID(buffer.type);
        buffer.handle = buffer_handle;
        return buffer;
    }

    void Destroy(GPUBuffer& buffer)
    {
        if (buffer.handle == NULL || buffer.id == 0)
            return;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);
        SDL_ReleaseGPUBuffer(device, static_cast<SDL_GPUBuffer*>(buffer.handle));

        switch (buffer.type)
        {
            case GPUBufferType::Vertex:
                Renderer::DecrementVertexBuffers();
                break;
            case GPUBufferType::Index:
                Renderer::DecrementIndexBuffers();
                break;
            case GPUBufferType::Storage:
                Renderer::DecrementStorageBuffers();
                break;
        }

        buffer.handle = NULL;
        buffer.id = 0;
    }

    void Bind(const GPUBuffer& buffer, u32 slot)
    {
        SDL_GPURenderPass* render_pass = static_cast<SDL_GPURenderPass*>(Renderer::GetActiveRenderPass());
        SDL_GPUBuffer* handle = static_cast<SDL_GPUBuffer*>(buffer.handle);
        const auto binding = (SDL_GPUBufferBinding){
            .buffer = handle,
            .offset = 0
        };

        switch (buffer.type)
        {
            case GPUBufferType::Vertex:
                SDL_BindGPUVertexBuffers(render_pass, slot, &binding, 1);
                break;
            case GPUBufferType::Index:
                SDL_BindGPUIndexBuffer(render_pass, &binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
                break;
            case GPUBufferType::Storage:
                SDL_BindGPUVertexStorageBuffers(render_pass, slot, &handle, 1);
                break;
        }
    }

    void Upload(GPUBuffer& buffer, const void* data, u32 size)
    {
        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);

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
        dest.buffer = static_cast<SDL_GPUBuffer*>(buffer.handle);
        dest.offset = 0;
        dest.size = size;

        SDL_UploadToGPUBuffer(copy_pass, &source, &dest, false);
        SDL_EndGPUCopyPass(copy_pass);

        // Submit the command buffer and release the transfer buffer
        SDL_SubmitGPUCommandBuffer(command_buffer);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    }

    u32 BufferTypeToSDLBufferUsage(GPUBufferType type)
    {
        u32 type_sdl = 0;
        switch (type)
        {
            case GPUBufferType::Vertex:
                type_sdl = SDL_GPU_BUFFERUSAGE_VERTEX;
                break;
            case GPUBufferType::Index:
                type_sdl = SDL_GPU_BUFFERUSAGE_INDEX;
                break;
            case GPUBufferType::Storage:
                type_sdl = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
                break;
        }
        return type_sdl;
    }

    string BufferTypeToString(GPUBufferType type)
    {
        string name;
        switch (type)
        {
            case GPUBufferType::Vertex:
                name = "Vertex Buffer";
                break;
            case GPUBufferType::Index:
                name = "Index Buffer";
                break;
            case GPUBufferType::Storage:
                name = "Shader Storage Buffer";
                break;
        }
        return name;
    }

    u32 CalculateBufferID(GPUBufferType type)
    {
        u32 id = 0;
        switch (type)
        {
            case GPUBufferType::Vertex:
                id = Renderer::IncrementVertexBuffers();
                break;
            case GPUBufferType::Index:
                id = Renderer::IncrementIndexBuffers();
                break;
            case GPUBufferType::Storage:
                id = Renderer::IncrementStorageBuffers();
        }
        return id;
    }
}
