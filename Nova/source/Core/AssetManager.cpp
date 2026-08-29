#include "Core/AssetManager.h"
#include "Core/Log.h"
#include "Core/Random.h"
#include "Graphics/Model.h"

#include <inttypes.h>

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

        for (auto& [handle, asset] : assets->loaded_assets)
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

        assets->loaded_assets.clear();
        assets->registry.clear();
    }

    AssetHandle ImportByName(const string& name, AssetType type)
    {
        if (IsAssetRegisteredByName(name))
        {
            const AssetHandle handle = FindAssetHandleByName(name);
            return handle;
        }

        const AssetHandle handle = Random::GenerateUUID();
        ImportByName(name, type, handle);
        return handle;
    }

    AssetHandle ImportByPath(const std::filesystem::path& path, AssetType type)
    {
        if (IsAssetRegisteredByPath(path))
        {
            const AssetHandle handle = FindAssetHandleByPath(path);
            return handle;
        }

        const AssetHandle handle = Random::GenerateUUID();
        ImportByPath(path, type, handle);
        return handle;
    }

    void ImportByName(const string& name, AssetType type, AssetHandle handle)
    {
        AssetMetadata metadata;
        metadata.name = name;
        metadata.type = type;

        if (metadata.type == AssetType::Invalid)
            return;

        const AssetHandle valid_handle = IsAssetRegisteredByName(metadata.name) ? FindAssetHandleByName(metadata.name) : handle;
        Asset* asset = IsAssetRegisteredByName(name) && IsAssetLoaded(valid_handle) ? assets->loaded_assets.at(valid_handle) : LoadAsset(valid_handle, metadata);
        if (asset != NULL)
        {
            assets->registry[valid_handle] = metadata;
            assets->loaded_assets[valid_handle] = asset;
        }
    }

    void ImportByPath(const std::filesystem::path& path, AssetType type, AssetHandle handle)
    {
        AssetMetadata metadata;
        metadata.path = path;
        metadata.name = path.stem().c_str();
        metadata.type = type;

        if (metadata.type == AssetType::Invalid)
            return;

        const AssetHandle valid_handle = IsAssetRegisteredByPath(metadata.path) ? FindAssetHandleByPath(metadata.path) : handle;
        Asset* asset = (IsAssetRegisteredByPath(metadata.path) && IsAssetLoaded(valid_handle)) ? assets->loaded_assets.at(valid_handle) : LoadAsset(valid_handle, metadata);
        if (asset != NULL)
        {
            assets->registry[valid_handle] = metadata;
            assets->loaded_assets[valid_handle] = asset;
        }
    }

    void Remove(AssetHandle handle)
    {
        const std::filesystem::path assetPath = assets->registry.at(handle).path;

        if (AssetManager::IsHandleValid(handle))
        {
            INFO("Removing asset \"%s\"...", assetPath.c_str());
            assets->registry.erase(handle);
            assets->loaded_assets.erase(handle);
        }
    }

    const AssetMap& GetAllAssets() { return assets->loaded_assets; }
    const AssetRegistry& GetRegistry() { return assets->registry; }

    u32 GetTotalAssetCount() { return assets->registry.size() == assets->loaded_assets.size() ? assets->registry.size() : assets->loaded_assets.size(); }

    Asset* GetAsset(AssetHandle handle)
    {
        if (!IsHandleValid(handle))
            return NULL;

        Asset* asset = assets->loaded_assets[handle];
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
        return assets->loaded_assets.find(handle) != assets->loaded_assets.end();
    }

    bool IsAssetRegisteredByName(const string& name)
    {
        auto IsNameRegistered = [&](const std::pair<AssetHandle, AssetMetadata>& pair)
        {
            return pair.second.name == name;
        };
        auto it = std::find_if(assets->registry.begin(), assets->registry.end(), IsNameRegistered);
        return it != assets->registry.end();
    }

    bool IsAssetRegisteredByPath(const std::filesystem::path& path)
    {
        auto IsPathRegistered = [&](const std::pair<AssetHandle, AssetMetadata>& pair)
        {
            return pair.second.path == path;
        };
        auto it = std::find_if(assets->registry.begin(), assets->registry.end(), IsPathRegistered);
        return it != assets->registry.end();
    }

    AssetHandle FindAssetHandleByName(const string& name)
    {
        AssetHandle searchedAssetHandle = AssetHandle_Invalid;
        auto IsNameRegistered = [&](const std::pair<AssetHandle, AssetMetadata>& pair)
        {
            return pair.second.name == name;
        };
        auto it = std::find_if(assets->registry.begin(), assets->registry.end(), IsNameRegistered);

        if (it != assets->registry.end())
            searchedAssetHandle = it->first;

        return searchedAssetHandle;
    }

    AssetHandle FindAssetHandleByPath(const std::filesystem::path& path)
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
