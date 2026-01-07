#pragma once

#include "pch.h"

#include "gl_vertex_buffers.h"
#include "points_renderer.h"

struct app_t;

class app_tool_t {
public:
	app_tool_t() {
		//
	}
	virtual ~app_tool_t() {
		//
	}

	virtual void init(app_t& app) = 0;
	virtual void update(app_t& app, f64 const dt) = 0;
	virtual void update_settings(app_t& app, f64 const dt) = 0;
	virtual void destroy() = 0;

	virtual u32 get_num_viewport_layers(app_t& app) = 0;
	virtual GLuint get_viewport_texture(app_t& app, u32 const layer) = 0;
	virtual GLuint get_viewport_texture_width(app_t& app, u32 const layer) = 0;
	virtual GLuint get_viewport_texture_height(app_t& app, u32 const layer) = 0;
	virtual ImVec4 get_viewport_tint(app_t& app, u32 const layer) {
		UNUSED(app);
		UNUSED(layer);
		return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	virtual bool get_wants_mouse_capture(app_t& app) {
		UNUSED(app);
		return true;
	}

	virtual std::string get_name() = 0;

	ImVec2 viewport_mouse_pos;
	ImVec2 viewport_mouse_delta;
	bool viewport_mouse_captured = false;
	bool viewport_mouse_hovered = false;
	bool viewport_mouse_captured_next_frame = false;
	bool viewport_mouse_clicked = false;
};

struct app_t {
	GLFWwindow* window = nullptr;
	gl_vertex_buffers_t quad_vbo;
	std::unordered_map<std::string, std::unique_ptr<app_tool_t>> tools;
	std::string tour_active_tool;

	gl_vertex_buffers_t points_vbo;
	gl_vertex_buffers_t mask_vbo;
	std::vector<vggt_camera_data_t> camera_data;

	// GPU copies of the VGGT per-camera buffers (indexed by camera index).
	// These are allocated as texture arrays to match the shader sampling logic.
	GLuint vggt_depth_tex_array = 0; // GL_TEXTURE_2D_ARRAY, GL_R32F
	GLuint vggt_conf_tex_array = 0; // GL_TEXTURE_2D_ARRAY, GL_R32F
	u32 vggt_tex_width = 0;
	u32 vggt_tex_height = 0;
};

void app_init(app_t& app);
void app_update(app_t& app, f64 const dt);
void app_imgui(app_t& app, f64 const dt);
void app_shutdown(app_t& app);

void app_load_vggt_output(app_t& app, std::string const& filename);
