#pragma once
#include <glm.hpp>

namespace Nova
{

    struct OrthographicCamera
    {
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;
        glm::mat4 viewProjectionMatrix;

        glm::vec3 position = {0.f, 0.f, 0.f};
        float rotation = 0.f;

        OrthographicCamera(float left, float right, float top, float bottom);

        void RecalculateViewMatrix();
    };

}
