#version 410 core

uniform sampler2D u_tex_color;
uniform sampler2D u_tex_depth;
uniform sampler2D u_tex_mask;
uniform sampler2D u_tex_eraser;
uniform vec2 u_viewport_size;
uniform float u_min_depth;
uniform float u_max_depth;

layout(location = 0) out vec4 o_dest;

void main(void) {
        ivec2 coord = ivec2(gl_FragCoord.xy);
        ivec2 flipped_coord = ivec2(coord.x, int(u_viewport_size.y) - 1 - coord.y);

        vec3 color = texelFetch(u_tex_color, coord, 0).xyz;
        float depth = texelFetch(u_tex_depth, flipped_coord, 0).x;
        float mask = texelFetch(u_tex_mask, coord, 0).x;
        float eraser = texelFetch(u_tex_eraser, coord, 0).x;

        vec4 tint = vec4(0.0, 0.0, 0.0, 0.0);

        if (mask < 0.01) {
                tint = vec4(0.0, 1.0, 0.0, 0.5);
        } else if (depth < u_min_depth || depth > u_max_depth) {
                tint = vec4(1.0, 0.0, 0.0, 0.5);
        } else if (eraser > 0.0) {
                tint = vec4(0.0, 0.0, 0.0, 0.5);
        }

        o_dest = mix(vec4(color, 1.0), vec4(tint.xyz, 1.0), tint.w);
}
