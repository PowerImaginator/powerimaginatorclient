#include "pch.h"

#include "app.h"
#include "camera_tool.h"
#include "exchange.h"
#include "io_utils.h"
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
		static_cast<camera_tool_t*>(app.tools["camera"].get())->restore_camera();
		app_navigate(app, "camera");
		reset_image();
	}

	ImGui::Separator();

	ImGui::TextWrapped("If you're satisfied with this image, click Next to update the scene using VGGT. If "
				   "not, click Back and adjust the prompt or camera.");

	if (ImGui::Button("Next")) {
		std::vector<u8> vggt_output_bin;
		exchange_run_vggt(g_exchange, vggt_output_bin);

		if (ENV_DEBUG_SAVE_EXCHANGE_IMAGES != 0) {
			write_file("exchange/vggt_output.bin", vggt_output_bin);
		}

		app_load_vggt_output(app, vggt_output_bin);
		static_cast<camera_tool_t*>(app.tools["camera"].get())->restore_camera();
		app_navigate(app, "camera");
		reset_image();
	}
}

void paint_result_tool_t::destroy() {
	if (colors_tex) {
		glDeleteTextures(1, &colors_tex);
		colors_tex = 0;
	}
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
		return colors_tex_height;
	} else {
		assert_release(false);
		return 0;
	}
}

std::string paint_result_tool_t::get_name() {
	return "Paint Result";
}

void paint_result_tool_t::import_image(std::vector<u8> const& colors, u32 width, u32 height) {
	assert_release(colors.size() == static_cast<size_t>(width) * static_cast<size_t>(height) * 3);

	std::vector<u8> colors_flipped;
	flip_image_y(colors_flipped, colors, static_cast<s32>(width), static_cast<s32>(height), 3);

	colors_tex_width = width;
	colors_tex_height = height;

	glBindTexture(GL_TEXTURE_2D, colors_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, static_cast<GLsizei>(colors_tex_width),
		static_cast<GLsizei>(colors_tex_height), 0, GL_RGB, GL_UNSIGNED_BYTE, colors_flipped.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void paint_result_tool_t::reset_image() {
	colors_tex_width = 1;
	colors_tex_height = 1;
	u8 black[] = {0, 0, 0};

	glBindTexture(GL_TEXTURE_2D, colors_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, black);
	glBindTexture(GL_TEXTURE_2D, 0);
}
