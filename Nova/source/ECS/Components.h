#pragma once
#include "Core/Asset.h"
#include "Graphics/Animator.h"
#include "Graphics/Camera.h"
#include "Graphics/Mesh.h"
#include <glm/vec3.hpp>

namespace Nova
{
    struct Component
    {
        bool has = false;
    };

    struct InternalComponent : public Component
    {
        // TODO: Store the UUID (Random 64 bit unsigned integer)
        string tag;
        bool is_active = false;
        bool is_alive = false;

        InternalComponent() = default;
        InternalComponent(const InternalComponent&) = default;
        InternalComponent(const string& tag, bool is_active = true)
        {
            this->tag = std::move(tag);
            this->is_active = is_active;
            this->is_alive = true;
        }
    };

    struct TransformComponent : public Component
    {
        glm::vec3 position = glm::vec3(0.f);
        glm::vec3 rotation = glm::vec3(0.f);
        glm::vec3 scale = glm::vec3(1.f);

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.f), const glm::vec3& scale = glm::vec3(1.f))
        {
            this->position = position;
            this->rotation = rotation;
            this->scale = scale;
        }
    };

    struct PerspectiveCameraComponent : public Component
    {
        Camera3D camera;
        bool is_primary = false;

        PerspectiveCameraComponent() = default;
        PerspectiveCameraComponent(const PerspectiveCameraComponent&) = default;
        PerspectiveCameraComponent(bool is_primary)
        {
            this->is_primary = is_primary;
            this->camera.position = glm::vec3(-3.f, 4.f, 5.f);
            this->camera.target = glm::vec3(0.f);
            this->camera.fov = 75.f;
            this->camera.clip_near = 0.1f;
            this->camera.clip_far = 100.f;
        }
    };

    struct MeshFilterComponent : public Component
    {
        PrimitiveMesh primitive;

        MeshFilterComponent() = default;
        MeshFilterComponent(const MeshFilterComponent&) = default;
        MeshFilterComponent(PrimitiveMesh primitive) { this->primitive = primitive; }
    };

    struct MeshRendererComponent : public Component
    {
        AssetHandle model; // Standard model

        MeshRendererComponent() = default;
        MeshRendererComponent(const MeshRendererComponent&) = default;
        MeshRendererComponent(AssetHandle model)
        {
            this->model = model;
        }
    };

    struct AnimatorComponent : public Component
    {
        Animator animator;
        AssetHandle model; // Animated model

        AnimatorComponent() = default;
        AnimatorComponent(const AnimatorComponent&) = default;
        AnimatorComponent(AssetHandle model)
        {
            this->model = model;
            // TODO: Create the animator with the model's skeleton
        }
    };
}
