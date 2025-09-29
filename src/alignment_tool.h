#pragma once

#include "pch.h"

#include "app.h"
#include "gl_render_pass.h"

class alignment_tool_t : public app_tool_t {
public:
	alignment_tool_t() {
		//
	}
	virtual ~alignment_tool_t() {
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
	virtual bool get_wants_mouse_capture(app_t& app);

	virtual std::string get_name();

	void import_cur_chunk(app_t& app);

	void reset();

	std::vector<glm::ivec2> keypoints;

private:
	void update_hovered_keypoint(app_t& app);
	void add_hovered_keypoint(app_t& app);
	void remove_last_keypoint(app_t& app);
	void remove_all_keypoints(app_t& app);
	void on_keypoints_changed(app_t& app);

	glm::ivec2 hovered_keypoint;
	std::vector<u8> colors;
	std::vector<f32> world_pos;
	std::vector<u8> mask;
	u32 width = 0, height = 0;
	GLuint keypoints_tex = 0;
	GLuint color_tex = 0;
	GLuint world_pos_tex = 0;
	GLuint mask_tex = 0;
	gl_render_pass_t color_pass;
	gl_render_pass_t keypoints_pass;
	bool lock_existing_points = false;
};
