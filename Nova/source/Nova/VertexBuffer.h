#pragma once

#include <stddef.h>
#include <stdint.h>

namespace Nova
{
    struct VertexBuffer
    {
        uint32_t rendererID;

        VertexBuffer(void* data, size_t size);
        ~VertexBuffer();

        void Bind();
        void Unbind();
    };

    void SetVertexAttributesPointers(uint16_t index, uint16_t size, uint64_t pointer);
}
