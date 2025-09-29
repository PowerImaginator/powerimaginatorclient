#version 410 core

uniform sampler2D u_tex_source;
uniform vec2 u_viewport_size;
uniform vec2 u_brush_pos;
uniform float u_brush_radius;
uniform bool u_use_tex_source;
uniform float u_brush_strength;

layout(location = 0) out vec4 o_dest;

void main(void) {
        vec2 q = gl_FragCoord.xy / u_viewport_size;
        vec2 b = u_brush_pos / u_viewport_size;
        b.y = 1.0 - b.y;

        float aspect = u_viewport_size.x / u_viewport_size.y;
        q.x *= aspect;
        b.x *= aspect;

        float dist = length(q - b);
        if (dist < u_brush_radius / u_viewport_size.x) {
                o_dest = vec4(u_brush_strength);
        } else {
                if (u_use_tex_source) {
                        o_dest = texelFetch(u_tex_source, ivec2(gl_FragCoord.xy), 0);
                } else {
                        o_dest = vec4(0.0);
                }
        }
}
