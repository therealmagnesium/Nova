#include "Core/AssetManager.h"
#include "Core/Log.h"
#include "Core/Random.h"
#include "Graphics/Model.h"

namespace Nova::AssetManager
{
    static AssetCollection* assets = NULL;
    Asset* LoadAsset(AssetHandle handle, AssetMetadata& metadata);

    void Init(AssetCollection* collection)
    {
        if (assets != NULL)
        {
            WARN("AssetManager::Init - %s", "Cannot initialize the asset manager more than once");
            return;
        }

        assets = collection;
        INFO("AssetManager::Init - %s", "The asset manager was successfully initialized");
    }

    void Clean()
    {
        INFO("AssetManager::Clean - %s", "The asset manager is unloading assets...");

        for (auto& [handle, asset] : assets->loadedAssets)
        {
            switch (asset->GetType())
            {
                case AssetType::Texture:
                    Textures::Unload(dynamic_cast<Texture&>(*asset));
                    break;

                case AssetType::Model:
                    Models::Unload(dynamic_cast<Model&>(*asset));
                    break;

                case AssetType::ModelAnimated:
                    Models::UnloadAnimated(dynamic_cast<AnimatedModel&>(*asset));
                    break;

                default:
                    break;
            }

            delete asset;
            asset = NULL;
        }

        assets->loadedAssets.clear();
        assets->registry.clear();
    }

    AssetHandle Import(const std::filesystem::path& path, AssetType type)
    {
        if (IsAssetRegistered(path))
        {
            const AssetHandle handle = FindAssetHandle(path);
            return handle;
        }

        const AssetHandle handle = Random::GenerateUUID();
        AssetManager::Import(path, type, handle);
        return handle;
    }

    void Import(const std::filesystem::path& path, AssetType type, AssetHandle handle)
    {
        AssetMetadata metadata;
        metadata.path = path;
        metadata.type = type;

        if (metadata.type == AssetType::Invalid)
            return;

        const AssetHandle validHandle = IsAssetRegistered(metadata.path) ? FindAssetHandle(metadata.path) : handle;
        Asset* asset = (IsAssetRegistered(metadata.path) && IsAssetLoaded(validHandle)) ? assets->loadedAssets.at(validHandle) : LoadAsset(validHandle, metadata);
        if (asset != NULL)
        {
            metadata.path = metadata.path;
            assets->registry[validHandle] = metadata;
            assets->loadedAssets[validHandle] = asset;
        }
    }

    void Remove(AssetHandle handle)
    {
        const std::filesystem::path assetPath = assets->registry[handle].path;

        if (AssetManager::IsHandleValid(handle))
        {
            INFO("Removing asset \"%s\"...", assetPath.c_str());
            assets->registry.erase(handle);
            assets->loadedAssets.erase(handle);
        }
    }

    const AssetMap& GetAllAssets() { return assets->loadedAssets; }
    const AssetRegistry& GetRegistry() { return assets->registry; }

    u32 GetTotalAssetCount() { return assets->registry.size() == assets->loadedAssets.size() ? assets->registry.size() : assets->loadedAssets.size(); }

    Asset* GetAsset(AssetHandle handle)
    {
        if (!IsHandleValid(handle))
            return NULL;

        Asset* asset = assets->loadedAssets[handle];
        return asset;
    }

    AssetType GetAssetType(AssetHandle handle)
    {
        AssetType type = AssetType::Invalid;
        Asset* asset = GetAsset(handle);

        if (asset != NULL)
            type = asset->GetType();

        return type;
    }

    std::filesystem::path GetAssetPath(AssetHandle handle) { return assets->registry.at(handle).path; }
    std::filesystem::path GetAssetPathAbsolute(AssetHandle handle)
    {
        const std::filesystem::path path = GetAssetPath(handle);
        return path;
    }

    bool IsAssetTypeRegistered(AssetType type)
    {
        auto HasType = [=](const std::pair<AssetHandle, AssetMetadata>& pair)
        {
            return pair.second.type == type;
        };
        auto it = std::find_if(assets->registry.begin(), assets->registry.end(), HasType);
        return it != assets->registry.end();
    }

    bool IsHandleValid(AssetHandle handle)
    {
        return handle != AssetHandle_Invalid && assets->registry.find(handle) != assets->registry.end();
    }

    bool IsAssetLoaded(AssetHandle handle)
    {
        return assets->loadedAssets.find(handle) != assets->loadedAssets.end();
    }

    bool IsAssetRegistered(const std::filesystem::path& path)
    {
        auto IsPathRegistered = [&](const std::pair<AssetHandle, AssetMetadata>& pair)
        {
            return pair.second.path == path;
        };
        auto it = std::find_if(assets->registry.begin(), assets->registry.end(), IsPathRegistered);
        return it != assets->registry.end();
    }

    AssetHandle FindAssetHandle(const std::filesystem::path& path)
    {
        AssetHandle searchedAssetHandle = AssetHandle_Invalid;
        auto IsPathRegistered = [&](const std::pair<AssetHandle, AssetMetadata>& pair)
        {
            return pair.second.path == path;
        };
        auto it = std::find_if(assets->registry.begin(), assets->registry.end(), IsPathRegistered);

        if (it != assets->registry.end())
            searchedAssetHandle = it->first;

        return searchedAssetHandle;
    }

    std::vector<AssetHandle> GetAllHandlesOfType(AssetType type)
    {
        std::vector<AssetHandle> handles;
        handles.reserve(assets->registry.size());

        for (const auto& [handle, metadata] : assets->registry)
            if (metadata.type == type)
                handles.emplace_back(handle);

        return handles;
    }

    Asset* LoadAsset(AssetHandle handle, AssetMetadata& metadata)
    {
        Asset* asset = NULL;

        switch (metadata.type)
        {
            case AssetType::AnimationClip:
            {
                AnimationClip animation = Animations::Load(metadata.path);
                if (animation.IsValid())
                {
                    asset = new AnimationClip(std::move(animation));
                    asset->handle = handle;
                }
                break;
            }

            case AssetType::Model:
            {
                Model model = Models::Load(metadata.path);
                if (model.meshes.size() > 0)
                {
                    asset = new Model(std::move(model));
                    asset->handle = handle;
                }
                break;
            }

            case AssetType::ModelAnimated:
            {
                AnimatedModel model = Models::LoadAnimated(metadata.path);
                if (model.meshes.size() > 0)
                {
                    asset = new AnimatedModel(std::move(model));
                    asset->handle = handle;
                }
                break;
            }

            case AssetType::Texture:
            {
                Texture texture = Textures::Load(metadata.path);
                if (texture.IsValid())
                {
                    asset = new Texture(std::move(texture));
                    asset->handle = handle;
                }
                break;
            }

            default:
                break;
        }

        return asset;
    }
}
