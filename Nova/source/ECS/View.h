#pragma once
#include "ECS/Entity.h"

#include <algorithm>

namespace Nova
{
    namespace Internal
    {
        template <typename T>
        inline EntityID PoolSize(const EntityRegistry& registry)
        {
            return static_cast<EntityID>(std::get<std::vector<T>>(registry.pool_components).size());
        }

        template <typename... Components>
        inline EntityID NarrowestPoolSize(const EntityRegistry& registry)
        {
            return std::min({ PoolSize<Components>(registry)... });
        }

        template <typename... Components>
        inline bool HasAll(const EntityRegistry& registry, EntityID id)
        {
            return (registry.HasComponent<Components>(id) && ...);
        }
    }

    template <typename... Components>
    struct View
    {
        static_assert(sizeof...(Components) > 0, "View - Must filter by at least one component type!");
        static_assert((std::is_base_of_v<Component, Components> && ...), "View - All filtered types must derive from Component!");

        EntityRegistry* registry = NULL;

        struct Iterator
        {
            EntityRegistry* registry;
            EntityID current;
            EntityID last;

            inline Entity operator*() const { return Entity{ current }; }
            inline bool operator!=(const Iterator& other) const { return current != other.current; }

            inline Iterator& operator++()
            {
                do
                {
                    ++current;
                } while (current < last && !Internal::HasAll<Components...>(*registry, current));
                return *this;
            }
        };

        inline Iterator begin() const
        {
            const EntityID last = Internal::NarrowestPoolSize<Components...>(*registry);
            EntityID first = 0;
            while (first < last && !Internal::HasAll<Components...>(*registry, first))
                ++first;
            return Iterator{ registry, first, last };
        }

        inline Iterator end() const
        {
            const EntityID last = Internal::NarrowestPoolSize<Components...>(*registry);
            return Iterator{ registry, last, last };
        }
    };

    namespace Views
    {
        template <typename... Components>
        inline View<Components...> Create(Scene& scene)
        {
            return View<Components...>{ &scene.registry };
        }

        template <typename... Components, typename Func>
        inline void Each(Scene& scene, Func&& func)
        {
            static_assert(sizeof...(Components) > 0, "Views::Each - Must filter by at least one component type!");
            static_assert((std::is_base_of_v<Component, Components> && ...), "Views::Each - All filtered types must derive from Component!");

            EntityRegistry& registry = scene.registry;
            const EntityID last = Internal::NarrowestPoolSize<Components...>(registry);

            for (EntityID id = 0; id < last; id++)
                if (Internal::HasAll<Components...>(registry, id))
                    func(Entity{ id }, *registry.GetComponent<Components>(id)...);
        }

        template <typename... Components>
        inline u64 Count(Scene& scene)
        {
            static_assert(sizeof...(Components) > 0, "Views::Count - Must filter by at least one component type!");

            u64 count = 0;
            const EntityRegistry& registry = scene.registry;
            const EntityID last = Internal::NarrowestPoolSize<Components...>(registry);
            for (EntityID id = 0; id < last; id++)
                if (Internal::HasAll<Components...>(registry, id))
                    count++;

            return count;
        }
    }
}
