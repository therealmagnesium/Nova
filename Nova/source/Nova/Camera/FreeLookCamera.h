#pragma once
#include <Shader.h>
#include <Window.h>
#include <glm.hpp>

namespace Nova
{
    struct FreeLookCamera
    {
        uint32_t width;
        uint32_t height;
        float speed = 0.1f;
        float sensitivity = 100.f;

        glm::vec3 position;
        glm::vec3 orientation = glm::vec3(0.f, 0.f, -1.f);
        glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);

        glm::mat4 viewMatrix = glm::mat4(1.f);
        glm::mat4 projectionMatrix = glm::mat4(1.f);
        glm::mat4 viewProjectionMatrix = glm::mat4(1.f);

        FreeLookCamera(const glm::vec3& positionIn, uint32_t width, uint32_t height);

        void HandleInputs(Window* window);
        void CalculateViewProjectionMatrix(float fov, float nearPlane, float farPlane, Shader& shader,
                                           const char* uniformName);
    };
}
