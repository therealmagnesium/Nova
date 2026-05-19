#version 460 core

layout(set = 2, binding = 0) uniform sampler2D texture_albedo;
layout(set = 3, binding = 0) uniform MaterialData
{
    vec4 albedo;
    float metallic;
    float roughness;
} material_data;

layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 result;

void main()
{
    vec4 albedo_texel = texture(texture_albedo, v_uv);
    result = albedo_texel * material_data.albedo * v_color;
}
