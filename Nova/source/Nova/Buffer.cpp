#include "Buffer.h"

#include <glad/gl.h>

namespace Nova
{
    VertexBuffer::VertexBuffer(float* data, size_t size)
    {
        glGenBuffers(1, &rendererID);
        glBindBuffer(GL_ARRAY_BUFFER, rendererID);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    }

    VertexBuffer::~VertexBuffer()
    {
        glDeleteBuffers(1, &rendererID);
    }

    void VertexBuffer::Bind()
    {
        glBindBuffer(GL_ARRAY_BUFFER, rendererID);
    }

    void VertexBuffer::Unbind()
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    IndexBuffer::IndexBuffer(uint32_t* data, uint32_t countIn) : count(countIn)
    {
        glGenBuffers(1, &rendererID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rendererID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), data, GL_STATIC_DRAW);
    }

    IndexBuffer::~IndexBuffer()
    {
        glDeleteBuffers(1, &rendererID);
    }

    void IndexBuffer::Bind()
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rendererID);
    }

    void IndexBuffer::Unbind()
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void SetVertexAttributesPointers(uint16_t index, uint16_t size, uint64_t pointer)
    {
        glEnableVertexAttribArray(index);
        glVertexAttribPointer(index, size, GL_FLOAT, GL_FALSE, size * sizeof(float), (void*)pointer);
    }

}
