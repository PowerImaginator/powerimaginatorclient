#include "pch.h"

#include "exchange.h"
#include "points_renderer.h"

void points_renderer_init(points_renderer_t& renderer) {
	gl_render_pass_init(renderer.points_pass, "shaders/points.vert", "shaders/points.frag",
		g_exchange.renderer_internal_width, g_exchange.renderer_internal_height,
		{{"o_color", {.internal_format = GL_RGB8, .format = GL_RGB, .type = GL_UNSIGNED_BYTE}},
			{"o_cam_pos", {.internal_format = GL_RGBA32F, .format = GL_RGBA, .type = GL_FLOAT}},
			{"o_world_pos", {.internal_format = GL_RGBA32F, .format = GL_RGBA, .type = GL_FLOAT}},
			{"##DEPTH",
				{.clear_depth = 1.0f,
					.internal_format = GL_DEPTH_COMPONENT32F,
					.format = GL_DEPTH_COMPONENT,
					.type = GL_FLOAT}}});

	std::vector<glm::ivec2> downsample_sizes;
	{
		u32 cur_downsample_width = g_exchange.renderer_internal_width / 2,
		    cur_downsample_height = g_exchange.renderer_internal_height / 2;
		while (cur_downsample_width > 0 && cur_downsample_height > 0) {
			downsample_sizes.push_back(glm::ivec2(cur_downsample_width, cur_downsample_height));
			cur_downsample_width /= 2;
			cur_downsample_height /= 2;
		}
	}

	for (auto const& size : downsample_sizes) {
		gl_render_pass_t cur_pass;
		gl_render_pass_init(cur_pass, "shaders/quad.vert", "shaders/downsample.frag", size.x, size.y,
			{{"o_color", {.internal_format = GL_RGB8, .format = GL_RGB, .type = GL_UNSIGNED_BYTE}},
				{"o_cam_pos", {.internal_format = GL_RGBA32F, .format = GL_RGBA, .type = GL_FLOAT}},
				{"##DEPTH",
					{.clear_depth = 1.0f,
						.internal_format = GL_DEPTH_COMPONENT32F,
						.format = GL_DEPTH_COMPONENT,
						.type = GL_FLOAT}}});
		renderer.downsample_passes.emplace_back(std::move(cur_pass));
	}

	gl_render_pass_init(renderer.hpr_pass, "shaders/quad.vert", "shaders/hpr.frag", renderer.points_pass.width,
		renderer.points_pass.height,
		{{"o_visibility", {.internal_format = GL_R32F, .format = GL_RED, .type = GL_FLOAT}}});
	renderer.hpr_pass.depth_func = 0;

	gl_render_pass_init(renderer.multiply_color_pass, "shaders/quad.vert", "shaders/multiply_visibility.frag",
		renderer.points_pass.width, renderer.points_pass.height,
		{{"o_dest", {.internal_format = GL_RGBA32F, .format = GL_RGBA, .type = GL_FLOAT}}});
	renderer.multiply_color_pass.depth_func = 0;

	gl_render_pass_init(renderer.multiply_world_pos_pass, "shaders/quad.vert", "shaders/multiply_visibility.frag",
		renderer.points_pass.width, renderer.points_pass.height,
		{{"o_dest", {.internal_format = GL_RGBA32F, .format = GL_RGBA, .type = GL_FLOAT}}});
	renderer.multiply_world_pos_pass.depth_func = 0;

	for (auto const& size : downsample_sizes) {
		gl_render_pass_t cur_push_pass;
		gl_render_pass_init(cur_push_pass, "shaders/quad.vert", "shaders/push_color.frag", size.x, size.y,
			{{"o_dest", {.internal_format = GL_RGBA32F, .format = GL_RGBA, .type = GL_FLOAT}}});
		renderer.push_color_passes.emplace_back(std::move(cur_push_pass));
	}

	for (u32 i = 0; i < downsample_sizes.size(); ++i) {
		u32 width = 0, height = 0;
		if (i == 0) {
			width = g_exchange.renderer_internal_width;
			height = g_exchange.renderer_internal_height;
		} else {
			width = downsample_sizes[i - 1].x;
			height = downsample_sizes[i - 1].y;
		}

		gl_render_pass_t cur_pull_pass;
		gl_render_pass_init(cur_pull_pass, "shaders/quad.vert", "shaders/pull_color.frag", width, height,
			{{"o_dest", {.internal_format = GL_RGBA32F, .format = GL_RGBA, .type = GL_FLOAT}}});
		renderer.pull_color_passes.emplace_back(std::move(cur_pull_pass));
	}
}

void points_renderer_render(points_renderer_t& renderer, fly_camera_t& camera, gl_vertex_buffers_t& points_vbo,
	gl_vertex_buffers_t& quad_vbo) {
	gl_render_pass_begin(renderer.points_pass);
	gl_render_pass_uniform_mat4(renderer.points_pass, "u_proj_mat", camera.proj_mat);
	gl_render_pass_uniform_mat4(renderer.points_pass, "u_view_mat", camera.view_mat);
	gl_render_pass_draw(renderer.points_pass, points_vbo);
	gl_render_pass_end(renderer.points_pass);

	for (u32 i = 0; i < renderer.downsample_passes.size(); ++i) {
		GLuint source_color = 0, source_cam_pos = 0, source_depth = 0;
		if (i == 0) {
			source_color = renderer.points_pass.internal_output_descriptors["o_color"].texture;
			source_cam_pos = renderer.points_pass.internal_output_descriptors["o_cam_pos"].texture;
			source_depth = renderer.points_pass.internal_output_descriptors["##DEPTH"].texture;
		} else {
			source_color = renderer.downsample_passes[i - 1].internal_output_descriptors["o_color"].texture;
			source_cam_pos =
				renderer.downsample_passes[i - 1].internal_output_descriptors["o_cam_pos"].texture;
			source_depth = renderer.downsample_passes[i - 1].internal_output_descriptors["##DEPTH"].texture;
		}
		gl_render_pass_begin(renderer.downsample_passes[i]);
		gl_render_pass_uniform_texture(renderer.downsample_passes[i], "u_tex_color", source_color, GL_TEXTURE0);
		gl_render_pass_uniform_texture(
			renderer.downsample_passes[i], "u_tex_cam_pos", source_cam_pos, GL_TEXTURE1);
		gl_render_pass_uniform_texture(renderer.downsample_passes[i], "u_tex_depth", source_depth, GL_TEXTURE2);
		gl_render_pass_draw(renderer.downsample_passes[i], quad_vbo);
		gl_render_pass_end(renderer.downsample_passes[i]);
	}

	gl_render_pass_begin(renderer.hpr_pass);
	gl_render_pass_uniform_texture(renderer.hpr_pass, "u_tex_cam_pos_levels[0]",
		renderer.points_pass.internal_output_descriptors["o_cam_pos"].texture, GL_TEXTURE0);
	for (u32 i = 0; i < points_renderer_t::MAX_LEVEL - 1; ++i) {
		gl_render_pass_uniform_texture(renderer.hpr_pass, "u_tex_cam_pos_levels[" + std::to_string(i + 1) + "]",
			renderer.downsample_passes[std::min(i, static_cast<u32>(renderer.downsample_passes.size()) - 1)]
				.internal_output_descriptors["o_cam_pos"]
				.texture,
			GL_TEXTURE0 + 1 + i);
	}
	gl_render_pass_uniform_int(renderer.hpr_pass, "u_num_levels", points_renderer_t::MAX_LEVEL);
	gl_render_pass_uniform_int(renderer.hpr_pass, "u_coarse_level", renderer.u_coarse_level);
	gl_render_pass_uniform_vec2(renderer.hpr_pass, "u_viewport_size",
		glm::vec2(renderer.points_pass.width, renderer.points_pass.height));
	gl_render_pass_uniform_float(renderer.hpr_pass, "u_fov_y", g_exchange.renderer_internal_fov_y);
	gl_render_pass_uniform_float(renderer.hpr_pass, "u_s_hpr", 10.0f * renderer.u_s0);
	gl_render_pass_uniform_float(renderer.hpr_pass, "u_occlusion_threshold", renderer.u_occlusion_threshold);
	gl_render_pass_draw(renderer.hpr_pass, quad_vbo);
	gl_render_pass_end(renderer.hpr_pass);

	gl_render_pass_begin(renderer.multiply_color_pass);
	gl_render_pass_uniform_texture(renderer.multiply_color_pass, "u_tex_source",
		renderer.points_pass.internal_output_descriptors["o_color"].texture, GL_TEXTURE0);
	gl_render_pass_uniform_texture(renderer.multiply_color_pass, "u_tex_factor",
		renderer.hpr_pass.internal_output_descriptors["o_visibility"].texture, GL_TEXTURE1);
	gl_render_pass_draw(renderer.multiply_color_pass, quad_vbo);
	gl_render_pass_end(renderer.multiply_color_pass);

	gl_render_pass_begin(renderer.multiply_world_pos_pass);
	gl_render_pass_uniform_texture(renderer.multiply_world_pos_pass, "u_tex_source",
		renderer.points_pass.internal_output_descriptors["o_world_pos"].texture, GL_TEXTURE0);
	gl_render_pass_uniform_texture(renderer.multiply_world_pos_pass, "u_tex_factor",
		renderer.hpr_pass.internal_output_descriptors["o_visibility"].texture, GL_TEXTURE1);
	gl_render_pass_draw(renderer.multiply_world_pos_pass, quad_vbo);
	gl_render_pass_end(renderer.multiply_world_pos_pass);

	for (u32 i = 0; i < renderer.push_color_passes.size(); ++i) {
		GLuint source_tex = 0;
		if (i == 0) {
			source_tex = renderer.multiply_color_pass.internal_output_descriptors["o_dest"].texture;
		} else {
			source_tex = renderer.push_color_passes[i - 1].internal_output_descriptors["o_dest"].texture;
		}
		gl_render_pass_begin(renderer.push_color_passes[i]);
		gl_render_pass_uniform_texture(renderer.push_color_passes[i], "u_tex_source", source_tex, GL_TEXTURE0);
		gl_render_pass_draw(renderer.push_color_passes[i], quad_vbo);
		gl_render_pass_end(renderer.push_color_passes[i]);
	}

	for (s32 si = renderer.pull_color_passes.size() - 1; si >= 0; --si) {
		u32 i = si;

		GLuint source_prev_tex = 0, source_cur_tex = 0;
		if (i == renderer.pull_color_passes.size() - 1) {
			source_prev_tex = renderer.push_color_passes[i].internal_output_descriptors["o_dest"].texture;
			source_cur_tex =
				renderer.push_color_passes[i - 1].internal_output_descriptors["o_dest"].texture;
		} else {
			source_prev_tex =
				renderer.pull_color_passes[i + 1].internal_output_descriptors["o_dest"].texture;
			if (i == 0) {
				source_cur_tex =
					renderer.multiply_color_pass.internal_output_descriptors["o_dest"].texture;
			} else {
				source_cur_tex =
					renderer.push_color_passes[i - 1].internal_output_descriptors["o_dest"].texture;
			}
		}

		gl_render_pass_begin(renderer.pull_color_passes[i]);
		gl_render_pass_uniform_texture(
			renderer.pull_color_passes[i], "u_tex_source_prev", source_prev_tex, GL_TEXTURE0);
		gl_render_pass_uniform_texture(
			renderer.pull_color_passes[i], "u_tex_source_cur", source_cur_tex, GL_TEXTURE1);
		gl_render_pass_draw(renderer.pull_color_passes[i], quad_vbo);
		gl_render_pass_end(renderer.pull_color_passes[i]);
	}
}

gl_render_pass_t* points_renderer_get_final_render_pass(points_renderer_t& renderer) {
	return &renderer.pull_color_passes[0];
}

GLuint points_renderer_get_final_fbo_texture(points_renderer_t& renderer) {
	return points_renderer_get_final_render_pass(renderer)->internal_output_descriptors["o_dest"].texture;
}

GLuint points_renderer_get_final_fbo_width(points_renderer_t& renderer) {
	return points_renderer_get_final_render_pass(renderer)->width;
}

GLuint points_renderer_get_final_fbo_height(points_renderer_t& renderer) {
	return points_renderer_get_final_render_pass(renderer)->height;
}
