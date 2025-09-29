#pragma once

#include "pch.h"

#include "fly_camera.h"
#include "gl_render_pass.h"
#include "gl_vertex_buffers.h"

struct points_renderer_t {
	gl_render_pass_t points_pass;
	std::vector<gl_render_pass_t> downsample_passes;
	gl_render_pass_t hpr_pass;
	gl_render_pass_t multiply_color_pass;
	gl_render_pass_t multiply_world_pos_pass;
	std::vector<gl_render_pass_t> push_color_passes;
	std::vector<gl_render_pass_t> pull_color_passes;

	static constexpr u32 MAX_LEVEL = 8; // MUST match length of u_tex_cam_pos_levels[...] array in hpr.frag

	float u_s0 = 0.005f;
	float u_occlusion_threshold = 0.1f;
	int u_coarse_level = 4;
};

void points_renderer_init(points_renderer_t& renderer);
void points_renderer_render(points_renderer_t& renderer, fly_camera_t& camera, gl_vertex_buffers_t& points_vbo,
	gl_vertex_buffers_t& quad_vbo);
gl_render_pass_t* points_renderer_get_final_render_pass(points_renderer_t& renderer);
GLuint points_renderer_get_final_fbo_texture(points_renderer_t& renderer);
GLuint points_renderer_get_final_fbo_width(points_renderer_t& renderer);
GLuint points_renderer_get_final_fbo_height(points_renderer_t& renderer);
