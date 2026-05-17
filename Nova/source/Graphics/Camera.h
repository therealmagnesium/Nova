#pragma once
#include <glm/glm.hpp>

namespace Nova
{
    struct Camera3D
    {
        glm::vec3 position;
        glm::vec3 up;
        float yaw = 0.f;
        float pitch = 0.f;
        float fov = 0.f;
        float clip_near = 0.f;
        float clip_far = 0.f;
    };

    namespace Cameras
    {
        void UpdateEditor(Camera3D& camera, float move_speed, float look_sensitivity);

        glm::mat4 GetMatrixView3D(const Camera3D& camera);
        glm::mat4 GetMatrixProjection3D(const Camera3D& camera);
    }
}
