#include "pch.h"

#include "app.h"
#include "eraser_2d_tool.h"
#include "exchange.h"
#include "paint_result_tool.h"

void paint_result_tool_t::init(app_t& app) {
	UNUSED(app);

	glGenTextures(1, &colors_tex);
	assert_release(colors_tex != 0);
	reset_image();
}

void paint_result_tool_t::update(app_t& app, f64 const dt) {
	UNUSED(app);
	UNUSED(dt);
}

void paint_result_tool_t::update_settings(app_t& app, f64 const dt) {
	UNUSED(dt);

	if (ImGui::Button("Back")) {
		navigate_back(app);
	}

	ImGui::Separator();

	ImGui::TextWrapped("If you're satisfied with this image, click Next. Otherwise, click Back and try painting a "
			   "different mask or changing parameters.");

	if (ImGui::Button("Next")) {
		exchange_run_depth_estimation(g_exchange, app.cur_chunk_id);
		static_cast<eraser_2d_tool_t*>(app.tools["eraser_2d"].get())->import_cur_chunk(app);
		app_navigate(app, "eraser_2d", false);
	}
}

void paint_result_tool_t::destroy() {
	//
}

u32 paint_result_tool_t::get_num_viewport_layers(app_t& app) {
	UNUSED(app);

	return 1;
}

GLuint paint_result_tool_t::get_viewport_texture(app_t& app, u32 const layer) {
	UNUSED(app);

	if (layer == 0) {
		return colors_tex;
	} else {
		assert_release(false);
		return 0;
	}
}

GLuint paint_result_tool_t::get_viewport_texture_width(app_t& app, u32 const layer) {
	UNUSED(app);

	if (layer == 0) {
		return colors_tex_width;
	} else {
		assert_release(false);
		return 0;
	}
}

GLuint paint_result_tool_t::get_viewport_texture_height(app_t& app, u32 const layer) {
	UNUSED(app);

	if (layer == 0) {
		return colors_tex_width;
	} else {
		assert_release(false);
		return 0;
	}
}

std::string paint_result_tool_t::get_name() {
	return "Paint Result";
}

void paint_result_tool_t::import_image(
	std::vector<u8> const& colors, u32 const width, u32 const height, u32 const num_chans) {
	assert_release(num_chans == 3);
	colors_tex_width = width;
	colors_tex_height = height;

	glBindTexture(GL_TEXTURE_2D, colors_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, colors_tex_width, colors_tex_height, 0, GL_RGB, GL_UNSIGNED_BYTE,
		colors.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void paint_result_tool_t::reset_image() {
	colors_tex_width = 0;
	colors_tex_height = 0;

	glBindTexture(GL_TEXTURE_2D, colors_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void paint_result_tool_t::navigate_back(app_t& app) {
	app_navigate(app, "paint", false);
	reset_image();
}
