#pragma once

#include "pch.h"

#include "fly_camera.h"
#include "gl_render_pass.h"
#include "gl_vertex_buffers.h"

struct vggt_camera_data_t {
	glm::mat4 extrinsic; // camera-to-world matrix
	glm::mat3 intrinsic; // intrinsic matrix
	std::vector<f32> confidence; // confidence buffer (width * height)
	std::vector<f32> depth; // depth buffer (width * height)
	u32 width;
	u32 height;
};

struct points_renderer_t {
	gl_render_pass_t points_pass;
};

void points_renderer_init(points_renderer_t& renderer);
void points_renderer_render(points_renderer_t& renderer, fly_camera_t& camera, gl_vertex_buffers_t& points_vbo,
	gl_vertex_buffers_t& quad_vbo, f32 confidence_threshold);
gl_render_pass_t* points_renderer_get_final_render_pass(points_renderer_t& renderer);
GLuint points_renderer_get_final_fbo_texture(points_renderer_t& renderer);
GLuint points_renderer_get_final_fbo_width(points_renderer_t& renderer);
GLuint points_renderer_get_final_fbo_height(points_renderer_t& renderer);
GLuint points_renderer_get_final_depth_texture(points_renderer_t& renderer);
