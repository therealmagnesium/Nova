#pragma once
#include "Core/Base.h"
#include <glm/glm.hpp>

namespace Nova
{
    struct Camera3D;
    struct Material;
    struct Mesh;
    struct Model;

    namespace Renderer
    {
        void Init();
        void Shutdown();
        bool BeginFrame();
        void EndFrame();

        void DrawMesh(const Mesh& mesh, const glm::mat4& transform, const Material& material);
        void DrawModel(const Model& model,
                       const glm::vec3& position = glm::vec3(0.f),
                       const glm::vec3& rotation = glm::vec3(0.f),
                       const glm::vec3& scale = glm::vec3(1.f));

        void* GetRenderPass();
        void* GetCommandBuffer();
        Camera3D* GetPrimaryCamera();
        const glm::mat4& GetMatrixView();
        const glm::mat4& GetMatrixProjection();

        void SetPrimaryCamera(Camera3D* camera);
        u32 IncrementVertexBuffers();
        u32 IncrementIndexBuffers();
        u32 IncrementStorageBuffers();
        u32 DecrementVertexBuffers();
        u32 DecrementIndexBuffers();
        u32 DecrementStorageBuffers();
        void Callback_OnResize();
    }
}
