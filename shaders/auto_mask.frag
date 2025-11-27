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

vec3 project_vggt_to_world(float px, float py, int camera_idx) {
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
    return world_pos_homogeneous.xyz / world_pos_homogeneous.w;
}

vec3 project_world_to_vggt(vec3 world_pos, int camera_idx) {
    float fx = u_camera_intrinsics[camera_idx][0][0];
    float fy = u_camera_intrinsics[camera_idx][1][1];
    float cx = u_camera_intrinsics[camera_idx][2][0];
    float cy = u_camera_intrinsics[camera_idx][2][1];

    vec4 world_pos_homogeneous = inverse(u_camera_extrinsics[camera_idx]) * vec4(world_pos, 1.0);
    vec3 cam_pos = world_pos_homogeneous.xyz * world_pos_homogeneous.w;

    float d = cam_pos.z;
    float px = ((cam_pos.x * fx) / d) + cx;
    float py = ((cam_pos.y * fy) / d) + cy;
    
    return vec3(px, py, d);
}

void main(void) {
    vec4 existing_color = texelFetch(u_tex_viewport_color, ivec2(gl_FragCoord.xy), 0);
    vec4 existing_world_pos = texelFetch(u_tex_viewport_world_pos, ivec2(gl_FragCoord.xy), 0);

    vec3 vggt_from_world = project_world_to_vggt(existing_world_pos.xyz, 0);
    vec3 world_from_vggt = project_vggt_to_world(vggt_from_world.x, vggt_from_world.y, 0);
    o_color.xyz = abs(existing_world_pos.xyz - world_from_vggt);
    o_color.w = existing_world_pos.w;
}
