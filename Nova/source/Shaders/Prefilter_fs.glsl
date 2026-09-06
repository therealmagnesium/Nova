#version 460 core
layout (location = 0) in vec3 v_position_local;
layout (location = 0) out vec4 o_result;

layout (set = 2, binding = 0) uniform samplerCube map_environment;

layout (std140, set = 3, binding = 0) uniform Uniforms
{
    float roughness;
} uniforms;

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    const float a = roughness * roughness;
    const float a2 = a * a;
    const float NdotH = max(dot(N, H), 0.0);
    const float NdotH2 = NdotH * NdotH;

    const float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

// ----------------------------------------------------------------------------
float RadicalInverse_VdC(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; 
}

// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

// ----------------------------------------------------------------------------
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	const float a = roughness * roughness;	
	const float phi = 2.0 * PI * Xi.x;
	const float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	const float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	const vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	const vec3 tangent   = normalize(cross(up, N));
	const vec3 bitangent = cross(N, tangent);
	
	const vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

// ----------------------------------------------------------------------------
void main()
{		
    const vec3 N = normalize(v_position_local);
    const vec3 R = N;
    const vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;
    
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        const vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        const vec3 H = ImportanceSampleGGX(Xi, N, uniforms.roughness);
        const vec3 L = normalize(2.0 * dot(V, H) * H - V);

        const float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            const float D     = DistributionGGX(N, H, uniforms.roughness);
            const float NdotH = max(dot(N, H), 0.0);
            const float HdotV = max(dot(H, V), 0.0);
            const float pdf   = D * NdotH / (4.0 * HdotV) + 0.0001; 

            const float resolution = 512.0; 
            const float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
            const float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            // Added max() clamp to prevent negative mip levels
            const float mipLevel = uniforms.roughness == 0.0 ? 0.0 : max(0.5 * log2(saSample / saTexel), 0.0); 
            
            prefilteredColor += textureLod(map_environment, L, mipLevel).rgb * NdotL;
            totalWeight      += NdotL;
        }
    }

    // Safeguarded against division by zero
    prefilteredColor = totalWeight > 0.0 ? prefilteredColor / totalWeight : prefilteredColor;
    o_result = vec4(prefilteredColor, 1.0);
}
