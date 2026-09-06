#version 460 core

layout (set = 2, binding = 0) uniform sampler2D texture_screen;
layout (set = 3, binding = 0) uniform RenderSettings
{
    float exposure; 
} settings;

layout (location = 0) in vec2 v_uv;
layout (location = 0) out vec4 result;

void main()
{
    const vec2 screen_uv = vec2(v_uv.x, 1.f - v_uv.y); // Still don't know why the screen texture is flipped if I don't flip the UV
    const float gamma = 2.2f; // Might add this as a uniform

    result = texture(texture_screen, screen_uv);
    result.rgb = vec3(1.f) - exp(-result.rgb * settings.exposure); // HDR tone mapping
    result.rgb = pow(result.rgb, vec3(1.f / gamma)); // Gamma correction
}
