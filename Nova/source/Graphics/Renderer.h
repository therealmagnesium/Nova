#pragma once
#include <glm/glm.hpp>

namespace Nova::Graphics
{
    struct Vertex
    {
        glm::vec3 position;
        glm::vec4 color;
    };

    namespace Renderer
    {
        void Init();
        void Shutdown();
        bool BeginFrame();
        void EndFrame();

        void* GetRenderPass();
    }
}
