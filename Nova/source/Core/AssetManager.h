#pragma once
#include "Core/Asset.h"
#include <vector>

namespace Nova
{
    struct AssetCollection
    {
        AssetMap loaded_assets;
        AssetRegistry registry;
    };

    namespace AssetManager
    {
        void Init(AssetCollection* collection);
        void Clean();

        AssetHandle ImportByName(const string& name, AssetType type);
        AssetHandle ImportByPath(const std::filesystem::path& path, AssetType type);
        void ImportByName(const string& name, AssetType type, AssetHandle handle);
        void ImportByPath(const std::filesystem::path& path, AssetType type, AssetHandle handle);
        void Remove(AssetHandle handle);

        const AssetMap& GetAllAssets();
        const AssetRegistry& GetRegistry();

        u32 GetTotalAssetCount();
        Asset* GetAsset(AssetHandle handle);
        AssetType GetAssetType(AssetHandle handle);
        std::filesystem::path GetAssetPath(AssetHandle handle);
        std::filesystem::path GetAssetPathAbsolute(AssetHandle handle);
        bool IsAssetTypeRegistered(AssetType type);
        bool IsHandleValid(AssetHandle handle);
        bool IsAssetLoaded(AssetHandle handle);
        bool IsAssetRegisteredByName(const string& name);
        bool IsAssetRegisteredByPath(const std::filesystem::path& path);
        AssetHandle FindAssetHandleByName(const string& name);
        AssetHandle FindAssetHandleByPath(const std::filesystem::path& path);
        std::vector<AssetHandle> GetAllHandlesOfType(AssetType type);

        template <typename T>
        inline T* GetAsset(AssetHandle handle)
        {
            static_assert(std::is_base_of_v<Asset, T>, "AssetManager::GetAsset - T must derive from Asset!");
            Asset* asset = GetAsset(handle);
            return dynamic_cast<T*>(asset);
        }
    }
}
