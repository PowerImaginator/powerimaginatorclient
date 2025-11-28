#version 410 core

uniform mat4 u_viewport_proj_mat;
uniform mat4 u_viewport_view_mat;
uniform vec2 u_viewport_resolution;
uniform sampler2D u_tex_viewport_color;
uniform sampler2D u_tex_viewport_world_pos;
uniform sampler2D u_tex_viewport_depth;

uniform mat4 u_camera_extrinsics[16]; // Max 16 cameras (adjust if needed)
uniform mat3 u_camera_intrinsics[16];
uniform vec2 u_camera_resolutions[16]; // Width and height for each camera
uniform sampler2DArray u_tex_camera_depth;
uniform sampler2DArray u_tex_camera_confidence;
uniform int u_num_cameras;

layout(location = 0) out vec4 o_color;

vec4 project_world_to_viewport(vec3 world_pos) {
    return u_viewport_proj_mat * u_viewport_view_mat * vec4(world_pos, 1.0);
}

vec4 project_vggt_to_world(float px, float py, int camera_idx) {
    float d = texelFetch(u_tex_camera_depth, ivec3(px, py, camera_idx), 0).r;

    float fx = u_camera_intrinsics[camera_idx][0][0];
    float fy = u_camera_intrinsics[camera_idx][1][1];
    float cx = u_camera_intrinsics[camera_idx][2][0];
    float cy = u_camera_intrinsics[camera_idx][2][1];

    vec3 cam_pos = vec3(
        (px - cx) * d / fx,
        (py - cy) * d / fy,
        d
    );

    vec4 world_pos_homogeneous = u_camera_extrinsics[camera_idx] * vec4(cam_pos, 1.0f);
    world_pos_homogeneous.xyz /= world_pos_homogeneous.w;

    return world_pos_homogeneous;
}

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

vec3 get_ro() {
    mat4 view_inv = inverse(u_viewport_view_mat);
    return view_inv[3].xyz;
}

vec3 get_rd() {
    // Get camera position from inverse view matrix
    mat4 view_inv = inverse(u_viewport_view_mat);
    // Convert screen space to NDC
    vec2 ndc = (gl_FragCoord.xy / u_viewport_resolution) * 2.0 - 1.0;
    // Create ray in clip space
    vec4 clip_ray = vec4(ndc, -1.0, 1.0);
    // Transform to view space
    vec4 view_ray = inverse(u_viewport_proj_mat) * clip_ray;
    view_ray.z = -1.0;
    view_ray.w = 0.0;
    // Transform to world space
    return normalize((view_inv * view_ray).xyz);
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
    vec4 existing_color = texelFetch(u_tex_viewport_color, ivec2(gl_FragCoord.xy), 0);
    vec4 existing_world_pos = texelFetch(u_tex_viewport_world_pos, ivec2(gl_FragCoord.xy), 0);

    const int MAX_STEPS = 300;
    const float FAR_DISTANCE = 10.0;
    const float STEP_SIZE = FAR_DISTANCE / float(MAX_STEPS);
    const int CAMERA_INDEX = 0;

    vec3 ro = get_ro();
    vec3 rd = get_rd();

    // BEGIN MASKING CODE
    bool is_masked = false;
    bool too_far = false;
    float total_dist = STEP_SIZE; // Start slightly away from camera to avoid self-intersection
    for (int i = 0; i < MAX_STEPS; ++i) {
        vec3 p = ro + total_dist * rd;
        
        vec4 viewport_clip_pos = project_world_to_viewport(p);
        vec3 viewport_ndc_pos = viewport_clip_pos.xyz / viewport_clip_pos.w;
        vec2 viewport_tex_coords = viewport_ndc_pos.xy * 0.5 + 0.5;
        float viewport_stored_depth = texture(u_tex_viewport_depth, viewport_tex_coords).r;
        float viewport_p_depth = viewport_ndc_pos.z * 0.5 + 0.5;
        if (viewport_stored_depth < 0.999 && viewport_p_depth >= viewport_stored_depth - 0.0001) {
            break;
        }

        int camera_index = 0;
        vec4 vggt_from_world = project_world_to_vggt(p, camera_index);

        bool invalid_projection = vggt_from_world.w < 0.001 ||
                                  vggt_from_world.z < 0.001 ||
                                  vggt_from_world.x != vggt_from_world.x || // NaN check
                                  vggt_from_world.y != vggt_from_world.y || // NaN check
                                  vggt_from_world.x < 0.0 ||
                                  vggt_from_world.x >= u_camera_resolutions[camera_index].x ||
                                  vggt_from_world.y < 0.0 ||
                                  vggt_from_world.y >= u_camera_resolutions[camera_index].y;
        if (invalid_projection) {
            is_masked = true;
            break;
        }

        float compare_d = bilinear_sample(u_tex_camera_depth, vggt_from_world.xy, 0);
        float compare_conf = bilinear_sample(u_tex_camera_confidence, vggt_from_world.xy, 0);
        if (compare_d < 0.001 || compare_d > 999.0 || vggt_from_world.z > compare_d) {
            is_masked = true;
            break;
        }

        total_dist += STEP_SIZE;

        if (total_dist >= FAR_DISTANCE || i == MAX_STEPS - 1) {
            too_far = true;
            break;
        }
    }
    // END MASKING CODE
    
    if (is_masked) {
        o_color = vec4(1.0, 0.0, 1.0, 1.0);
    } else if (too_far) {
        o_color = vec4(0.0, 0.0, 1.0, 1.0);
    } else {
        o_color = existing_color;
    }
}
