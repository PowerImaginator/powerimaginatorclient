#version 410 core

uniform mat4 u_proj_mat;
uniform mat4 u_view_mat;

layout(location = 0) in vec3 a_position;

out vec3 v_world_pos;

void main() {
        v_world_pos = a_position;
        gl_Position = u_proj_mat * u_view_mat * vec4(a_position, 1.0);
}
