#include "OrthographicCamera.h"
#include <gtc/matrix_transform.hpp>

namespace Nova
{

    OrthographicCamera::OrthographicCamera(float left, float right, float top, float bottom)
        : projectionMatrix(glm::ortho(left, right, top, bottom, -1.f, 1.f)), viewMatrix(1.f)
    {
        viewProjectionMatrix = projectionMatrix * viewMatrix;
    }

    void OrthographicCamera::RecalculateViewMatrix()
    {
        glm::mat4 transform =
            glm::translate(glm::mat4(1.f), position) * glm::rotate(glm::mat4(1.f), rotation, glm::vec3(0.f, 0.f, 1.f));

        viewMatrix = glm::inverse(transform);
        viewProjectionMatrix = projectionMatrix * viewMatrix;
    }
}
