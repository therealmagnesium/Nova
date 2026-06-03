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

layout(location = 0) in vec3 v_position_world;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;

layout(location = 0) out vec4 result;

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------

void main()
{
    const vec3 N = normalize(v_normal);
    const vec3 V = normalize(uniforms.camera_position - v_position_world);
    const vec3 albedo = texture(texture_albedo, v_uv).rgb * uniforms.material.albedo.rgb;
    const vec3 sun_direction = uniforms.light.direction_intensity.xyz;
    const vec3 sun_color = uniforms.light.color.rgb;
    const float sun_intensity = uniforms.light.direction_intensity.w;
    const float metallic = uniforms.material.pbr.x;
    const float roughness = uniforms.material.pbr.y;

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    const vec3 L = normalize(-sun_direction);
    const vec3 H = normalize(V + L);

    // Cook-Torrance BRDF
    const float NDF = DistributionGGX(N, H, roughness);   
    const float G = GeometrySmith(N, V, L, roughness);      
    const vec3 F = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

    const vec3 numerator    = NDF * G * F; 
    const float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    const vec3 specular = numerator / denominator;

    const vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;	  

    const float NdotL = max(dot(N, L), 0.0);        
    const vec3 Lo = (kD * albedo / PI + specular) * sun_color * NdotL;
    const vec3 ambience = vec3(0.03f) * albedo;

    result.rgb = ambience + Lo * sun_intensity;
    result.a = 1.f;
}
