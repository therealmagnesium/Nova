#pragma once
#include <glm/glm.hpp>

namespace Nova
{
    struct DirectionalLight
    {
        glm::vec4 color = glm::vec4(1.f);
        glm::vec3 direction = glm::vec3(0.f, -1.f, 0.f);
        float intensity = 1.f;
    };

    inline const DirectionalLight Stub_DirectionalLight;
}
