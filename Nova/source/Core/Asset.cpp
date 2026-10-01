#include "Core/Asset.h"
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
}
