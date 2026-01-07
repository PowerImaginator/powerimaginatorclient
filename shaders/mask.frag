#version 410 core

layout(location = 0) out vec4 o_color;

in vec3 v_world_pos;

uniform mat4 u_camera_extrinsics[16]; // camera-to-world
uniform mat4 u_camera_world_to_cam[16]; // world-to-camera (pre-inverted on CPU)
uniform mat3 u_camera_intrinsics[16];
uniform vec2 u_camera_resolutions[16];
uniform sampler2DArray u_tex_camera_depth;
uniform sampler2DArray u_tex_camera_confidence;
uniform int u_num_cameras;

vec4 project_world_to_vggt(vec3 world_pos, int camera_idx) {
    float fx = u_camera_intrinsics[camera_idx][0][0];
    float fy = u_camera_intrinsics[camera_idx][1][1];
    float cx = u_camera_intrinsics[camera_idx][2][0];
    float cy = u_camera_intrinsics[camera_idx][2][1];

    vec4 cam_pos_homogeneous = inverse(u_camera_extrinsics[camera_idx]) * vec4(world_pos, 1.0);
    vec3 cam_pos = cam_pos_homogeneous.xyz / cam_pos_homogeneous.w;

    float d = cam_pos.z;
    // Prevent division by zero or negative depth
    if (d <= 0.001) {
        return vec4(-1.0, -1.0, -1.0, -1.0); // Invalid projection
    }

    float px = ((cam_pos.x * fx) / d) + cx;
    float py = ((cam_pos.y * fy) / d) + cy;

    return vec4(
        // Pixel coordinates in [0, resolution]
        px, py,
        // Depth (in the units of the VGGT depth textures)
        d,
        // w (to check if something is behind the camera)
        cam_pos_homogeneous.w
        );
}

float bilinear_sample(sampler2DArray the_texture, vec2 pixel_coords, int camera_index) {
    ivec3 bl = ivec3(floor(pixel_coords.x), floor(pixel_coords.y), camera_index);
    ivec3 br = ivec3(ceil(pixel_coords.x), floor(pixel_coords.y), camera_index);
    ivec3 tl = ivec3(floor(pixel_coords.x), ceil(pixel_coords.y), camera_index);
    ivec3 tr = ivec3(ceil(pixel_coords.x), ceil(pixel_coords.y), camera_index);

    float sbl = texelFetch(the_texture, bl, 0).r;
    float sbr = texelFetch(the_texture, br, 0).r;
    float stl = texelFetch(the_texture, tl, 0).r;
    float str = texelFetch(the_texture, tr, 0).r;

    float sb = mix(sbl, sbr, pixel_coords.x - floor(pixel_coords.x));
    float st = mix(stl, str, pixel_coords.x - floor(pixel_coords.x));
    return mix(sb, st, pixel_coords.y - floor(pixel_coords.y));
}

void main(void) {
    // Use the world position directly instead of raymarching
    vec3 p = v_world_pos;
    
    // Apply the same masking logic as auto_mask.frag
    bool is_invalid_in_all_cameras = true;
    bool is_visible_from_any_camera = false;

    for (int camera_index = 0; camera_index < u_num_cameras; ++camera_index) {
        vec4 vggt_from_world = project_world_to_vggt(p, camera_index);

        bool invalid_projection = vggt_from_world.w < 0.001 ||
                                vggt_from_world.z < 0.001 ||
                                vggt_from_world.x != vggt_from_world.x || // NaN check
                                vggt_from_world.y != vggt_from_world.y || // NaN check
                                vggt_from_world.x < 1.0 ||
                                vggt_from_world.x >= u_camera_resolutions[camera_index].x - 1 ||
                                vggt_from_world.y < 1.0 ||
                                vggt_from_world.y >= u_camera_resolutions[camera_index].y - 1;
        if (invalid_projection) {
            // it's outside the camera frustum
            continue;
        }

        float compare_d = bilinear_sample(u_tex_camera_depth, vggt_from_world.xy, camera_index);
        float compare_conf = bilinear_sample(u_tex_camera_confidence, vggt_from_world.xy, camera_index);

        if (compare_conf < 5.0 || compare_d < 0.001 || compare_d > 999.0) {
            // No data for that point exists (sky?)
            continue;
        }

        is_invalid_in_all_cameras = false;

        if (vggt_from_world.z > compare_d + 0.001) {
            // it's masked (occluded)
            continue;
        }

        is_visible_from_any_camera = true;
        break;
    }

    // Discard the fragment if it's masked (not visible from any camera but has valid data)
    if (!is_visible_from_any_camera && !is_invalid_in_all_cameras) {
        discard;
    }

    o_color = vec4(1.0);
}
