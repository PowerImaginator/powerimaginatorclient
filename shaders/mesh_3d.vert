#version 410 core

uniform mat4 u_proj_mat;
uniform mat4 u_view_mat;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color;

out vec3 v_color;

void main() {
        v_color = a_color;
        gl_Position = u_proj_mat * u_view_mat * vec4(a_position, 1.0);
}
