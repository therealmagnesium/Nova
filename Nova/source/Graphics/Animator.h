#pragma once
#include "Graphics/Animation.h"

#include <glm/glm.hpp>

namespace Nova
{
    // INTERNAL - references the pre-resolved binding for whichever clip is currently playing.
    // Callers never construct or read this directly; it's populated by Animators::Play/CrossfadeTo.
    struct AnimationTrack
    {
        const BoundAnimationClip* binding = NULL;
        float time = 0.f; // Playback head, in ticks
        bool loop = true;
    };

    inline const AnimationTrack Stub_AnimationTrack;

    // A single character's runtime animation state. One Animator per animated instance in
    // the scene - two characters playing the same AnimationClip each need their own Animator,
    // since playback time and bone_matrices are per-instance.
    struct Animator
    {
        const Skeleton* skeleton = NULL;

        AnimationTrack track_current;
        AnimationTrack track_previous;  // Only meaningful while crossfade_duration > 0
        float crossfade_time = 0.f;     // Elapsed time into the current crossfade, in seconds
        float crossfade_duration = 0.f; // 0 = not crossfading

        // Final skinning matrices (global_inverse * bone_world * bone_offset), recomputed every
        // Update() call. This is exactly what gets uploaded to the GPU bone matrix SSBO per draw.
        glm::mat4 bone_matrices[MAX_BONES]{glm::mat4(1.f)};

        inline bool IsValid() const { return skeleton != NULL && skeleton->IsValid(); }
    };

    inline const Animator Stub_Animator;

    namespace Animators
    {
        Animator Create(const Skeleton& skeleton);

        // Immediately replaces whatever is currently playing - no blending. Use this for the
        // very first clip an Animator plays, or for hard cuts (e.g. a respawn). If 'clip' hasn't
        // been explicitly bound to this Animator's skeleton via Animations::Bind, it's bound
        // automatically here (with a warning) at the cost of a one-time resolve.
        void Play(Animator& animator, const AnimationClip& clip, bool loop = true);

        // Smoothly blends from whatever is currently playing into 'clip' over 'duration' seconds.
        // Falls back to Play() if nothing is currently playing or duration <= 0. Same auto-bind
        // behavior as Play() applies here.
        void CrossfadeTo(Animator& animator, const AnimationClip& clip, float duration, bool loop = true);

        // Advances playback and recomputes animator.bone_matrices. Call once per animated
        // instance per frame, before any Renderer::DrawAnimatedModel call that uses it.
        void Update(Animator& animator, float delta_time);
    }
}
