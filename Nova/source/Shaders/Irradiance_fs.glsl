#version 460 core
layout (location = 0) in vec3 v_position_local;
layout (location = 0) out vec4 o_result;

layout (set = 2, binding = 0) uniform samplerCube map_environment;

const float PI = 3.14159265359;

void main()
{		
    const vec3 N = normalize(v_position_local);

    vec3 irradiance = vec3(0.0);   
    
    // Fix the singularity bug by checking if N is aligned with the up vector
    vec3 up = (abs(N.y) < 0.999) ? vec3(0.0, 1.0, 0.0) : vec3(0.0, 0.0, 1.0);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));
       
    const float sampleDelta = 0.025;
    float sample_count = 0.0;
    
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        { 
            // Spherical to cartesian (in tangent space)
            const vec3 tangent_sample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta)); 
            // Tangent space to world 
            const vec3 sample_vec = tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * N; 

            // Include the differential solid angle weighting correctly
            irradiance += texture(map_environment, sample_vec).rgb * cos(theta) * sin(theta);
            sample_count++;
        }
    }

    // Correct mathematical normalization factor for Riemann hemisphere integration
    irradiance = PI * irradiance * (1.0 / float(sample_count)); 
    o_result = vec4(irradiance, 1.0);
}
