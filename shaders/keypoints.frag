#version 410 core

uniform sampler2D u_tex_world_pos;
uniform sampler2D u_tex_mask;
uniform usampler2D u_tex_keypoints;
uniform ivec2 u_viewport_size;
uniform int u_num_keypoints;
uniform ivec2 u_hovered_keypoint;

layout(location = 0) out vec4 o_dest;

void main(void) {
        ivec2 coord_base = ivec2(gl_FragCoord.xy);

        vec4 world_pos_sample = texelFetch(u_tex_world_pos, coord_base, 0);
        vec4 mask_sample = texelFetch(u_tex_mask, coord_base, 0);

        vec2 coord_base_f = vec2(coord_base);
        vec2 flipped_hovered_keypoint_f = vec2(u_hovered_keypoint.x, u_viewport_size.y - 1 - u_hovered_keypoint.y);
        float hover_dist = distance(coord_base_f, flipped_hovered_keypoint_f);

        bool has_keypoint = false;
        for (int i = 0; i < u_num_keypoints; ++i) {
                uvec2 keypoint_u = texelFetch(u_tex_keypoints, ivec2(i, 0), 0).xy;
                vec2 keypoint = vec2(keypoint_u);
                float keypoint_dist = distance(coord_base_f, vec2(keypoint.x, u_viewport_size.y - 1 - keypoint.y));
                if (keypoint_dist < 4.0) {
                        has_keypoint = true;
                        break;
                }
        }

        if (hover_dist < 6.0) {
                o_dest = vec4(0.0, 1.0, 1.0, hover_dist < 2.0 ? 1.0 : 0.5);
        } else if (has_keypoint) {
                o_dest = vec4(0.0, 0.0, 1.0, 0.5);
        } else if (world_pos_sample.w > 0.0 && mask_sample.x < 0.01) {
                o_dest = vec4(1.0, 1.0, 1.0, 0.5);
        } else {
                o_dest = vec4(0.0);
        }
}
