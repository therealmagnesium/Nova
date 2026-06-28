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

layout (set = 2, binding = 0) uniform sampler2D texture_albedo;
layout (set = 2, binding = 1) uniform sampler2D texture_normal;
layout (set = 2, binding = 2) uniform sampler2D texture_metallic;
layout (set = 2, binding = 3) uniform sampler2D texture_roughness;
layout (set = 2, binding = 4) uniform samplerCube map_irradiance;
layout (set = 2, binding = 5) uniform samplerCube map_prefilter;
layout (set = 2, binding = 6) uniform sampler2D map_brdf_lut;

layout (std140, set = 3, binding = 0) uniform UniformData
{
    LightData light;
    MaterialData material;
    vec3 camera_position;
} uniforms;

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec2 v_uv;
layout (location = 3) in vec3 v_tangent;

layout (location = 0) out vec4 result;

const float PI = 3.14159265359f;

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
vec4 FresnelSchlick(float cosTheta, vec4 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
vec4 FresnelSchlickRoughness(float cosTheta, vec4 F0, float roughness)
{
    return F0 + (max(vec4(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
vec4 CalculateDirectLighting(vec4 F0, vec3 N, vec3 V, vec4 albedo, float metallic, float roughness)
{
    const vec3 sun_direction = uniforms.light.direction_intensity.xyz;
    const vec3 sun_color = uniforms.light.color.rgb;
    const float sun_intensity = uniforms.light.direction_intensity.w;

    const vec3 L = normalize(-sun_direction);
    const vec3 H = normalize(V + L);

    // Cook-Torrance BRDF
    const float NDF = DistributionGGX(N, H, roughness);   
    const float G = GeometrySmith(N, V, L, roughness);      
    const vec4 F = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

    const vec4 numerator    = NDF * G * F; 
    const float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    const vec4 specular = numerator / denominator;

    const vec4 kS = F;
    vec4 kD = vec4(1.0) - kS;
    kD *= 1.0 - metallic;	  

    const float NdotL = max(dot(N, L), 0.0);        
    const vec4 Lo = (kD * albedo / PI + specular) * vec4(sun_color, 1.f) * NdotL;
    return Lo * sun_intensity;
}

vec4 CalculateIndirectLighting(vec4 F0, vec3 N, vec3 V, vec3 R, vec4 albedo, float metallic, float roughness)
{
    const vec4 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness); 
    const vec4 kD = (vec4(1.0) - F) * (1.0 - metallic); 
    const vec3 irradiance = texture(map_irradiance, N).rgb;
    const vec4 diffuse = vec4(irradiance, 1.f) * albedo;
    
    // Sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    const vec3 prefiltered_color = textureLod(map_prefilter, R, roughness * MAX_REFLECTION_LOD).rgb;    
    const vec2 brdf = texture(map_brdf_lut, vec2(max(dot(N, V), 0.0), roughness)).rg;
    const vec4 specular = vec4(prefiltered_color, 1.f) * (F * brdf.x + brdf.y);

    return kD * diffuse + specular; 
}

void main()
{
    const vec3 N = normalize(v_normal);
    const vec3 T = normalize(v_tangent);
    const vec3 B = cross(N, T);
    const mat3 TBN = mat3(T, B, N);

    const vec4 albedo = texture(texture_albedo, v_uv) * uniforms.material.albedo;
    const vec3 normal_tangent = texture(texture_normal, v_uv).rgb * 2.f - 1.f;
    const vec3 normal_world = normalize(TBN * normal_tangent);

    const vec3 V = normalize(uniforms.camera_position - v_position);
    const vec3 R = reflect(-V, normal_world);

    const float metallic = texture(texture_metallic, v_uv).r * uniforms.material.pbr.x;
    const float roughness = texture(texture_roughness, v_uv).r * uniforms.material.pbr.y;

    vec4 F0 = vec4(vec3(0.04), 1.f); 
    F0 = mix(F0, albedo, metallic);

    const vec4 Lo = CalculateDirectLighting(F0, normal_world, V, albedo, metallic, roughness);
    const vec4 ambience = CalculateIndirectLighting(F0, normal_world, V, R, albedo, metallic, roughness);
    result = ambience + Lo;
}
