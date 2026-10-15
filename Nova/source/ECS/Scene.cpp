#include "ECS/Scene.h"
#include "ECS/Components.h"
#include "ECS/Entity.h"
#include "ECS/View.h"

#include "Graphics/Renderer.h"
#include "Graphics/Lights.h"

#include "Core/Application.h"
#include "Core/AssetManager.h"
#include "Core/Log.h"
#include "Core/Serialization.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace Nova::Scenes
{
    static Camera3D* primary_editor_camera = NULL;
    static Camera3D* primary_runtime_camera = NULL;
    static Scene* active_scene = NULL;

    void Editor_OnUpdate(Scene& scene);
    void Editor_OnRender(Scene& scene);
    void Runtime_OnUpdate(Scene& scene);
    void Runtime_OnRender(Scene& scene);

    void RenderAllStandardModelsAndPrimitives(Scene& scene);
    void RenderAllAnimatedModels(Scene& scene);

    Scene Create(u64 entity_count_estimate)
    {
        Scene scene;

        // Reserve capacity across all component vectors in the tuple
        const auto ReserveSpaceForComponents = [entity_count_estimate](auto&... component_vectors)
        {
            (component_vectors.reserve(entity_count_estimate), ...);
        };
        std::apply(ReserveSpaceForComponents, scene.registry.pool_components);

        Entity main_camera = CreateEntity(scene, "Main Camera");
        auto& transform_camera = scene.registry.GetComponent<TransformComponent>(main_camera.id);
        auto& cc = scene.registry.AddComponent<PerspectiveCameraComponent>(main_camera.id);
        cc.is_primary = true;
        cc.camera.fov = 75.f;
        cc.camera.clip_near = 0.1f;
        cc.camera.clip_far = 500.f;
        transform_camera.position = glm::vec3(2.f, 5.f, 4.f);

        Entity sun = CreateEntity(scene, "Directional Light");
        auto& transform_sun = scene.registry.GetComponent<TransformComponent>(sun.id);
        const auto& dlc = scene.registry.AddComponent<DirectionalLightComponent>(sun.id, glm::vec4(0.96f, 0.92f, 0.88f, 1.f), 1.f, true);
        transform_sun.rotation = glm::vec3(-0.6f, -0.92f, -0.8f);

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
        scene.entity_count = 0;
    }

    Entity CreateEntity(Scene& scene, const std::string& tag)
    {
        Entity entity;
        entity.id = scene.registry.CreateEntityID();

        scene.registry.AddComponent<InternalComponent>(entity.id, tag, true);
        scene.registry.AddComponent<TransformComponent>(entity.id);

        scene.entity_count++;
        return entity;
    }

    void DestroyEntity(Scene& scene, Entity& entity)
    {
        if (entity.id < 0)
            return;

        scene.pending_removal.push_back(entity.id);
        entity.id = -1;
        scene.entity_count--;
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

    void FlushPendingRemoval(Scene& scene)
    {
        for (const EntityID id : scene.pending_removal)
        {
            // Reset all components for this entity ID using template fold expressions
            const auto ResetEntityComponents = [id](auto&... component_vectors)
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
            scene.registry.free_indices.push_back(id);
        }

        scene.pending_removal.clear();
    }

    void Copy(const Scene& source, Scene& destination)
    {
        if (&source == &destination)
        {
            WARN("%s", "Scenes::Copy - Source and destination are the same scene, skipping!");
            return;
        }

        destination.registry = source.registry;
        destination.pending_removal.clear();
    }

    Scene Import(const std::filesystem::path& path)
    {
        Scene scene;

        try
        {
            const YAML::Node node_root = YAML::LoadFile(path);
            const u64 entity_count = node_root["Entity Count"].as<u64>();

            // Reserve capacity across all component vectors in the tuple
            const auto ReserveSpaceForComponents = [entity_count](auto&... component_vectors)
            {
                (component_vectors.reserve(entity_count), ...);
            };
            std::apply(ReserveSpaceForComponents, scene.registry.pool_components);

            if (node_root["Scene"].IsDefined())
                scene.handle = node_root["Scene"].as<AssetHandle>();

            EntityRegistry& registry = scene.registry;
            std::unordered_map<UUID, EntityID> entities_by_uuid;
            std::vector<std::pair<EntityID, UUID>> pending_camera_targets;

            const YAML::Node node_registry = node_root["Entity Registry"];
            if (node_registry.IsSequence())
            {
                for (const YAML::Node& node_entity : node_registry)
                {
                    const Entity entity = CreateEntity(scene, node_entity["Tag"].as<std::string>());
                    const EntityID id = entity.id;

                    auto& internal = registry.GetComponent<InternalComponent>(id);
                    internal.is_active = node_entity["Is Active?"].as<bool>(true);
                    internal.id = node_entity["UUID"].as<UUID>(internal.id);
                    entities_by_uuid[internal.id] = id;

                    const YAML::Node node_components = node_entity["Components"];
                    if (!node_components.IsMap())
                        continue;

                    // CreateEntity already added the Transform Component, so it only needs to be filled in
                    if (const YAML::Node node = node_components["Transform Component"]; node.IsDefined())
                    {
                        auto& transform = registry.GetComponent<TransformComponent>(id);
                        transform.position = node["Position"].as<glm::vec3>();
                        transform.rotation = node["Rotation"].as<glm::vec3>();
                        transform.scale = node["Scale"].as<glm::vec3>();
                    }

                    if (const YAML::Node node = node_components["Perspective Camera"]; node.IsDefined())
                    {
                        auto& pc = registry.AddComponent<PerspectiveCameraComponent>(id, node["Is Primary?"].as<bool>(false));
                        pc.camera.fov = node["FOV"].as<float>();
                        pc.camera.clip_near = node["Near Clip"].as<float>();
                        pc.camera.clip_far = node["Far Clip"].as<float>();
                        pc.camera.target = node["Target"].as<glm::vec3>();

                        // The target may be serialized after this entity, so resolve it once every entity exists
                        const UUID target_uuid = node["Target Entity"].as<UUID>(Stub_UUID);
                        if (target_uuid != Stub_UUID)
                            pending_camera_targets.emplace_back(id, target_uuid);
                    }

                    if (const YAML::Node node = node_components["Mesh Filter"]; node.IsDefined())
                    {
                        auto& mesh_filter = registry.AddComponent<MeshFilterComponent>(id);
                        mesh_filter.source_type = Meshes::StringViewToMeshSource(node["Source Type"].as<std::string>());
                        mesh_filter.primitive = Meshes::StringViewToPrimitive(node["Primitive"].as<std::string>());
                        mesh_filter.asset_model = node["Model"].as<AssetHandle>(AssetHandle_Invalid);
                    }

                    if (const YAML::Node node = node_components["Mesh Renderer"]; node.IsDefined())
                    {
                        auto& mesh_renderer = registry.AddComponent<MeshRendererComponent>(id);

                        // Each entry is a single-pair map: { "Material <i>": <handle> }
                        const YAML::Node node_materials = node["Materials"];
                        if (node_materials.IsSequence())
                        {
                            for (const YAML::Node& node_material : node_materials)
                                for (auto it = node_material.begin(); it != node_material.end(); it++)
                                    mesh_renderer.material_overrides.emplace_back(it->second.as<AssetHandle>());
                        }
                    }

                    if (const YAML::Node node = node_components["Directional Light"]; node.IsDefined())
                    {
                        registry.AddComponent<DirectionalLightComponent>(id,
                                                                         node["Color"].as<glm::vec4>(),
                                                                         node["Intensity"].as<float>(1.f),
                                                                         node["Is Primary?"].as<bool>(false));
                    }

                    if (const YAML::Node node = node_components["Animator"]; node.IsDefined())
                    {
                        const AssetHandle asset_model = node["Model"].as<AssetHandle>(AssetHandle_Invalid);
                        AssetHandle asset_clip = node["Clip"].as<AssetHandle>(AssetHandle_Invalid);
                        const bool loop = node["Loop"].as<bool>(true);

                        if (!AssetManager::IsHandleValid(asset_model))
                        {
                            // No model assigned (e.g. freshly added in the editor) - restore the empty component as-is
                            registry.AddComponent<AnimatorComponent>(id);
                        }
                        else
                        {
                            const bool has_clip = AssetManager::IsHandleValid(asset_clip);
                            auto& ac = registry.AddComponent<AnimatorComponent>(id, asset_model, has_clip ? &asset_clip : NULL, has_clip ? 1 : 0);

                            // The constructor always starts the first clip looping, so re-issue Play if it wasn't
                            if (has_clip && !loop)
                            {
                                const AnimationClip* clip = AssetManager::GetAsset<AnimationClip>(asset_clip);
                                if (clip != NULL)
                                    Animators::Play(ac.animator, *clip, false);
                            }
                        }
                    }
                }
            }

            // Second pass: hook up camera targets now that every UUID is known
            for (const auto& [id, target_uuid] : pending_camera_targets)
            {
                const auto it = entities_by_uuid.find(target_uuid);
                if (it == entities_by_uuid.end())
                {
                    WARN("Scenes::Import - Camera target entity with UUID %lu was not found in \"%s\"", target_uuid, path.string().c_str());
                    continue;
                }

                registry.GetComponent<PerspectiveCameraComponent>(id).target_entity = Entity{ it->second };
            }
        }
        catch (const YAML::Exception& e)
        {
            ERROR("Scenes::Import - Failed to import scene \"%s\"! (yaml-cpp): %s", path.string().c_str(), e.what());
            return {};
        }

        return scene;
    }

    void Export(const std::filesystem::path& path, Scene& scene)
    {
        EntityRegistry& registry = scene.registry;

        YAML::Emitter out;
        out << YAML::BeginMap;

        out << YAML::Key << "Scene" << YAML::Value << scene.handle;
        out << YAML::Key << "Entity Count" << YAML::Value << scene.entity_count;

        out << YAML::Key << "Entity Registry" << YAML::BeginSeq;
        for (Entity entity : Views::Create<InternalComponent>(scene))
        {
            const EntityID id = entity.id;
            const auto& internal = registry.GetComponent<InternalComponent>(id);
            const auto& transform = registry.GetComponent<TransformComponent>(id);

            out << YAML::BeginMap;
            out << YAML::Key << "Tag" << YAML::Value << internal.tag;
            out << YAML::Key << "Is Active?" << YAML::Value << internal.is_active;
            out << YAML::Key << "UUID" << YAML::Value << internal.id;

            out << YAML::Key << "Components" << YAML::Value << YAML::BeginMap;

            out << YAML::Key << "Transform Component" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Position" << YAML::Value << transform.position;
            out << YAML::Key << "Rotation" << YAML::Value << transform.rotation;
            out << YAML::Key << "Scale" << YAML::Value << transform.scale;
            out << YAML::EndMap;

            if (registry.HasComponent<PerspectiveCameraComponent>(id))
            {
                const auto& pc = registry.GetComponent<PerspectiveCameraComponent>(id);

                UUID target_uuid = Stub_UUID;
                if (pc.target_entity.IsValid() && registry.HasComponent<InternalComponent>(pc.target_entity.id))
                    target_uuid = registry.GetComponent<InternalComponent>(pc.target_entity.id).id;

                out << YAML::Key << "Perspective Camera" << YAML::Value << YAML::BeginMap;
                out << YAML::Key << "Is Primary?" << YAML::Value << pc.is_primary;
                out << YAML::Key << "Target Entity" << YAML::Value << target_uuid;
                out << YAML::Key << "FOV" << YAML::Value << pc.camera.fov;
                out << YAML::Key << "Near Clip" << YAML::Value << pc.camera.clip_near;
                out << YAML::Key << "Far Clip" << YAML::Value << pc.camera.clip_far;
                out << YAML::Key << "Target" << YAML::Value << pc.camera.target;
                out << YAML::EndMap; // Perspective Camera
            }

            if (registry.HasComponent<MeshFilterComponent>(id))
            {
                const auto& mesh_filter = registry.GetComponent<MeshFilterComponent>(id);

                out << YAML::Key << "Mesh Filter" << YAML::Value << YAML::BeginMap;
                out << YAML::Key << "Source Type" << YAML::Value << Meshes::SourceTypeToStringView(mesh_filter.source_type);
                out << YAML::Key << "Primitive" << YAML::Value << Meshes::PrimitiveTypeToStringView(mesh_filter.primitive);
                out << YAML::Key << "Model" << YAML::Value << mesh_filter.asset_model;
                out << YAML::EndMap; // Mesh Filter
            }

            if (registry.HasComponent<MeshRendererComponent>(id))
            {
                const auto& mesh_renderer = registry.GetComponent<MeshRendererComponent>(id);

                out << YAML::Key << "Mesh Renderer" << YAML::Value << YAML::BeginMap;
                out << YAML::Key << "Materials" << YAML::Value << YAML::BeginSeq;
                for (u32 i = 0; i < mesh_renderer.material_overrides.size(); i++)
                {
                    const AssetHandle asset_material = mesh_renderer.material_overrides[i];

                    out << YAML::BeginMap;
                    out << YAML::Key << std::format("Material {}", i) << YAML::Value << asset_material;
                    out << YAML::EndMap;
                }
                out << YAML::EndSeq;
                out << YAML::EndMap; // Mesh Renderer
            }

            if (registry.HasComponent<DirectionalLightComponent>(id))
            {
                const auto& dlc = registry.GetComponent<DirectionalLightComponent>(id);

                // The light's direction is not serialized: Editor_OnUpdate derives it from the
                // entity's transform rotation every frame, so the Transform Component already covers it.
                out << YAML::Key << "Directional Light" << YAML::Value << YAML::BeginMap;
                out << YAML::Key << "Is Primary?" << YAML::Value << dlc.is_primary;
                out << YAML::Key << "Intensity" << YAML::Value << dlc.light.intensity;
                out << YAML::Key << "Color" << YAML::Value << dlc.light.color;
                out << YAML::EndMap; // Directional Light
            }

            if (registry.HasComponent<AnimatorComponent>(id))
            {
                const auto& ac = registry.GetComponent<AnimatorComponent>(id);
                const AnimationTrack& track = ac.animator.track_current;

                // The Animator doesn't keep a list of every bound clip, only what's currently playing,
                // so the playing clip (if any) is what gets saved.
                const bool has_clip = track.binding != NULL && track.binding->clip != NULL;
                const AssetHandle asset_clip = has_clip ? track.binding->clip->handle : AssetHandle_Invalid;

                out << YAML::Key << "Animator" << YAML::Value << YAML::BeginMap;
                out << YAML::Key << "Model" << YAML::Value << ac.asset_model;
                out << YAML::Key << "Clip" << YAML::Value << asset_clip;
                out << YAML::Key << "Loop" << YAML::Value << (has_clip ? track.loop : true);
                out << YAML::EndMap; // Animator
            }

            out << YAML::EndMap; // Components
            out << YAML::EndMap; // Entity
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;

        try
        {
            std::ofstream fout(path);
            fout << out.c_str();
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            ERROR("Scenes::Export - Failed to export scene \"%s\"! (std::filesystem): %s", path.string().c_str(), e.what());
            return;
        }

        INFO("Scene was successfully exported to \"%s\"", path.string().c_str());
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

        for (Entity entity : Views::Create<TransformComponent, DirectionalLightComponent>(scene))
        {
            const auto& internal = entity.GetComponent<InternalComponent>();
            const auto& transform = entity.GetComponent<TransformComponent>();
            auto& dlc = entity.GetComponent<DirectionalLightComponent>();
            dlc.light.direction = transform.rotation;

            if (dlc.is_primary && internal.is_active)
            {
                Renderer::SetSun(dlc.light);
                break;
            }
            else
                Renderer::SetSun(Stub_DirectionalLight);
        }
    }

    void Editor_OnRender(Scene& scene)
    {
        RenderAllStandardModelsAndPrimitives(scene);
        RenderAllAnimatedModels(scene);
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
        RenderAllStandardModelsAndPrimitives(scene);
        RenderAllAnimatedModels(scene);
    }

    void RenderAllStandardModelsAndPrimitives(Scene& scene)
    {
        for (Entity entity : Views::Create<TransformComponent, MeshFilterComponent, MeshRendererComponent>(scene))
        {
            const auto& internal = entity.GetComponent<InternalComponent>();
            if (!internal.is_active)
                continue;

            const auto& transform = entity.GetComponent<TransformComponent>();
            const auto& filter = entity.GetComponent<MeshFilterComponent>();
            const auto& renderer = entity.GetComponent<MeshRendererComponent>();

            switch (filter.source_type)
            {
                case MeshSource::Primitive:
                {
                    const bool check = !renderer.material_overrides.empty() && AssetManager::IsHandleValid(renderer.material_overrides[0]);
                    const Material& material = check ? *AssetManager::GetAsset<Material>(renderer.material_overrides[0]) : Stub_Material;
                    const glm::mat4 transform_matrix = transform.CalculateMatrix();
                    Renderer::DrawPrimitive(filter.primitive, transform_matrix, material);
                    break;
                }

                case MeshSource::Model:
                {
                    if (!AssetManager::IsHandleValid(filter.asset_model))
                        continue;

                    const Model* model = AssetManager::GetAsset<Model>(filter.asset_model);
                    Renderer::DrawModel(*model, transform.position, transform.rotation, transform.scale, renderer.material_overrides);
                    break;
                }

                default:
                    break;
            }
        }
    }

    void RenderAllAnimatedModels(Scene& scene)
    {
        for (Entity entity : Views::Create<TransformComponent, AnimatorComponent>(scene))
        {
            const auto& internal = entity.GetComponent<InternalComponent>();
            if (!internal.is_active)
                continue;

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
