#include "FreeLookCamera.h"
#include <GLFW/glfw3.h>

#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/rotate_vector.hpp>
#include <gtx/vector_angle.hpp>

namespace Nova
{
    static bool firstClick = true;

    FreeLookCamera::FreeLookCamera(const glm::vec3& positionIn, uint32_t widthIn, uint32_t heightIn)
        : position(positionIn), width(widthIn), height(heightIn)
    {
    }

    void FreeLookCamera::HandleInputs(Window* window)
    {
        if (glfwGetKey((GLFWwindow*)window->GetHandle(), GLFW_KEY_W) == GLFW_PRESS)
            position += speed * orientation;

        if (glfwGetKey((GLFWwindow*)window->GetHandle(), GLFW_KEY_S) == GLFW_PRESS)
            position += speed * -orientation;

        if (glfwGetKey((GLFWwindow*)window->GetHandle(), GLFW_KEY_A) == GLFW_PRESS)
            position += speed * -glm::normalize(glm::cross(orientation, up));

        if (glfwGetKey((GLFWwindow*)window->GetHandle(), GLFW_KEY_D) == GLFW_PRESS)
            position += speed * glm::normalize(glm::cross(orientation, up));

        if (glfwGetKey((GLFWwindow*)window->GetHandle(), GLFW_KEY_SPACE) == GLFW_PRESS)
            position += speed * up;

        if (glfwGetMouseButton((GLFWwindow*)window->GetHandle(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            glfwSetInputMode((GLFWwindow*)window->GetHandle(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

            if (firstClick)
            {
                glfwSetCursorPos((GLFWwindow*)window->GetHandle(), (width / 2.f), (height / 2.f));
                firstClick = false;
            }

            double mouseX;
            double mouseY;
            glfwGetCursorPos((GLFWwindow*)window->GetHandle(), &mouseX, &mouseY);

            float rotX = sensitivity * (mouseY - ((float)height / 2)) / height;
            float rotY = sensitivity * (mouseX - ((float)width / 2)) / width;

            glm::vec3 newOrientation =
                glm::rotate(orientation, glm::radians(-rotX), glm::normalize(glm::cross(orientation, up)));

            if (!((glm::angle(newOrientation, up) <= glm::radians(5.f)) ||
                  (glm::angle(newOrientation, -up) <= glm::radians(5.f))))
            {
                orientation = newOrientation;
            }

            orientation = glm::rotate(orientation, glm::radians(-rotY), up);
            glfwSetCursorPos((GLFWwindow*)window->GetHandle(), (width / 2.f), (height / 2.f));
        }
        else if (glfwGetMouseButton((GLFWwindow*)window->GetHandle(), GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
        {
            glfwSetInputMode((GLFWwindow*)window->GetHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            firstClick = true;
        }
    }

    void FreeLookCamera::CalculateViewProjectionMatrix(float fov, float nearPlane, float farPlane, Shader& shader,
                                                       const char* uniformName)
    {
        viewMatrix = glm::lookAt(position, position + orientation, up);
        projectionMatrix = glm::perspective(glm::radians(fov), (float)width / height, nearPlane, farPlane);
        viewProjectionMatrix = projectionMatrix * viewMatrix;

        shader.SetUniformMatrix4fv(uniformName, 1, glm::value_ptr(viewProjectionMatrix));
    }

}
