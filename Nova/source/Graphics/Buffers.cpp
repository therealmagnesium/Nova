#include "Graphics/Buffers.h"
#include "Core/Application.h"
#include "Core/Log.h"
#include "Graphics/Renderer.h"

#include <SDL3/SDL_gpu.h>

namespace Nova::Buffers
{
    struct QueuedCopy
    {
        u32 src_offset;
        SDL_GPUBuffer* dest;
        u32 size;
    };

    struct UploadStream
    {
        SDL_GPUTransferBuffer* transfer_buffer = NULL;
        void* mapped = NULL;
        u32 capacity = 0;
        u32 write_cursor = 0;
        std::vector<QueuedCopy> queued_copies;
    };

    struct BufferCache
    {
        SDL_GPUBuffer* buffer_vertex = NULL;
        SDL_GPUBuffer* buffer_index = NULL;
    };

    u32 BufferTypeToSDLBufferUsage(GPUBufferType type);
    string BufferTypeToString(GPUBufferType type);
    u32 CalculateBufferID(GPUBufferType type);

    UploadStream* CreateUploadStream(u32 capacity_bytes);
    void DestroyUploadStream(UploadStream* stream);
    u32 StreamWrite(UploadStream* stream, const void* data, u32 size);                     // returns the byte offset written to
    void StreamQueueCopy(UploadStream* stream, u32 src_offset, GPUBuffer& dest, u32 size); // queues a transfer->dest copy for the next Flush
    void StreamFlush(UploadStream* stream);

    static BufferCache cache;

    GPUBuffer Create(GPUBufferType type, u32 size)
    {
        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);

        const SDL_GPUBufferCreateInfo buffer_info = {
            .usage = BufferTypeToSDLBufferUsage(type),
            .size = size,
        };

        SDL_GPUBuffer* buffer_handle = SDL_CreateGPUBuffer(device, &buffer_info);
        if (buffer_handle == NULL)
        {
            ERROR("Buffers::Create - Failed to create %s!",
                  BufferTypeToString(type).c_str());
            return {};
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
        const SDL_GPUBufferBinding binding = { .buffer = handle, .offset = 0 };

        switch (buffer.type)
        {
            case GPUBufferType::Vertex:
                if (handle == cache.buffer_vertex)
                    return;

                SDL_BindGPUVertexBuffers(render_pass, slot, &binding, 1);
                cache.buffer_vertex = handle;
                break;
            case GPUBufferType::Index:
                if (handle == cache.buffer_index)
                    return;

                SDL_BindGPUIndexBuffer(render_pass, &binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
                cache.buffer_index = handle;
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
        const SDL_GPUTransferBufferCreateInfo transfer_buffer_info = {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = size
        };

        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_info);
        if (transfer_buffer == NULL)
        {
            ERROR("Buffers::Upload - %s", "Failed to create GPU transfer buffer!");
            return;
        }

        // Map the transfer buffer and write CPU data into it
        void* mapped = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
        memcpy(mapped, data, size);
        SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

        // Record a copy pass to upload from transfer → GPU buffer
        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

        const SDL_GPUTransferBufferLocation source = {
            .transfer_buffer = transfer_buffer,
            .offset = 0
        };

        const SDL_GPUBufferRegion dest = {
            .buffer = static_cast<SDL_GPUBuffer*>(buffer.handle),
            .offset = 0,
            .size = size
        };

        SDL_UploadToGPUBuffer(copy_pass, &source, &dest, false);
        SDL_EndGPUCopyPass(copy_pass);

        // Submit the command buffer and release the transfer buffer
        SDL_SubmitGPUCommandBuffer(command_buffer);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    }

    void ResetBindingCache()
    {
        cache.buffer_vertex = NULL;
        cache.buffer_index = NULL;
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

    UploadStream* CreateUploadStream(u32 capacity_bytes)
    {
        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);

        SDL_GPUTransferBufferCreateInfo info = {};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = capacity_bytes;

        SDL_GPUTransferBuffer* handle = SDL_CreateGPUTransferBuffer(device, &info);
        if (handle == NULL)
        {
            ERROR("Buffers::CreateUploadStream - %s",
                  "Failed to create GPU transfer buffer!");
            return NULL;
        }

        UploadStream* stream = new UploadStream();
        stream->transfer_buffer = handle;
        stream->capacity = capacity_bytes;
        return stream;
    }

    void DestroyUploadStream(UploadStream* stream)
    {
        if (stream == NULL)
            return;

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);
        SDL_ReleaseGPUTransferBuffer(device, stream->transfer_buffer);
        delete stream;
    }

    u32 StreamWrite(UploadStream* stream, const void* data, u32 size)
    {
        if (stream == NULL || stream->write_cursor + size > stream->capacity)
        {
            ERROR("%s", "Buffers::StreamWrite - Upload stream is full or invalid, "
                        "dropping write! Consider raising its capacity.");
            return 0;
        }

        // Mapped lazily on the first write of the frame, unmapped in StreamFlush.
        if (stream->mapped == NULL)
        {
            const Window& window = Application::GetWindow();
            SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);
            // cycle=true: don't stall waiting for a previous frame's copy pass to
            // finish reading this transfer buffer - SDL_GPU transparently hands back a
            // fresh backing allocation instead of blocking the CPU here.
            stream->mapped =
                SDL_MapGPUTransferBuffer(device, stream->transfer_buffer, true);
        }

        const u32 offset = stream->write_cursor;
        memcpy(static_cast<u8*>(stream->mapped) + offset, data, size);
        stream->write_cursor += size;
        return offset;
    }

    void StreamQueueCopy(UploadStream* stream, u32 src_offset, GPUBuffer& dest, u32 size)
    {
        if (stream == NULL)
            return;

        stream->queued_copies.emplace_back(
            src_offset, static_cast<SDL_GPUBuffer*>(dest.handle), size);
    }

    void StreamFlush(UploadStream* stream)
    {
        if (stream == NULL || stream->queued_copies.empty())
            return; // Nothing was written this frame - e.g. zero animated draws - skip
                    // the submit entirely.

        const Window& window = Application::GetWindow();
        SDL_GPUDevice* device = static_cast<SDL_GPUDevice*>(window.gpu_device);

        SDL_UnmapGPUTransferBuffer(device, stream->transfer_buffer);
        stream->mapped = NULL;

        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

        for (const QueuedCopy& copy : stream->queued_copies)
        {
            SDL_GPUTransferBufferLocation source = {};
            source.transfer_buffer = stream->transfer_buffer;
            source.offset = copy.src_offset;

            SDL_GPUBufferRegion dest = {};
            dest.buffer = copy.dest;
            dest.offset = 0;
            dest.size = copy.size;

            // cycle=true: this destination SSBO was also written last frame and may
            // still be being read by last frame's draw calls when this copy executes.
            SDL_UploadToGPUBuffer(copy_pass, &source, &dest, true);
        }

        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(command_buffer);

        stream->write_cursor = 0;
        stream->queued_copies.clear();
    }
}
