#pragma once
#include "Core/Asset.h"
#include "Core/Base.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace Nova
{
    inline constexpr u32 MAX_BONE_INFLUENCE = 4; // Bone influences per vertex (bone_ids/bone_weights are vec4)
    inline constexpr u32 MAX_BONES = 128;        // Upper bound on bones per skeleton - must match MAX_BONES in SkinnedPBR_vs.glsl

    // A single bone in a skeleton's hierarchy. Skeletons are built once at model-load time
    // from Assimp's bone + node data and are immutable afterward.
    struct BoneInfo
    {
        std::string name;
        glm::mat4 offset = glm::mat4(1.f); // Inverse bind-pose matrix: mesh space -> bone-local space
        glm::mat4 parent_offset = glm::mat4(1.f);

        // Rest-pose local transform (relative to parent), decomposed once at load time.
        // Used as a fallback for bones that a given AnimationClip doesn't animate.
        glm::vec3 local_bind_translation = glm::vec3(0.f);
        glm::quat local_bind_rotation = glm::quat(1.f, 0.f, 0.f, 0.f);
        glm::vec3 local_bind_scale = glm::vec3(1.f);

        s32 parent_index = -1; // Index into Skeleton::bones, -1 for a root bone
    };

    // The full bone hierarchy of an AnimatedModel. One Skeleton is loaded per model file;
    // multiple AnimationClips (even ones authored against a differently-named rig) can be
    // bound against it independently via Animations::Bind.
    struct Skeleton
    {
        std::vector<BoneInfo> bones;
        std::unordered_map<std::string, u32> bone_indices_by_name;
        glm::mat4 global_inverse_transform = glm::mat4(1.f); // Inverse of the scene root node's transform

        // Name of the node directly ABOVE the skeleton's root bone(s) - in a Blender export this
        // is almost always the Armature OBJECT node itself (as opposed to any individual bone).
        // It's entirely possible to animate THAT node directly (e.g. a rigger rotating the whole
        // armature object for a big sweeping turn, rather than posing an internal root bone) - if
        // an AnimationClip has a channel with this exact name, Animations::Bind treats it as ROOT
        // MOTION and Animator::Update applies it as an extra, ANIMATED transform on top of the
        // entire skeleton every frame, instead of silently discarding it as an unmatched channel.
        std::string root_motion_node_name;

        inline bool IsValid() const { return !bones.empty(); }
    };

    inline const Skeleton Stub_Skeleton;

    struct Vec3Key
    {
        glm::vec3 value = glm::vec3(0.f);
        float time = 0.f; // In ticks, matching AnimationClip::ticks_per_second
    };

    struct QuatKey
    {
        glm::quat value = glm::quat(1.f, 0.f, 0.f, 0.f);
        float time = 0.f;
    };

    // Per-bone keyframe data, keyed by bone NAME rather than skeleton index. This is what
    // makes an AnimationClip reusable: the same walk cycle exported from Mixamo can be bound
    // to any skeleton that shares its bone naming convention, regardless of index order.
    struct BoneChannel
    {
        std::string bone_name;
        std::vector<Vec3Key> position_keys;
        std::vector<QuatKey> rotation_keys;
        std::vector<Vec3Key> scale_keys;
    };

    // INTERNAL - the result of resolving an AnimationClip's bone-name channels against a specific
    // Skeleton. Callers never touch this type directly: Animations::Bind produces and caches it
    // on the owning AnimationClip, and Animators::Play/CrossfadeTo look it up by skeleton behind
    // the scenes. It exists as its own type purely so a clip can cache resolutions for several
    // skeletons at once without re-walking bone names on every Play() call.
    struct BoundAnimationClip
    {
        const struct AnimationClip* clip = NULL;
        std::vector<s32> channel_to_bone_index; // Parallel to clip->channels; -1 if the skeleton has no matching bone
        s32 root_motion_channel_index = -1;     // Index into clip->channels matching Skeleton::root_motion_node_name; -1 if this clip has none
    };

    // A reusable animation asset, independent of any specific skeleton.
    struct AnimationClip : public Asset
    {
        std::string name;
        float duration = 0.f; // In ticks
        float ticks_per_second = 25.f;
        std::vector<BoneChannel> channels;

        // Cache of per-skeleton bindings populated by Animations::Bind, keyed by Skeleton
        // identity. Mutable so Bind can be called through a const AnimationClip& - populating
        // this cache doesn't change the clip's actual animation data, just memoizes a derived
        // lookup. Note this means moving/reallocating a Skeleton (e.g. an AnimatedModel stored
        // in a resizing std::vector) invalidates cached entries keyed to its old address; a
        // stale entry is simply orphaned and Animators::Play/CrossfadeTo transparently re-binds
        // against the new address, at the cost of one extra resolve.
        mutable std::unordered_map<const Skeleton*, BoundAnimationClip> bindings_by_skeleton;

        inline bool IsValid() const { return !channels.empty() && duration > 0.f; }
    };

    inline const AnimationClip Stub_AnimationClip;

    namespace Animations
    {
        /**
         * @brief Loads the first animation found in a model file as a standalone, reusable clip.
         * @param path The path of the model/animation file (fbx, glTF, etc.) containing the clip*/
        AnimationClip Load(const std::filesystem::path& path);
        void Unload(AnimationClip& clip);

        /**
         * @brief Resolves a clip's bone-name channels against a specific skeleton and caches the
         * result on the clip, ready for Animators::Play/CrossfadeTo. Idempotent - safe to call
         * more than once for the same clip+skeleton pair. Not strictly required before playing a
         * clip (Play/CrossfadeTo bind on demand if you forget), but calling it once up front - e.g.
         * right after loading a model and its clips - keeps that one-time resolve cost off the hot path.
         * @param clip The reusable clip to bind
         * @param skeleton The skeleton to resolve the clip's bone-name channels against*/
        void Bind(const AnimationClip& clip, const Skeleton& skeleton);
    }
}
