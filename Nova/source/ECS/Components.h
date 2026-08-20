#pragma once
#include "ECS/Entity.h"

#include "Core/AssetManager.h"
#include "Core/Log.h"
#include "Core/Random.h"

#include "Graphics/Animator.h"
#include "Graphics/Camera.h"
#include "Graphics/Model.h"

#include <glm/vec3.hpp>

namespace Nova
{
    struct Component
    {
        bool has = false;
    };

    struct InternalComponent : public Component
    {
        string tag;
        UUID id;
        bool is_active = false;
        // TODO: Maybe store the scene context in here?

        InternalComponent() = default;
        InternalComponent(const InternalComponent&) = default;
        InternalComponent(const string& tag, bool is_active = true)
        {
            this->tag = tag;
            this->is_active = is_active;
            this->id = Random::GenerateUUID();
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
        Entity target_entity;
        bool is_primary = false;

        PerspectiveCameraComponent() = default;
        PerspectiveCameraComponent(const PerspectiveCameraComponent&) = default;
        PerspectiveCameraComponent(bool is_primary, const Entity& target_entity = Stub_Entity)
        {
            this->is_primary = is_primary;
            this->target_entity = target_entity;
            this->camera.position = glm::vec3(0.f);
            this->camera.target = glm::vec3(0.f);
            this->camera.fov = 75.f;
            this->camera.clip_near = 0.1f;
            this->camera.clip_far = 100.f;
        }
    };

    struct MeshFilterComponent : public Component
    {
        PrimitiveMesh primitive;
        Material material;

        MeshFilterComponent() = default;
        MeshFilterComponent(const MeshFilterComponent&) = default;
        MeshFilterComponent(PrimitiveMesh primitive, const Material& material)
        {
            this->primitive = primitive;
            this->material = material;
        }
    };

    struct MeshRendererComponent : public Component
    {
        AssetHandle asset_model = AssetHandle_Invalid; // Standard model

        MeshRendererComponent() = default;
        MeshRendererComponent(const MeshRendererComponent&) = default;
        MeshRendererComponent(AssetHandle asset_model)
        {
            this->asset_model = asset_model;
        }
    };

    struct AnimatorComponent : public Component
    {
        Animator animator;
        AssetHandle asset_model = AssetHandle_Invalid; // Animated model

        AnimatorComponent() = default;
        AnimatorComponent(const AnimatorComponent&) = default;
        AnimatorComponent(AssetHandle asset_model, AssetHandle* assets_clips = NULL, u16 clip_count = 0)
        {
            if (!AssetManager::IsHandleValid(asset_model))
            {
                WARN("AnimatorComponent::AnimatorComponent - %s", "Component left uninitialized since \"asset_model\" was an invalid handle");
                return;
            }

            this->asset_model = asset_model;
            const AnimatedModel* model = AssetManager::GetAsset<AnimatedModel>(asset_model);

            this->animator = Animators::Create(model->skeleton);
            if (assets_clips == NULL || clip_count == 0)
                return;

            for (u16 i = 0; i < clip_count; i++)
            {
                const AnimationClip* clip = AssetManager::GetAsset<AnimationClip>(assets_clips[i]);
                Animations::Bind(*clip, model->skeleton);

                if (i == 0)
                    Animators::Play(animator, *clip, true);
            }
        }
    };
}
