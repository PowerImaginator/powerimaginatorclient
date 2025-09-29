#pragma once

#include "pch.h"

#include "app.h"
#include "fly_camera.h"
#include "points_renderer.h"

class camera_tool_t : public app_tool_t {
public:
	camera_tool_t() {
		//
	}
	virtual ~camera_tool_t() {
		//
	}

	virtual void init(app_t& app);
	virtual void update(app_t& app, f64 const dt);
	virtual void update_settings(app_t& app, f64 const dt);
	virtual void destroy();

	virtual u32 get_num_viewport_layers(app_t& app);
	virtual GLuint get_viewport_texture(app_t& app, u32 const layer);
	virtual GLuint get_viewport_texture_width(app_t& app, u32 const layer);
	virtual GLuint get_viewport_texture_height(app_t& app, u32 const layer);

	virtual std::string get_name();

	void backup_camera();
	void restore_camera();

	points_renderer_t points_renderer;
	fly_camera_t camera;
	fly_camera_t camera_backup;
};
