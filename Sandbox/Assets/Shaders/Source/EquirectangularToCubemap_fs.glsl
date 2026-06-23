#version 460 core
layout (location = 0) noperspective in vec3 v_position_local;
layout (location = 0) out vec4 o_result;

layout (set = 2, binding = 0) uniform sampler2D map_equirectangular;

vec2 SampleSphericalMap(vec3 v)
{
    const float PI = 3.14159265359;

    v = normalize(v);

    float phi = atan(v.z, v.x);
    float theta = acos(clamp(v.y,-1.0,1.0));

    return vec2(
        phi / (2.0 * PI) + 0.5,
        1.f - theta/PI
    );
}

void main()
{		
    const vec2 uv = SampleSphericalMap(normalize(v_position_local));
    const vec3 color = textureLod(map_equirectangular, uv, 0.f).rgb; 
    o_result = vec4(color, 1.f);
}
