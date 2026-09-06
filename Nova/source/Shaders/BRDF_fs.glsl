#version 460 core
layout (location = 0) in vec2 v_uv;
layout (location = 0) out vec2 o_result;

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
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
// Optimized version specifically for Z-Up normal vector integration
vec3 ImportanceSampleGGX(vec2 Xi, float roughness)
{
	const float a = roughness * roughness;	
	const float phi = 2.0 * PI * Xi.x;
	const float cos_theta = sqrt(max((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y), 0.0));
	const float sin_theta = sqrt(max(1.0 - cos_theta * cos_theta, 0.0));
	
	// Tangent-space H vector matches world-space directly because N is (0,0,1)
	vec3 H;
	H.x = cos(phi) * sin_theta;
	H.y = sin(phi) * sin_theta;
	H.z = cos_theta;
	
	return H;
}

// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    const float a = roughness;
    const float k = (a * a) / 2.0; // k for IBL geometry updates

    const float nom   = NdotV;
    const float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// ----------------------------------------------------------------------------
float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    const float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    const float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// ----------------------------------------------------------------------------
vec2 IntegrateBRDF(float NdotV, float roughness)
{
    vec3 V;
    V.x = sqrt(max(1.0 - NdotV * NdotV, 0.0));
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0; 

    const uint SAMPLE_COUNT = 1024u;

    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        const vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        const vec3 H  = ImportanceSampleGGX(Xi, roughness);
        const vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        const float NdotL = max(L.z, 0.0);
        const float NdotH = max(H.z, 0.0);
        const float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0)
        {
            const float G = GeometrySmith(NdotV, NdotL, roughness);
            
            // Fixed: Added the missing 4.0 multiplier and protected against NaN/Div0
            const float denominator = 4.0 * NdotH * NdotV;
            if (denominator > 0.0001)
            {
                const float G_Vis = (G * VdotH) / denominator;
                const float Fc = pow(1.0 - VdotH, 5.0);

                A += (1.0 - Fc) * G_Vis;
                B += Fc * G_Vis;
            }
        }
    }
    
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}

// ----------------------------------------------------------------------------
void main() 
{
    // Prevent strict 0.0 boundaries to avoid edge artifacts on low UV resolutions
    const float clamped_uv_x = max(v_uv.x, 0.001);
    vec2 brdf = IntegrateBRDF(clamped_uv_x, v_uv.y);
    o_result = brdf;
}
