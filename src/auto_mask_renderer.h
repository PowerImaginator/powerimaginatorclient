#pragma once

#include "pch.h"

#include "fly_camera.h"
#include "gl_render_pass.h"
#include "gl_vertex_buffers.h"
#include "mesh_renderer.h" // for vggt_camera_data_t

struct auto_mask_renderer_t {
	gl_render_pass_t auto_mask_pass;
	std::vector<vggt_camera_data_t> camera_data;
	GLuint depth_texture_array = 0;
	GLuint confidence_texture_array = 0;
};

void auto_mask_renderer_init(auto_mask_renderer_t& renderer, GLuint width, GLuint height);
void auto_mask_renderer_load_camera_data(
	auto_mask_renderer_t& renderer, std::vector<vggt_camera_data_t> const& camera_data);
void auto_mask_renderer_render(auto_mask_renderer_t& renderer, fly_camera_t& camera, gl_vertex_buffers_t& quad_vbo,
	GLuint viewport_color_tex, GLuint viewport_depth_tex);
gl_render_pass_t* auto_mask_renderer_get_final_render_pass(auto_mask_renderer_t& renderer);
GLuint auto_mask_renderer_get_final_fbo_texture(auto_mask_renderer_t& renderer);
GLuint auto_mask_renderer_get_final_fbo_width(auto_mask_renderer_t& renderer);
GLuint auto_mask_renderer_get_final_fbo_height(auto_mask_renderer_t& renderer);
