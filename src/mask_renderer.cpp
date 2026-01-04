#include "pch.h"

#include "mask_renderer.h"

void mask_renderer_init(mask_renderer_t& renderer) {
	gl_render_pass_init(renderer.mask_pass, "shaders/points.vert", "shaders/points.frag", RENDERER_INTERNAL_WIDTH,
		RENDERER_INTERNAL_HEIGHT,
		{{"o_color", {.internal_format = GL_RGBA8, .format = GL_RGBA, .type = GL_UNSIGNED_BYTE}},
			{"##DEPTH",
				{.clear_depth = 1.0f,
					.internal_format = GL_DEPTH_COMPONENT32F,
					.format = GL_DEPTH_COMPONENT,
					.type = GL_FLOAT}}});
}

void mask_renderer_render(mask_renderer_t& renderer, fly_camera_t& camera, gl_vertex_buffers_t& points_vbo,
	gl_vertex_buffers_t& quad_vbo) {
	UNUSED(quad_vbo);

	gl_render_pass_begin(renderer.mask_pass);
	gl_render_pass_uniform_mat4(renderer.mask_pass, "u_proj_mat", camera.proj_mat);
	gl_render_pass_uniform_mat4(renderer.mask_pass, "u_view_mat", camera.view_mat);
	gl_render_pass_draw(renderer.mask_pass, points_vbo);
	gl_render_pass_end(renderer.mask_pass);
}

gl_render_pass_t* mask_renderer_get_final_render_pass(mask_renderer_t& renderer) {
	return &renderer.mask_pass;
}

GLuint mask_renderer_get_final_fbo_texture(mask_renderer_t& renderer) {
	return renderer.mask_pass.internal_output_descriptors["o_color"].texture;
}

GLuint mask_renderer_get_final_fbo_width(mask_renderer_t& renderer) {
	return renderer.mask_pass.width;
}

GLuint mask_renderer_get_final_fbo_height(mask_renderer_t& renderer) {
	return renderer.mask_pass.height;
}

GLuint mask_renderer_get_final_depth_texture(mask_renderer_t& renderer) {
	return renderer.mask_pass.internal_output_descriptors["##DEPTH"].texture;
}
