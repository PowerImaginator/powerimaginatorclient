#include "app.h"
#include "camera_tool.h"
#include "pch.h"

void app_quad_vbo_init(gl_vertex_buffers_t& quad_vbo) {
	quad_vbo.mode = GL_TRIANGLE_STRIP;
	gl_vertex_buffers_upload(quad_vbo, "a_position",
		std::vector<f32>{-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f}, 2, GL_FLOAT, GL_FALSE);
	gl_vertex_buffers_upload(quad_vbo, "a_tex_coord",
		std::vector<f32>{0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f}, 2, GL_FLOAT, GL_FALSE);
}

void app_add_tools(app_t& app) {
	app.tools.emplace("camera", std::make_unique<camera_tool_t>());
}

void app_init(app_t& app) {
	app_quad_vbo_init(app.quad_vbo);

	app.points_vbo.mode = GL_POINTS;
	app.mask_vbo.mode = GL_TRIANGLES;

	app_add_tools(app);
	for (auto& [name, tool] : app.tools) {
		tool->init(app);
	}
	app_load_vggt_output(app, ENV_VGGT_OUTPUT_SOURCE);
	app.tour_active_tool = "camera";
}

void app_update(app_t& app, f64 const dt) {
	for (auto& [name, tool] : app.tools) {
		tool->update(app, dt);
	}
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

	ImVec2 viewport_size = total_viewport_size;
	ImVec2 viewport_pos_1 = ImVec2(total_viewport_pos.x, total_viewport_pos.y);

	ImGui::SetNextWindowPos(viewport_pos_1, ImGuiCond_Always);
	ImGui::SetNextWindowSize(viewport_size, ImGuiCond_Always);
	ImGui::Begin(app.tools[app.tour_active_tool]->get_name().c_str(), nullptr,
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
	app_imgui_viewport_for_tool(app, app.tools[app.tour_active_tool]);
	ImGui::End();
}

void app_shutdown(app_t& app) {
	UNUSED(app);
}

void app_load_vggt_output(app_t& app, std::string const& filename) {
	std::ifstream file(filename, std::ios::binary);
	if (!file) {
		std::cerr << "Failed to open file: " << filename << std::endl;
		return;
	}

	// Read number of images
	u32 num_images = 0;
	file.read(reinterpret_cast<char*>(&num_images), sizeof(u32));

	app.camera_data.clear();
	app.camera_data.reserve(num_images);

	std::vector<f32> positions;
	std::vector<u8> colors;
	std::vector<f32> mask_positions;

	glm::mat4 first_cam_to_world(1.0f);
	bool stored_first_camera = false;

	glm::mat4 opengl_conversion(1.0f);
	opengl_conversion[1][1] = -1.0f;
	opengl_conversion[2][2] = -1.0f;

	for (u32 img_idx = 0; img_idx < num_images; ++img_idx) {
		// Read width and height
		u32 width = 0, height = 0;
		file.read(reinterpret_cast<char*>(&width), sizeof(u32));
		file.read(reinterpret_cast<char*>(&height), sizeof(u32));

		// Read intrinsic matrix (3x3, f32)
		// NumPy writes row-major, GLM expects column-major, so we transpose
		f32 intrinsic_data[9];
		file.read(reinterpret_cast<char*>(intrinsic_data), sizeof(f32) * 9);
		glm::mat3 intrinsic;
		intrinsic[0][0] = intrinsic_data[0]; // fx
		intrinsic[1][0] = intrinsic_data[1]; // 0
		intrinsic[2][0] = intrinsic_data[2]; // 0
		intrinsic[0][1] = intrinsic_data[3]; // 0
		intrinsic[1][1] = intrinsic_data[4]; // fy
		intrinsic[2][1] = intrinsic_data[5]; // 0
		intrinsic[0][2] = intrinsic_data[6]; // cx
		intrinsic[1][2] = intrinsic_data[7]; // cy
		intrinsic[2][2] = intrinsic_data[8]; // 1

		// Read extrinsic matrix (4x4, f32) - camera-to-world in VGGT/OpenCV coordinates
		// NumPy writes row-major, GLM expects column-major, so we transpose
		f32 extrinsic_data[16];
		file.read(reinterpret_cast<char*>(extrinsic_data), sizeof(f32) * 16);
		glm::mat4 cam_to_world;
		for (u32 col = 0; col < 4; ++col) {
			for (u32 row = 0; row < 4; ++row) {
				cam_to_world[col][row] = extrinsic_data[row * 4 + col];
			}
		}

		if (!stored_first_camera) {
			first_cam_to_world = cam_to_world;
			stored_first_camera = true;
		}

		// Read confidence buffer (H*W, f32)
		std::vector<f32> confidence(width * height);
		file.read(reinterpret_cast<char*>(confidence.data()), sizeof(f32) * width * height);

		// Read depth buffer (H*W, f32)
		std::vector<f32> depth(width * height);
		file.read(reinterpret_cast<char*>(depth.data()), sizeof(f32) * width * height);

		vggt_camera_data_t camera_info;
		camera_info.extrinsic = glm::inverse(first_cam_to_world) * opengl_conversion * cam_to_world;
		camera_info.intrinsic = intrinsic;
		camera_info.confidence = confidence;
		camera_info.depth = depth;
		camera_info.width = width;
		camera_info.height = height;
		app.camera_data.push_back(std::move(camera_info));

		// Read color buffer (H*W*3, u8)
		std::vector<u8> color(width * height * 3);
		file.read(reinterpret_cast<char*>(color.data()), sizeof(u8) * width * height * 3);

		f32 fx = intrinsic[0][0];
		f32 fy = intrinsic[1][1];
		f32 cx = intrinsic[2][0];
		f32 cy = intrinsic[2][1];

		std::vector<f32> tmp_all_positions;
		for (u32 y = 0; y < height; ++y) {
			for (u32 x = 0; x < width; ++x) {
				u32 idx = y * width + x;
				f32 d = depth[idx];
				f32 conf = confidence[idx];

				bool valid_point = conf > 5.0f && d > 0.001f && d < 999.0f;

				// After checking for valid point, make sure we clamp any negative, zero, or infinite depths so we have valid points for tmp_all_positions which will be used to construct the mask
				if (d < 0.001f) {
					d = 0.001f;
				}
				if (d > 999.0f) {
					d = 999.0f;
				}

				f32 px = static_cast<f32>(x);
				f32 py = static_cast<f32>(y);

				glm::vec3 cam_pos;
				cam_pos.x = (px - cx) * d / fx;
				cam_pos.y = (py - cy) * d / fy;
				cam_pos.z = d;

				// Transform to world space using aligned extrinsic
				glm::vec4 world_pos_homogeneous =
					app.camera_data.back().extrinsic * glm::vec4(cam_pos, 1.0f);
				glm::vec3 world_pos = glm::vec3(world_pos_homogeneous) / world_pos_homogeneous.w;

				tmp_all_positions.push_back(world_pos.x);
				tmp_all_positions.push_back(world_pos.y);
				tmp_all_positions.push_back(world_pos.z);

				if (valid_point) {
					// Add position and color
					positions.push_back(world_pos.x);
					positions.push_back(world_pos.y);
					positions.push_back(world_pos.z);

					u32 color_idx = idx * 3;
					colors.push_back(color[color_idx]);
					colors.push_back(color[color_idx + 1]);
					colors.push_back(color[color_idx + 2]);
				}
			}
		}

		for (u32 y = 0; y < height; ++y) {
			for (u32 x = 0; x < width; ++x) {
				u32 idx1 = y * width + x;
				f32 d1 = depth[idx1];
				f32 conf1 = confidence[idx1];
				u32 idx2 = y * width + (x + 1);
				f32 d2 = depth[idx2];
				f32 conf2 = confidence[idx2];
				u32 idx3 = (y + 1) * width + x;
				f32 d3 = depth[idx3];
				f32 conf3 = confidence[idx3];
				u32 idx4 = (y + 1) * width + (x + 1);
				f32 d4 = depth[idx4];
				f32 conf4 = confidence[idx4];

				bool valid_point1 = conf1 > 5.0f && d1 > 0.001f && d1 < 999.0f;
				bool valid_point2 = conf2 > 5.0f && d2 > 0.001f && d2 < 999.0f;
				bool valid_point3 = conf3 > 5.0f && d3 > 0.001f && d3 < 999.0f;
				bool valid_point4 = conf4 > 5.0f && d4 > 0.001f && d4 < 999.0f;

				if (!valid_point1 || !valid_point2 || !valid_point3 || !valid_point4) {
					// INSERT_YOUR_CODE
					// Collect the world positions for the four points of the quad
					glm::vec3 p1(tmp_all_positions[idx1 * 3 + 0], tmp_all_positions[idx1 * 3 + 1],
						tmp_all_positions[idx1 * 3 + 2]);
					glm::vec3 p2(tmp_all_positions[idx2 * 3 + 0], tmp_all_positions[idx2 * 3 + 1],
						tmp_all_positions[idx2 * 3 + 2]);
					glm::vec3 p3(tmp_all_positions[idx3 * 3 + 0], tmp_all_positions[idx3 * 3 + 1],
						tmp_all_positions[idx3 * 3 + 2]);
					glm::vec3 p4(tmp_all_positions[idx4 * 3 + 0], tmp_all_positions[idx4 * 3 + 1],
						tmp_all_positions[idx4 * 3 + 2]);

					// Compute lengths of both diagonals
					float diag1 = glm::length(p1 - p4); // diagonal p1 <-> p4
					float diag2 = glm::length(p2 - p3); // diagonal p2 <-> p3

					// Split along the shorter diagonal
					if (diag1 <= diag2) {
						// Split along p1-p4
						// Triangle 1: p1, p2, p4
						mask_positions.push_back(p1.x);
						mask_positions.push_back(p1.y);
						mask_positions.push_back(p1.z);
						mask_positions.push_back(p2.x);
						mask_positions.push_back(p2.y);
						mask_positions.push_back(p2.z);
						mask_positions.push_back(p4.x);
						mask_positions.push_back(p4.y);
						mask_positions.push_back(p4.z);
						// Triangle 2: p1, p4, p3
						mask_positions.push_back(p1.x);
						mask_positions.push_back(p1.y);
						mask_positions.push_back(p1.z);
						mask_positions.push_back(p4.x);
						mask_positions.push_back(p4.y);
						mask_positions.push_back(p4.z);
						mask_positions.push_back(p3.x);
						mask_positions.push_back(p3.y);
						mask_positions.push_back(p3.z);
					} else {
						// Split along p2-p3
						// Triangle 1: p1, p2, p3
						mask_positions.push_back(p1.x);
						mask_positions.push_back(p1.y);
						mask_positions.push_back(p1.z);
						mask_positions.push_back(p2.x);
						mask_positions.push_back(p2.y);
						mask_positions.push_back(p2.z);
						mask_positions.push_back(p3.x);
						mask_positions.push_back(p3.y);
						mask_positions.push_back(p3.z);
						// Triangle 2: p2, p4, p3
						mask_positions.push_back(p2.x);
						mask_positions.push_back(p2.y);
						mask_positions.push_back(p2.z);
						mask_positions.push_back(p4.x);
						mask_positions.push_back(p4.y);
						mask_positions.push_back(p4.z);
						mask_positions.push_back(p3.x);
						mask_positions.push_back(p3.y);
						mask_positions.push_back(p3.z);
					}
				}
			}
		}
	}

	// Upload to vertex buffers
	if (!positions.empty()) {
		app.points_vbo.mode = GL_POINTS;
		gl_vertex_buffers_upload(app.points_vbo, "a_position", positions, 3, GL_FLOAT, GL_FALSE);
		gl_vertex_buffers_upload(app.points_vbo, "a_color", colors, 3, GL_UNSIGNED_BYTE, GL_TRUE);
		std::cout << "Loaded " << positions.size() / 3 << " points from VGGT output" << std::endl;
	} else {
		std::cerr << "No valid points found in VGGT output" << std::endl;
	}

	if (!mask_positions.empty()) {
		app.mask_vbo.mode = GL_TRIANGLES;
		gl_vertex_buffers_upload(app.mask_vbo, "a_position", mask_positions, 3, GL_FLOAT, GL_FALSE);
		std::cout << "Loaded " << mask_positions.size() / 3 << " triangles from VGGT output" << std::endl;
	} else {
		std::cerr << "No masked content found in VGGT output" << std::endl;
	}

	// Camera data stored on app_t for use by renderers/tools
}
