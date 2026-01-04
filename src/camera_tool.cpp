#include "io_utils.h"
#include "pch.h"

#include "app.h"
#include "camera_tool.h"
#include "fly_camera.h"
#include "points_renderer.h"

void camera_tool_t::init(app_t& app) {
	UNUSED(app);

	points_renderer_init(points_renderer);

	gl_render_pass_init(bake_co3ne_pass, "shaders/mesh_3d.vert", "shaders/mesh_3d.frag", RENDERER_INTERNAL_WIDTH,
		RENDERER_INTERNAL_HEIGHT,
		{{"o_color",
			 {.clear_color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
				 .internal_format = GL_RGBA8,
				 .format = GL_RGBA,
				 .type = GL_UNSIGNED_BYTE}},
			{"##DEPTH",
				{.clear_depth = 1.0f,
					.internal_format = GL_DEPTH_COMPONENT32F,
					.format = GL_DEPTH_COMPONENT,
					.type = GL_FLOAT}}});

	gl_render_pass_init(bake_mask_pass, "shaders/mask.vert", "shaders/mask.frag", RENDERER_INTERNAL_WIDTH,
		RENDERER_INTERNAL_HEIGHT,
		{{"o_color",
			 {.clear_color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
				 .internal_format = GL_RGBA8,
				 .format = GL_RGBA,
				 .type = GL_UNSIGNED_BYTE}},
			{"##DEPTH",
				{.clear_depth = 1.0f,
					.internal_format = GL_DEPTH_COMPONENT32F,
					.format = GL_DEPTH_COMPONENT,
					.type = GL_FLOAT}}});

	gl_render_pass_init(bake_combined_pass, "shaders/quad.vert", "shaders/combine_mask_mesh.frag",
		RENDERER_INTERNAL_WIDTH, RENDERER_INTERNAL_HEIGHT,
		{{"o_dest",
			{.clear_color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
				.internal_format = GL_RGBA8,
				.format = GL_RGBA,
				.type = GL_UNSIGNED_BYTE}}});
}

void camera_tool_t::update(app_t& app, f64 const dt) {
	if (viewport_mouse_captured) {
		fly_camera_mouse_update(camera, viewport_mouse_delta.x, viewport_mouse_delta.y);
		fly_camera_key(camera, GLFW_KEY_W,
			viewport_mouse_captured && glfwGetKey(app.window, GLFW_KEY_W) == GLFW_PRESS);
		fly_camera_key(camera, GLFW_KEY_S,
			viewport_mouse_captured && glfwGetKey(app.window, GLFW_KEY_S) == GLFW_PRESS);
		fly_camera_key(camera, GLFW_KEY_A,
			viewport_mouse_captured && glfwGetKey(app.window, GLFW_KEY_A) == GLFW_PRESS);
		fly_camera_key(camera, GLFW_KEY_D,
			viewport_mouse_captured && glfwGetKey(app.window, GLFW_KEY_D) == GLFW_PRESS);
		fly_camera_key(camera, GLFW_KEY_LEFT_SHIFT,
			viewport_mouse_captured && glfwGetKey(app.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
	} else {
		fly_camera_keys_reset(camera);
	}

	fly_camera_update(camera, static_cast<f32>(dt),
		static_cast<f32>(points_renderer.points_pass.width) /
			static_cast<f32>(points_renderer.points_pass.height),
		0.1f, 100.0f);

	points_renderer_render(points_renderer, camera, app.points_vbo, app.quad_vbo);
}

void camera_tool_t::update_settings(app_t& app, f64 const dt) {
	UNUSED(app);
	UNUSED(dt);

	ImGui::TextWrapped(
		"Hold your left mouse button inside the viewport, then use WASD to move and drag your mouse to "
		"look around.");

	ImGui::Separator();

	if (ImGui::Button("Bake!")) {
		bake(app);
	}
}

void camera_tool_t::destroy() {
	//
}

u32 camera_tool_t::get_num_viewport_layers(app_t& app) {
	UNUSED(app);

	return 1;
}

GLuint camera_tool_t::get_viewport_texture(app_t& app, u32 const layer) {
	UNUSED(app);

	if (layer == 0) {
		return points_renderer_get_final_fbo_texture(points_renderer);
	} else {
		assert_release(false);
		return 0;
	}
}

GLuint camera_tool_t::get_viewport_texture_width(app_t& app, u32 const layer) {
	UNUSED(app);

	if (layer == 0) {
		return points_renderer_get_final_fbo_width(points_renderer);
	} else {
		assert_release(false);
		return 0;
	}
}

GLuint camera_tool_t::get_viewport_texture_height(app_t& app, u32 const layer) {
	UNUSED(app);

	if (layer == 0) {
		return points_renderer_get_final_fbo_height(points_renderer);
	} else {
		assert_release(false);
		return 0;
	}
}

std::string camera_tool_t::get_name() {
	return "Camera";
}

void camera_tool_t::backup_camera() {
	camera_backup = camera;
}

void camera_tool_t::restore_camera() {
	camera = camera_backup;
}

void camera_tool_t::bake_co3ne() {
	std::vector<u8> color_buffer;
	gl_render_pass_download(points_renderer.points_pass, "o_color", color_buffer);
	std::vector<f32> world_pos_buffer;
	gl_render_pass_download(points_renderer.points_pass, "o_world_pos", world_pos_buffer);

	std::vector<u8> valid_points_color;
	std::vector<f64> valid_points_world_pos;
	for (size_t j = 0; j < RENDERER_INTERNAL_HEIGHT; ++j) {
		for (size_t i = 0; i < RENDERER_INTERNAL_WIDTH; ++i) {
			size_t base_idx = j * RENDERER_INTERNAL_WIDTH + i;
			if (world_pos_buffer[base_idx * 4 + 3] > 0.0f) {
				valid_points_world_pos.push_back(world_pos_buffer[base_idx * 4 + 0]);
				valid_points_world_pos.push_back(world_pos_buffer[base_idx * 4 + 1]);
				valid_points_world_pos.push_back(world_pos_buffer[base_idx * 4 + 2]);
				valid_points_color.push_back(color_buffer[base_idx * 3 + 0]);
				valid_points_color.push_back(color_buffer[base_idx * 3 + 1]);
				valid_points_color.push_back(color_buffer[base_idx * 3 + 2]);
			}
		}
	}

	GEO::Mesh mesh;
	size_t num_points = valid_points_world_pos.size() / 3;
	mesh.vertices.create_vertices(num_points);

	for (size_t i = 0; i < num_points; ++i) {
		mesh.vertices.point(i) = {valid_points_world_pos[i * 3 + 0], valid_points_world_pos[i * 3 + 1],
			valid_points_world_pos[i * 3 + 2]};
	}

	GEO::Attribute<GEO::vec3> colors(mesh.vertices.attributes(), "color");
	for (size_t i = 0; i < num_points; ++i) {
		colors[i] = GEO::vec3(valid_points_color[i * 3 + 0] / 255.0f, valid_points_color[i * 3 + 1] / 255.0f,
			valid_points_color[i * 3 + 2] / 255.0f);
	}

	// We may want to expose these parameters (especially radius) to the user, perhaps via IMGUI
	GEO::Co3Ne_smooth(mesh, 30, 2);
	GEO::Co3Ne_reconstruct(mesh, 0.01);

	std::vector<f32> positions;
	std::vector<f32> vertex_colors;

	for (GEO::index_t f = 0; f < mesh.facets.nb(); ++f) {
		if (mesh.facets.nb_vertices(f) == 3) {
			for (GEO::index_t lv = 0; lv < 3; ++lv) {
				GEO::index_t v = mesh.facets.vertex(f, lv);
				const double* pos = mesh.vertices.point_ptr(v);
				positions.push_back(pos[0]);
				positions.push_back(pos[1]);
				positions.push_back(pos[2]);
				GEO::vec3 col = colors[v];
				vertex_colors.push_back(col.x);
				vertex_colors.push_back(col.y);
				vertex_colors.push_back(col.z);
			}
		}
	}

	gl_vertex_buffers_t vbo;
	gl_vertex_buffers_upload(vbo, "a_position", positions, 3, GL_FLOAT, GL_FALSE);
	gl_vertex_buffers_upload(vbo, "a_color", vertex_colors, 3, GL_FLOAT, GL_FALSE);

	gl_render_pass_begin(bake_co3ne_pass);
	gl_render_pass_uniform_mat4(bake_co3ne_pass, "u_proj_mat", camera.proj_mat);
	gl_render_pass_uniform_mat4(bake_co3ne_pass, "u_view_mat", camera.view_mat);
	gl_render_pass_draw(bake_co3ne_pass, vbo);
	gl_render_pass_end(bake_co3ne_pass);
}

void camera_tool_t::bake_mask(app_t& app) {
	gl_render_pass_begin(bake_mask_pass);
	gl_render_pass_uniform_mat4(bake_mask_pass, "u_proj_mat", camera.proj_mat);
	gl_render_pass_uniform_mat4(bake_mask_pass, "u_view_mat", camera.view_mat);
	gl_render_pass_draw(bake_mask_pass, app.mask_vbo);
	gl_render_pass_end(bake_mask_pass);
}

void camera_tool_t::bake_combined(app_t& app) {
	GLuint mesh_color_texture = bake_co3ne_pass.internal_output_descriptors["o_color"].texture;
	GLuint mesh_depth_texture = bake_co3ne_pass.internal_output_descriptors["##DEPTH"].texture;
	GLuint mask_color_texture = bake_mask_pass.internal_output_descriptors["o_color"].texture;
	GLuint mask_depth_texture = bake_mask_pass.internal_output_descriptors["##DEPTH"].texture;

	gl_render_pass_begin(bake_combined_pass);
	gl_render_pass_uniform_texture(bake_combined_pass, "u_tex_mesh_color", mesh_color_texture, GL_TEXTURE0);
	gl_render_pass_uniform_texture(bake_combined_pass, "u_tex_mesh_depth", mesh_depth_texture, GL_TEXTURE1);
	gl_render_pass_uniform_texture(bake_combined_pass, "u_tex_mask_color", mask_color_texture, GL_TEXTURE2);
	gl_render_pass_uniform_texture(bake_combined_pass, "u_tex_mask_depth", mask_depth_texture, GL_TEXTURE3);
	gl_render_pass_draw(bake_combined_pass, app.quad_vbo);
	gl_render_pass_end(bake_combined_pass);
}

void camera_tool_t::bake(app_t& app) {
	bake_co3ne();
	bake_mask(app);
	bake_combined(app);

	std::vector<u8> combined_color_buffer;
	gl_render_pass_download(bake_combined_pass, "o_dest", combined_color_buffer);
	std::vector<u8> combined_color_buffer_flipped;
	flip_image_y(combined_color_buffer_flipped, combined_color_buffer, RENDERER_INTERNAL_WIDTH,
		RENDERER_INTERNAL_HEIGHT, 4);
	stbi_write_png("exchange/output.png", RENDERER_INTERNAL_WIDTH, RENDERER_INTERNAL_HEIGHT, 4,
		combined_color_buffer_flipped.data(), RENDERER_INTERNAL_WIDTH * 4);
}
