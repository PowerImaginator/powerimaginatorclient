#include "pch.h"

#include "alignment_tool.h"
#include "app.h"
#include "autoeraser.h"
#include "camera_tool.h"
#include "eraser_2d_tool.h"
#include "exchange.h"
#include "io_utils.h"
#include "meshgrid_utils.h"
#include "paint_result_tool.h"
#include "paint_tool.h"

void app_quad_vbo_init(gl_vertex_buffers_t& quad_vbo) {
	quad_vbo.mode = GL_TRIANGLE_STRIP;
	gl_vertex_buffers_upload(quad_vbo, "a_position",
		std::vector<f32>{-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f}, 2, GL_FLOAT, GL_FALSE);
	gl_vertex_buffers_upload(quad_vbo, "a_tex_coord",
		std::vector<f32>{0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f}, 2, GL_FLOAT, GL_FALSE);
}

void app_add_tools(app_t& app) {
	app.tools.emplace("camera", std::make_unique<camera_tool_t>());
	app.tools.emplace("paint", std::make_unique<paint_tool_t>());
	app.tools.emplace("paint_result", std::make_unique<paint_result_tool_t>());
	app.tools.emplace("eraser_2d", std::make_unique<eraser_2d_tool_t>());
	app.tools.emplace("alignment", std::make_unique<alignment_tool_t>());
}

void app_init(app_t& app) {
	app_quad_vbo_init(app.quad_vbo);

	app.points_vbo.mode = GL_POINTS;

	app.cur_chunk_id = 0;
	exchange_allocate_chunk(
		g_exchange, app.cur_chunk_id, g_exchange.renderer_internal_width, g_exchange.renderer_internal_height);

	app.combined_points = vtkSmartPointer<vtkPoints>::New();
	app.combined_colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
	app.combined_colors->SetName("Colors");
	app.combined_colors->SetNumberOfComponents(3);
	app.combined_points_backup = vtkSmartPointer<vtkPoints>::New();
	app.combined_colors_backup = vtkSmartPointer<vtkUnsignedCharArray>::New();
	app.combined_colors_backup->SetName("Colors");
	app.combined_colors_backup->SetNumberOfComponents(3);
	app.cur_chunk_points = vtkSmartPointer<vtkPoints>::New();
	app.cur_chunk_colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
	app.cur_chunk_colors->SetName("Colors");
	app.cur_chunk_colors->SetNumberOfComponents(3);

	app_add_tools(app);
	for (auto& [name, tool] : app.tools) {
		tool->init(app);
	}
	app.tour_active_tool = "paint";
	static_cast<paint_tool_t*>(app.tools["paint"].get())->fill();
}

void app_update(app_t& app, f64 const dt) {
	for (auto& [name, tool] : app.tools) {
		tool->update(app, dt);
	}
}

void app_imgui_settings(app_t& app, f64 const dt) {
	ImGui::Begin("Control Panel");
	app.tools["camera"]->update_settings(app, dt);
	app.tools["paint"]->update_settings(app, dt);
	app.tools["paint_result"]->update_settings(app, dt);
	ImGui::End();
}

void app_imgui_viewport_for_tool(app_t& app, std::unique_ptr<app_tool_t>& tool) {
	ImVec2 preview_window_size = ImGui::GetContentRegionAvail();

	bool window_focused = ImGui::IsWindowFocused();

	u32 num_layers = tool->get_num_viewport_layers(app);
	for (u32 i = 0; i < num_layers; ++i) {
		ImGui::PushID(i);

		GLuint fbo_texture = tool->get_viewport_texture(app, i);
		GLuint fbo_width = tool->get_viewport_texture_width(app, 0);
		GLuint fbo_height = tool->get_viewport_texture_height(app, 0);

		f32 const preview_resize_ratio =
			std::min(preview_window_size.x / fbo_width, preview_window_size.y / fbo_height);
		ImVec2 preview_image_size = {fbo_width * preview_resize_ratio, fbo_height * preview_resize_ratio};
		ImVec2 preview_image_position = {(preview_window_size.x - preview_image_size.x) * 0.5f,
			(preview_window_size.y - preview_image_size.y) * 0.5f};

		if (i == 0) {
			ImGui::SetItemAllowOverlap();
			ImGui::SetCursorPos(preview_image_position);
			ImGui::InvisibleButton(
				"##ViewportInvisibleButton", preview_image_size, ImGuiButtonFlags_MouseButtonLeft);

			ImGuiIO& io = ImGui::GetIO();
			bool mouse_use_delta = false;

			bool const is_hovered = ImGui::IsItemHovered();
			tool->viewport_mouse_hovered = is_hovered;
			tool->viewport_mouse_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

			if (tool->get_wants_mouse_capture(app)) {
				bool const is_mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
				bool const mouse_pressed = window_focused && is_hovered && is_mouse_down &&
					!tool->viewport_mouse_captured && !tool->viewport_mouse_captured_next_frame;
				bool const mouse_released = !is_mouse_down && tool->viewport_mouse_captured;

				if (mouse_pressed) {
					tool->viewport_mouse_captured_next_frame = true;
					ImGui::SetNextFrameWantCaptureMouse(true);
				} else if (tool->viewport_mouse_captured_next_frame) {
					tool->viewport_mouse_captured_next_frame = false;
					tool->viewport_mouse_captured = true;
					ImGui::SetNextFrameWantCaptureMouse(true);
					ImGui::SetMouseCursor(ImGuiMouseCursor_None);
					glfwSetInputMode(app.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				} else if (mouse_released) {
					tool->viewport_mouse_captured = false;
					ImGui::SetNextFrameWantCaptureMouse(false);
					ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
					glfwSetInputMode(app.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				} else if (tool->viewport_mouse_captured) {
					mouse_use_delta = true;

					tool->viewport_mouse_delta =
						ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.0f);
					ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
					ImGui::SetNextFrameWantCaptureMouse(true);

					tool->viewport_mouse_delta.x =
						(tool->viewport_mouse_delta.x / preview_image_size.x) * fbo_width;
					tool->viewport_mouse_delta.y =
						(tool->viewport_mouse_delta.y / preview_image_size.y) * fbo_height;

					tool->viewport_mouse_pos.x += tool->viewport_mouse_delta.x;
					tool->viewport_mouse_pos.y += tool->viewport_mouse_delta.y;
				}
			}

			if (!mouse_use_delta) {
				ImVec2 button_min = ImGui::GetItemRectMin();
				ImVec2 mouse_pos_relative =
					ImVec2(((io.MousePos.x - button_min.x) / preview_image_size.x) * fbo_width,
						((io.MousePos.y - button_min.y) / preview_image_size.y) * fbo_height);
				ImVec2 old_mouse_pos_relative = tool->viewport_mouse_pos;
				tool->viewport_mouse_pos = ImVec2(mouse_pos_relative.x, mouse_pos_relative.y);
				tool->viewport_mouse_delta = ImVec2(mouse_pos_relative.x - old_mouse_pos_relative.x,
					mouse_pos_relative.y - old_mouse_pos_relative.y);
			}
		}

		ImGui::SetItemAllowOverlap();
		ImGui::SetCursorPos(preview_image_position);
		ImGui::Image(fbo_texture, preview_image_size, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f),
			tool->get_viewport_tint(app, i), ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

		ImGui::PopID();
	}
}

void app_imgui(app_t& app, f64 const dt) {
	ImVec2 main_viewport_size = ImGui::GetMainViewport()->Size;

	f32 sidebar_width = 360.0f;
	f32 items_margin = 16.0f;
	ImVec2 sidebar_size = ImVec2(sidebar_width, main_viewport_size.y - items_margin * 2.0f);
	ImVec2 sidebar_pos = ImVec2(main_viewport_size.x - sidebar_size.x - items_margin, items_margin);
	ImGui::SetNextWindowPos(sidebar_pos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(sidebar_size, ImGuiCond_Always);
	ImGui::Begin("Control Panel", nullptr,
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
	app.tools[app.tour_active_tool]->update_settings(app, dt);
	ImGui::End();

	ImVec2 total_viewport_size = ImVec2(
		main_viewport_size.x - sidebar_width - items_margin * 3.0f, main_viewport_size.y - items_margin * 2.0f);
	ImVec2 total_viewport_pos = ImVec2(items_margin, items_margin);

	ImVec2 viewport_size =
		ImVec2(app.split_with_camera ? (total_viewport_size.x - items_margin) * 0.5f : total_viewport_size.x,
			total_viewport_size.y);
	ImVec2 viewport_pos_1 = ImVec2(total_viewport_pos.x, total_viewport_pos.y);
	ImVec2 viewport_pos_2 = ImVec2(total_viewport_pos.x + viewport_size.x + items_margin, total_viewport_pos.y);

	ImGui::SetNextWindowPos(viewport_pos_1, ImGuiCond_Always);
	ImGui::SetNextWindowSize(viewport_size, ImGuiCond_Always);
	ImGui::Begin(app.tools[app.tour_active_tool]->get_name().c_str(), nullptr,
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
	app_imgui_viewport_for_tool(app, app.tools[app.tour_active_tool]);
	ImGui::End();

	if (app.split_with_camera) {
		ImGui::SetNextWindowPos(viewport_pos_2, ImGuiCond_Always);
		ImGui::SetNextWindowSize(viewport_size, ImGuiCond_Always);
		ImGui::Begin(app.tools["camera"]->get_name().c_str(), nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		app_imgui_viewport_for_tool(app, app.tools["camera"]);
		ImGui::End();
	}
}

void app_shutdown(app_t& app) {
	UNUSED(app);
}

void app_sync_points_with_vbo(app_t& app) {
	std::vector<f32> positions;
	std::vector<u8> colors;

	vtkIdType num_points = app.cur_chunk_points->GetNumberOfPoints();
	for (vtkIdType i = 0; i < num_points; ++i) {
		double pt[3];
		app.cur_chunk_points->GetPoint(i, pt);
		positions.emplace_back(static_cast<f32>(pt[0]));
		positions.emplace_back(static_cast<f32>(pt[1]));
		positions.emplace_back(static_cast<f32>(pt[2]));
	}
	for (vtkIdType i = 0; i < num_points; ++i) {
		double* color = app.cur_chunk_colors->GetTuple(i);
		colors.emplace_back(static_cast<u8>(color[0]));
		colors.emplace_back(static_cast<u8>(color[1]));
		colors.emplace_back(static_cast<u8>(color[2]));
	}

	num_points = app.combined_points->GetNumberOfPoints();
	for (vtkIdType i = 0; i < num_points; ++i) {
		double pt[3];
		app.combined_points->GetPoint(i, pt);
		positions.emplace_back(static_cast<f32>(pt[0]));
		positions.emplace_back(static_cast<f32>(pt[1]));
		positions.emplace_back(static_cast<f32>(pt[2]));
	}
	for (vtkIdType i = 0; i < num_points; ++i) {
		double* color = app.combined_colors->GetTuple(i);
		colors.emplace_back(static_cast<u8>(color[0]));
		colors.emplace_back(static_cast<u8>(color[1]));
		colors.emplace_back(static_cast<u8>(color[2]));
	}

	gl_vertex_buffers_upload(app.points_vbo, "a_position", positions, 3, GL_FLOAT, GL_FALSE);
	gl_vertex_buffers_upload(app.points_vbo, "a_color", colors, 3, GL_UNSIGNED_BYTE, GL_TRUE);
}

void app_inpaint_export(app_t& app) {
	camera_tool_t* camera_tool = static_cast<camera_tool_t*>(app.tools["camera"].get());
	gl_render_pass_t* camera_multiply_world_pos_pass = &camera_tool->points_renderer.multiply_world_pos_pass;
	gl_render_pass_t* camera_final_pass = points_renderer_get_final_render_pass(camera_tool->points_renderer);

	paint_tool_t* paint_tool = static_cast<paint_tool_t*>(app.tools["paint"].get());

	std::vector<f32> color_floats;
	gl_render_pass_download(*camera_final_pass, "o_dest", color_floats);
	std::vector<u8> color_bytes;
	color_bytes.resize(color_floats.size());
	for (size_t i = 0; i < color_floats.size(); ++i) {
		color_bytes[i] = static_cast<u8>(color_floats[i] * 255.0f);
	}
	std::vector<u8> color_bytes_flipped;
	flip_image_y(color_bytes_flipped, color_bytes, camera_final_pass->width, camera_final_pass->height, 4);
	exchange_write_colors_in(g_exchange, app.cur_chunk_id, camera_final_pass->width, camera_final_pass->height, 4,
		color_bytes_flipped);

	std::vector<f32> world_pos_floats;
	gl_render_pass_download(*camera_multiply_world_pos_pass, "o_dest", world_pos_floats);
	std::vector<f32> world_pos_floats_flipped;
	flip_image_y(world_pos_floats_flipped, world_pos_floats, camera_multiply_world_pos_pass->width,
		camera_multiply_world_pos_pass->height, 4);
	exchange_write_world_pos_in(g_exchange, app.cur_chunk_id, camera_multiply_world_pos_pass->width,
		camera_multiply_world_pos_pass->height, 4, world_pos_floats_flipped);

	std::vector<u8> mask_bytes;
	gl_render_pass_download(paint_tool->copy_pass, "o_dest", mask_bytes);
	std::vector<u8> mask_bytes_flipped;
	flip_image_y(mask_bytes_flipped, mask_bytes, paint_tool->copy_pass.width, paint_tool->copy_pass.height, 4);
	exchange_write_mask_in(g_exchange, app.cur_chunk_id, paint_tool->copy_pass.width, paint_tool->copy_pass.height,
		4, mask_bytes_flipped);

	// Write the current camera matrices to files
	{
		const glm::mat4& view_mat = camera_tool->camera.view_mat;
		const glm::mat4& proj_mat = camera_tool->camera.proj_mat;

		// Write view matrix
		std::vector<f32> view_mat_floats(16);
		const f32* view_ptr = glm::value_ptr(view_mat);
		for (int i = 0; i < 16; ++i) {
			view_mat_floats[i] = view_ptr[i];
		}
		exchange_write_view_mat_in(g_exchange, app.cur_chunk_id, view_mat_floats);

		// Write projection matrix
		std::vector<f32> proj_mat_floats(16);
		const f32* proj_ptr = glm::value_ptr(proj_mat);
		for (int i = 0; i < 16; ++i) {
			proj_mat_floats[i] = proj_ptr[i];
		}
		exchange_write_proj_mat_in(g_exchange, app.cur_chunk_id, proj_mat_floats);
	}
}

void app_inpaint_import_paint_result(app_t& app) {
	std::vector<u8> const& colors = *exchange_load_colors_out(g_exchange, app.cur_chunk_id,
		g_exchange.renderer_internal_width, g_exchange.renderer_internal_height, 3);
	std::vector<u8> colors_flipped;
	flip_image_y(
		colors_flipped, colors, g_exchange.renderer_internal_width, g_exchange.renderer_internal_height, 3);
	static_cast<paint_result_tool_t*>(app.tools["paint_result"].get())
		->import_image(
			colors_flipped, g_exchange.renderer_internal_width, g_exchange.renderer_internal_height, 3);
}

void app_inpaint_visualize(app_t& app) {
	app_inpaint_discard_from_world(app);

	app.combined_points_backup->Reset();
	app.combined_colors_backup->Reset();
	app.combined_colors_backup->SetNumberOfComponents(3);
	app.combined_points_backup->DeepCopy(app.combined_points);
	app.combined_colors_backup->DeepCopy(app.combined_colors);
	app.combined_points_backup->Squeeze();
	app.combined_colors_backup->Squeeze();
	app.can_discard = true;

	// Download eraser 2D mask
	eraser_2d_tool_t* eraser_2d_tool = static_cast<eraser_2d_tool_t*>(app.tools["eraser_2d"].get());
	{
		std::vector<u8> eraser_2d_mask_bytes;
		gl_render_pass_download(eraser_2d_tool->copy_pass, "o_dest", eraser_2d_mask_bytes);
		std::vector<u8> eraser_2d_mask_bytes_flipped;
		flip_image_y(eraser_2d_mask_bytes_flipped, eraser_2d_mask_bytes, eraser_2d_tool->copy_pass.width,
			eraser_2d_tool->copy_pass.height, 4);
		exchange_write_eraser_2d_mask_in(g_exchange, app.cur_chunk_id, eraser_2d_tool->copy_pass.width,
			eraser_2d_tool->copy_pass.height, 4, eraser_2d_mask_bytes_flipped);
	}

	//
	// Load and align chunk
	//

	std::vector<u8> const& colors = *exchange_load_colors_out(g_exchange, app.cur_chunk_id,
		g_exchange.renderer_internal_width, g_exchange.renderer_internal_height, 3);
	std::vector<f32> const& depths = *exchange_load_depth_out(g_exchange, app.cur_chunk_id,
		g_exchange.renderer_internal_width, g_exchange.renderer_internal_height, 1);

	vtkNew<vtkPoints> cur_chunk_points_init;
	vtkNew<vtkUnsignedCharArray> cur_chunk_colors_init;
	cur_chunk_colors_init->SetNumberOfComponents(3);
	build_meshgrid(cur_chunk_points_init, cur_chunk_colors_init, g_exchange.renderer_internal_width,
		g_exchange.renderer_internal_height, g_exchange.renderer_internal_fov_y, depths, colors);

	std::vector<f32> const& world_pos_floats = *exchange_load_world_pos_in(g_exchange, app.cur_chunk_id,
		g_exchange.renderer_internal_width, g_exchange.renderer_internal_height, 4);

	std::vector<u8> const& mask = *exchange_load_mask_in(g_exchange, app.cur_chunk_id,
		g_exchange.renderer_internal_width, g_exchange.renderer_internal_height, 1);

	vtkNew<vtkPoints> cur_chunk_points_aligned;
	vtkNew<vtkPoints> combined_points_after_align;
	align_meshgrid(cur_chunk_points_aligned, cur_chunk_points_init, combined_points_after_align,
		app.combined_points_backup, world_pos_floats, mask, g_exchange.renderer_internal_width,
		g_exchange.renderer_internal_height, app.cur_chunk_id,
		static_cast<alignment_tool_t*>(app.tools["alignment"].get())->keypoints,
		app.meshgrid_average_ref_and_cur_tps);

	std::unordered_map<u64, u64> identity_idxs_map;
	for (u64 i = 0; i < static_cast<u64>(cur_chunk_points_aligned->GetNumberOfPoints()); ++i) {
		identity_idxs_map[i] = i;
	}
	std::vector<u8> const& eraser_2d_mask = *exchange_load_eraser_2d_mask_in(g_exchange, app.cur_chunk_id,
		g_exchange.renderer_internal_width, g_exchange.renderer_internal_height, 1);
	f32 const min_depth = eraser_2d_tool->min_depth;
	f32 const max_depth = eraser_2d_tool->max_depth;
	vtkNew<vtkPoints> cur_chunk_points_filtered;
	vtkNew<vtkUnsignedCharArray> cur_chunk_colors_filtered;
	cur_chunk_colors_filtered->SetNumberOfComponents(3);
	std::unordered_map<u64, u64> idxs_map_filtered;
	remove_meshgrid_unwanted(cur_chunk_points_filtered, cur_chunk_colors_filtered, idxs_map_filtered,
		cur_chunk_points_aligned, cur_chunk_colors_init, identity_idxs_map, depths, mask, eraser_2d_mask,
		min_depth, max_depth);

	/*
	vtkNew<vtkPoints> cur_chunk_points_no_outliers;
	vtkNew<vtkUnsignedCharArray> cur_chunk_colors_no_outliers;
	cur_chunk_colors_no_outliers->SetNumberOfComponents(3);
	std::unordered_map<u64, u64> idxs_map_outliers;
	eraser_2d_tool_t* eraser_2d_tool = static_cast<eraser_2d_tool_t*>(app.tools["eraser_2d"].get());
	remove_meshgrid_outliers(cur_chunk_points_no_outliers, cur_chunk_colors_no_outliers, idxs_map_outliers,
		cur_chunk_points_aligned, cur_chunk_colors_init, depths, eraser_2d_tool->min_depth,
		eraser_2d_tool->max_depth);
	*/

	vtkPoints* FINAL_POINTS = cur_chunk_points_filtered /* cur_chunk_points_no_outliers */;
	vtkUnsignedCharArray* FINAL_COLORS = cur_chunk_colors_filtered /* cur_chunk_colors_no_outliers */;
	std::unordered_map<u64, u64>* FINAL_IDXS_MAP = &idxs_map_filtered /* &idxs_map_outliers */;

	app.cur_chunk_points->Reset();
	app.cur_chunk_points->DeepCopy(FINAL_POINTS);
	app.cur_chunk_points->Squeeze();
	app.cur_chunk_colors->Reset();
	app.cur_chunk_colors->SetNumberOfComponents(3);
	app.cur_chunk_colors->DeepCopy(FINAL_COLORS);
	app.cur_chunk_colors->Squeeze();

	//
	// Erase everything inpainted by chunk
	//

	glm::mat4 cur_chunk_view_mat, cur_chunk_proj_mat;
	{
		std::vector<f32> const& view_mat_data = *exchange_load_view_mat_in(g_exchange, app.cur_chunk_id);
		std::vector<f32> const& proj_mat_data = *exchange_load_proj_mat_in(g_exchange, app.cur_chunk_id);
		assert_release(view_mat_data.size() == 16);
		assert_release(proj_mat_data.size() == 16);
		memcpy(glm::value_ptr(cur_chunk_view_mat), view_mat_data.data(), sizeof(f32) * 16);
		memcpy(glm::value_ptr(cur_chunk_proj_mat), proj_mat_data.data(), sizeof(f32) * 16);
	}

	std::vector<f32> autoeraser_depths;
	autoeraser_prepare_depth_buffer(autoeraser_depths, FINAL_POINTS, *FINAL_IDXS_MAP, cur_chunk_view_mat,
		cur_chunk_proj_mat, g_exchange.renderer_internal_width, g_exchange.renderer_internal_height);
	autoeraser_erase_points(app.combined_points, app.combined_colors, combined_points_after_align,
		app.combined_colors_backup, autoeraser_depths, cur_chunk_view_mat, cur_chunk_proj_mat,
		g_exchange.renderer_internal_width, g_exchange.renderer_internal_height);

	//
	// End
	//

	app_sync_points_with_vbo(app);
}

void app_inpaint_add_to_world(app_t& app) {
	vtkIdType num_points = app.cur_chunk_points->GetNumberOfPoints();
	for (vtkIdType i = 0; i < num_points; ++i) {
		double pt[3];
		app.cur_chunk_points->GetPoint(i, pt);
		app.combined_points->InsertNextPoint(pt);
	}

	vtkIdType num_colors = app.cur_chunk_colors->GetNumberOfTuples();
	for (vtkIdType i = 0; i < num_colors; ++i) {
		double* color = app.cur_chunk_colors->GetTuple(i);
		app.combined_colors->InsertNextTuple(color);
	}

	app.cur_chunk_points->Reset();
	app.cur_chunk_points->Squeeze();
	app.cur_chunk_colors->Reset();
	app.cur_chunk_colors->Squeeze();

	++app.cur_chunk_id;
	exchange_allocate_chunk(
		g_exchange, app.cur_chunk_id, g_exchange.renderer_internal_width, g_exchange.renderer_internal_height);
	exchange_free_chunk(g_exchange, app.cur_chunk_id - 1);

	app.combined_points_backup->Reset();
	app.combined_colors_backup->Reset();
	app.combined_colors_backup->SetNumberOfComponents(3);
	app.combined_points_backup->Squeeze();
	app.combined_colors_backup->Squeeze();
	app.can_discard = false;

	app_sync_points_with_vbo(app);

	static_cast<camera_tool_t*>(app.tools["camera"].get())->restore_camera();
	static_cast<paint_tool_t*>(app.tools["paint"].get())->clear();
	static_cast<paint_result_tool_t*>(app.tools["paint_result"].get())->reset_image();
	static_cast<eraser_2d_tool_t*>(app.tools["eraser_2d"].get())->reset();
	static_cast<alignment_tool_t*>(app.tools["alignment"].get())->reset();
	app_navigate(app, "camera", false);
}

void app_inpaint_discard_from_world(app_t& app) {
	if (!app.can_discard) {
		return;
	}

	app.cur_chunk_points->Reset();
	app.cur_chunk_points->Squeeze();
	app.cur_chunk_colors->Reset();
	app.cur_chunk_colors->Squeeze();

	app.combined_points->Reset();
	app.combined_colors->Reset();
	app.combined_colors->SetNumberOfComponents(3);
	app.combined_points->DeepCopy(app.combined_points_backup);
	app.combined_colors->DeepCopy(app.combined_colors_backup);
	app.combined_points->Squeeze();
	app.combined_colors->Squeeze();

	app.combined_points_backup->Reset();
	app.combined_colors_backup->Reset();
	app.combined_colors_backup->SetNumberOfComponents(3);
	app.combined_points_backup->Squeeze();
	app.combined_colors_backup->Squeeze();
	app.can_discard = false;

	app_sync_points_with_vbo(app);
}

bool app_navigate(app_t& app, std::string const& next_tool, bool split_with_camera) {
	if (app.tools.find(next_tool) == app.tools.end()) {
		assert_release(false);
		return false;
	}

	// Camera is backed up in camera_tool.cpp when the Next button is pressed
	static_cast<camera_tool_t*>(app.tools["camera"].get())->restore_camera();

	app.tour_active_tool = next_tool;
	app.split_with_camera = split_with_camera;

	// Make sure it never shows camera split twice
	assert_release(!app.split_with_camera || app.tour_active_tool != "camera");

	return true;
}

void app_export_ply(app_t& app) {
	nfdu8char_t* out_path = nullptr;
	nfdu8filteritem_t filters[1] = {{"PLY files", "ply"}};
	nfdsavedialogu8args_t args = {.filterList = filters,
		.filterCount = 1,
		.defaultPath = nullptr,
		.defaultName = nullptr,
		.parentWindow = {.type = 0, .handle = nullptr}};

	nfdresult_t result = NFD_SaveDialogU8_With(&out_path, &args);
	if (result != NFD_OKAY) {
		return;
	}

	vtkNew<vtkPolyData> polyData;
	polyData->SetPoints(app.combined_points);
	assert(app.combined_colors->GetNumberOfTuples() == app.combined_points->GetNumberOfPoints());
	app.combined_colors->SetName("Colors");
	polyData->GetPointData()->SetScalars(app.combined_colors);

	vtkNew<vtkPLYWriter> writer;
	writer->SetFileName(out_path);
	writer->SetInputData(polyData);
	writer->SetFileTypeToBinary();
	writer->SetColorModeToDefault();
	writer->SetArrayName("Colors");
	writer->Write();

	NFD_FreePathU8(out_path);
}
