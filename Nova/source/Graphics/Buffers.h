#pragma once
#include "Core/Base.h"

namespace Nova
{
    using GPUBufferHandle = void*;

    enum class GPUBufferType : u8
    {
        Vertex = 0,
        Index
    };

    struct GPUBuffer
    {
        GPUBufferHandle handle = NULL;
        u32 id = 0;
        GPUBufferType type = GPUBufferType::Vertex;
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
