#include "pch.h"

#include "alignment_tool.h"
#include "app.h"
#include "exchange.h"
#include "io_utils.h"

void alignment_tool_t::init(app_t& app) {
	UNUSED(app);

	gl_render_pass_init(color_pass, "shaders/quad.vert", "shaders/copy.frag", g_exchange.renderer_internal_width,
		g_exchange.renderer_internal_height,
		{
			{"o_dest", {.internal_format = GL_RGBA8, .format = GL_RGBA, .type = GL_UNSIGNED_BYTE}},
		});

	gl_render_pass_init(keypoints_pass, "shaders/quad.vert", "shaders/keypoints.frag",
		g_exchange.renderer_internal_width, g_exchange.renderer_internal_height,
		{
			{"o_dest", {.internal_format = GL_RGBA8, .format = GL_RGBA, .type = GL_UNSIGNED_BYTE}},
		});
}

void alignment_tool_t::update(app_t& app, f64 const dt) {
	UNUSED(dt);

	update_hovered_keypoint(app);
	if (viewport_mouse_clicked) {
		add_hovered_keypoint(app);
	}

	gl_render_pass_begin(color_pass);
	gl_render_pass_uniform_texture(color_pass, "u_tex_source", color_tex, GL_TEXTURE0);
	gl_render_pass_draw(color_pass, app.quad_vbo);
	gl_render_pass_end(color_pass);

	gl_render_pass_begin(keypoints_pass);
	gl_render_pass_uniform_ivec2(
		keypoints_pass, "u_viewport_size", glm::ivec2(keypoints_pass.width, keypoints_pass.height));
	gl_render_pass_uniform_texture(keypoints_pass, "u_tex_world_pos", world_pos_tex, GL_TEXTURE0);
	gl_render_pass_uniform_texture(keypoints_pass, "u_tex_mask", mask_tex, GL_TEXTURE1);
	gl_render_pass_uniform_texture(keypoints_pass, "u_tex_keypoints", keypoints_tex, GL_TEXTURE2);
	gl_render_pass_uniform_int(keypoints_pass, "u_num_keypoints", static_cast<GLint>(keypoints.size()));
	gl_render_pass_uniform_ivec2(
		keypoints_pass, "u_hovered_keypoint", glm::ivec2(hovered_keypoint.x, hovered_keypoint.y));
	gl_render_pass_draw(keypoints_pass, app.quad_vbo);
	gl_render_pass_end(keypoints_pass);
}

void alignment_tool_t::update_settings(app_t& app, f64 const dt) {
	UNUSED(dt);

	if (ImGui::Button("Back")) {
		app_inpaint_discard_from_world(app);
		app_navigate(app, "eraser_2d", false);
		reset();
	}

	ImGui::Separator();

	ImGui::TextWrapped("Please click on lots of points which are good for depth alignment. You'll probably want to "
			   "maximize the window in order to see keypoints more clearly.\nChoose keypoints that "
			   "are not in blurry regions, or in regions with fine-grained detail such as grass and trees. "
			   "Make sure you add plenty of keypoints spread along the border of the masked region.");

	ImGui::Text("%d keypoints selected", static_cast<int>(keypoints.size()));
	ImGui::BeginDisabled(keypoints.empty());
	if (ImGui::Button("Remove Last Keypoint")) {
		remove_last_keypoint(app);
	}
	ImGui::SameLine();
	if (ImGui::Button("Remove All Keypoints")) {
		remove_all_keypoints(app);
	}
	ImGui::EndDisabled();

	if (keypoints.size() < 5) {
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "At least 5 keypoints are required");
	}

	ImGui::Checkbox("Lock Existing Points", &lock_existing_points);
	ImGui::TextWrapped("If checked, existing scene content will remain exactly the same, rather than averaging "
			   "depth data with new content. If the existing scene looks worse after clicking Apply, "
			   "toggle this box and try again.");
	app.meshgrid_average_ref_and_cur_tps = !lock_existing_points;

	ImGui::BeginDisabled(
		keypoints.size() < 5 || app.cur_chunk_points->GetNumberOfPoints() > 0); // ensure not already applied
	if (ImGui::Button("Apply")) {
		app_inpaint_visualize(app);
	}
	ImGui::EndDisabled();
	ImGui::BeginDisabled(app.cur_chunk_points->GetNumberOfPoints() == 0); // ensure already applied
	ImGui::SameLine();
	if (ImGui::Button("Cancel")) {
		app_inpaint_discard_from_world(app);
	}
	if (ImGui::Button("Next")) {
		app_inpaint_visualize(app);
		app_inpaint_add_to_world(app);
		reset();
	}
	ImGui::EndDisabled();
}

void alignment_tool_t::destroy() {
	//
}

u32 alignment_tool_t::get_num_viewport_layers(app_t& app) {
	UNUSED(app);

	return 2;
}

GLuint alignment_tool_t::get_viewport_texture(app_t& app, u32 const layer) {
	UNUSED(app);

	if (layer == 0) {
		return color_pass.internal_output_descriptors["o_dest"].texture;
	} else if (layer == 1) {
		return keypoints_pass.internal_output_descriptors["o_dest"].texture;
	} else {
		assert_release(false);
	}

	return 0;
}

GLuint alignment_tool_t::get_viewport_texture_width(app_t& app, u32 const layer) {
	UNUSED(app);

	if (layer == 0) {
		return color_pass.width;
	} else if (layer == 1) {
		return keypoints_pass.width;
	} else {
		assert_release(false);
	}

	return 0;
}

GLuint alignment_tool_t::get_viewport_texture_height(app_t& app, u32 const layer) {
	UNUSED(app);

	if (layer == 0) {
		return color_pass.height;
	} else if (layer == 1) {
		return keypoints_pass.height;
	} else {
		assert_release(false);
	}

	return 0;
}

bool alignment_tool_t::get_wants_mouse_capture(app_t& app) {
	UNUSED(app);
	return false;
}

std::string alignment_tool_t::get_name() {
	return "Alignment";
}

void alignment_tool_t::import_cur_chunk(app_t& app) {
	width = g_exchange.renderer_internal_width;
	height = g_exchange.renderer_internal_height;
	colors = *exchange_load_colors_out(g_exchange, app.cur_chunk_id, width, height, 3);
	world_pos = *exchange_load_world_pos_in(g_exchange, app.cur_chunk_id, width, height, 4);
	mask = *exchange_load_mask_in(g_exchange, app.cur_chunk_id, width, height, 1);

	std::vector<u8> colors_flipped;
	flip_image_y(colors_flipped, colors, width, height, 3);
	std::vector<f32> world_pos_flipped;
	flip_image_y(world_pos_flipped, world_pos, width, height, 4);
	std::vector<u8> mask_flipped;
	flip_image_y(mask_flipped, mask, width, height, 1);

	glGenTextures(1, &keypoints_tex);
	assert_release(keypoints_tex != 0);
	glGenTextures(1, &color_tex);
	assert_release(color_tex != 0);
	glGenTextures(1, &world_pos_tex);
	assert_release(world_pos_tex != 0);
	glGenTextures(1, &mask_tex);
	assert_release(mask_tex != 0);
	glBindTexture(GL_TEXTURE_2D, keypoints_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, 0, 0, 0, GL_RG_INTEGER, GL_UNSIGNED_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, color_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, colors_flipped.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, world_pos_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, world_pos_flipped.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, mask_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, mask_flipped.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void alignment_tool_t::reset() {
	keypoints.clear();
	hovered_keypoint = glm::ivec2(-1, -1);
	colors.clear();
	world_pos.clear();
	mask.clear();
	width = 0;
	height = 0;

	glBindTexture(GL_TEXTURE_2D, color_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, world_pos_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 0, 0, 0, GL_RGBA, GL_FLOAT, nullptr);
	glBindTexture(GL_TEXTURE_2D, mask_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 0, 0, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void alignment_tool_t::update_hovered_keypoint(app_t& app) {
	UNUSED(app);

	if (std::abs(viewport_mouse_delta.x) < 0.01f && std::abs(viewport_mouse_delta.y) < 0.01f) {
		return;
	}

	hovered_keypoint = glm::ivec2(-1, -1);

	if (!viewport_mouse_hovered) {
		return;
	}

	s32 const SEARCH_RADIUS = 16;
	f32 cur_min_dist = std::numeric_limits<f32>::max();
	for (s32 row = viewport_mouse_pos.y - SEARCH_RADIUS; row <= viewport_mouse_pos.y + SEARCH_RADIUS; ++row) {
		for (s32 col = viewport_mouse_pos.x - SEARCH_RADIUS; col <= viewport_mouse_pos.x + SEARCH_RADIUS;
			++col) {
			if (row < 0 || row >= static_cast<s32>(height) || col < 0 || col >= static_cast<s32>(width)) {
				continue;
			}

			u64 const row_col_i = row * width + col;
			if (world_pos[row_col_i * 4 + 3] < 0.001f || mask[row_col_i] > 0) {
				continue;
			}

			f32 const dist = glm::distance(
				glm::vec2(col, row), glm::vec2(viewport_mouse_pos.x, viewport_mouse_pos.y));
			if (dist < cur_min_dist) {
				hovered_keypoint = glm::ivec2(col, row);
				cur_min_dist = dist;
			}
		}
	}
}

void alignment_tool_t::add_hovered_keypoint(app_t& app) {
	UNUSED(app);

	if (hovered_keypoint.x == -1 || hovered_keypoint.y == -1) {
		return;
	}

	bool already_has_keypoint = false;
	for (u32 i = 0; i < keypoints.size(); ++i) {
		if (keypoints[i] == hovered_keypoint) {
			already_has_keypoint = true;
			break;
		}
	}

	if (already_has_keypoint) {
		return;
	}

	keypoints.emplace_back(hovered_keypoint);

	on_keypoints_changed(app);
}

void alignment_tool_t::remove_last_keypoint(app_t& app) {
	UNUSED(app);

	if (keypoints.empty()) {
		return;
	}

	keypoints.pop_back();

	on_keypoints_changed(app);
}

void alignment_tool_t::remove_all_keypoints(app_t& app) {
	UNUSED(app);
	keypoints.clear();
	on_keypoints_changed(app);
}

void alignment_tool_t::on_keypoints_changed(app_t& app) {
	UNUSED(app);

	std::vector<u32> keypoints_texture_data(keypoints.size() * 2);
	for (u32 i = 0; i < keypoints.size(); ++i) {
		keypoints_texture_data[i * 2] = keypoints[i].x;
		keypoints_texture_data[i * 2 + 1] = keypoints[i].y;
	}

	glBindTexture(GL_TEXTURE_2D, keypoints_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, static_cast<GLsizei>(keypoints_texture_data.size() / 2), 1, 0, GL_RG_INTEGER,
		GL_UNSIGNED_INT, keypoints_texture_data.data());
	glBindTexture(GL_TEXTURE_2D, 0);
}
