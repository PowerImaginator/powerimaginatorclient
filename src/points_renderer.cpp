#include "pch.h"

#include "points_renderer.h"

void points_renderer_init(points_renderer_t& renderer) {
	gl_render_pass_init(renderer.points_pass, "shaders/points.vert", "shaders/points.frag", RENDERER_INTERNAL_WIDTH,
		RENDERER_INTERNAL_HEIGHT,
		{{"o_color", {.internal_format = GL_RGB8, .format = GL_RGB, .type = GL_UNSIGNED_BYTE}},
			{"o_cam_pos", {.internal_format = GL_RGBA32F, .format = GL_RGBA, .type = GL_FLOAT}},
			{"o_world_pos", {.internal_format = GL_RGBA32F, .format = GL_RGBA, .type = GL_FLOAT}},
			{"##DEPTH",
				{.clear_depth = 1.0f,
					.internal_format = GL_DEPTH_COMPONENT32F,
					.format = GL_DEPTH_COMPONENT,
					.type = GL_FLOAT}}});
}

void points_renderer_render(points_renderer_t& renderer, fly_camera_t& camera, gl_vertex_buffers_t& points_vbo,
	gl_vertex_buffers_t& quad_vbo, f32 confidence_threshold) {
	UNUSED(quad_vbo);

	gl_render_pass_begin(renderer.points_pass);
	gl_render_pass_uniform_mat4(renderer.points_pass, "u_proj_mat", camera.proj_mat);
	gl_render_pass_uniform_mat4(renderer.points_pass, "u_view_mat", camera.view_mat);
	gl_render_pass_uniform_float(renderer.points_pass, "u_confidence_threshold", confidence_threshold);
	gl_render_pass_draw(renderer.points_pass, points_vbo);
	gl_render_pass_end(renderer.points_pass);
}

gl_render_pass_t* points_renderer_get_final_render_pass(points_renderer_t& renderer) {
	return &renderer.points_pass;
}

GLuint points_renderer_get_final_fbo_texture(points_renderer_t& renderer) {
	return renderer.points_pass.internal_output_descriptors["o_color"].texture;
}

GLuint points_renderer_get_final_fbo_width(points_renderer_t& renderer) {
	return renderer.points_pass.width;
}

GLuint points_renderer_get_final_fbo_height(points_renderer_t& renderer) {
	return renderer.points_pass.height;
}

GLuint points_renderer_get_final_depth_texture(points_renderer_t& renderer) {
	return renderer.points_pass.internal_output_descriptors["##DEPTH"].texture;
}
