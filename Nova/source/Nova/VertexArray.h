#pragma once
#include "Buffer.h"

namespace Nova
{

    struct VertexArray
    {
        uint32_t rendererID;

        VertexArray();
        ~VertexArray();

        void LinkVBO(VertexBuffer& vbo, uint32_t layout);
        void Bind();
        void Unbind();
    };

}
