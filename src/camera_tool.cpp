#include "io_utils.h"
#include "pch.h"

#include "app.h"
#include "camera_tool.h"
#include "exchange.h"
#include "fly_camera.h"
#include "points_renderer.h"
#include "paint_result_tool.h"

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

	points_renderer_render(points_renderer, camera, app.points_vbo, app.quad_vbo, confidence_threshold);
}

void camera_tool_t::update_settings(app_t& app, f64 const dt) {
	UNUSED(dt);

	ImGui::BeginDisabled(true);
	ImGui::Button("Back");
	ImGui::EndDisabled();

	ImGui::Separator();

	ImGui::TextWrapped(
		"Hold your left mouse button inside the viewport, then use WASD to move and drag your mouse to "
		"look around. Once you've chosen the angle you want to edit, click Next.");

	char const* model_labels[] = {"Qwen Image Edit 2511", "Flux Klein 4B", "Flux Klein 9B"};
	int selected_model = static_cast<int>(model);
	if (ImGui::Combo("Model", &selected_model, model_labels, IM_ARRAYSIZE(model_labels))) {
		model = static_cast<exchange_edit_model_t>(selected_model);
	}

	ImGui::InputText("Prompt", &prompt);
	if (model == exchange_edit_model_t::QwenImageEdit2511) {
		char const* acceleration_labels[] = {"none", "regular", "high"};
		int selected_acceleration = static_cast<int>(qwen_acceleration);
		if (ImGui::Combo("Qwen Acceleration", &selected_acceleration, acceleration_labels,
				IM_ARRAYSIZE(acceleration_labels))) {
			qwen_acceleration = static_cast<exchange_qwen_acceleration_t>(selected_acceleration);
		}

		ImGui::InputText("Negative Prompt", &negative_prompt);
	}

	ImGui::InputScalar("Seed", ImGuiDataType_U32, &seed);
	ImGui::SameLine();
	if (ImGui::Button("Random")) {
		seed = random_u32();
	}
	int steps_int = static_cast<int>(num_inference_steps);
	ImGui::SliderInt("Steps", &steps_int, 1, 50);
	num_inference_steps = static_cast<u32>(steps_int);

	ImGui::Separator();

	if (ImGui::Button("Next")) {
		run_generation(app);
	}

	if (ImGui::CollapsingHeader("Advanced")) {
		ImGui::SeparatorText("Generation");
		ImGui::SliderFloat("Confidence Threshold", &confidence_threshold, 0.0f, 20.0f);
		if (model == exchange_edit_model_t::QwenImageEdit2511) {
			ImGui::SliderFloat("Guidance Scale", &guidance_scale, 1.0f, 20.0f);
		}
		ImGui::Checkbox("Enable Safety Checker", &enable_safety_checker);
		ImGui::Checkbox("Debug Save Bake Inputs", &debug_save_bake_inputs);
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

void camera_tool_t::run_generation(app_t& app) {
	backup_camera();

	if (!has_visible_scene_content()) {
		std::vector<u8> color_rgb;
		std::vector<u8> mask;
		color_rgb.assign(static_cast<size_t>(RENDERER_INTERNAL_WIDTH) * static_cast<size_t>(RENDERER_INTERNAL_HEIGHT) * 3,
			0);
		mask.assign(static_cast<size_t>(RENDERER_INTERNAL_WIDTH) * static_cast<size_t>(RENDERER_INTERNAL_HEIGHT), 255);

		if (ENV_DEBUG_SAVE_EXCHANGE_IMAGES != 0 || debug_save_bake_inputs) {
			write_image("exchange/color.png", RENDERER_INTERNAL_WIDTH, RENDERER_INTERNAL_HEIGHT, 3,
				color_rgb.data(), RENDERER_INTERNAL_WIDTH * 3);
			write_image("exchange/mask.png", RENDERER_INTERNAL_WIDTH, RENDERER_INTERNAL_HEIGHT, 1, mask.data(),
				RENDERER_INTERNAL_WIDTH);
		}

		exchange_set_bake_inputs(g_exchange, RENDERER_INTERNAL_WIDTH, RENDERER_INTERNAL_HEIGHT, color_rgb, mask);
		exchange_run_image_edit(g_exchange, model, prompt, negative_prompt, seed, num_inference_steps,
			guidance_scale, qwen_acceleration, enable_safety_checker);

		static_cast<paint_result_tool_t*>(app.tools["paint_result"].get())
			->import_image(g_exchange.paint_result.colors, g_exchange.paint_result.width,
				g_exchange.paint_result.height);
		app_navigate(app, "paint_result");
		return;
	}

	bake(app);

	std::vector<u8> combined_rgba;
	gl_render_pass_download(bake_combined_pass, "o_dest", combined_rgba);
	std::vector<u8> combined_rgba_flipped;
	flip_image_y(combined_rgba_flipped, combined_rgba, RENDERER_INTERNAL_WIDTH, RENDERER_INTERNAL_HEIGHT, 4);

	std::vector<u8> color_rgb;
	std::vector<u8> mask;
	color_rgb.resize(static_cast<size_t>(RENDERER_INTERNAL_WIDTH) * static_cast<size_t>(RENDERER_INTERNAL_HEIGHT) * 3);
	mask.resize(static_cast<size_t>(RENDERER_INTERNAL_WIDTH) * static_cast<size_t>(RENDERER_INTERNAL_HEIGHT));

	for (size_t i = 0; i < mask.size(); ++i) {
		u8 r = combined_rgba_flipped[i * 4 + 0];
		u8 g = combined_rgba_flipped[i * 4 + 1];
		u8 b = combined_rgba_flipped[i * 4 + 2];
		u8 a = combined_rgba_flipped[i * 4 + 3];

		color_rgb[i * 3 + 0] = r;
		color_rgb[i * 3 + 1] = g;
		color_rgb[i * 3 + 2] = b;

		bool is_mask_pixel = a < 255;
		mask[i] = is_mask_pixel ? 255 : 0;
	}

	bool all_pixels_masked = true;
	for (u8 const value : mask) {
		if (value == 0) {
			all_pixels_masked = false;
			break;
		}
	}

	if (all_pixels_masked) {
		for (u8& value : color_rgb) {
			value = 0;
		}
	}

	if (ENV_DEBUG_SAVE_EXCHANGE_IMAGES != 0 || debug_save_bake_inputs) {
		write_image("exchange/color.png", RENDERER_INTERNAL_WIDTH, RENDERER_INTERNAL_HEIGHT, 3, color_rgb.data(),
			RENDERER_INTERNAL_WIDTH * 3);
		write_image("exchange/mask.png", RENDERER_INTERNAL_WIDTH, RENDERER_INTERNAL_HEIGHT, 1, mask.data(),
			RENDERER_INTERNAL_WIDTH);
	}

	exchange_set_bake_inputs(g_exchange, RENDERER_INTERNAL_WIDTH, RENDERER_INTERNAL_HEIGHT, color_rgb, mask);
	exchange_run_image_edit(g_exchange, model, prompt, negative_prompt, seed, num_inference_steps, guidance_scale,
		qwen_acceleration, enable_safety_checker);

	static_cast<paint_result_tool_t*>(app.tools["paint_result"].get())
		->import_image(g_exchange.paint_result.colors, g_exchange.paint_result.width, g_exchange.paint_result.height);
	app_navigate(app, "paint_result");
}

bool camera_tool_t::has_visible_scene_content() {
	std::vector<f32> world_pos_buffer;
	gl_render_pass_download(points_renderer.points_pass, "o_world_pos", world_pos_buffer);

	for (size_t i = 0; i + 3 < world_pos_buffer.size(); i += 4) {
		if (world_pos_buffer[i + 3] > 0.0f) {
			return true;
		}
	}

	return false;
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
	gl_render_pass_uniform_float(bake_mask_pass, "u_confidence_threshold", confidence_threshold);

	// Bind VGGT camera data for auto-unmasking during baking.
	// The shader caps at 16 cameras.
	u32 const max_cameras = 16;
	u32 const num_cameras = static_cast<u32>(std::min<size_t>(app.camera_data.size(), max_cameras));
	gl_render_pass_uniform_int(bake_mask_pass, "u_num_cameras", static_cast<GLint>(num_cameras));

	if (num_cameras > 0 && app.vggt_depth_tex_array && app.vggt_conf_tex_array) {
		std::vector<glm::mat4> cam_to_world;
		std::vector<glm::mat4> world_to_cam;
		std::vector<glm::mat3> intrinsics;
		std::vector<glm::vec2> resolutions;
		cam_to_world.reserve(num_cameras);
		world_to_cam.reserve(num_cameras);
		intrinsics.reserve(num_cameras);
		resolutions.reserve(num_cameras);

		for (u32 i = 0; i < num_cameras; ++i) {
			auto const& cam = app.camera_data[i];
			cam_to_world.push_back(cam.extrinsic);
			world_to_cam.push_back(glm::inverse(cam.extrinsic));
			intrinsics.push_back(cam.intrinsic);
			resolutions.push_back(glm::vec2(static_cast<f32>(cam.width), static_cast<f32>(cam.height)));
		}

		gl_render_pass_uniform_mat4_array(bake_mask_pass, "u_camera_extrinsics", cam_to_world);
		gl_render_pass_uniform_mat4_array(bake_mask_pass, "u_camera_world_to_cam", world_to_cam);
		gl_render_pass_uniform_mat3_array(bake_mask_pass, "u_camera_intrinsics", intrinsics);
		gl_render_pass_uniform_vec2_array(bake_mask_pass, "u_camera_resolutions", resolutions);

		gl_render_pass_uniform_texture_array(
			bake_mask_pass, "u_tex_camera_depth", app.vggt_depth_tex_array, GL_TEXTURE4);
		gl_render_pass_uniform_texture_array(
			bake_mask_pass, "u_tex_camera_confidence", app.vggt_conf_tex_array, GL_TEXTURE5);
	}

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
}
