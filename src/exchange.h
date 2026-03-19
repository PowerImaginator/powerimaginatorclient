#pragma once

#include "pch.h"

struct exchange_image_t {
	u32 width = 0;
	u32 height = 0;
	std::vector<u8> colors;
};

struct exchange_t {
	std::string inpaint_server_url;
	std::string vggt_server_url;

	exchange_image_t bake_color;
	std::vector<u8> bake_mask;
	exchange_image_t paint_result;
	std::vector<exchange_image_t> generated_outputs;
};

extern exchange_t g_exchange;

void exchange_init(exchange_t& exchange, std::string const& inpaint_server_url,
	std::string const& vggt_server_url);
void exchange_destroy(exchange_t& exchange);

void exchange_set_bake_inputs(exchange_t& exchange, u32 width, u32 height, std::vector<u8> const& color,
	std::vector<u8> const& mask);
void exchange_run_inpainting(exchange_t& exchange, std::string const& prompt, std::string const& negative_prompt,
	u32 seed, u32 num_inference_steps, f32 strength);
void exchange_run_vggt(exchange_t& exchange, std::vector<u8>& output_bin);
