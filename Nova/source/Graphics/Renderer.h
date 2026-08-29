#pragma once
#include "Core/Base.h"
#include "Graphics/RenderPass.h"
#include "Graphics/Texture.h"

#include <glm/glm.hpp>

namespace Nova
{
    struct AnimatedModel;
    struct Animator;
    struct Camera3D;
    struct DirectionalLight;
    struct EnvironmentMap;
    struct Material;
    struct Mesh;
    struct Model;
    enum class PrimitiveMesh : u8;

    namespace Renderer
    {
        void Init();
        void Shutdown();
        bool BeginFrame();
        void EndFrame();

        void Flush();
        void DrawPrimitive(PrimitiveMesh primitive, const glm::mat4& transform, const Material& material);
        void DrawMesh(const Mesh& mesh, const glm::mat4& transform, const Material& material);
        void DrawModel(const Model& model, const glm::vec3& position = glm::vec3(0.f), const glm::vec3& rotation = glm::vec3(0.f), const glm::vec3& scale = glm::vec3(1.f));
        void DrawAnimatedModel(const AnimatedModel& model, const Animator& animator, const glm::vec3& position = glm::vec3(0.f), const glm::vec3& rotation = glm::vec3(0.f), const glm::vec3& scale = glm::vec3(1.f));
        void DrawTextureCompositing(const Texture& screen_texture);
        void DrawSkybox(const EnvironmentMap& environment_map);

        float GetExposure();
        void* GetCommandBuffer();
        Camera3D* GetPrimaryCamera();
        RenderPassHandle GetActiveRenderPass();
        TextureHandle GetSwapchainHandle();
        const Texture& GetTextureSwapchain();
        const Texture& GetTextureDepthStencil();
        const glm::mat4& GetMatrixView();
        const glm::mat4& GetMatrixProjection();
        const Mesh& GetPrimitiveMesh(PrimitiveMesh primitive);

        void SetSun(const DirectionalLight& sun);
        void SetExposure(float exposure);
        void SetActiveRenderPass(RenderPassHandle render_pass);
        void SetPrimaryCamera(Camera3D* camera);
        void ToggleWireframeMode();
        u32 IncrementVertexBuffers();
        u32 IncrementIndexBuffers();
        u32 IncrementStorageBuffers();
        u32 DecrementVertexBuffers();
        u32 DecrementIndexBuffers();
        u32 DecrementStorageBuffers();
        void Callback_OnResize();
    }
}
