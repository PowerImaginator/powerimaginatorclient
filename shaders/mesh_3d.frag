#version 410 core

in vec3 v_color;

layout(location = 0) out vec3 o_color;

void main(void) {
        o_color = v_color;
}
