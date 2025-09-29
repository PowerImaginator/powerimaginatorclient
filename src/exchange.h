#pragma once

#include "pch.h"

struct exchange_chunk_t {
	u32 width = 0, height = 0;
	std::vector<u8> colors_in;
	std::vector<f32> world_pos_in;
	std::vector<u8> mask_in;
	std::vector<f32> view_mat_in;
	std::vector<f32> proj_mat_in;
	std::vector<u8> colors_out;
	std::vector<f32> depth_out;
	std::vector<u8> eraser_2d_mask_in;
};

struct exchange_t {
	std::unordered_map<u32, exchange_chunk_t> chunks;

	std::string server_url;
	std::string api_token;
	u32 renderer_internal_width;
	u32 renderer_internal_height;
	f32 renderer_internal_fov_y;
	bool renderer_allow_brush_strength;
};

extern exchange_t g_exchange;

void exchange_init(exchange_t& exchange, std::string const& server_url, std::string const& api_token);
void exchange_destroy(exchange_t& exchange);

void exchange_allocate_chunk(exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height);
void exchange_free_chunk(exchange_t& exchange, u32 const chunk_id);

void exchange_write_colors_in(exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height,
	u32 const chans, std::vector<u8> const& colors);
void exchange_write_mask_in(exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height,
	u32 const chans, std::vector<u8> const& mask);
void exchange_write_world_pos_in(exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height,
	u32 const chans, std::vector<f32> const& world_pos);
void exchange_write_view_mat_in(exchange_t& exchange, u32 const chunk_id, std::vector<f32> const& view_mat);
void exchange_write_proj_mat_in(exchange_t& exchange, u32 const chunk_id, std::vector<f32> const& proj_mat);
void exchange_write_eraser_2d_mask_in(exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height,
	u32 const chans, std::vector<u8> const& eraser_2d_mask);

std::vector<u8> const* exchange_load_colors_in(
	exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height, u32 const chans);
std::vector<u8> const* exchange_load_colors_out(
	exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height, u32 const chans);
std::vector<u8> const* exchange_load_mask_in(
	exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height, u32 const chans);
std::vector<f32> const* exchange_load_world_pos_in(
	exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height, u32 const chans);
std::vector<f32> const* exchange_load_depth_out(
	exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height, u32 const chans);
std::vector<f32> const* exchange_load_view_mat_in(exchange_t& exchange, u32 const chunk_id);
std::vector<f32> const* exchange_load_proj_mat_in(exchange_t& exchange, u32 const chunk_id);
std::vector<u8> const* exchange_load_eraser_2d_mask_in(
	exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height, u32 const chans);

void exchange_run_inpainting(exchange_t& exchange, u32 const chunk_id, std::string const& prompt,
	std::string const& negative_prompt, u32 const seed, u32 const num_inference_steps, f32 const strength);
void exchange_run_depth_estimation(exchange_t& exchange, u32 const chunk_id);
