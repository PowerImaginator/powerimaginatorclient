#pragma once

#include "pch.h"

#include "fly_camera.h"
#include "gl_render_pass.h"
#include "gl_vertex_buffers.h"

struct mask_renderer_t {
	gl_render_pass_t mask_pass;
};

void mask_renderer_init(mask_renderer_t& renderer);
void mask_renderer_render(mask_renderer_t& renderer, fly_camera_t& camera, gl_vertex_buffers_t& mask_vbo);
gl_render_pass_t* mask_renderer_get_final_render_pass(mask_renderer_t& renderer);
GLuint mask_renderer_get_final_fbo_texture(mask_renderer_t& renderer);
GLuint mask_renderer_get_final_fbo_width(mask_renderer_t& renderer);
GLuint mask_renderer_get_final_fbo_height(mask_renderer_t& renderer);
GLuint mask_renderer_get_final_depth_texture(mask_renderer_t& renderer);
