#pragma once
#include "Core/Base.h"
#include <glm/glm.hpp>

namespace Nova::Graphics
{
    struct Vertex
    {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec2 uv;
    };

    namespace Renderer
    {
        void Init();
        void Shutdown();
        bool BeginFrame();
        void EndFrame();

        void* GetRenderPass();
        u32 IncrementVertexBuffers();
        u32 IncrementIndexBuffers();
        u32 DecrementVertexBuffers();
        u32 DecrementIndexBuffers();
    }
}
