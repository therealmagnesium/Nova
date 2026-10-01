#include "Core/Asset.h"
#include "Core/AssetManager.h"
#include <flat_map>

namespace Nova
{
    std::string_view AssetTypeToStringView(AssetType type)
    {
        switch (type)
        {
            case AssetType::AnimationClip:
                return "Animation CLip";
            case AssetType::AudioClip:
                return "Audio Clip";
            case AssetType::Material:
                return "Material";
            case AssetType::Model:
                return "Standard Model";
            case AssetType::ModelAnimated:
                return "Animated Model";
            case AssetType::Texture:
                return "Texture";
            default:
                return "Invalid";
        }
    }

    AssetType StringViewToAssetType(std::string_view view)
    {
        const local_persist std::flat_map<std::string_view, AssetType> lookup = {
            { "Animation Clip", AssetType::AnimationClip },
            { "Audio Clip", AssetType::AudioClip },
            { "Material", AssetType::Material },
            { "Standard Model", AssetType::Model },
            { "Animated Model", AssetType::ModelAnimated },
            { "Texture", AssetType::Texture },
            { "Invalid", AssetType::Invalid },
        };

        return lookup.contains(view) ? lookup.at(view) : AssetType::Invalid;
    }

    AssetType GuessAssetTypeFromPath(const std::filesystem::path& path)
    {
        local_persist const std::unordered_map<std::filesystem::path, AssetType> k_ExtensionMap = {
            { ".png", AssetType::Texture },
            { ".jpg", AssetType::Texture },
            { ".jpeg", AssetType::Texture },
            { ".hdr", AssetType::Texture },
            { ".wav", AssetType::AudioClip },
            { ".mp3", AssetType::AudioClip },
            { ".ogg", AssetType::AudioClip },
            { ".mat", AssetType::Material },
        };

        const std::filesystem::path extension = path.extension();

        if (extension == ".fbx")
        {
            // Folder convention resolves what the extension alone can't.
            // NOTE: still can't tell Model apart from ModelAnimated this way -
            // that needs to actually inspect the file for a skeleton. Defaulting
            // to Model until that exists.
            for (const auto& segment : path)
            {
                if (segment == "Animations")
                    return AssetType::AnimationClip;
            }
            return AssetType::Model;
        }

        const auto it = k_ExtensionMap.find(extension);
        return it != k_ExtensionMap.end() ? it->second : AssetType::Invalid;
    }

    AssetType PathToAssetType(const std::filesystem::path& path)
    {
        local_persist std::unordered_map<std::filesystem::path, AssetType> path_cache;

        const auto cached = path_cache.find(path);
        if (cached != path_cache.end())
            return cached->second;

        // Trust the registry first - it's authoritative, and it already
        // disambiguates cases (like .fbx) that extension alone can't.
        AssetType type = AssetType::Invalid;
        if (AssetManager::IsAssetRegisteredByPath(path))
        {
            const AssetHandle handle = AssetManager::FindAssetHandleByPath(path);
            type = AssetManager::GetAssetType(handle);
        }

        // Not imported yet - fall back to a best-effort guess.
        if (type == AssetType::Invalid)
            type = GuessAssetTypeFromPath(path);

        path_cache.emplace(path, type);
        return type;
    }
}
