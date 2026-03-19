#version 410 core

uniform mat4 u_proj_mat;
uniform mat4 u_view_mat;

layout(location = 0) in vec3 a_position;
layout(location = 1) in float a_confidence;

out vec3 v_world_pos;
out float v_confidence;

void main() {
        v_world_pos = a_position;
        v_confidence = a_confidence;
        gl_Position = u_proj_mat * u_view_mat * vec4(a_position, 1.0);
}
