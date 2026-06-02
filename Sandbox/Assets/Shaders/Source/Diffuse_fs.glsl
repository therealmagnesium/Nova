#version 460 core

struct LightData
{
    vec4 direction_intensity;
    vec4 color;
};

struct MaterialData
{
    vec4 albedo;
    vec4 pbr; // x = metallic, y = roughness, z = ao, w = idk yet
};

layout(set = 2, binding = 0) uniform sampler2D texture_albedo;
layout(set = 3, binding = 0) uniform UniformData
{
    LightData light;
    MaterialData material;
    vec3 camera_position;
} uniforms;

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 result;

void main()
{
    const vec3 N = normalize(v_normal);
    const vec4 albedo_texel = texture(texture_albedo, v_uv);
    const vec4 albedo_color = albedo_texel * uniforms.material.albedo;
    const float diffuse = max(dot(N, normalize(-uniforms.light.direction_intensity.xyz)), 0.f);
    const vec4 diffuse_color = diffuse * uniforms.light.color * uniforms.light.direction_intensity.w;
    const vec4 ambience_color = vec4(vec3(0.2f), 1.f);
    
    result = albedo_color * (ambience_color + diffuse_color);
}
