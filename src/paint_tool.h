#pragma once

#include "pch.h"

#include "app.h"
#include "gl_render_pass.h"

class paint_tool_t : public app_tool_t {
public:
	paint_tool_t() {
		//
	}
	virtual ~paint_tool_t() {
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

	void clear();
	void fill();

	std::string prompt = "A comfortable living room, elegant interior design";
	std::string negative_prompt = "BadDream, (UnrealisticDream:1.2)";
	u32 seed = 3;
	u32 num_inference_steps = 30;
	f32 strength = 1.0f;

	gl_render_pass_t brush_pass;
	gl_render_pass_t brush_preview_pass;
	gl_render_pass_t copy_pass;
	f32 brush_radius = 16.0f;
	f32 brush_strength = 1.0f;

private:
	void navigate_back(app_t& app);
};
