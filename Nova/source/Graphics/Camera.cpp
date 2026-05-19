#include "Graphics/Camera.h"
#include "Core/Application.h"
#include "Core/Input.h"

#include <SDL3/SDL_mouse.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

namespace Nova::Cameras
{
    void Camera3D_Move(Camera3D& camera, const glm::vec3& delta, float move_speed);
    void Camera3D_Rotate(Camera3D& camera, const glm::vec2& delta, float look_sensitivity);

    void UpdateEditor(Camera3D& camera, float move_speed, float look_sensitivity)
    {
        glm::vec3 movement;
        movement.x = Input::GetAxisAlt(InputAxis::Horizontal);
        movement.y = Input::IsKeyDown(KEY_SPACE) - Input::IsKeyDown(KEY_LEFT_CTRL);
        movement.z = Input::GetAxisAlt(InputAxis::Vertical) * -1.f;
        Camera3D_Move(camera, movement, move_speed);

        if (Input::IsKeyDown(KEY_LEFT_ALT))
        {
            const glm::vec2 mouse_delta = Input::GetMouseRelative() * glm::vec2(1.f, -1.f);

            if (Input::IsMouseDown(MOUSE_BUTTON_RIGHT))
            {
                SDL_HideCursor();
                Camera3D_Rotate(camera, mouse_delta, look_sensitivity);
            }
        }

        if (Input::IsMouseReleased(MOUSE_BUTTON_RIGHT))
            SDL_ShowCursor();
    }

    glm::mat4 GetMatrixView3D(const Camera3D& camera)
    {
        glm::vec3 forward;
        forward.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        forward.y = sin(glm::radians(camera.pitch));
        forward.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        forward = glm::normalize(forward);
        return glm::lookAt(camera.position, camera.position + forward, camera.up);
    }

    glm::mat4 GetMatrixProjection3D(const Camera3D& camera)
    {
        const float aspect_ratio = (float)Application::GetScreenWidth() / (float)Application::GetScreenHeight();
        return glm::perspectiveRH_ZO(glm::radians(camera.fov), aspect_ratio,
                                     camera.clip_near, camera.clip_far);
    }

    void Camera3D_Move(Camera3D& camera, const glm::vec3& delta, float move_speed)
    {
        glm::vec3 direction_forward;
        direction_forward.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        direction_forward.y = sin(glm::radians(camera.pitch));
        direction_forward.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        direction_forward = glm::normalize(direction_forward);

        const glm::vec3 direction_right = glm::normalize(glm::cross(direction_forward, camera.up));

        camera.position += direction_right * move_speed * delta.x;
        camera.position += camera.up * move_speed * delta.y;
        camera.position += direction_forward * move_speed * delta.z;
    }

    void Camera3D_Rotate(Camera3D& camera, const glm::vec2& delta, float look_sensitivity)
    {
        const float yaw_sign = camera.up.y < 0 ? -1.0f : 1.0f;
        camera.yaw += yaw_sign * delta.x * look_sensitivity;
        camera.pitch += delta.y * look_sensitivity;
    }
}
