#pragma once
#include "Core/Base.h"
#include "Core/Log.h"
#include <tuple>
#include <vector>

namespace Nova
{
    struct Entity;
    struct Component;
    struct InternalComponent;
    struct TransformComponent;
    struct PerspectiveCameraComponent;
    struct MeshFilterComponent;
    struct MeshRendererComponent;
    struct AnimatorComponent;

    using EntityID = s64;
    using ComponentPool = std::tuple<
        std::vector<InternalComponent>,
        std::vector<TransformComponent>,
        std::vector<PerspectiveCameraComponent>,
        std::vector<MeshFilterComponent>,
        std::vector<MeshRendererComponent>,
        std::vector<AnimatorComponent>>;

    struct EntityRegistry
    {
        ComponentPool pool_components;
        std::vector<EntityID> free_indices;
        u64 next_available_index = 0;

        inline EntityID CreateEntityID()
        {
            if (!free_indices.empty())
            {
                const EntityID id = free_indices.back();
                free_indices.pop_back();
                return id;
            }

            return next_available_index++;
        }

        // 1. Non-const overload: returns a mutable reference
        template <typename T>
        inline T& GetComponent(EntityID entity_id)
        {
            static_assert(std::is_base_of_v<Component, T>, "EntityRegistry::GetComponent - T must derive from Component!");
            std::vector<T>& components = std::get<std::vector<T>>(pool_components);
            ASSERT(entity_id >= 0 && entity_id < static_cast<EntityID>(components.size()) && components[entity_id].has, "EntityRegistry::GetComponent - Invalid entity ID!");

            return components[entity_id];
        }

        // 2. Const overload: returns a read-only reference
        template <typename T>
        inline const T& GetComponent(EntityID entity_id) const
        {
            static_assert(std::is_base_of_v<Component, T>, "EntityRegistry::GetComponent - T must derive from Component!");
            const std::vector<T>& components = std::get<std::vector<T>>(pool_components);
            ASSERT(entity_id >= 0 && entity_id < static_cast<EntityID>(components.size()) && components[entity_id].has, "EntityRegistry::GetComponent - Invalid entity ID!");

            return components[entity_id];
        }

        template <typename T>
        inline bool HasComponent(EntityID entity_id) const
        {
            static_assert(std::is_base_of_v<Component, T>, "EntityRegistry::HasComponent - T must derive from Component!");
            const auto& component = GetComponent<T>(entity_id);
            return component.has;
        }

        template <typename T, typename... Args>
        inline T& AddComponent(EntityID entity_id, Args&&... args)
        {
            static_assert(std::is_base_of_v<Component, T>, "EntityRegistry::AddComponent - T must derive from Component!");
            ASSERT(entity_id >= 0, "EntityRegistry::AddComponent - Entity ID \"%ld\" is not larger than 0!", entity_id);

            // Resize ALL component vectors synchronously to keep their pool sizes uniform
            const auto ResizeAllPools = [target_size = entity_id + 1](auto&... component_vectors)
            {
                auto resize_pool = [target_size](auto& vec)
                {
                    if (target_size > static_cast<EntityID>(vec.size()))
                    {
                        vec.resize(target_size);
                    }
                };
                (resize_pool(component_vectors), ...);
            };
            std::apply(ResizeAllPools, pool_components);

            std::vector<T>& components = std::get<std::vector<T>>(pool_components);
            components[entity_id] = T(std::forward<Args>(args)...);
            components[entity_id].has = true;
            return components[entity_id];
        }

        template <typename T>
        inline void RemoveComponent(EntityID entity_id)
        {
            static_assert(std::is_base_of_v<Component, T>, "EntityRegistry::RemoveComponent - T must derive from Component!");
            auto& components = std::get<std::vector<T>>(pool_components);

            if (entity_id < static_cast<EntityID>(components.size()))
            {
                components[entity_id] = T();
                components[entity_id].has = false;
            }
        }
    };

    enum class SceneState : u8
    {
        Editor,
        Runtime,
        RuntimePhysics,
    };

    struct Scene
    {
        EntityRegistry registry;
        SceneState state = SceneState::Editor;
    };

    namespace Scenes
    {
        Scene Create(u64 entity_count_estimate);
        void Destroy(Scene& scene);

        Entity CreateEntity(Scene& scene, const string& tag = "Entity"); // TODO: Pass in UUID
        void DestroyEntity(Scene& scene, Entity& entity);

        void Play(Scene& scene);
        void Stop(Scene& scene);
        void UpdateSubsystems(Scene& scene);
        void RenderSubsystems(Scene& scene);
        void Copy(const Scene& source, Scene& destination);

        Scene* GetActive();
        void SetActive(Scene& scene);
    }
}
