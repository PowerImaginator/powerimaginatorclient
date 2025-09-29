#pragma once

#include "pch.h"

void build_meshgrid(vtkSmartPointer<vtkPoints> out_points, vtkSmartPointer<vtkUnsignedCharArray> out_colors,
	u32 const width, u32 const height, f32 const fov_y, std::vector<f32> const& in_depths,
	std::vector<u8> const& in_colors);

void align_meshgrid(vtkSmartPointer<vtkPoints> out_cur_chunk_points,
	vtkSmartPointer<vtkPoints> const& in_cur_chunk_points, vtkSmartPointer<vtkPoints> out_combined_points,
	vtkSmartPointer<vtkPoints> const& in_combined_points, std::vector<f32> const& in_world_pos_floats,
	std::vector<u8> const& in_mask, u32 const width, u32 const height, u32 const chunk_id,
	std::vector<glm::ivec2> const& in_keypoints_colrow, bool average_ref_and_cur_tps);

void remove_meshgrid_outliers(vtkSmartPointer<vtkPoints> out_cur_chunk_points,
	vtkSmartPointer<vtkUnsignedCharArray> out_cur_chunk_colors,
	std::unordered_map<u64, u64>& out_cur_chunk_idxs_map, vtkSmartPointer<vtkPoints> const& in_cur_chunk_points,
	vtkSmartPointer<vtkUnsignedCharArray> const& in_cur_chunk_colors, std::vector<f32> const& in_cur_chunk_depths,
	f32 const min_depth, f32 const max_depth);

/*
void remove_meshgrid_unmasked(vtkSmartPointer<vtkPoints> out_cur_chunk_points,
	vtkSmartPointer<vtkUnsignedCharArray> out_cur_chunk_colors,
	std::unordered_map<u64, u64>& out_cur_chunk_idxs_map, vtkSmartPointer<vtkPoints> const& in_cur_chunk_points,
	vtkSmartPointer<vtkUnsignedCharArray> const& in_cur_chunk_colors,
	std::unordered_map<u64, u64>& in_cur_chunk_idxs_map, std::vector<u8> const& in_mask);
*/

void remove_meshgrid_unwanted(vtkSmartPointer<vtkPoints> out_cur_chunk_points,
	vtkSmartPointer<vtkUnsignedCharArray> out_cur_chunk_colors,
	std::unordered_map<u64, u64>& out_cur_chunk_idxs_map, vtkSmartPointer<vtkPoints> const& in_cur_chunk_points,
	vtkSmartPointer<vtkUnsignedCharArray> const& in_cur_chunk_colors,
	std::unordered_map<u64, u64>& in_cur_chunk_idxs_map, std::vector<f32> const& in_cur_chunk_depths,
	std::vector<u8> const& in_mask, std::vector<u8> const& eraser_2d_mask, f32 const min_depth,
	f32 const max_depth);
