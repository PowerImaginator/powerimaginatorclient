#include "pch.h"

#include "meshgrid_utils.h"

void build_meshgrid(vtkSmartPointer<vtkPoints> out_points, vtkSmartPointer<vtkUnsignedCharArray> out_colors,
	u32 const width, u32 const height, f32 const fov_y, std::vector<f32> const& in_depths,
	std::vector<u8> const& in_colors) {
	out_points->Reset();
	out_colors->Reset();
	out_colors->SetNumberOfComponents(3);

	for (u32 row = 0; row < height; ++row) {
		for (u32 col = 0; col < width; ++col) {
			u32 const row_col_i = row * width + col;

			f32 const u = static_cast<f32>(col) / static_cast<f32>(width - 1);
			f32 const v = 1.0f - static_cast<f32>(row) / static_cast<f32>(height - 1);

			f32 const nx = u * 2.0f - 1.0f;
			f32 const ny = v * 2.0f - 1.0f;
			f32 d = in_depths[row_col_i];

			f32 const h = tanf(fov_y * 0.5f) * d;
			f32 const w = h * (static_cast<f32>(width) / static_cast<f32>(height));

			f32 const x = nx * w;
			f32 const y = ny * h;
			f32 const z = -d;

			out_points->InsertNextPoint(x, y, z);
			out_colors->InsertNextTuple3(in_colors[row_col_i * 3 + 0], in_colors[row_col_i * 3 + 1],
				in_colors[row_col_i * 3 + 2]);
		}
	}

	out_points->Squeeze();
	out_colors->Squeeze();
}

void align_meshgrid(vtkSmartPointer<vtkPoints> out_cur_chunk_points,
	vtkSmartPointer<vtkPoints> const& in_cur_chunk_points, vtkSmartPointer<vtkPoints> out_combined_points,
	vtkSmartPointer<vtkPoints> const& in_combined_points, std::vector<f32> const& in_world_pos_floats,
	std::vector<u8> const& in_mask, u32 const width, u32 const height, u32 const chunk_id,
	std::vector<glm::ivec2> const& in_keypoints_colrow, bool average_ref_and_cur_tps) {
	UNUSED(in_world_pos_floats);
	UNUSED(in_mask);
	UNUSED(height);

	out_cur_chunk_points->Reset();
	out_combined_points->Reset();

	if (chunk_id == 0) {
		out_cur_chunk_points->DeepCopy(in_cur_chunk_points);
		out_cur_chunk_points->Squeeze();
		out_combined_points->Squeeze();
		return;
	}

	vtkNew<vtkPoints> cur_chunk_keypoints;
	vtkNew<vtkPoints> ref_keypoints;

	for (u64 i = 0; i < in_keypoints_colrow.size(); ++i) {
		u64 const row_col_i = in_keypoints_colrow[i].y * width + in_keypoints_colrow[i].x;
		cur_chunk_keypoints->InsertNextPoint(in_cur_chunk_points->GetPoint(row_col_i));
		ref_keypoints->InsertNextPoint(in_world_pos_floats[row_col_i * 4 + 0],
			in_world_pos_floats[row_col_i * 4 + 1], in_world_pos_floats[row_col_i * 4 + 2]);
	}

	std::cout << "Rigid alignment" << std::endl;
	vtkNew<vtkLandmarkTransform> lm_cur_to_ref;
	lm_cur_to_ref->SetModeToSimilarity();
	lm_cur_to_ref->SetSourceLandmarks(cur_chunk_keypoints);
	lm_cur_to_ref->SetTargetLandmarks(ref_keypoints);
	lm_cur_to_ref->Update();
	vtkNew<vtkPoints> cur_chunk_keypoints_after_lm;
	lm_cur_to_ref->TransformPoints(cur_chunk_keypoints, cur_chunk_keypoints_after_lm);

	std::cout << "TPS alignment" << std::endl;

	vtkNew<vtkThinPlateSplineTransform> tps_cur_to_ref;
	tps_cur_to_ref->SetBasisToR();
	tps_cur_to_ref->SetSourceLandmarks(cur_chunk_keypoints_after_lm);
	tps_cur_to_ref->SetTargetLandmarks(ref_keypoints);
	tps_cur_to_ref->Update();

	vtkNew<vtkThinPlateSplineTransform> tps_ref_to_cur;
	tps_ref_to_cur->SetBasisToR();
	tps_ref_to_cur->SetSourceLandmarks(ref_keypoints);
	tps_ref_to_cur->SetTargetLandmarks(cur_chunk_keypoints_after_lm);
	tps_ref_to_cur->Update();

	std::cout << "Applying transforms to current chunk" << std::endl;
	vtkNew<vtkPoints> cur_chunk_after_lm;
	lm_cur_to_ref->TransformPoints(in_cur_chunk_points, cur_chunk_after_lm);
	vtkNew<vtkPoints> cur_chunk_after_tps;
	tps_cur_to_ref->TransformPoints(cur_chunk_after_lm, cur_chunk_after_tps);

	if (average_ref_and_cur_tps) {
		std::cout << "Applying TPS transform to previous chunks" << std::endl;
		tps_ref_to_cur->TransformPoints(in_combined_points, out_combined_points);

		std::cout << "Averaging TPS transforms" << std::endl;
		for (vtkIdType i = 0; i < cur_chunk_after_tps->GetNumberOfPoints(); ++i) {
			double ptBefore[3], ptAfter[3], ptOut[3];
			cur_chunk_after_lm->GetPoint(i, ptBefore);
			cur_chunk_after_tps->GetPoint(i, ptAfter);
			for (u64 j = 0; j < 3; ++j) {
				ptOut[j] = (ptBefore[j] + ptAfter[j]) * 0.5f;
			}
			cur_chunk_after_tps->SetPoint(i, ptOut);
		}
		for (vtkIdType i = 0; i < out_combined_points->GetNumberOfPoints(); ++i) {
			double ptBefore[3], ptAfter[3], ptOut[3];
			in_combined_points->GetPoint(i, ptBefore);
			out_combined_points->GetPoint(i, ptAfter);
			for (u64 j = 0; j < 3; ++j) {
				ptOut[j] = (ptBefore[j] + ptAfter[j]) * 0.5f;
			}
			out_combined_points->SetPoint(i, ptOut);
		}
	} else {
		out_combined_points->DeepCopy(in_combined_points);
	}

	out_cur_chunk_points->DeepCopy(cur_chunk_after_tps);

	std::cout << "Alignment finished" << std::endl;

	out_cur_chunk_points->Squeeze();
	out_combined_points->Squeeze();
}

void remove_meshgrid_outliers(vtkSmartPointer<vtkPoints> out_cur_chunk_points,
	vtkSmartPointer<vtkUnsignedCharArray> out_cur_chunk_colors,
	std::unordered_map<u64, u64>& out_cur_chunk_idxs_map, vtkSmartPointer<vtkPoints> const& in_cur_chunk_points,
	std::unordered_map<u64, u64>& in_cur_chunk_idxs_map) {
	UNUSED(out_cur_chunk_points);
	UNUSED(out_cur_chunk_colors);
	UNUSED(out_cur_chunk_idxs_map);
	UNUSED(in_cur_chunk_points);
	UNUSED(in_cur_chunk_idxs_map);

	// TODO: Fix me
	// Important to get the output idxs_map correct
	/*
	vtkNew<vtkPolyData> poly_data_1;
	poly_data_1->SetPoints(in_cur_chunk_points);
	poly_data_1->GetPointData()->SetScalars(in_cur_chunk_colors);

	std::cout << "Removing outliers" << std::endl;
	vtkNew<vtkStatisticalOutlierRemoval> outlier_removal;
	outlier_removal->SetStandardDeviationFactor(0.5);
	outlier_removal->SetSampleSize(128);
	outlier_removal->SetGenerateOutliers(true);
	outlier_removal->SetInputData(poly_data_1);
	outlier_removal->Update();
	std::cout << "Done removing outliers" << std::endl;

	vtkIdType const* const point_map = outlier_removal->GetPointMap();
	for (u64 i = 0; i < static_cast<u64>(in_cur_chunk_points->GetNumberOfPoints()); ++i) {
		if (point_map[i] < 0) {
			continue;
		}
		out_cur_chunk_idxs_map[i] = static_cast<u64>(point_map[i]);
	}

	out_cur_chunk_points->Reset();
	out_cur_chunk_colors->Reset();
	out_cur_chunk_points->DeepCopy(outlier_removal->GetOutput()->GetPoints());
	out_cur_chunk_colors->DeepCopy(outlier_removal->GetOutput()->GetPointData()->GetScalars());
	out_cur_chunk_points->Squeeze();
	out_cur_chunk_colors->Squeeze();
	*/
}

void remove_meshgrid_unwanted(vtkSmartPointer<vtkPoints> out_cur_chunk_points,
	vtkSmartPointer<vtkUnsignedCharArray> out_cur_chunk_colors,
	std::unordered_map<u64, u64>& out_cur_chunk_idxs_map, vtkSmartPointer<vtkPoints> const& in_cur_chunk_points,
	vtkSmartPointer<vtkUnsignedCharArray> const& in_cur_chunk_colors,
	std::unordered_map<u64, u64>& in_cur_chunk_idxs_map, std::vector<f32> const& in_cur_chunk_depths,
	std::vector<u8> const& in_mask, std::vector<u8> const& eraser_2d_mask, f32 const min_depth,
	f32 const max_depth) {
	out_cur_chunk_points->Reset();
	out_cur_chunk_colors->Reset();
	out_cur_chunk_colors->SetNumberOfComponents(3);

	u64 num_inserted = 0;
	for (u64 i = 0; i < in_mask.size(); ++i) {
		if (in_mask[i] == 0) {
			continue;
		}
		if (eraser_2d_mask[i] > 0) {
			continue;
		}
		if (in_cur_chunk_depths[i] < min_depth || in_cur_chunk_depths[i] > max_depth) {
			continue;
		}
		if (in_cur_chunk_idxs_map.find(i) == in_cur_chunk_idxs_map.end()) {
			continue;
		}
		u64 idx = in_cur_chunk_idxs_map.at(i);
		out_cur_chunk_points->InsertNextPoint(in_cur_chunk_points->GetPoint(idx));
		out_cur_chunk_colors->InsertNextTuple(in_cur_chunk_colors->GetTuple(idx));
		out_cur_chunk_idxs_map[i] = num_inserted;
		++num_inserted;
	}

	out_cur_chunk_points->Squeeze();
	out_cur_chunk_colors->Squeeze();
}
