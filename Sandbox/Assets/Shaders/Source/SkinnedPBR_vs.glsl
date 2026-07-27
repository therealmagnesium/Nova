#version 460 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in vec3 a_tangent;
layout (location = 4) in uvec4 a_bone_ids;
layout (location = 5) in vec4 a_bone_weights;

layout (location = 0) out vec3 v_position;
layout (location = 1) out vec3 v_normal;
layout (location = 2) out vec2 v_uv;
layout (location = 3) out vec3 v_tangent;

// Storage buffers share set 0 with vertex samplers/storage textures in SDL_GPU's SPIR-V
// binding convention - matches Nova's existing vertex uniform buffers living at set 1.
layout (std430, set = 0, binding = 0) readonly buffer BoneMatrices
{
    mat4 bones[];
} bone_matrices;

layout (std140, set = 1, binding = 0) uniform MVP {
    mat4 model;
    mat4 view_projection;
    mat4 normal;
} mvp;

void main()
{
    const mat4 skin_matrix =
        bone_matrices.bones[a_bone_ids.x] * a_bone_weights.x +
        bone_matrices.bones[a_bone_ids.y] * a_bone_weights.y +
        bone_matrices.bones[a_bone_ids.z] * a_bone_weights.z +
        bone_matrices.bones[a_bone_ids.w] * a_bone_weights.w;

    const vec4 position_skinned = skin_matrix * vec4(a_position, 1.f);
    const vec4 position_world = mvp.model * position_skinned;

    gl_Position = mvp.view_projection * position_world;
    v_position = position_world.xyz;
    v_uv = a_uv;

    // Assumes rigid (non-scaling) bone transforms, true for the overwhelming majority of
    // game rigs - this lets us skin normals with the skin matrix's linear part directly
    // instead of an inverse-transpose per vertex. Revisit if a rig relies on bone scaling.
    const mat3 skin_normal_matrix = mat3(skin_matrix);

    const vec3 N = normalize(mat3(mvp.normal) * skin_normal_matrix * a_normal);
    vec3 T = normalize(mat3(mvp.normal) * skin_normal_matrix * a_tangent);
    T = normalize(T - dot(T, N) * N);

    v_normal = N;
    v_tangent = T;
}
