#version 460 core

layout (location = 0) in vec3 a_position;
layout (location = 0) out vec3 v_position_local;

layout (std140, set = 1, binding = 0) uniform MVPData{
    mat4 view_projection;
} mvp;

void main()
{
    v_position_local = a_position;  
    gl_Position =  mvp.view_projection * vec4(v_position_local, 1.0);
}

