#pragma once
#include <glm/glm.hpp>

namespace Nova
{
    struct DirectionalLight
    {
        glm::vec4 color = glm::vec4(0.f);
        glm::vec3 direction = glm::vec3(0.f, 0.f, 0.f);
        float intensity = 0.f;
    };

    inline const DirectionalLight Stub_DirectionalLight;
}
