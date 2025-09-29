#pragma once

#include "pch.h"

#include "app.h"
#include "gl_render_pass.h"

#define DEFAULT_MIN_DEPTH 0.01f
#define DEFAULT_MAX_DEPTH 15.0f

class eraser_2d_tool_t : public app_tool_t {
public:
	eraser_2d_tool_t() {
		//
	}
	virtual ~eraser_2d_tool_t() {
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
	virtual ImVec4 get_viewport_tint(app_t& app, u32 const layer);

	virtual std::string get_name();

	void import_cur_chunk(app_t& app);

	void clear();
	void reset();

	gl_render_pass_t brush_pass;
	gl_render_pass_t brush_preview_pass;
	gl_render_pass_t copy_pass;
	gl_render_pass_t final_preview_pass;
	f32 brush_radius = 16.0f;
	f32 brush_strength = 1.0f;
	f32 min_depth = DEFAULT_MIN_DEPTH;
	f32 max_depth = DEFAULT_MAX_DEPTH;

	GLuint depth_tex = 0;

private:
	void navigate_back(app_t& app);
};
