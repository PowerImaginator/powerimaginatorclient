#pragma once

#include "pch.h"

#include "app.h"
#include "exchange.h"
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

	std::string prompt = "Repair this image of a comfortable living room, elegant interior design. Fill green "
			     "masked regions with new content.";
	std::string negative_prompt = "";
	exchange_edit_model_t model = exchange_edit_model_t::FluxKlein9B;
	exchange_qwen_acceleration_t qwen_acceleration = exchange_qwen_acceleration_t::Regular;
	u32 seed = 3;
	u32 num_inference_steps = 8;
	f32 guidance_scale = 4.5f;
	bool enable_safety_checker = true;
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
