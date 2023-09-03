#include "Shader.h"
#include "Util.h"

#include <GLFW/glfw3.h>
#include <fstream>
#include <glad/gl.h>
#include <glm.hpp>
#include <sstream>

namespace Nova
{
    struct ShaderProgramSource
    {
        std::string vertSource;
        std::string fragSource;
    };

    static uint32_t CompileShader(uint32_t type, const std::string& source)
    {
        uint32_t id = glCreateShader(type);
        const char* cstrSource = source.c_str();
        glShaderSource(id, 1, &cstrSource, NULL);
        glCompileShader(id);

        int result = 0;
        glGetShaderiv(id, GL_COMPILE_STATUS, &result);
        if (result == GL_FALSE)
        {
            int length = 0;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
            char* message = (char*)alloca(length * sizeof(char));
            glGetShaderInfoLog(id, length, &length, message);
            printf("%s\n", message);

            glDeleteShader(id);
            return 0;
        }

        return id;
    }

    static uint32_t CreateShader(const std::string& vertSource, const std::string& fragSource)
    {
        uint32_t program = glCreateProgram();
        uint32_t vs = CompileShader(GL_VERTEX_SHADER, vertSource);
        uint32_t fs = CompileShader(GL_FRAGMENT_SHADER, fragSource);

        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        glValidateProgram(program);

        glDeleteShader(vs);
        glDeleteShader(fs);

        return program;
    }

    static ShaderProgramSource ParseShader(const char* path)
    {
        std::ifstream stream(path);
        if (!stream.good())
        {
            ERROR_RETURN({}, "Failed to open %s!\n", path);
        }

        enum class ShaderType
        {
            NONE = -1,
            VERTEX = 0,
            FRAGMENT = 1
        };

        std::string line;
        std::stringstream ss[2];
        ShaderType type = ShaderType::NONE;

        while (std::getline(stream, line))
        {
            if (line.find("//!type") != std::string::npos)
            {
                if (line.find("vertex") != std::string::npos)
                    type = ShaderType::VERTEX;
                else if (line.find("fragment") != std::string::npos)
                    type = ShaderType::FRAGMENT;
            }
            else
            {
                ss[(int)type] << line << "\n";
            }
        }

        return (ShaderProgramSource){ss[0].str(), ss[1].str()};
    }

    Shader::Shader(const char* path)
    {
        ShaderProgramSource source = ParseShader(path);
        rendererID = CreateShader(source.vertSource, source.fragSource);
        printf("%s\n%s\n", source.vertSource.c_str(), source.fragSource.c_str());
    }

    Shader::~Shader()
    {
        glDeleteProgram(rendererID);
    }

    void Shader::Bind()
    {
        glUseProgram(rendererID);
    }

    void Shader::Unbind()
    {
        glUseProgram(0);
    }

    uint32_t Shader::GetUniformLocation(const char* name)
    {
        if (uniformLocationCache.find(std::string(name)) != uniformLocationCache.end())
            return uniformLocationCache[name];

        uint32_t location = glGetUniformLocation(rendererID, name);
        if (location == -1)
            printf("Warning: Uniform %s does not exist!\n", name);

        uniformLocationCache[name] = location;
        return location;
    }

    void Shader::SetUniform4f(const char* name, float v0, float v1, float v2, float v3)
    {
        glUniform4f(GetUniformLocation(name), v0, v1, v2, v3);
    }

    void Shader::SetUniformMatrix4fv(const char* name, size_t count, float* value)
    {
        glUniformMatrix4fv(GetUniformLocation(name), count, GL_FALSE, value);
    }
}
