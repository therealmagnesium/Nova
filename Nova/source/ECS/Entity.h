#pragma once
#include "ECS/Scene.h"

namespace Nova
{
    struct Entity
    {
        EntityID id = -1;
        Scene* context = NULL;

        template <typename T>
        inline T* GetComponent()
        {
            return context->registry.GetComponent<T>(id);
        }

        template <typename T>
        inline const T* GetComponent() const
        {
            return context->registry.GetComponent<T>(id);
        }

        template <typename T>
        inline bool HasComponent() const
        {
            return context->registry.HasComponent<T>(id);
        }

        template <typename T, typename... Args>
        inline T* AddComponent(Args&&... args)
        {
            return context->registry.AddComponent<T>(id, std::forward<Args>(args)...);
        }

        template <typename T>
        inline void RemoveComponent()
        {
            context->registry.RemoveComponent<T>(id);
        }

        inline bool operator==(const Entity& other) const
        {
            return id == other.id && context == other.context;
        }

        inline bool operator!=(const Entity& other) const
        {
            return !(*this == other);
        }
    };

    inline const Entity Stub_Entity;
}
