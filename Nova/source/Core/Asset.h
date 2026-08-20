#pragma once
#include "Core/Base.h"
#include <unordered_map>
#include <filesystem>

namespace Nova
{
    using AssetHandle = u64;
    using AssetRegistry = std::unordered_map<AssetHandle, struct AssetMetadata>;
    using AssetMap = std::unordered_map<AssetHandle, struct Asset*>;

    enum class AssetType : u8
    {
        Invalid = 0,
        AnimationClip,
        AudioClip,
        Material,
        Model,
        ModelAnimated,
        Texture,
        _Length,
    };

    struct Asset
    {
        AssetHandle handle = 0;
        virtual AssetType GetType() const { return AssetType::Invalid; }
    };

    struct AssetMetadata
    {
        string name;
        std::filesystem::path path;
        AssetType type = AssetType::Invalid;
    };

    inline const AssetHandle AssetHandle_Invalid = 0;
}
