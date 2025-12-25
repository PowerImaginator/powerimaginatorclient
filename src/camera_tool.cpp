#include "pch.h"

#include "app.h"
#include "camera_tool.h"
#include "fly_camera.h"
#include "points_renderer.h"

void camera_tool_t::init(app_t& app) {
	UNUSED(app);

	points_renderer_init(points_renderer);
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
		std::vector<f32> world_pos_buffer;
		gl_render_pass_download(points_renderer.points_pass, "o_world_pos", world_pos_buffer);

		std::vector<f32> valid_points_world_pos;
		for (size_t i = 0; i < world_pos_buffer.size(); i += 4) {
			if (world_pos_buffer[i + 3] > 0.0f) {
				valid_points_world_pos.push_back(world_pos_buffer[i + 0]);
				valid_points_world_pos.push_back(world_pos_buffer[i + 1]);
				valid_points_world_pos.push_back(world_pos_buffer[i + 2]);
			}
		}

		std::cout << "Valid points: " << valid_points_world_pos.size() / 3 << std::endl;
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
