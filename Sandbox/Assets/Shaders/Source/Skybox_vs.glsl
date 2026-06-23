#version 460 core
layout (location = 0) in vec3 a_position;
layout (location = 0) out vec3 v_position_local;

layout (std140, set = 1, binding = 0) uniform MVPData {
    mat4 view;
    mat4 projection;
} mvp;

void main()
{
    v_position_local = a_position;

	const mat4 static_view = mat4(mat3(mvp.view));
	const vec4 clip_position = mvp.projection * static_view * vec4(v_position_local, 1.0);

	gl_Position = clip_position.xyww;
}
