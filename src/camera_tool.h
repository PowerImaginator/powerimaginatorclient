#pragma once

#include "pch.h"

#include "app.h"
#include "fly_camera.h"
#include "gl_render_pass.h"
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
	void run_generation(app_t& app);

	std::string prompt = "A comfortable living room, elegant interior design";
	std::string negative_prompt = "BadDream, (UnrealisticDream:1.2)";
	u32 seed = 3;
	u32 num_inference_steps = 30;
	f32 strength = 1.0f;
	f32 confidence_threshold = 5.0f;
	bool debug_save_bake_inputs = false;

	points_renderer_t points_renderer;
	fly_camera_t camera;
	fly_camera_t camera_backup;

	gl_render_pass_t bake_co3ne_pass;
	gl_render_pass_t bake_mask_pass;
	gl_render_pass_t bake_combined_pass;

private:
	bool has_visible_scene_content();
	void bake_co3ne();
	void bake_mask(app_t& app);
	void bake_combined(app_t& app);
	void bake(app_t& app);
};
