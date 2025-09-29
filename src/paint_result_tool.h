#pragma once

#include "pch.h"

#include "app.h"

class paint_result_tool_t : public app_tool_t {
public:
	paint_result_tool_t() {
		//
	}
	virtual ~paint_result_tool_t() {
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

	void import_image(std::vector<u8> const& colors, u32 const width, u32 const height, u32 const num_chans);
	void reset_image();

	GLuint colors_tex = 0;
	GLuint colors_tex_width = 0, colors_tex_height = 0;

private:
	void navigate_back(app_t& app);
};
