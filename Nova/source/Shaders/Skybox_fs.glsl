#version 460 core
layout (location = 0) in vec3 v_position_local;
layout (location = 0) out vec4 o_result;

layout (set = 2, binding = 0) uniform samplerCube map_environment;

void main()
{		
    const vec3 environment_color = textureLod(map_environment, v_position_local, 0.0).rgb;    
    o_result = vec4(environment_color, 1.0);
}
