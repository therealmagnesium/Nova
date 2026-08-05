#include "Graphics/Camera.h"
#include "Core/Application.h"
#include "Core/Input.h"

#include <SDL3/SDL_mouse.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Nova::Cameras
{
    static constexpr glm::vec3 k_WorldUp = glm::vec3(0.f, 1.f, 0.f);

    void Camera3D_Pan(Camera3D& camera, const glm::vec2& delta, float pan_speed);
    void Camera3D_Orbit(Camera3D& camera, const glm::vec2& delta, float orbit_sensitivity);
    void Camera3D_Zoom(Camera3D& camera, float delta);

    void UpdateEditor(Camera3D& camera, float pan_speed, float orbit_sensitivity)
    {
        const bool is_panning = Input::IsMouseDown(MOUSE_BUTTON_LEFT);
        const bool is_orbiting = Input::IsMouseDown(MOUSE_BUTTON_RIGHT);
        const bool is_zooming = fabsf(Input::GetMouseScroll().y) > 0.f;

        if (is_panning || is_orbiting)
        {
            SDL_HideCursor();

            const glm::vec2 mouse_delta = Input::GetMouseRelative();
            const float distance_to_target = glm::length(camera.target - camera.position);
            const float orbit_pan_scale = distance_to_target * 0.01f;

            /* If both buttons are somehow held at once, orbiting wins - it's the more
             * "global" of the two operations.*/
            if (is_orbiting)
                Camera3D_Orbit(camera, mouse_delta, orbit_sensitivity * orbit_pan_scale);
            else
                Camera3D_Pan(camera, mouse_delta, pan_speed * orbit_pan_scale);
        }

        if (Input::IsMouseReleased(MOUSE_BUTTON_LEFT) || Input::IsMouseReleased(MOUSE_BUTTON_RIGHT))
            SDL_ShowCursor();

        if (is_zooming)
        {
            const float zoom_delta = Input::GetMouseScroll().y;
            Camera3D_Zoom(camera, zoom_delta);
        }
    }

    glm::mat4 GetMatrixView3D(const Camera3D& camera)
    {
        return glm::lookAt(camera.position, camera.target, k_WorldUp);
    }

    glm::mat4 GetMatrixProjection3D(const Camera3D& camera)
    {
        const float aspect_ratio = (float)Application::GetScreenWidth() / (float)Application::GetScreenHeight();
        return glm::perspectiveRH_ZO(glm::radians(camera.fov), aspect_ratio, camera.clip_near, camera.clip_far);
    }

    void Camera3D_Pan(Camera3D& camera, const glm::vec2& delta, float pan_speed)
    {
        const glm::vec3 forward = glm::normalize(camera.target - camera.position);
        const glm::vec3 right = glm::normalize(glm::cross(forward, k_WorldUp));
        const glm::vec3 up = glm::cross(right, forward);

        /* Position and target shift by the SAME offset, sliding the camera sideways/vertically
         * without changing look direction or distance to target - a standard two-axis pan that
         * stays correct regardless of the current orbit angle. */
        const glm::vec3 pan_offset = right * -delta.x * pan_speed + up * delta.y * pan_speed;
        camera.position += pan_offset;
        camera.target += pan_offset;
    }

    void Camera3D_Orbit(Camera3D& camera, const glm::vec2& delta, float orbit_sensitivity)
    {
        const glm::vec3 offset = camera.position - camera.target;
        const glm::vec3 forward = glm::normalize(-offset);
        const glm::vec3 right = glm::normalize(glm::cross(forward, k_WorldUp));

        // Two quaternion rotations are applied to the OFFSET from the target, and position is
        // reconstructed from the rotated offset - so what actually spins is the point the camera
        // orbits around, giving the effect of rotating the entire scene rather than swivelling
        // the view in place from an arbitrary point (the old yaw/pitch free-look behavior).
        const glm::quat yaw_rotation = glm::angleAxis(glm::radians(-delta.x * orbit_sensitivity), k_WorldUp);
        const glm::quat pitch_rotation = glm::angleAxis(glm::radians(-delta.y * orbit_sensitivity), right);
        glm::vec3 new_offset = yaw_rotation * pitch_rotation * offset;

        // Guard against orbiting through the poles: if this frame's pitch would swing the
        // camera's forward vector to within ~5 degrees of straight up/down, glm::lookAt's basis
        // becomes degenerate (forward parallel to WORLD_UP) and the view snaps/flips. Keep this
        // frame's yaw but drop the pitch component when that would happen.
        const float up_alignment = glm::abs(glm::dot(glm::normalize(new_offset), k_WorldUp));
        if (up_alignment > 0.995f)
            new_offset = yaw_rotation * offset;

        camera.position = camera.target + new_offset;
    }

    void Camera3D_Zoom(Camera3D& camera, float delta)
    {
        const float distance_to_target = glm::length(camera.target - camera.position);
        if (delta > 0.f && distance_to_target <= 1.f)
            return;

        const glm::vec3 forward = glm::normalize(camera.target - camera.position);
        camera.position += forward * delta;
    }
}
