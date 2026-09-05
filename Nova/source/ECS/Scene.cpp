#include "ECS/Scene.h"
#include "ECS/Components.h"
#include "ECS/Entity.h"
#include "ECS/View.h"

#include "Graphics/Renderer.h"

#include "Core/Application.h"
#include "Core/AssetManager.h"
#include "Core/Log.h"

namespace Nova::Scenes
{
    static Camera3D* primary_editor_camera = NULL;
    static Camera3D* primary_runtime_camera = NULL;
    static Scene* active_scene = NULL;

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

        entity.AddComponent<InternalComponent>(tag, true);
        entity.AddComponent<TransformComponent>();

        return entity;
    }

    void DestroyEntity(Scene& scene, Entity& entity)
    {
        if (entity.id < 0)
            return;

        // Reset all components for this entity ID using template fold expressions
        const auto ResetEntityComponents = [id = entity.id](auto&... component_vectors)
        {
            auto reset_component = [id](auto& vec)
            {
                if (id < static_cast<EntityID>(vec.size()) && vec[id].has)
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
    }

    void Play(Scene& scene) { scene.state = SceneState::Runtime; }
    void Stop(Scene& scene)
    {
        scene.state = SceneState::Editor;

        if (primary_editor_camera != NULL)
            Renderer::SetPrimaryCamera(primary_editor_camera);
    }

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

    void Copy(const Scene& source, Scene& destination)
    {
        if (&source == &destination)
        {
            WARN("%s", "Scenes::Copy - Source and destination are the same scene, skipping!");
            return;
        }

        destination.registry = source.registry;
    }

    void Editor_OnUpdate(Scene& scene)
    {
        if (primary_editor_camera == NULL)
            primary_editor_camera = Renderer::GetPrimaryCamera();

        for (Entity entity : Views::Create<TransformComponent, PerspectiveCameraComponent>(scene))
        {
            const auto& transform = entity.GetComponent<TransformComponent>();
            auto& cc = entity.GetComponent<PerspectiveCameraComponent>();
            cc.camera.position = transform.position;
        }
    }

    void Editor_OnRender(Scene& scene)
    {
        // Render all standard models in the scene
        for (Entity entity : Views::Create<TransformComponent, MeshRendererComponent>(scene))
        {
            const auto& transform = entity.GetComponent<TransformComponent>();
            const auto& mesh_renderer = entity.GetComponent<MeshRendererComponent>();

            if (!AssetManager::IsHandleValid(mesh_renderer.asset_model))
                continue;

            const auto model = AssetManager::GetAsset<Model>(mesh_renderer.asset_model);
            Renderer::DrawModel(*model, transform.position, transform.rotation, transform.scale);
        }

        // Render all animated models in the scene
        for (Entity entity : Views::Create<TransformComponent, AnimatorComponent>(scene))
        {
            const auto& transform = entity.GetComponent<TransformComponent>();
            const auto& ac = entity.GetComponent<AnimatorComponent>();

            if (!AssetManager::IsHandleValid(ac.asset_model) || !ac.animator.IsValid())
                continue;

            const auto model = AssetManager::GetAsset<AnimatedModel>(ac.asset_model);
            Renderer::DrawAnimatedModel(*model, ac.animator, transform.position, transform.rotation, transform.scale);
        }
    }

    void Runtime_OnUpdate(Scene& scene)
    {
        // Set every camera's transform to the values from the entity's transform component
        for (Entity entity : Views::Create<TransformComponent, PerspectiveCameraComponent>(scene))
        {
            const auto& transform = entity.GetComponent<TransformComponent>();
            auto& cc = entity.GetComponent<PerspectiveCameraComponent>();

            cc.camera.position = transform.position;

            if (cc.target_entity.IsValid())
                cc.camera.target = cc.target_entity.GetComponent<TransformComponent>().position;

            if (cc.is_primary)
                primary_runtime_camera = &cc.camera;
        }

        Renderer::SetPrimaryCamera(primary_runtime_camera);

        // Update all animated models' animators in the scene
        for (Entity entity : Views::Create<TransformComponent, AnimatorComponent>(scene))
        {
            const auto& transform = entity.GetComponent<TransformComponent>();
            auto& ac = entity.GetComponent<AnimatorComponent>();

            if (!AssetManager::IsHandleValid(ac.asset_model) || !ac.animator.IsValid())
                continue;

            Animators::Update(ac.animator, Application::GetDeltaTime());
        }
    }

    void Runtime_OnRender(Scene& scene)
    {
        // Render all standard models in the scene
        for (Entity entity : Views::Create<TransformComponent, MeshRendererComponent>(scene))
        {
            const auto& transform = entity.GetComponent<TransformComponent>();
            const auto& mesh_renderer = entity.GetComponent<MeshRendererComponent>();

            if (!AssetManager::IsHandleValid(mesh_renderer.asset_model))
                continue;

            const auto model = AssetManager::GetAsset<Model>(mesh_renderer.asset_model);
            Renderer::DrawModel(*model, transform.position, transform.rotation, transform.scale);
        }

        // Render all animated models in the scene
        for (Entity entity : Views::Create<TransformComponent, AnimatorComponent>(scene))
        {
            const auto& transform = entity.GetComponent<TransformComponent>();
            const auto& ac = entity.GetComponent<AnimatorComponent>();

            if (!AssetManager::IsHandleValid(ac.asset_model) || !ac.animator.IsValid())
                continue;

            const auto model = AssetManager::GetAsset<AnimatedModel>(ac.asset_model);
            Renderer::DrawAnimatedModel(*model, ac.animator, transform.position, transform.rotation, transform.scale);
        }
    }

    Scene* GetActive() { return active_scene; }
    void SetActive(Scene& scene) { active_scene = &scene; }
}
