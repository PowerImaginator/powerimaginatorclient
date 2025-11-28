#include "pch.h"

#include "mesh_renderer.h"

#include <fstream>

void mesh_renderer_init(mesh_renderer_t& renderer) {
	gl_render_pass_init(renderer.mesh_pass, "shaders/mesh.vert", "shaders/mesh.frag", RENDERER_INTERNAL_WIDTH,
		RENDERER_INTERNAL_HEIGHT,
		{{"o_color", {.internal_format = GL_RGB8, .format = GL_RGB, .type = GL_UNSIGNED_BYTE}},
			{"o_cam_pos", {.internal_format = GL_RGBA32F, .format = GL_RGBA, .type = GL_FLOAT}},
			{"o_world_pos", {.internal_format = GL_RGBA32F, .format = GL_RGBA, .type = GL_FLOAT}},
			{"##DEPTH",
				{.clear_depth = 1.0f,
					.internal_format = GL_DEPTH_COMPONENT32F,
					.format = GL_DEPTH_COMPONENT,
					.type = GL_FLOAT}}});
	// renderer.mesh_pass.cull_face = GL_BACK;

	gl_render_pass_init(renderer.auto_mask_pass, "shaders/quad.vert", "shaders/auto_mask.frag",
		RENDERER_INTERNAL_WIDTH, RENDERER_INTERNAL_HEIGHT,
		{{"o_color", {.internal_format = GL_RGBA32F, .format = GL_RGBA, .type = GL_FLOAT}}});

	renderer.depth_texture_array = 0;
	renderer.confidence_texture_array = 0;
}

void mesh_renderer_upload_camera_textures(
	mesh_renderer_t& renderer, std::vector<vggt_camera_data_t> const& camera_data) {
	if (camera_data.empty()) {
		return;
	}

	// Find maximum width and height
	u32 max_width = 0;
	u32 max_height = 0;
	for (auto const& cam : camera_data) {
		max_width = std::max(max_width, cam.width);
		max_height = std::max(max_height, cam.height);
	}

	u32 num_images = static_cast<u32>(camera_data.size());

	// Create depth texture array
	if (renderer.depth_texture_array == 0) {
		glGenTextures(1, &renderer.depth_texture_array);
	}
	glBindTexture(GL_TEXTURE_2D_ARRAY, renderer.depth_texture_array);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R32F, max_width, max_height, num_images, 0, GL_RED, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Upload depth data for each image
	for (u32 i = 0; i < num_images; ++i) {
		auto const& cam = camera_data[i];
		if (cam.width == max_width && cam.height == max_height) {
			// Direct upload if size matches
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, cam.width, cam.height, 1, GL_RED, GL_FLOAT,
				cam.depth.data());
		} else {
			// Need to pad or resize - for now, just upload what we have (will be cropped)
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, cam.width, cam.height, 1, GL_RED, GL_FLOAT,
				cam.depth.data());
		}
	}

	// Create confidence texture array
	if (renderer.confidence_texture_array == 0) {
		glGenTextures(1, &renderer.confidence_texture_array);
	}
	glBindTexture(GL_TEXTURE_2D_ARRAY, renderer.confidence_texture_array);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R32F, max_width, max_height, num_images, 0, GL_RED, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Upload confidence data for each image
	for (u32 i = 0; i < num_images; ++i) {
		auto const& cam = camera_data[i];
		if (cam.width == max_width && cam.height == max_height) {
			// Direct upload if size matches
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, cam.width, cam.height, 1, GL_RED, GL_FLOAT,
				cam.confidence.data());
		} else {
			// Need to pad or resize - for now, just upload what we have (will be cropped)
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, cam.width, cam.height, 1, GL_RED, GL_FLOAT,
				cam.confidence.data());
		}
	}

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void mesh_renderer_render(
	mesh_renderer_t& renderer, fly_camera_t& camera, gl_vertex_buffers_t& mesh_vbo, gl_vertex_buffers_t& quad_vbo) {
	gl_render_pass_begin(renderer.mesh_pass);
	gl_render_pass_uniform_mat4(renderer.mesh_pass, "u_proj_mat", camera.proj_mat);
	gl_render_pass_uniform_mat4(renderer.mesh_pass, "u_view_mat", camera.view_mat);
	gl_render_pass_draw(renderer.mesh_pass, mesh_vbo);
	gl_render_pass_end(renderer.mesh_pass);

	gl_render_pass_begin(renderer.auto_mask_pass);
	gl_render_pass_uniform_mat4(renderer.auto_mask_pass, "u_viewport_proj_mat", camera.proj_mat);
	gl_render_pass_uniform_mat4(renderer.auto_mask_pass, "u_viewport_view_mat", camera.view_mat);
	gl_render_pass_uniform_vec2(renderer.auto_mask_pass, "u_viewport_resolution",
		glm::vec2(static_cast<f32>(renderer.auto_mask_pass.width),
			static_cast<f32>(renderer.auto_mask_pass.height)));
	gl_render_pass_uniform_texture(renderer.auto_mask_pass, "u_tex_viewport_color",
		renderer.mesh_pass.internal_output_descriptors["o_color"].texture, GL_TEXTURE0);
	gl_render_pass_uniform_texture(renderer.auto_mask_pass, "u_tex_viewport_world_pos",
		renderer.mesh_pass.internal_output_descriptors["o_world_pos"].texture, GL_TEXTURE1);
	gl_render_pass_uniform_texture(renderer.auto_mask_pass, "u_tex_viewport_depth",
		renderer.mesh_pass.internal_output_descriptors["##DEPTH"].texture, GL_TEXTURE2);

	// Upload camera data arrays
	if (!renderer.camera_data.empty()) {
		std::vector<glm::mat4> extrinsics;
		std::vector<glm::mat3> intrinsics;
		std::vector<glm::vec2> resolutions;
		extrinsics.reserve(renderer.camera_data.size());
		intrinsics.reserve(renderer.camera_data.size());
		resolutions.reserve(renderer.camera_data.size());
		for (auto const& cam : renderer.camera_data) {
			extrinsics.push_back(cam.extrinsic);
			intrinsics.push_back(cam.intrinsic);
			resolutions.push_back(glm::vec2(static_cast<f32>(cam.width), static_cast<f32>(cam.height)));
		}
		gl_render_pass_uniform_mat4_array(renderer.auto_mask_pass, "u_camera_extrinsics", extrinsics);
		gl_render_pass_uniform_mat3_array(renderer.auto_mask_pass, "u_camera_intrinsics", intrinsics);
		gl_render_pass_uniform_vec2_array(renderer.auto_mask_pass, "u_camera_resolutions", resolutions);
		gl_render_pass_uniform_int(
			renderer.auto_mask_pass, "u_num_cameras", static_cast<GLint>(renderer.camera_data.size()));

		// Bind texture arrays - use GL_TEXTURE3 and GL_TEXTURE4 to avoid conflicts
		if (renderer.depth_texture_array != 0) {
			gl_render_pass_uniform_texture_array(renderer.auto_mask_pass, "u_tex_camera_depth",
				renderer.depth_texture_array, GL_TEXTURE3);
		}
		if (renderer.confidence_texture_array != 0) {
			gl_render_pass_uniform_texture_array(renderer.auto_mask_pass, "u_tex_camera_confidence",
				renderer.confidence_texture_array, GL_TEXTURE4);
		}
	}

	gl_render_pass_draw(renderer.auto_mask_pass, quad_vbo);
	gl_render_pass_end(renderer.auto_mask_pass);
}

gl_render_pass_t* mesh_renderer_get_final_render_pass(mesh_renderer_t& renderer) {
	return &renderer.mesh_pass;
}

GLuint mesh_renderer_get_final_fbo_texture(mesh_renderer_t& renderer) {
	return renderer.auto_mask_pass.internal_output_descriptors["o_color"].texture;
}

GLuint mesh_renderer_get_final_fbo_width(mesh_renderer_t& renderer) {
	return renderer.auto_mask_pass.width;
}

GLuint mesh_renderer_get_final_fbo_height(mesh_renderer_t& renderer) {
	return renderer.auto_mask_pass.height;
}

void load_vggt_mesh(std::string const& filename, gl_vertex_buffers_t& mesh_vbo, f32 const proximity_threshold,
	std::vector<vggt_camera_data_t>& out_camera_data) {
	std::ifstream file(filename, std::ios::binary);
	if (!file) {
		std::cerr << "Failed to open file: " << filename << std::endl;
		return;
	}

	// Read number of images
	u32 num_images = 0;
	file.read(reinterpret_cast<char*>(&num_images), sizeof(u32));

	out_camera_data.clear();
	out_camera_data.reserve(num_images);

	std::vector<f32> positions;
	std::vector<u8> colors;

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

		// Store camera data for this image with transformed extrinsic
		vggt_camera_data_t camera_data;
		camera_data.extrinsic = glm::inverse(first_cam_to_world) * opengl_conversion * cam_to_world;
		camera_data.intrinsic = intrinsic;
		camera_data.confidence = confidence;
		camera_data.depth = depth;
		camera_data.width = width;
		camera_data.height = height;
		out_camera_data.push_back(std::move(camera_data));

		// Read color buffer (H*W*3, u8)
		std::vector<u8> color(width * height * 3);
		file.read(reinterpret_cast<char*>(color.data()), sizeof(u8) * width * height * 3);

		// Convert depth map to 3D positions (same as points renderer)
		std::vector<glm::vec3> world_positions(width * height);
		std::vector<bool> valid_pixel(width * height, false);

		f32 fx = camera_data.intrinsic[0][0];
		f32 fy = camera_data.intrinsic[1][1];
		f32 cx = camera_data.intrinsic[2][0];
		f32 cy = camera_data.intrinsic[2][1];

		for (u32 y = 0; y < height; ++y) {
			for (u32 x = 0; x < width; ++x) {
				u32 idx = y * width + x;
				f32 d = depth[idx];
				f32 conf = confidence[idx];

				// Skip points with low confidence or invalid depth
				if (conf < 5.0f || d <= 0.001f || d >= 999.0f) {
					valid_pixel[idx] = false;
					continue;
				}

				valid_pixel[idx] = true;

				// Convert pixel coordinates to camera space
				f32 px = static_cast<f32>(x);
				f32 py = static_cast<f32>(y);

				glm::vec3 cam_pos;
				cam_pos.x = (px - cx) * d / fx;
				cam_pos.y = (py - cy) * d / fy;
				cam_pos.z = d;

				// Transform to world space using extrinsic matrix
				glm::vec4 world_pos_homogeneous = camera_data.extrinsic * glm::vec4(cam_pos, 1.0f);
				world_positions[idx] = glm::vec3(world_pos_homogeneous) / world_pos_homogeneous.w;
			}
		}

		// Create meshgrid quads - for each quad, check if all vertices are close enough
		// Each quad is made of 2 triangles: (x,y), (x+1,y), (x,y+1) and (x+1,y), (x+1,y+1), (x,y+1)
		for (u32 y = 0; y < height - 1; ++y) {
			for (u32 x = 0; x < width - 1; ++x) {
				u32 idx_tl = y * width + x; // top-left
				u32 idx_tr = y * width + (x + 1); // top-right
				u32 idx_bl = (y + 1) * width + x; // bottom-left
				u32 idx_br = (y + 1) * width + (x + 1); // bottom-right

				// Check if all pixels are valid
				if (!valid_pixel[idx_tl] || !valid_pixel[idx_tr] || !valid_pixel[idx_bl] ||
					!valid_pixel[idx_br]) {
					continue;
				}

				glm::vec3 const& v_tl = world_positions[idx_tl];
				glm::vec3 const& v_tr = world_positions[idx_tr];
				glm::vec3 const& v_bl = world_positions[idx_bl];
				glm::vec3 const& v_br = world_positions[idx_br];

				// Check if all vertices are within proximity threshold
				f32 max_dist = 0.0f;
				max_dist = std::max(max_dist, glm::distance(v_tl, v_tr));
				max_dist = std::max(max_dist, glm::distance(v_tl, v_bl));
				max_dist = std::max(max_dist, glm::distance(v_tl, v_br));
				max_dist = std::max(max_dist, glm::distance(v_tr, v_bl));
				max_dist = std::max(max_dist, glm::distance(v_tr, v_br));
				max_dist = std::max(max_dist, glm::distance(v_bl, v_br));

				if (max_dist > proximity_threshold) {
					continue;
				}

				// Get colors for each vertex
				u32 color_idx_tl = idx_tl * 3;
				u32 color_idx_tr = idx_tr * 3;
				u32 color_idx_bl = idx_bl * 3;
				u32 color_idx_br = idx_br * 3;

				// Add first triangle: (top-left, bottom-left, top-right) - counter-clockwise winding
				positions.push_back(v_tl.x);
				positions.push_back(v_tl.y);
				positions.push_back(v_tl.z);
				colors.push_back(color[color_idx_tl]);
				colors.push_back(color[color_idx_tl + 1]);
				colors.push_back(color[color_idx_tl + 2]);

				positions.push_back(v_bl.x);
				positions.push_back(v_bl.y);
				positions.push_back(v_bl.z);
				colors.push_back(color[color_idx_bl]);
				colors.push_back(color[color_idx_bl + 1]);
				colors.push_back(color[color_idx_bl + 2]);

				positions.push_back(v_tr.x);
				positions.push_back(v_tr.y);
				positions.push_back(v_tr.z);
				colors.push_back(color[color_idx_tr]);
				colors.push_back(color[color_idx_tr + 1]);
				colors.push_back(color[color_idx_tr + 2]);

				// Add second triangle: (top-right, bottom-left, bottom-right) - counter-clockwise winding
				positions.push_back(v_tr.x);
				positions.push_back(v_tr.y);
				positions.push_back(v_tr.z);
				colors.push_back(color[color_idx_tr]);
				colors.push_back(color[color_idx_tr + 1]);
				colors.push_back(color[color_idx_tr + 2]);

				positions.push_back(v_bl.x);
				positions.push_back(v_bl.y);
				positions.push_back(v_bl.z);
				colors.push_back(color[color_idx_bl]);
				colors.push_back(color[color_idx_bl + 1]);
				colors.push_back(color[color_idx_bl + 2]);

				positions.push_back(v_br.x);
				positions.push_back(v_br.y);
				positions.push_back(v_br.z);
				colors.push_back(color[color_idx_br]);
				colors.push_back(color[color_idx_br + 1]);
				colors.push_back(color[color_idx_br + 2]);
			}
		}
	}

	// Upload to vertex buffers
	if (!positions.empty()) {
		mesh_vbo.mode = GL_TRIANGLES;
		gl_vertex_buffers_upload(mesh_vbo, "a_position", positions, 3, GL_FLOAT, GL_FALSE);
		gl_vertex_buffers_upload(mesh_vbo, "a_color", colors, 3, GL_UNSIGNED_BYTE, GL_TRUE);
		std::cout << "Loaded " << positions.size() / 3 << " vertices (" << positions.size() / 9
			  << " triangles) from VGGT output as mesh" << std::endl;
	} else {
		std::cerr << "No valid mesh quads found in VGGT output" << std::endl;
	}
}
