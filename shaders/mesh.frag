#version 410 core

in vec3 v_color;
in vec3 v_world_pos;
in vec3 v_cam_pos;

layout(location = 0) out vec3 o_color;
layout(location = 1) out vec4 o_cam_pos;
layout(location = 2) out vec4 o_world_pos;

void main(void) {
        o_color = v_color;
        o_cam_pos = vec4(v_cam_pos, 1.0);
        o_world_pos = vec4(v_world_pos, 1.0);
}
