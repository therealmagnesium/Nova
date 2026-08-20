#pragma once
#include "ECS/Scene.h"

namespace Nova
{
    struct Entity
    {
        EntityID id = -1;

        template <typename T>
        inline T* GetComponent(Scene& context) const
        {
            return context.registry.GetComponent<T>(id);
        }

        template <typename T>
        inline const T* GetComponent(const Scene& context) const
        {
            return context.registry.GetComponent<T>(id);
        }

        template <typename T>
        inline bool HasComponent(const Scene& context) const
        {
            return context.registry.HasComponent<T>(id);
        }

        template <typename T, typename... Args>
        inline T* AddComponent(Scene& context, Args&&... args) const
        {
            return context.registry.AddComponent<T>(id, std::forward<Args>(args)...);
        }

        template <typename T>
        inline void RemoveComponent(Scene& context) const
        {
            context.registry.RemoveComponent<T>(id);
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
