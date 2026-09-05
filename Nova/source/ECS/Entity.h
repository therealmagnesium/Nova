#pragma once
#include "Core/Log.h"
#include "ECS/Scene.h"

namespace Nova
{
    struct Entity
    {
        EntityID id = -1;

        // Non-const overload for modifying components
        template <typename T>
        inline T& GetComponent()
        {
            Scene* context = Scenes::GetActive();
            ASSERT(context != NULL, "Entity::GetComponent - Cannot retrieve component since there is no active scene set to be the context!");
            return context->registry.GetComponent<T>(id);
        }

        // Const overload for read-only access
        template <typename T>
        inline const T& GetComponent() const
        {
            Scene* context = Scenes::GetActive();
            ASSERT(context != NULL, "Entity::GetComponent - Cannot retrieve component since there is no active scene set to be the context!");
            return context->registry.GetComponent<T>(id);
        }

        template <typename T>
        inline bool HasComponent() const
        {
            const Scene* context = Scenes::GetActive();
            ASSERT(context != NULL, "Entity::HasComponent - %s", "Cannot retrieve component since there is no active scene set to be the context!");
            return context->registry.HasComponent<T>(id);
        }

        template <typename T, typename... Args>
        inline T& AddComponent(Args&&... args)
        {
            Scene* context = Scenes::GetActive();
            ASSERT(context != NULL, "Entity::AddComponent - %s", "Cannot add component since there is no active scene set to be the context!");
            return context->registry.AddComponent<T>(id, std::forward<Args>(args)...);
        }

        template <typename T>
        inline void RemoveComponent()
        {
            Scene* context = Scenes::GetActive();
            ASSERT(context != NULL, "Entity::RemoveComponent - %s", "Cannot remove component since there is no active scene set to be the context!");
            context->registry.RemoveComponent<T>(id);
        }

        inline bool operator==(const Entity& other) const
        {
            return id == other.id;
        }

        inline bool operator!=(const Entity& other) const
        {
            return !(*this == other);
        }

        inline bool IsValid() const { return id >= 0; }
    };

    inline const Entity Stub_Entity;
}
