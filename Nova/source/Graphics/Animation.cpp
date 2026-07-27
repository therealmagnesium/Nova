#include "Graphics/Animation.h"
#include "Core/Log.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <SDL3/SDL_filesystem.h>

namespace Nova::Animations
{
    AnimationClip Load(const std::filesystem::path& path)
    {
        AnimationClip clip;

        const std::filesystem::path path_base = SDL_GetBasePath();
        const std::filesystem::path path_full = path_base / path;

        // No post-process flags needed - we only read the animation channels, not mesh/vertex data.
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path_full, 0);

        if (!scene || !scene->HasAnimations())
        {
            ERROR("Animations::Load - Failed to load an animation from \"%s\" and gave an Assimp error: \"%s\"!", path_full.c_str(), importer.GetErrorString());
            return Stub_AnimationClip;
        }

        // Nova loads one clip per file. Files exporting multiple takes should be split upstream (e.g. via a DCC tool or Assimp's own scene splitting).
        const aiAnimation* ai_animation = scene->mAnimations[0];

        clip.name = ai_animation->mName.C_Str();
        clip.duration = static_cast<float>(ai_animation->mDuration);
        clip.ticks_per_second = ai_animation->mTicksPerSecond != 0.0 ? static_cast<float>(ai_animation->mTicksPerSecond) : 25.f;
        clip.channels.reserve(ai_animation->mNumChannels);

        for (u32 i = 0; i < ai_animation->mNumChannels; i++)
        {
            const aiNodeAnim* ai_channel = ai_animation->mChannels[i];

            BoneChannel channel;
            channel.bone_name = ai_channel->mNodeName.C_Str();

            channel.position_keys.reserve(ai_channel->mNumPositionKeys);
            for (u32 k = 0; k < ai_channel->mNumPositionKeys; k++)
            {
                const aiVectorKey& key = ai_channel->mPositionKeys[k];
                channel.position_keys.emplace_back((Vec3Key){glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z), static_cast<float>(key.mTime)});
            }

            channel.rotation_keys.reserve(ai_channel->mNumRotationKeys);
            for (u32 k = 0; k < ai_channel->mNumRotationKeys; k++)
            {
                const aiQuatKey& key = ai_channel->mRotationKeys[k];
                channel.rotation_keys.emplace_back((QuatKey){glm::quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z), static_cast<float>(key.mTime)});
            }

            channel.scale_keys.reserve(ai_channel->mNumScalingKeys);
            for (u32 k = 0; k < ai_channel->mNumScalingKeys; k++)
            {
                const aiVectorKey& key = ai_channel->mScalingKeys[k];
                channel.scale_keys.emplace_back((Vec3Key){glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z), static_cast<float>(key.mTime)});
            }

            clip.channels.emplace_back(std::move(channel));
        }

        INFO("Animation clip \"%s\" loaded successfully from \"%s\" with %ld channels", clip.name.c_str(), path_full.c_str(), clip.channels.size());
        return clip;
    }

    void Unload(AnimationClip& clip)
    {
        clip.channels.clear();
        clip.bindings_by_skeleton.clear();
        clip.name.clear();
        clip.duration = 0.f;
    }

    void Bind(const AnimationClip& clip, const Skeleton& skeleton)
    {
        if (!clip.IsValid() || !skeleton.IsValid())
        {
            ERROR("%s", "Animations::Bind - Attempted to bind an invalid clip or skeleton!");
            return;
        }

        // Idempotent - a clip already bound to this exact skeleton is a cheap no-op rather than
        // a re-resolve, so callers (including Animators::Play's lazy fallback) can call this freely.
        if (clip.bindings_by_skeleton.contains(&skeleton))
            return;

        BoundAnimationClip bound;
        bound.clip = &clip;
        bound.channel_to_bone_index.resize(clip.channels.size(), -1);

        std::string unmatched_names; // Collected for one readable diagnostic line rather than a WARN per bone
        u32 unmatched_channel_count = 0;
        for (u32 i = 0; i < clip.channels.size(); i++)
        {
            const auto it = skeleton.bone_indices_by_name.find(clip.channels[i].bone_name);
            if (it != skeleton.bone_indices_by_name.end())
            {
                bound.channel_to_bone_index[i] = static_cast<s32>(it->second);
                continue;
            }

            // Not a bone, but it might still be meaningful: if this channel's name matches the
            // node directly above the skeleton's root bone(s) - almost always the Blender Armature
            // OBJECT itself - it's a real animated transform (e.g. a rigger turning the whole rig
            // for a big sweeping move) rather than an unused helper node, and gets applied as root
            // motion in Animator::Update instead of being silently discarded.
            if (!skeleton.root_motion_node_name.empty() && clip.channels[i].bone_name == skeleton.root_motion_node_name)
            {
                bound.root_motion_channel_index = static_cast<s32>(i);
                continue;
            }

            unmatched_channel_count++;
            if (!unmatched_names.empty())
                unmatched_names += ", ";
            unmatched_names += clip.channels[i].bone_name;
        }

        // A PARTIAL mismatch is easy to miss visually - an unmatched root/hip bone silently drops
        // all root motion/turning while every other bone keeps animating normally, which looks like
        // "the character doesn't turn" rather than an obvious loading failure. Naming the exact
        // bones that failed makes a mismatch between a model and a separately-exported clip (e.g.
        // two different Mixamo downloads) immediately visible instead of silently swallowed.
        if (unmatched_channel_count == clip.channels.size())
            WARN("Animations::Bind - None of clip \"%s\"'s %ld channels matched a bone in the target skeleton, check bone naming conventions!", clip.name.c_str(), clip.channels.size());
        else if (unmatched_channel_count > 0)
            WARN("Animations::Bind - Clip \"%s\" has %d/%ld channels with no matching bone in the target skeleton: %s", clip.name.c_str(), unmatched_channel_count, clip.channels.size(), unmatched_names.c_str());

        clip.bindings_by_skeleton.emplace(&skeleton, std::move(bound));
    }
}
