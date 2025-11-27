#include "pch.h"

#include "app.h"
#include "fly_camera.h"
#include "mesh_camera_tool.h"
#include "mesh_renderer.h"

void mesh_camera_tool_t::init(app_t& app) {
	mesh_renderer_init(mesh_renderer);
	load_vggt_mesh(ENV_VGGT_OUTPUT_SOURCE, app.mesh_vbo, 0.1f, mesh_renderer.camera_data);
	mesh_renderer_upload_camera_textures(mesh_renderer, mesh_renderer.camera_data);
}

void mesh_camera_tool_t::update(app_t& app, f64 const dt) {
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
		static_cast<f32>(mesh_renderer.mesh_pass.width) / static_cast<f32>(mesh_renderer.mesh_pass.height),
		0.1f, 100.0f);

	mesh_renderer_render(mesh_renderer, camera, app.mesh_vbo, app.quad_vbo);
}

void mesh_camera_tool_t::update_settings(app_t& app, f64 const dt) {
	UNUSED(app);
	UNUSED(dt);

	ImGui::TextWrapped(
		"Hold your left mouse button inside the viewport, then use WASD to move and drag your mouse to "
		"look around.");
}

void mesh_camera_tool_t::destroy() {
	//
}

u32 mesh_camera_tool_t::get_num_viewport_layers(app_t& app) {
	UNUSED(app);

	return 1;
}

GLuint mesh_camera_tool_t::get_viewport_texture(app_t& app, u32 const layer) {
	UNUSED(app);

	if (layer == 0) {
		return mesh_renderer_get_final_fbo_texture(mesh_renderer);
	} else {
		assert_release(false);
		return 0;
	}
}

GLuint mesh_camera_tool_t::get_viewport_texture_width(app_t& app, u32 const layer) {
	UNUSED(app);

	if (layer == 0) {
		return mesh_renderer_get_final_fbo_width(mesh_renderer);
	} else {
		assert_release(false);
		return 0;
	}
}

GLuint mesh_camera_tool_t::get_viewport_texture_height(app_t& app, u32 const layer) {
	UNUSED(app);

	if (layer == 0) {
		return mesh_renderer_get_final_fbo_height(mesh_renderer);
	} else {
		assert_release(false);
		return 0;
	}
}

std::string mesh_camera_tool_t::get_name() {
	return "Mesh Camera";
}

void mesh_camera_tool_t::backup_camera() {
	camera_backup = camera;
}

void mesh_camera_tool_t::restore_camera() {
	camera = camera_backup;
}
