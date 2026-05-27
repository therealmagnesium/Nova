#version 460 core

layout(set = 1, binding = 0) uniform MVP {
    mat4 model;
    mat4 view_projection;
    mat4 normal;
} mvp;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

layout(location = 0) out vec3 v_normal;
layout(location = 1) out vec2 v_uv;

void main()
{
    v_normal = mat3(mvp.normal) * a_normal;
    v_uv = a_uv;

    const vec4 position_local = vec4(a_position, 1.f);
    const vec4 position_world = mvp.model * position_local;

    gl_Position = mvp.view_projection * position_world;
}
