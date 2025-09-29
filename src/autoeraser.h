#pragma once

#include "pch.h"

glm::vec4 autoeraser_project_point_to_fragcoord(glm::vec3 const& point, glm::mat4 const& view_matrix,
	glm::mat4 const& projection_matrix, GLuint const viewport_width, GLuint const viewport_height);

void autoeraser_prepare_depth_buffer(std::vector<f32>& out_gl_depths, vtkSmartPointer<vtkPoints> const& points,
	std::unordered_map<u64, u64>& idxs_map, glm::mat4 const& view_mat, glm::mat4 const& proj_mat, u32 const width,
	u32 const height);

float autoeraser_sample_depth(glm::vec2 const& uv, std::vector<f32> const& depths, u32 const width, u32 const height);

void autoeraser_erase_points(vtkSmartPointer<vtkPoints>& out_points, vtkSmartPointer<vtkUnsignedCharArray>& out_colors,
	vtkSmartPointer<vtkPoints> const& in_points, vtkSmartPointer<vtkUnsignedCharArray> const& in_colors,
	std::vector<f32> const& in_cur_chunk_depths, glm::mat4 const& cur_chunk_view_mat, glm::mat4 const& proj_mat,
	u32 const cur_chunk_width, u32 const cur_chunk_height);
