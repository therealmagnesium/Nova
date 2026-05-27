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

        /*
        GPUBuffer() = default;
        GPUBuffer(const GPUBuffer&) = delete;
        GPUBuffer& operator=(const GPUBuffer&) = delete;

        GPUBuffer(GPUBuffer&& other) noexcept
        {
            other.id = 0;
            other.handle = NULL;
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
        }*/
    };

    inline const GPUBuffer Stub_GPUBuffer;

    namespace Buffers
    {
        GPUBuffer Create(GPUBufferType type, u32 size);
        void Destroy(GPUBuffer& buffer);
        void Bind(const GPUBuffer& buffer, u32 slot = 0);
        void Upload(GPUBuffer& buffer, const void* data, u32 size);
    }
}
