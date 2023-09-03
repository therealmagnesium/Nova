#pragma once

#include <stddef.h>
#include <stdint.h>

namespace Nova
{
    struct VertexBuffer
    {
        uint32_t rendererID;

        VertexBuffer(float* data, size_t size);
        ~VertexBuffer();

        void Bind();
        void Unbind();
    };

    struct IndexBuffer
    {
        uint32_t rendererID;
        uint32_t count;

        IndexBuffer(uint32_t* data, uint32_t countIn);
        ~IndexBuffer();

        void Bind();
        void Unbind();
    };

    void SetVertexAttributesPointers(uint16_t index, uint16_t size, uint64_t pointer);
}
