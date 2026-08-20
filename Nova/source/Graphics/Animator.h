#pragma once
#include "Graphics/Animation.h"
#include "Core/Asset.h"

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
    struct Animator : public Asset
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
        Animator Create();
        Animator Create(const Skeleton& skeleton);

        /** @brief Immediately replaces whatever is currently playing - no blending
         * Use this for the very first clip an Animator plays, or for hard cuts
         * If 'clip' hasn't been explicitly bound to this Animator's skeleton via Animations::Bind, it's bound
         * automatically here (with a warning) at the cost of a one-time resolve
         * @param [in] animator The animator to play clips from
         * @param [in] clip The clip to play (must be bound to the skeleton the animator was crated with, look at Animations::Bind)
         * @param [in] loop The animator to play clips from
         * */
        void Play(Animator& animator, const AnimationClip& clip, bool loop = true);

        /** @brief Smoothly blends from whatever is currently playing into [clip] over [duration] seconds
         * Falls back to Play() if nothing is currently playing or duration <= 0
         * Same auto-bind behavior as Play() applies here
         * @param [in] animator The animator used to crossfade from one clip to another
         * @param [in] clip The clip to be transitioned to
         * @param [in] duration How long (in seconds) it takes to transition to [clip]
         * @param [in] loop Should [clip] loop once the transition is complete?
         * */
        void CrossfadeTo(Animator& animator, const AnimationClip& clip, float duration, bool loop = true);

        // Advances playback and recomputes animator.bone_matrices. Call once per animated
        // instance per frame, before any Renderer::DrawAnimatedModel call that uses it.
        void Update(Animator& animator, float delta_time);

        /** @brief Immediately snaps the animator back to the skeleton's rest pose
         * The skeleton's rest pose could classify as a T-pose/A-pose, or whatever the model was authored in
         * Clears any in-progress playback/crossfade
         * Use this anytime an animated instance needs a clean rest pose outside of active gameplay
         * Nova calls this automatically when a scene transitions out of Runtime
         * (see Scenes::Stop) so a character doesn't keep rendering mid-animation while editing.
         * @param [in] animator The animator which contains a pointer to the skeleton to reset */
        void ResetToBindPose(Animator& animator);
    }
}
