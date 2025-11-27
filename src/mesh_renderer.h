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

struct mesh_renderer_t {
	gl_render_pass_t mesh_pass;
	gl_render_pass_t auto_mask_pass;
	std::vector<vggt_camera_data_t> camera_data; // Camera data for each image
	GLuint depth_texture_array = 0; // Texture array for depth data
	GLuint confidence_texture_array = 0; // Texture array for confidence data
};

void mesh_renderer_init(mesh_renderer_t& renderer);
void mesh_renderer_upload_camera_textures(
	mesh_renderer_t& renderer, std::vector<vggt_camera_data_t> const& camera_data);
void mesh_renderer_render(
	mesh_renderer_t& renderer, fly_camera_t& camera, gl_vertex_buffers_t& mesh_vbo, gl_vertex_buffers_t& quad_vbo);
gl_render_pass_t* mesh_renderer_get_final_render_pass(mesh_renderer_t& renderer);
GLuint mesh_renderer_get_final_fbo_texture(mesh_renderer_t& renderer);
GLuint mesh_renderer_get_final_fbo_width(mesh_renderer_t& renderer);
GLuint mesh_renderer_get_final_fbo_height(mesh_renderer_t& renderer);

void load_vggt_mesh(std::string const& filename, gl_vertex_buffers_t& mesh_vbo, f32 const proximity_threshold,
	std::vector<vggt_camera_data_t>& out_camera_data);
