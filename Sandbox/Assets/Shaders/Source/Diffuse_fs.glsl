#version 460 core

layout(set = 2, binding = 0) uniform sampler2D texture_albedo;
layout(set = 3, binding = 0) uniform MaterialData
{
    vec4 albedo;
    float metallic;
    float roughness;
} material_data;

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 result;

void main()
{
    const vec3 N = normalize(v_normal);
    const vec3 sun_direction = vec3(-0.4f, -1.f, -0.8f);
    const vec3 sun_color = vec3(1.f, 0.92f, 0.86f);
    const vec4 albedo_texel = texture(texture_albedo, v_uv);
    const vec4 albedo_color = albedo_texel * material_data.albedo;
    const float diffuse = max(dot(N, normalize(-sun_direction)), 0.f);
    const vec4 diffuse_color = diffuse * vec4(sun_color, 1.f);
    const float ambience = 0.2f;
    
    result = albedo_color * (vec4(vec3(ambience), 1.f) + diffuse_color);
}
