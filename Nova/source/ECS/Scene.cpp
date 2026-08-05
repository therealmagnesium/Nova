#include "ECS/Scene.h"
#include "ECS/Entity.h"

namespace Nova::Scenes
{
    void Editor_OnUpdate(Scene& scene);
    void Editor_OnRender(Scene& scene);
    void Runtime_OnUpdate(Scene& scene);
    void Runtime_OnRender(Scene& scene);

    Scene Create(u64 entity_count_estimate)
    {
        Scene scene;

        // Reserve capacity across all component vectors in the tuple
        const auto ReserveSpaceForComponents = [entity_count_estimate](auto&... component_vectors)
        {
            (component_vectors.reserve(entity_count_estimate), ...);
        };
        std::apply(ReserveSpaceForComponents, scene.registry.pool_components);

        return scene;
    }

    void Destroy(Scene& scene)
    {
        // Clear all component vectors in the tuple
        const auto ClearAllComponents = [](auto&... component_vectors)
        {
            (component_vectors.clear(), ...);
        };
        std::apply(ClearAllComponents, scene.registry.pool_components);

        scene.registry.free_indices.clear();
        scene.registry.next_available_index = 0;
    }

    Entity CreateEntity(Scene& scene, const string& tag)
    {
        Entity entity;
        entity.id = scene.registry.CreateEntityID();
        entity.context = &scene;

        entity.AddComponent<InternalComponent>(tag, true);
        entity.AddComponent<TransformComponent>();

        return entity;
    }

    void DestroyEntity(Scene& scene, Entity& entity)
    {
        if (entity.id < 0 || entity.context == nullptr)
            return;

        // Reset all components for this entity ID using template fold expressions
        const auto ResetEntityComponents = [id = entity.id](auto&... component_vectors)
        {
            auto reset_component = [id](auto& vec)
            {
                if (id < static_cast<EntityID>(vec.size()))
                {
                    using ComponentType = typename std::decay_t<decltype(vec)>::value_type;
                    vec[id] = ComponentType(); // Reset to default constructed state
                }
            };
            (reset_component(component_vectors), ...);
        };
        std::apply(ResetEntityComponents, scene.registry.pool_components);

        // Recycle the entity ID so it can be reused by CreateEntity
        scene.registry.free_indices.push_back(entity.id);

        // Invalidate the entity instance passed in
        entity.id = -1;
        entity.context = NULL;
    }

    void Play(Scene& scene) { scene.state = SceneState::Runtime; }
    void Stop(Scene& scene) { scene.state = SceneState::Editor; }

    void UpdateSubsystems(Scene& scene)
    {
        switch (scene.state)
        {
            case SceneState::Editor:
                Editor_OnUpdate(scene);
                break;
            case SceneState::Runtime:
                Runtime_OnUpdate(scene);
                break;
            default:
                break;
        }
    }

    void RenderSubsystems(Scene& scene)
    {
        switch (scene.state)
        {
            case SceneState::Editor:
                Editor_OnRender(scene);
                break;
            case SceneState::Runtime:
                Runtime_OnRender(scene);
                break;
            default:
                break;
        }
    }

    void Editor_OnUpdate(Scene& scene) {}
    void Editor_OnRender(Scene& scene) {}

    void Runtime_OnUpdate(Scene& scene) {}
    void Runtime_OnRender(Scene& scene) {}

}
