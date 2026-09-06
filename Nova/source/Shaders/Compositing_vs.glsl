#version 460 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;

layout (location = 0) out vec2 v_uv;

void main()
{
    gl_Position = vec4(a_position, 1.f);
    v_uv = a_uv;
}
