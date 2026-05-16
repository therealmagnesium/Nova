#pragma once
#include "Core/Base.h"
#include <glm/glm.hpp>

namespace Nova
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
        void* GetCommandBuffer();
        const glm::mat4& GetMatrixView();
        const glm::mat4& GetMatrixProjection();

        u32 IncrementVertexBuffers();
        u32 IncrementIndexBuffers();
        u32 DecrementVertexBuffers();
        u32 DecrementIndexBuffers();
    }
}
