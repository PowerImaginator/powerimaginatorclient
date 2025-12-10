#include "pch.h"

#include "auto_mask_renderer.h"

void auto_mask_renderer_init(auto_mask_renderer_t& renderer, GLuint width, GLuint height) {
	gl_render_pass_init(renderer.auto_mask_pass, "shaders/quad.vert", "shaders/auto_mask.frag", width, height,
		{{"o_color", {.internal_format = GL_RGBA32F, .format = GL_RGBA, .type = GL_FLOAT}}});
}

void auto_mask_renderer_load_camera_data(
	auto_mask_renderer_t& renderer, std::vector<vggt_camera_data_t> const& camera_data) {
	if (camera_data.empty()) {
		return;
	}

	// If already loaded with matching shapes, skip re-upload
	if (!renderer.camera_data.empty() && renderer.camera_data.size() == camera_data.size()) {
		bool same_shape = true;
		for (size_t i = 0; i < camera_data.size(); ++i) {
			if (renderer.camera_data[i].width != camera_data[i].width ||
				renderer.camera_data[i].height != camera_data[i].height) {
				same_shape = false;
				break;
			}
		}
		if (same_shape) {
			return;
		}
	}

	u32 max_width = 0;
	u32 max_height = 0;
	for (auto const& cam : camera_data) {
		max_width = std::max(max_width, cam.width);
		max_height = std::max(max_height, cam.height);
	}

	u32 num_images = static_cast<u32>(camera_data.size());

	if (renderer.depth_texture_array == 0) {
		glGenTextures(1, &renderer.depth_texture_array);
	}
	glBindTexture(GL_TEXTURE_2D_ARRAY, renderer.depth_texture_array);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R32F, max_width, max_height, num_images, 0, GL_RED, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	for (u32 i = 0; i < num_images; ++i) {
		auto const& cam = camera_data[i];
		glTexSubImage3D(
			GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, cam.width, cam.height, 1, GL_RED, GL_FLOAT, cam.depth.data());
	}

	if (renderer.confidence_texture_array == 0) {
		glGenTextures(1, &renderer.confidence_texture_array);
	}
	glBindTexture(GL_TEXTURE_2D_ARRAY, renderer.confidence_texture_array);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R32F, max_width, max_height, num_images, 0, GL_RED, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	for (u32 i = 0; i < num_images; ++i) {
		auto const& cam = camera_data[i];
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, cam.width, cam.height, 1, GL_RED, GL_FLOAT,
			cam.confidence.data());
	}

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	renderer.camera_data = camera_data;
}

void auto_mask_renderer_render(auto_mask_renderer_t& renderer, fly_camera_t& camera, gl_vertex_buffers_t& quad_vbo,
	GLuint viewport_color_tex, GLuint viewport_depth_tex) {
	gl_render_pass_begin(renderer.auto_mask_pass);
	gl_render_pass_uniform_mat4(renderer.auto_mask_pass, "u_viewport_proj_mat", camera.proj_mat);
	gl_render_pass_uniform_mat4(renderer.auto_mask_pass, "u_viewport_view_mat", camera.view_mat);
	gl_render_pass_uniform_vec2(renderer.auto_mask_pass, "u_viewport_resolution",
		glm::vec2(static_cast<f32>(renderer.auto_mask_pass.width),
			static_cast<f32>(renderer.auto_mask_pass.height)));
	gl_render_pass_uniform_texture(
		renderer.auto_mask_pass, "u_tex_viewport_color", viewport_color_tex, GL_TEXTURE0);
	gl_render_pass_uniform_texture(
		renderer.auto_mask_pass, "u_tex_viewport_depth", viewport_depth_tex, GL_TEXTURE2);

	if (!renderer.camera_data.empty()) {
		std::vector<glm::mat4> extrinsics;
		std::vector<glm::mat3> intrinsics;
		std::vector<glm::vec2> resolutions;
		extrinsics.reserve(renderer.camera_data.size());
		intrinsics.reserve(renderer.camera_data.size());
		resolutions.reserve(renderer.camera_data.size());
		for (auto const& cam : renderer.camera_data) {
			extrinsics.push_back(cam.extrinsic);
			intrinsics.push_back(cam.intrinsic);
			resolutions.push_back(glm::vec2(static_cast<f32>(cam.width), static_cast<f32>(cam.height)));
		}

		gl_render_pass_uniform_mat4_array(renderer.auto_mask_pass, "u_camera_extrinsics", extrinsics);
		gl_render_pass_uniform_mat3_array(renderer.auto_mask_pass, "u_camera_intrinsics", intrinsics);
		gl_render_pass_uniform_vec2_array(renderer.auto_mask_pass, "u_camera_resolutions", resolutions);
		gl_render_pass_uniform_int(
			renderer.auto_mask_pass, "u_num_cameras", static_cast<GLint>(renderer.camera_data.size()));

		if (renderer.depth_texture_array != 0) {
			gl_render_pass_uniform_texture_array(renderer.auto_mask_pass, "u_tex_camera_depth",
				renderer.depth_texture_array, GL_TEXTURE3);
		}
		if (renderer.confidence_texture_array != 0) {
			gl_render_pass_uniform_texture_array(renderer.auto_mask_pass, "u_tex_camera_confidence",
				renderer.confidence_texture_array, GL_TEXTURE4);
		}
	}

	gl_render_pass_draw(renderer.auto_mask_pass, quad_vbo);
	gl_render_pass_end(renderer.auto_mask_pass);
}

gl_render_pass_t* auto_mask_renderer_get_final_render_pass(auto_mask_renderer_t& renderer) {
	return &renderer.auto_mask_pass;
}

GLuint auto_mask_renderer_get_final_fbo_texture(auto_mask_renderer_t& renderer) {
	return renderer.auto_mask_pass.internal_output_descriptors["o_color"].texture;
}

GLuint auto_mask_renderer_get_final_fbo_width(auto_mask_renderer_t& renderer) {
	return renderer.auto_mask_pass.width;
}

GLuint auto_mask_renderer_get_final_fbo_height(auto_mask_renderer_t& renderer) {
	return renderer.auto_mask_pass.height;
}
