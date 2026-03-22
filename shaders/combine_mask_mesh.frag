#version 410 core

uniform sampler2D u_tex_mesh_color;
uniform sampler2D u_tex_mesh_depth;
uniform sampler2D u_tex_mask_color;
uniform sampler2D u_tex_mask_depth;

layout(location = 0) out vec4 o_dest;

void main(void) {
        vec4 mesh_color = texelFetch(u_tex_mesh_color, ivec2(gl_FragCoord.xy), 0);
        vec4 mesh_depth = texelFetch(u_tex_mesh_depth, ivec2(gl_FragCoord.xy), 0);
        vec4 mask_color = texelFetch(u_tex_mask_color, ivec2(gl_FragCoord.xy), 0);
        vec4 mask_depth = texelFetch(u_tex_mask_depth, ivec2(gl_FragCoord.xy), 0);

        if (mask_color.a > 0.0 && mask_depth.r < mesh_depth.r) {
                o_dest = vec4(0.0, 1.0, 0.0, 1.0); // vec4(0.0);
        } else if (mesh_color.a > 0.0) {
                o_dest = mesh_color;
        } else {
                o_dest = vec4(0.0, 1.0, 0.0, 1.0); // vec4(0.0);
        }
}
