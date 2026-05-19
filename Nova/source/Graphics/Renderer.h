#pragma once
#include "Core/Base.h"
#include <glm/glm.hpp>

namespace Nova
{
    struct Camera3D;
    struct Mesh;
    struct Material;

    namespace Renderer
    {
        void Init();
        void Shutdown();
        bool BeginFrame();
        void EndFrame();

        void DrawMesh(const Mesh& mesh, const glm::mat4& transform, const Material& material);

        void* GetRenderPass();
        void* GetCommandBuffer();
        Camera3D* GetPrimaryCamera();
        const glm::mat4& GetMatrixView();
        const glm::mat4& GetMatrixProjection();

        void SetPrimaryCamera(Camera3D* camera);
        u32 IncrementVertexBuffers();
        u32 IncrementIndexBuffers();
        u32 DecrementVertexBuffers();
        u32 DecrementIndexBuffers();
    }
}
