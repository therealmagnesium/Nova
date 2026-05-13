#pragma once
#include "Core/Base.h"

namespace Nova::Graphics
{
    enum class GPUBufferType : u8
    {
        Vertex = 0,
        Index
    };

    using GPUBuffer = void;

    namespace Buffers
    {
        GPUBuffer* Create(GPUBufferType type, u32 size);
        void Destroy(GPUBuffer* buffer);
        void Bind(GPUBuffer* buffer, u32 slot = 0);
        void Upload(GPUBuffer* buffer, void* data, u32 size);
    }
}
