#pragma once

#include <stdint.h>
#include <string>
#include <unordered_map>

namespace Nova
{
    struct Shader
    {
        uint32_t rendererID;
        std::unordered_map<std::string, int> uniformLocationCache;

        Shader(const char* path);
        ~Shader();

        void Bind();
        void Unbind();

        uint32_t GetUniformLocation(const char* name);
        void SetUniform4f(const char* name, float v0, float v1, float v2, float v3);
        void SetUniformMatrix4fv(const char* name, size_t count, float* value);
    };
}
