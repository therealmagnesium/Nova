#pragma once
#include "Core/Base.h"

namespace Nova
{
    using GPUBufferHandle = void*;

    enum class GPUBufferType : u8
    {
        Vertex = 0, // VBO
        Index,      // IBO
        Storage,    // SSBO
    };

    struct GPUBuffer
    {
        GPUBufferHandle handle = NULL;
        u32 id = 0;
        GPUBufferType type = GPUBufferType::Vertex;

        GPUBuffer() = default;
        GPUBuffer(const GPUBuffer&) = delete;
        GPUBuffer& operator=(const GPUBuffer&) = delete;

        GPUBuffer(GPUBuffer&& other) noexcept
        {
            handle = other.handle;
            id = other.id;
            type = other.type;
            other.handle = NULL;
            other.id = 0;
        }
        GPUBuffer& operator=(GPUBuffer&& other) noexcept
        {
            if (this != &other)
            {
                handle = other.handle;
                id = other.id;
                type = other.type;
                other.handle = NULL;
                other.id = 0;
            }
            return *this;
        }
    };

    namespace Buffers
    {
        GPUBuffer Create(GPUBufferType type, u32 size);
        void Destroy(GPUBuffer& buffer);
        void Bind(const GPUBuffer& buffer, u32 slot = 0);
        void Upload(GPUBuffer& buffer, const void* data, u32 size);
        void ResetBindingCache();

        // --- Streaming upload API ---
        // For data that changes every frame (skinning matrices, dynamic UBOs, particle instance
        // data, etc). One UploadStream owns a single persistently-mapped transfer buffer sized up
        // front at creation. StreamWrite is a pure CPU memcpy - call it as many times as you like
        // during the frame, including from inside an active render pass. StreamFlush records every
        // pending write as ONE copy pass on its own command buffer and submits it - call it exactly
        // once per frame, at a point where no render pass is currently open (Renderer::EndFrame is
        // the right place). See Renderer.cpp's bone matrix pool for the intended usage pattern.
        struct UploadStream;

        UploadStream* CreateUploadStream(u32 capacity_bytes);
        void DestroyUploadStream(UploadStream* stream);
        u32 StreamWrite(UploadStream* stream, const void* data, u32 size);                     // returns the byte offset written to
        void StreamQueueCopy(UploadStream* stream, u32 src_offset, GPUBuffer& dest, u32 size); // queues a transfer->dest copy for the next Flush
        void StreamFlush(UploadStream* stream);
    }
}
