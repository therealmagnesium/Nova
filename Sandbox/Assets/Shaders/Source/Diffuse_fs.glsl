#version 460 core

layout (location = 0) in vec4 v_color;
layout (location = 1) in vec2 v_uv;
layout (location = 0) out vec4 result;

layout (set = 2, binding = 0) uniform sampler2D texture_albedo;

void main()
{
   result = texture(texture_albedo, v_uv); 
}
