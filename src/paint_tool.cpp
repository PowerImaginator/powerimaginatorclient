#include "pch.h"

#include "camera_tool.h"
#include "exchange.h"
#include "gl_render_pass.h"
#include "io_utils.h"
#include "paint_tool.h"

void paint_tool_t::init(app_t& app) {
	UNUSED(app);

	gl_render_pass_init(brush_pass, "shaders/quad.vert", "shaders/paint_brush.frag",
		g_exchange.renderer_internal_width, g_exchange.renderer_internal_height,
		{
			{"o_dest", {.internal_format = GL_RGBA8, .format = GL_RGBA, .type = GL_UNSIGNED_BYTE}},
		});
	gl_render_pass_init(brush_preview_pass, "shaders/quad.vert", "shaders/paint_brush.frag",
		g_exchange.renderer_internal_width, g_exchange.renderer_internal_height,
		{
			{"o_dest", {.internal_format = GL_RGBA8, .format = GL_RGBA, .type = GL_UNSIGNED_BYTE}},
		});
	gl_render_pass_init(copy_pass, "shaders/quad.vert", "shaders/copy.frag", g_exchange.renderer_internal_width,
		g_exchange.renderer_internal_height,
		{
			{"o_dest", {.internal_format = GL_RGBA8, .format = GL_RGBA, .type = GL_UNSIGNED_BYTE}},
		});
}

void paint_tool_t::update(app_t& app, f64 const dt) {
	UNUSED(app);
	UNUSED(dt);

	if (viewport_mouse_hovered) {
		brush_radius += ImGui::GetIO().MouseWheel;
		brush_radius = std::min(std::max(brush_radius, 4.0f), 256.0f);

		gl_render_pass_begin(brush_preview_pass);
		gl_render_pass_uniform_bool(brush_preview_pass, "u_use_tex_source", false);
		gl_render_pass_uniform_vec2(brush_preview_pass, "u_viewport_size",
			glm::vec2(brush_preview_pass.width, brush_preview_pass.height));
		gl_render_pass_uniform_vec2(
			brush_preview_pass, "u_brush_pos", glm::vec2(viewport_mouse_pos.x, viewport_mouse_pos.y));
		gl_render_pass_uniform_float(brush_preview_pass, "u_brush_radius", brush_radius);
		gl_render_pass_uniform_float(brush_preview_pass, "u_brush_strength", 1.0f /* brush_strength */);
		gl_render_pass_draw(brush_preview_pass, app.quad_vbo);
		gl_render_pass_end(brush_preview_pass);
	}

	if (viewport_mouse_captured) {
		gl_render_pass_begin(brush_pass);
		gl_render_pass_uniform_bool(brush_pass, "u_use_tex_source", true);
		gl_render_pass_uniform_texture(brush_pass, "u_tex_source",
			copy_pass.internal_output_descriptors["o_dest"].texture, GL_TEXTURE0);
		gl_render_pass_uniform_vec2(
			brush_pass, "u_viewport_size", glm::vec2(brush_pass.width, brush_pass.height));
		gl_render_pass_uniform_vec2(
			brush_pass, "u_brush_pos", glm::vec2(viewport_mouse_pos.x, viewport_mouse_pos.y));
		gl_render_pass_uniform_float(brush_pass, "u_brush_radius", brush_radius);
		gl_render_pass_uniform_float(brush_pass, "u_brush_strength", brush_strength);
		gl_render_pass_draw(brush_pass, app.quad_vbo);
		gl_render_pass_end(brush_pass);

		gl_render_pass_begin(copy_pass);
		gl_render_pass_uniform_texture(copy_pass, "u_tex_source",
			brush_pass.internal_output_descriptors["o_dest"].texture, GL_TEXTURE0);
		gl_render_pass_draw(copy_pass, app.quad_vbo);
		gl_render_pass_end(copy_pass);
	}
}

void paint_tool_t::update_settings(app_t& app, f64 const dt) {
	UNUSED(dt);

	if (ImGui::Button("Back")) {
		navigate_back(app);
	}

	ImGui::Separator();

	ImGui::TextWrapped("Please click and drag to paint over any empty or noticeably blurry areas in this image.");

	ImGui::InputText("Prompt", &prompt);
	ImGui::InputText("Negative Prompt", &negative_prompt);
	ImGui::InputScalar("Seed", ImGuiDataType_U32, &seed);
	ImGui::SameLine();
	if (ImGui::Button("Random")) {
		seed = random_u32();
	}
	int steps_int = static_cast<int>(num_inference_steps);
	ImGui::SliderInt("Steps", &steps_int, 1, 50);
	num_inference_steps = static_cast<u32>(steps_int);

	ImGui::SliderFloat("Brush Radius", &brush_radius, 4.0f, 256.0f);

	if (g_exchange.renderer_allow_brush_strength) {
		ImGui::SliderFloat("Brush Strength", &brush_strength, 0.0f, 1.0f);
		if (brush_strength < 0.01f) {
			brush_strength = 0.0f;
		}
	} else {
		static int radio_selection = 1;
		ImGui::RadioButton("Paint", &radio_selection, 1);
		ImGui::SameLine();
		ImGui::RadioButton("Erase", &radio_selection, 0);
		brush_strength = radio_selection == 1 ? 1.0f : 0.0f;
		ImGui::SameLine();
	}
	if (ImGui::Button("Erase All")) {
		clear();
	}

	if (ImGui::Button("Generate")) {
		app_inpaint_export(app);
		exchange_run_inpainting(
			g_exchange, app.cur_chunk_id, prompt, negative_prompt, seed, num_inference_steps, strength);
		app_inpaint_import_paint_result(app);
		app_navigate(app, "paint_result", false);
	}

	if (ImGui::CollapsingHeader("Advanced")) {
		ImGui::TextWrapped(
			"Depending on the server, you may be able to achieve an effect similar to upscaling "
			"by setting this to a value less than 1.0 and painting over the region you want to "
			"refine. If you're trying to generate new content, you should probably leave this at 1.0.");
		ImGui::SliderFloat("Global Strength", &strength, 0.0f, 1.0f);
		if (strength > 0.99f) {
			strength = 1.0f;
		}
	}
}

void paint_tool_t::destroy() {
	//
}

u32 paint_tool_t::get_num_viewport_layers(app_t& app) {
	UNUSED(app);

	if (viewport_mouse_hovered) {
		return 3;
	} else {
		return 2;
	}
}

GLuint paint_tool_t::get_viewport_texture(app_t& app, u32 const layer) {
	if (layer == 0) {
		return app.tools["camera"]->get_viewport_texture(app, 0);
	} else if (layer == 1) {
		return copy_pass.internal_output_descriptors["o_dest"].texture;
	} else if (layer == 2) {
		return brush_preview_pass.internal_output_descriptors["o_dest"].texture;
	} else {
		assert_release(false);
	}
}

GLuint paint_tool_t::get_viewport_texture_width(app_t& app, u32 const layer) {
	UNUSED(app);
	if (layer == 0) {
		return app.tools["camera"]->get_viewport_texture_width(app, 0);
	} else if (layer == 1) {
		return copy_pass.width;
	} else if (layer == 2) {
		return brush_preview_pass.width;
	} else {
		assert_release(false);
	}
}

GLuint paint_tool_t::get_viewport_texture_height(app_t& app, u32 const layer) {
	UNUSED(app);
	if (layer == 0) {
		return app.tools["camera"]->get_viewport_texture_height(app, 0);
	} else if (layer == 1) {
		return copy_pass.height;
	} else if (layer == 2) {
		return brush_preview_pass.height;
	} else {
		assert_release(false);
	}
}

ImVec4 paint_tool_t::get_viewport_tint(app_t& app, u32 const layer) {
	UNUSED(app);
	if (layer == 0) {
		return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	} else if (layer == 1) {
		return ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
	} else if (layer == 2) {
		return ImVec4(0.0f, 0.0f, 1.0f, 0.5f);
	} else {
		assert_release(false);
	}
}

std::string paint_tool_t::get_name() {
	return "Paint";
}

void paint_tool_t::navigate_back(app_t& app) {
	static_cast<camera_tool_t*>(app.tools["camera"].get())->restore_camera();
	app_navigate(app, "camera", false);
	clear();
}

void paint_tool_t::clear() {
	gl_render_pass_clear_with_override(copy_pass, "o_dest", glm::vec4(0.0f), 1.0f);
}

void paint_tool_t::fill() {
	gl_render_pass_clear_with_override(copy_pass, "o_dest", glm::vec4(1.0f), 1.0f);
}
