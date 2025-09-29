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

	ImGui::BeginDisabled(true);
	ImGui::Button("Back");
	ImGui::EndDisabled();

	ImGui::Separator();

	ImGui::TextWrapped(
		"Hold your left mouse button inside the viewport, then use WASD to move and drag your mouse to "
		"look around. Once you've chosen the angle you want to inpaint, click Next.");

	if (ImGui::Button("Next")) {
		backup_camera(); // IMPORTANT: Needs to come before app_navigate which restores camera
		app_navigate(app, "paint", false);
	}

	if (ImGui::CollapsingHeader("Advanced")) {
		ImGui::TextWrapped("Adjusting these settings may help if you see holes in objects which should "
				   "be rendered solid.");
		ImGui::DragFloat("u_s0", &points_renderer.u_s0, 0.0001f, 0.0f, 1.0f);
		ImGui::DragFloat("u_occlusion_threshold", &points_renderer.u_occlusion_threshold, 0.01f, 0.0f, 1.0f);
		ImGui::SliderInt(
			"u_coarse_level", &points_renderer.u_coarse_level, 0, points_renderer_t::MAX_LEVEL - 1);
	}

	ImGui::Separator();

	ImGui::TextWrapped("Once you're finished creating the scene, export a PLY file for further editing in tools "
			   "like MeshLab and Blender.");

	if (ImGui::Button("Export PLY")) {
		app_export_ply(app);
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
	}
}

GLuint camera_tool_t::get_viewport_texture_width(app_t& app, u32 const layer) {
	UNUSED(app);

	if (layer == 0) {
		return points_renderer_get_final_fbo_width(points_renderer);
	} else {
		assert_release(false);
	}

	return 0;
}

GLuint camera_tool_t::get_viewport_texture_height(app_t& app, u32 const layer) {
	UNUSED(app);

	if (layer == 0) {
		return points_renderer_get_final_fbo_height(points_renderer);
	} else {
		assert_release(false);
	}

	return 0;
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
