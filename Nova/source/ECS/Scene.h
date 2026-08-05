#pragma once
#include "Core/Base.h"
#include "ECS/Components.h"

namespace Nova
{
    struct Entity;

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

        template <typename T>
        inline T* GetComponent(EntityID entity_id)
        {
            static_assert(std::is_base_of_v<Component, T>, "EntityRegistry::GetComponent - T must derive from Component!");
            std::vector<T>& components = std::get<std::vector<T>>(pool_components);
            if (entity_id >= static_cast<EntityID>(components.size()) || !components[entity_id].has)
                return NULL;

            return &components[entity_id];
        }

        template <typename T>
        inline const T* GetComponent(EntityID entity_id) const
        {
            static_assert(std::is_base_of_v<Component, T>, "EntityRegistry::GetComponent - T must derive from Component!");
            const std::vector<T>& components = std::get<std::vector<T>>(pool_components);
            if (entity_id >= static_cast<EntityID>(components.size()) || !components[entity_id].has)
                return NULL;

            return &components[entity_id];
        }

        template <typename T>
        inline bool HasComponent(EntityID entity_id) const
        {
            static_assert(std::is_base_of_v<Component, T>, "EntityRegistry::HasComponent - T must derive from Component!");
            const Component* component = GetComponent<T>(entity_id);
            if (component == NULL)
                return false;

            return component->has;
        }

        template <typename T, typename... Args>
        inline T* AddComponent(EntityID entity_id, Args&&... args)
        {
            static_assert(std::is_base_of_v<Component, T>, "EntityRegistry::AddComponent - T must derive from Component!");
            std::vector<T>& components = std::get<std::vector<T>>(pool_components);
            if (entity_id >= static_cast<EntityID>(components.size()))
                components.resize(entity_id + 1);

            components[entity_id] = T(std::forward<Args>(args)...);
            components[entity_id].has = true;
            return &components[entity_id];
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
        SceneState state;
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
    }
}
