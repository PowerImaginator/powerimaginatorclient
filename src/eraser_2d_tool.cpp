#include "pch.h"

#include "alignment_tool.h"
#include "eraser_2d_tool.h"
#include "exchange.h"
#include "gl_render_pass.h"

void eraser_2d_tool_t::init(app_t& app) {
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
	gl_render_pass_init(final_preview_pass, "shaders/quad.vert", "shaders/eraser_2d_preview.frag",
		g_exchange.renderer_internal_width, g_exchange.renderer_internal_height,
		{
			{"o_dest", {.internal_format = GL_RGBA8, .format = GL_RGBA, .type = GL_UNSIGNED_BYTE}},
		});

	glGenTextures(1, &depth_tex);
	assert_release(depth_tex != 0);
	reset();
}

void eraser_2d_tool_t::update(app_t& app, f64 const dt) {
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

	gl_render_pass_begin(final_preview_pass);
	gl_render_pass_uniform_texture(final_preview_pass, "u_tex_color",
		app.tools["paint_result"]->get_viewport_texture(app, 0), GL_TEXTURE0);
	gl_render_pass_uniform_texture(final_preview_pass, "u_tex_depth", depth_tex, GL_TEXTURE1);
	gl_render_pass_uniform_texture(
		final_preview_pass, "u_tex_mask", app.tools["paint"]->get_viewport_texture(app, 1), GL_TEXTURE2);
	gl_render_pass_uniform_texture(final_preview_pass, "u_tex_eraser",
		copy_pass.internal_output_descriptors["o_dest"].texture, GL_TEXTURE3);
	gl_render_pass_uniform_vec2(
		final_preview_pass, "u_viewport_size", glm::vec2(final_preview_pass.width, final_preview_pass.height));
	gl_render_pass_uniform_float(final_preview_pass, "u_min_depth", min_depth);
	gl_render_pass_uniform_float(final_preview_pass, "u_max_depth", max_depth);
	gl_render_pass_draw(final_preview_pass, app.quad_vbo);
	gl_render_pass_end(final_preview_pass);
}

void eraser_2d_tool_t::update_settings(app_t& app, f64 const dt) {
	UNUSED(dt);

	if (ImGui::Button("Back")) {
		navigate_back(app);
	}

	ImGui::Separator();

	ImGui::TextWrapped(
		"Please click and drag to erase unwanted content. Use the Min/Max Depth sliders below to filter by "
		"distance - removing far away objects may help maintain accuracy.");

	ImGui::SliderFloat("Brush Radius", &brush_radius, 4.0f, 256.0f);

	static int radio_selection = 1;
	ImGui::RadioButton("Erase", &radio_selection, 1);
	ImGui::SameLine();
	ImGui::RadioButton("Keep", &radio_selection, 0);
	brush_strength = radio_selection == 1 ? 1.0f : 0.0f;
	ImGui::SameLine();
	if (ImGui::Button("Keep All")) {
		clear();
	}

	ImGui::DragFloat("Min Depth", &min_depth, 0.01f, 0.0f, 100.0f);
	ImGui::DragFloat("Max Depth", &max_depth, 0.01f, 0.0f, 100.0f);

	if (ImGui::Button("Next")) {
		if (app.cur_chunk_id == 0) {
			app_inpaint_visualize(app);
			app_inpaint_add_to_world(app);
		} else {
			static_cast<alignment_tool_t*>(app.tools["alignment"].get())->import_cur_chunk(app);
			app_navigate(app, "alignment", true);
		}
	}
}

void eraser_2d_tool_t::destroy() {
	//
}

u32 eraser_2d_tool_t::get_num_viewport_layers(app_t& app) {
	UNUSED(app);

	if (viewport_mouse_hovered) {
		return 2;
	} else {
		return 1;
	}
}

GLuint eraser_2d_tool_t::get_viewport_texture(app_t& app, u32 const layer) {
	UNUSED(app);
	if (layer == 0) {
		return final_preview_pass.internal_output_descriptors["o_dest"].texture;
	} else if (layer == 1) {
		return brush_preview_pass.internal_output_descriptors["o_dest"].texture;
	} else {
		assert_release(false);
	}
}

GLuint eraser_2d_tool_t::get_viewport_texture_width(app_t& app, u32 const layer) {
	UNUSED(app);
	if (layer == 0) {
		return final_preview_pass.width;
	} else if (layer == 1) {
		return brush_preview_pass.width;
	} else {
		assert_release(false);
	}
}

GLuint eraser_2d_tool_t::get_viewport_texture_height(app_t& app, u32 const layer) {
	UNUSED(app);
	if (layer == 0) {
		return final_preview_pass.height;
	} else if (layer == 1) {
		return brush_preview_pass.height;
	} else {
		assert_release(false);
	}
}

ImVec4 eraser_2d_tool_t::get_viewport_tint(app_t& app, u32 const layer) {
	UNUSED(app);
	if (layer == 0) {
		return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	} else if (layer == 1) {
		return ImVec4(0.0f, 0.0f, 1.0f, 0.5f);
	} else {
		assert_release(false);
	}
}

std::string eraser_2d_tool_t::get_name() {
	return "Eraser (2D)";
}

void eraser_2d_tool_t::navigate_back(app_t& app) {
	app_navigate(app, "paint_result", false);
	reset();
}

void eraser_2d_tool_t::import_cur_chunk(app_t& app) {
	std::vector<f32> const& depths = *exchange_load_depth_out(g_exchange, app.cur_chunk_id,
		g_exchange.renderer_internal_width, g_exchange.renderer_internal_height, 1);
	glBindTexture(GL_TEXTURE_2D, depth_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, g_exchange.renderer_internal_width, g_exchange.renderer_internal_height,
		0, GL_RED, GL_FLOAT, depths.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void eraser_2d_tool_t::clear() {
	gl_render_pass_clear_with_override(copy_pass, "o_dest", glm::vec4(0.0f), 1.0f);
}

void eraser_2d_tool_t::reset() {
	clear();
	min_depth = DEFAULT_MIN_DEPTH;
	max_depth = DEFAULT_MAX_DEPTH;

	glBindTexture(GL_TEXTURE_2D, depth_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 0, 0, 0, GL_RED, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
}
