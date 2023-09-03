#include "VertexArray.h"
#include <glad/gl.h>

namespace Nova
{
    VertexArray::VertexArray()
    {
        glGenVertexArrays(1, &rendererID);
    }

    VertexArray::~VertexArray()
    {
        glDeleteVertexArrays(1, &rendererID);
    }

    void VertexArray::LinkVBO(VertexBuffer& vbo, uint32_t layout)
    {
        vbo.Bind();
        glEnableVertexAttribArray(layout);
        glVertexAttribPointer(layout, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        vbo.Unbind();
    }

    void VertexArray::Bind()
    {
        glBindVertexArray(rendererID);
    }

    void VertexArray::Unbind()
    {
        glBindVertexArray(0);
    }

}
