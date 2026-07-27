#include "Graphics/Animator.h"
#include "Core/Log.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Nova::Animators
{
    struct BoneLocalTRS
    {
        glm::vec3 translation = glm::vec3(0.f);
        glm::quat rotation = glm::quat(1.f, 0.f, 0.f, 0.f);
        glm::vec3 scale = glm::vec3(1.f);
    };

    const BoundAnimationClip* ResolveBinding(Animator& animator, const AnimationClip& clip);
    void AdvanceTrackTime(AnimationTrack& track, float delta_time);
    glm::vec3 SampleVec3Keys(const std::vector<Vec3Key>& keys, float time);
    glm::quat SampleQuatKeys(const std::vector<QuatKey>& keys, float time);
    void SampleTrackIntoLocalTRS(const AnimationTrack& track, BoneLocalTRS* out_local_trs, u32 bone_count);
    void ResetToBindPose(const Skeleton& skeleton, BoneLocalTRS* out_local_trs);
    glm::mat4 ResolveBoneWorldTransform(u32 bone_index, const Skeleton& skeleton, const glm::mat4* local_transforms, glm::mat4* world_transforms, bool* resolved, const glm::mat4& root_motion_transform, bool has_root_motion);

    Animator Create(const Skeleton& skeleton)
    {
        Animator animator;
        animator.skeleton = &skeleton;

        for (u32 i = 0; i < MAX_BONES; i++)
            animator.bone_matrices[i] = glm::mat4(1.f);

        return animator;
    }

    void Play(Animator& animator, const AnimationClip& clip, bool loop)
    {
        const BoundAnimationClip* binding = ResolveBinding(animator, clip);
        if (binding == NULL)
            return;

        animator.track_current = (AnimationTrack){.binding = binding, .time = 0.f, .loop = loop};
        animator.track_previous = Stub_AnimationTrack;
        animator.crossfade_time = 0.f;
        animator.crossfade_duration = 0.f;
    }

    void CrossfadeTo(Animator& animator, const AnimationClip& clip, float duration, bool loop)
    {
        // Nothing to blend from yet (or the caller asked for an instant switch) - same as Play().
        if (animator.track_current.binding == NULL || duration <= 0.f)
        {
            Play(animator, clip, loop);
            return;
        }

        const BoundAnimationClip* binding = ResolveBinding(animator, clip);
        if (binding == NULL)
            return;

        animator.track_previous = animator.track_current;
        animator.track_current = (AnimationTrack){.binding = binding, .time = 0.f, .loop = loop};
        animator.crossfade_time = 0.f;
        animator.crossfade_duration = duration;
    }

    void Update(Animator& animator, float delta_time)
    {
        if (!animator.IsValid())
            return;

        const Skeleton& skeleton = *animator.skeleton;
        const u32 bone_count = static_cast<u32>(skeleton.bones.size());

        const bool is_crossfading = animator.crossfade_duration > 0.f && animator.track_previous.binding != NULL;

        if (animator.track_current.binding != NULL)
            AdvanceTrackTime(animator.track_current, delta_time);
        if (is_crossfading)
            AdvanceTrackTime(animator.track_previous, delta_time);

        // Every bone starts at its rest pose; only bones with a channel in the active clip(s) get overwritten.
        // This keeps un-animated helper/prop bones locked to their bind pose instead of collapsing to the origin.
        BoneLocalTRS local_trs[MAX_BONES];
        ResetToBindPose(skeleton, local_trs);

        if (animator.track_current.binding != NULL)
            SampleTrackIntoLocalTRS(animator.track_current, local_trs, bone_count);

        // Root motion (see Skeleton::root_motion_node_name) - most clips don't animate this node
        // at all, in which case has_root_motion is false and behavior is identical to before this
        // feature existed. Sourced from track_current only, even mid-crossfade: blending two
        // different root-motion sources (or one against a non-root-motion clip) is rare enough
        // that a same-frame snap partway through a short crossfade is an acceptable simplification.
        const bool has_root_motion = animator.track_current.binding != NULL && animator.track_current.binding->root_motion_channel_index >= 0;
        glm::mat4 root_motion_transform = glm::mat4(1.f);
        if (has_root_motion)
        {
            const AnimationTrack& track = animator.track_current;
            const BoneChannel& root_motion_channel = track.binding->clip->channels[track.binding->root_motion_channel_index];
            const glm::vec3 root_motion_translation = SampleVec3Keys(root_motion_channel.position_keys, track.time);
            const glm::quat root_motion_rotation = SampleQuatKeys(root_motion_channel.rotation_keys, track.time);
            const glm::vec3 root_motion_scale = SampleVec3Keys(root_motion_channel.scale_keys, track.time);
            root_motion_transform = glm::translate(glm::mat4(1.f), root_motion_translation) *
                                    glm::mat4_cast(root_motion_rotation) *
                                    glm::scale(glm::mat4(1.f), root_motion_scale);
        }

        if (is_crossfading)
        {
            BoneLocalTRS local_trs_previous[MAX_BONES];
            ResetToBindPose(skeleton, local_trs_previous);
            SampleTrackIntoLocalTRS(animator.track_previous, local_trs_previous, bone_count);

            const float blend_factor = glm::clamp(animator.crossfade_time / animator.crossfade_duration, 0.f, 1.f);
            for (u32 i = 0; i < bone_count; i++)
            {
                local_trs[i].translation = glm::mix(local_trs_previous[i].translation, local_trs[i].translation, blend_factor);
                local_trs[i].rotation = glm::slerp(local_trs_previous[i].rotation, local_trs[i].rotation, blend_factor);
                local_trs[i].scale = glm::mix(local_trs_previous[i].scale, local_trs[i].scale, blend_factor);
            }

            animator.crossfade_time += delta_time;
            if (animator.crossfade_time >= animator.crossfade_duration)
            {
                animator.crossfade_time = 0.f;
                animator.crossfade_duration = 0.f;
                animator.track_previous = Stub_AnimationTrack;
            }
        }

        glm::mat4 local_transforms[MAX_BONES];
        for (u32 i = 0; i < bone_count; i++)
        {
            local_transforms[i] = glm::translate(glm::mat4(1.f), local_trs[i].translation) *
                                  glm::mat4_cast(local_trs[i].rotation) *
                                  glm::scale(glm::mat4(1.f), local_trs[i].scale);
        }

        // Bones aren't guaranteed to be stored in parent-before-child order (they're collected in
        // whatever order Assimp's per-mesh bone lists happen to enumerate them), so world transforms
        // are resolved recursively with memoization rather than a single flat forward pass.
        glm::mat4 world_transforms[MAX_BONES];
        bool resolved[MAX_BONES] = {};
        for (u32 i = 0; i < bone_count; i++)
            ResolveBoneWorldTransform(i, skeleton, local_transforms, world_transforms, resolved, root_motion_transform, has_root_motion);

        for (u32 i = 0; i < bone_count && i < MAX_BONES; i++)
            animator.bone_matrices[i] = skeleton.global_inverse_transform * world_transforms[i] * skeleton.bones[i].offset;
    }

    const BoundAnimationClip* ResolveBinding(Animator& animator, const AnimationClip& clip)
    {
        if (!animator.IsValid())
        {
            ERROR("%s", "Animators::Play/CrossfadeTo - Attempted to play a clip on an invalid Animator!");
            return NULL;
        }

        if (!clip.IsValid())
        {
            ERROR("%s", "Animators::Play/CrossfadeTo - Attempted to play an invalid AnimationClip!");
            return NULL;
        }

        const auto it = clip.bindings_by_skeleton.find(animator.skeleton);
        if (it != clip.bindings_by_skeleton.end())
            return &it->second;

        // Not explicitly bound ahead of time - bind it now. Correct either way, but a mid-gameplay
        // resolve is a needless cost you can avoid by calling Animations::Bind once up front.
        WARN("Animators::Play/CrossfadeTo - Clip \"%s\" wasn't bound to this skeleton via Animations::Bind, binding it now.", clip.name.c_str());
        Animations::Bind(clip, *animator.skeleton);
        return &clip.bindings_by_skeleton.at(animator.skeleton);
    }

    void AdvanceTrackTime(AnimationTrack& track, float delta_time)
    {
        const float ticks_per_second = track.binding->clip->ticks_per_second;
        const float duration = track.binding->clip->duration;

        track.time += delta_time * ticks_per_second;
        track.time = track.loop ? fmodf(track.time, duration) : glm::min(track.time, duration);
    }

    void ResetToBindPose(const Skeleton& skeleton, BoneLocalTRS* out_local_trs)
    {
        for (u32 i = 0; i < skeleton.bones.size(); i++)
        {
            out_local_trs[i].translation = skeleton.bones[i].local_bind_translation;
            out_local_trs[i].rotation = skeleton.bones[i].local_bind_rotation;
            out_local_trs[i].scale = skeleton.bones[i].local_bind_scale;
        }
    }

    void SampleTrackIntoLocalTRS(const AnimationTrack& track, BoneLocalTRS* out_local_trs, u32 bone_count)
    {
        const AnimationClip& clip = *track.binding->clip;
        for (u32 c = 0; c < clip.channels.size(); c++)
        {
            const s32 bone_index = track.binding->channel_to_bone_index[c];
            if (bone_index < 0 || static_cast<u32>(bone_index) >= bone_count)
                continue; // Channel targets a bone this skeleton doesn't have - see Animations::Bind

            const BoneChannel& channel = clip.channels[c];
            out_local_trs[bone_index].translation = SampleVec3Keys(channel.position_keys, track.time);
            out_local_trs[bone_index].rotation = SampleQuatKeys(channel.rotation_keys, track.time);
            out_local_trs[bone_index].scale = SampleVec3Keys(channel.scale_keys, track.time);
        }
    }

    glm::vec3 SampleVec3Keys(const std::vector<Vec3Key>& keys, float time)
    {
        if (keys.empty())
            return glm::vec3(0.f);
        if (keys.size() == 1 || time <= keys.front().time)
            return keys.front().value;
        if (time >= keys.back().time)
            return keys.back().value;

        for (u32 i = 0; i + 1 < keys.size(); i++)
        {
            if (time < keys[i + 1].time)
            {
                const float span = keys[i + 1].time - keys[i].time;
                const float t = span > 0.f ? (time - keys[i].time) / span : 0.f;
                return glm::mix(keys[i].value, keys[i + 1].value, t);
            }
        }
        return keys.back().value;
    }

    glm::quat SampleQuatKeys(const std::vector<QuatKey>& keys, float time)
    {
        if (keys.empty())
            return glm::quat(1.f, 0.f, 0.f, 0.f);
        if (keys.size() == 1 || time <= keys.front().time)
            return keys.front().value;
        if (time >= keys.back().time)
            return keys.back().value;

        for (u32 i = 0; i + 1 < keys.size(); i++)
        {
            if (time < keys[i + 1].time)
            {
                const float span = keys[i + 1].time - keys[i].time;
                const float t = span > 0.f ? (time - keys[i].time) / span : 0.f;
                return glm::slerp(keys[i].value, keys[i + 1].value, t);
            }
        }
        return keys.back().value;
    }

    glm::mat4 ResolveBoneWorldTransform(u32 bone_index, const Skeleton& skeleton, const glm::mat4* local_transforms, glm::mat4* world_transforms, bool* resolved, const glm::mat4& root_motion_transform, bool has_root_motion)
    {
        if (resolved[bone_index])
            return world_transforms[bone_index];

        const BoneInfo& bone = skeleton.bones[bone_index];
        const glm::mat4 parent_world = bone.parent_index >= 0
                                           ? ResolveBoneWorldTransform(static_cast<u32>(bone.parent_index), skeleton, local_transforms, world_transforms, resolved, root_motion_transform, has_root_motion)
                                           : glm::mat4(1.f);

        // For a root bone (no bone parent), an active clip's animated root motion REPLACES the
        // node's static parent_offset entirely for this frame - same "channel overrides bind-pose
        // fallback" relationship as any other animated bone, not an extra multiplicative layer on
        // top of it. Falls back to the model's own static parent_offset (e.g. the Blender axis-
        // conversion correction) whenever the active clip doesn't animate this node - see
        // Skeleton::root_motion_node_name.
        const glm::mat4 parent_offset = (bone.parent_index < 0 && has_root_motion) ? root_motion_transform : bone.parent_offset;

        world_transforms[bone_index] = parent_world * parent_offset * local_transforms[bone_index];
        resolved[bone_index] = true;
        return world_transforms[bone_index];
    }
}
