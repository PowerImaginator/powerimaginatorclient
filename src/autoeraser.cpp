#include "pch.h"

#include "autoeraser.h"

glm::vec4 autoeraser_project_point_to_fragcoord(glm::vec3 const& point, glm::mat4 const& view_mat,
	glm::mat4 const& proj_mat, GLuint const viewport_width, GLuint const viewport_height) {
	// Transform to clip space
	glm::vec4 clip_pos = proj_mat * view_mat * glm::vec4(point, 1.0f);
	if (clip_pos.w <= 0.000001f) {
		return glm::vec4(0.0f);
	}

	// Convert from clip space to normalized device coordinates (NDC)
	glm::vec3 ndc = glm::vec3(clip_pos) / clip_pos.w;

	// Convert NDC to window coordinates (like gl_FragCoord)
	// gl_FragCoord.xy = (ndc.xy * 0.5 + 0.5) * viewport_size
	// gl_FragCoord.z = (ndc.z * 0.5 + 0.5)
	// gl_FragCoord.w = 1.0 / clip_pos.w
	glm::vec4 frag_coord;
	frag_coord.x = (ndc.x * 0.5f + 0.5f) * viewport_width;
	frag_coord.y = (ndc.y * 0.5f + 0.5f) * viewport_height;
	frag_coord.z = ndc.z * 0.5f + 0.5f; // Assuming depth range [0,1]
	frag_coord.w = 1.0f / clip_pos.w; // 1/w for perspective correction

	return frag_coord;
}

void autoeraser_prepare_depth_buffer(std::vector<f32>& out_gl_depths, vtkSmartPointer<vtkPoints> const& points,
	std::unordered_map<u64, u64>& idxs_map, glm::mat4 const& view_mat, glm::mat4 const& proj_mat, u32 const width,
	u32 const height) {
	out_gl_depths.resize(width * height);
	for (u32 row = 0; row < height; ++row) {
		for (u32 col = 0; col < width; ++col) {
			u32 const row_col_i = row * width + col;

			if (idxs_map.find(row_col_i) == idxs_map.end()) {
				out_gl_depths[row_col_i] = 1.0f;
				continue;
			}

			u64 const idx = idxs_map.at(row_col_i);
			double pt[3];
			points->GetPoint(idx, pt);
			glm::vec3 const point(pt[0], pt[1], pt[2]);
			glm::vec4 const frag_coord =
				autoeraser_project_point_to_fragcoord(point, view_mat, proj_mat, width, height);
			out_gl_depths[row_col_i] = frag_coord.z;
		}
	}
}

float autoeraser_sample_depth(glm::vec2 const& uv, std::vector<f32> const& depths, u32 const width, u32 const height) {
	// Clamp UV coordinates to valid range [0,1]
	glm::vec2 clamped_uv = glm::clamp(uv / glm::vec2(width, height), 0.0f, 1.0f);

	// Convert UV coordinates to pixel coordinates
	// UV (0,0) is bottom-left, but depth map row 0 is top
	// So we need to flip the Y coordinate
	float x = clamped_uv.x * (width - 1);
	float y = (1.0f - clamped_uv.y) * (height - 1); // Flip Y: UV bottom-to-top -> depth map top-to-bottom

	// Get integer and fractional parts for bilinear interpolation
	u32 x0 = static_cast<u32>(x);
	u32 y0 = static_cast<u32>(y);
	u32 x1 = glm::min(x0 + 1, width - 1);
	u32 y1 = glm::min(y0 + 1, height - 1);

	float fx = x - x0;
	float fy = y - y0;

	// Sample the four corner depths
	float depth00 = depths[y0 * width + x0]; // Top-left
	float depth10 = depths[y0 * width + x1]; // Top-right
	float depth01 = depths[y1 * width + x0]; // Bottom-left
	float depth11 = depths[y1 * width + x1]; // Bottom-right

	// If any of the corner depths are invalid, return invalid value
	if (depth00 >= 0.999999f || depth10 >= 0.999999f || depth01 >= 0.999999f || depth11 >= 0.999999f) {
		return 1.0f;
	}

	// Bilinear interpolation
	float depth_top = glm::mix(depth00, depth10, fx);
	float depth_bottom = glm::mix(depth01, depth11, fx);
	float final_depth = glm::mix(depth_top, depth_bottom, fy);

	return final_depth;

	/*
	glm::vec2 const clamped_uv = glm::clamp(uv / glm::vec2(width, height), 0.0f, 1.0f);
	u32 const x = clamped_uv.x * (f32)(width - 1);
	u32 const y = (1.0f - clamped_uv.y) * (f32)(height - 1); // Flip Y: UV bottom-to-top -> depth map top-to-bottom
	return depths[y * width + x];
	*/
}

void autoeraser_erase_points(vtkSmartPointer<vtkPoints>& out_points, vtkSmartPointer<vtkUnsignedCharArray>& out_colors,
	vtkSmartPointer<vtkPoints> const& in_points, vtkSmartPointer<vtkUnsignedCharArray> const& in_colors,
	std::vector<f32> const& in_cur_chunk_depths, glm::mat4 const& cur_chunk_view_mat, glm::mat4 const& proj_mat,
	u32 const cur_chunk_width, u32 const cur_chunk_height) {
	out_points->Reset();
	out_colors->Reset();
	out_colors->SetNumberOfComponents(3);

	for (vtkIdType i = 0; i < in_points->GetNumberOfPoints(); ++i) {
		double pt[3];
		in_points->GetPoint(i, pt);
		glm::vec3 const point(pt[0], pt[1], pt[2]);
		glm::vec4 const frag_coord = autoeraser_project_point_to_fragcoord(
			point, cur_chunk_view_mat, proj_mat, cur_chunk_width, cur_chunk_height);

		bool should_keep = true;

		if (frag_coord.w > 0.000001f && frag_coord.x >= 0.0f && frag_coord.x < cur_chunk_width &&
			frag_coord.y >= 0.0f && frag_coord.y < cur_chunk_height) {
			float compare_depth = autoeraser_sample_depth(glm::vec2(frag_coord.x, frag_coord.y),
				in_cur_chunk_depths, cur_chunk_width, cur_chunk_height);
			if (compare_depth < 0.9999999f) {
				if (frag_coord.z < compare_depth + 0.0000001f) {
					should_keep = false;
				}
			}
		}

		if (should_keep) {
			out_points->InsertNextPoint(pt);
			out_colors->InsertNextTuple3(
				in_colors->GetTuple(i)[0], in_colors->GetTuple(i)[1], in_colors->GetTuple(i)[2]);
		}
	}

	out_points->Squeeze();
	out_colors->Squeeze();
}
