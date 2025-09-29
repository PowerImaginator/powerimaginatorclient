#include "pch.h"

#include "exchange.h"
#include "io_utils.h"

void convert_chans(std::vector<u8>& out, u32 const out_chans, std::vector<u8> const& in, u32 const width,
	u32 const height, u32 const in_chans) {
	out.clear();
	out.resize(width * height * out_chans);
	if (in_chans == out_chans) {
		// copy
		out = in;
	} else if (out_chans == 1) {
		// average
		for (u32 i = 0; i < width * height; ++i) {
			f32 avg = 0.0f;
			for (u32 c = 0; c < in_chans; ++c) {
				avg += static_cast<f32>(in[i * in_chans + c]);
			}
			avg /= static_cast<f32>(in_chans);
			out[i] = static_cast<u8>(avg);
		}
	} else if (out_chans == 3 && in_chans == 4) {
		// drop alpha
		for (u32 i = 0; i < width * height; ++i) {
			for (u32 c = 0; c < 3; ++c) {
				out[i * 3 + c] = in[i * 4 + c];
			}
		}
	} else {
		// unsupported
		assert_release(false);
	}
}

exchange_t g_exchange;

void exchange_init(exchange_t& exchange, std::string const& server_url, std::string const& api_token) {
	exchange.server_url = server_url;
	exchange.api_token = api_token;

	nlohmann::json result;
	http_get(result, server_url, "/config?api_token=" + httplib::encode_uri_component(exchange.api_token), 200);
	exchange.renderer_internal_width = result["width"];
	exchange.renderer_internal_height = result["height"];
	exchange.renderer_internal_fov_y = result["fov_y"];
	exchange.renderer_allow_brush_strength = result["allow_brush_strength"];
}

void exchange_destroy(exchange_t& exchange) {
	UNUSED(exchange);
}

void exchange_allocate_chunk(exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height) {
	assert_release(exchange.chunks.find(chunk_id) == exchange.chunks.end());
	exchange.chunks[chunk_id] = exchange_chunk_t();
	exchange.chunks[chunk_id].width = width;
	exchange.chunks[chunk_id].height = height;
}

void exchange_free_chunk(exchange_t& exchange, const u32 chunk_id) {
	exchange.chunks.erase(chunk_id);
}

void exchange_write_colors_in(exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height,
	u32 const chans, std::vector<u8> const& colors) {
	assert_release(exchange.chunks.find(chunk_id) != exchange.chunks.end());
	exchange_chunk_t& chunk = exchange.chunks[chunk_id];
	assert_release(width == chunk.width && height == chunk.height);
	convert_chans(chunk.colors_in, 3, colors, width, height, chans);
}

void exchange_write_mask_in(exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height,
	u32 const chans, std::vector<u8> const& mask) {
	assert_release(exchange.chunks.find(chunk_id) != exchange.chunks.end());
	exchange_chunk_t& chunk = exchange.chunks[chunk_id];
	assert_release(width == chunk.width && height == chunk.height);
	convert_chans(chunk.mask_in, 1, mask, width, height, chans);
}

void exchange_write_world_pos_in(exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height,
	u32 const chans, std::vector<f32> const& world_pos) {
	assert_release(exchange.chunks.find(chunk_id) != exchange.chunks.end());
	exchange_chunk_t& chunk = exchange.chunks[chunk_id];
	assert_release(width == chunk.width && height == chunk.height);
	assert_release(chans == 4);
	chunk.world_pos_in = world_pos;
}

void exchange_write_view_mat_in(exchange_t& exchange, u32 const chunk_id, std::vector<f32> const& view_mat) {
	assert_release(exchange.chunks.find(chunk_id) != exchange.chunks.end());
	exchange_chunk_t& chunk = exchange.chunks[chunk_id];
	chunk.view_mat_in = view_mat;
}

void exchange_write_proj_mat_in(exchange_t& exchange, u32 const chunk_id, std::vector<f32> const& proj_mat) {
	assert_release(exchange.chunks.find(chunk_id) != exchange.chunks.end());
	exchange_chunk_t& chunk = exchange.chunks[chunk_id];
	chunk.proj_mat_in = proj_mat;
}

void exchange_write_eraser_2d_mask_in(exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height,
	u32 const chans, std::vector<u8> const& eraser_2d_mask) {
	assert_release(exchange.chunks.find(chunk_id) != exchange.chunks.end());
	exchange_chunk_t& chunk = exchange.chunks[chunk_id];
	assert_release(width == chunk.width && height == chunk.height);
	convert_chans(chunk.eraser_2d_mask_in, 1, eraser_2d_mask, width, height, chans);
}

std::vector<u8> const* exchange_load_colors_in(
	exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height, u32 const chans) {
	assert_release(exchange.chunks.find(chunk_id) != exchange.chunks.end());
	exchange_chunk_t const& chunk = exchange.chunks[chunk_id];
	assert_release(chunk.colors_in.size() == width * height * chans);
	return &chunk.colors_in;
}

std::vector<u8> const* exchange_load_colors_out(
	exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height, u32 const chans) {
	assert_release(exchange.chunks.find(chunk_id) != exchange.chunks.end());
	exchange_chunk_t const& chunk = exchange.chunks[chunk_id];
	assert_release(chunk.colors_out.size() == width * height * chans);
	return &chunk.colors_out;
}

std::vector<u8> const* exchange_load_mask_in(
	exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height, u32 const chans) {
	assert_release(exchange.chunks.find(chunk_id) != exchange.chunks.end());
	exchange_chunk_t const& chunk = exchange.chunks[chunk_id];
	assert_release(chunk.mask_in.size() == width * height * chans);
	return &chunk.mask_in;
}

std::vector<f32> const* exchange_load_world_pos_in(
	exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height, u32 const chans) {
	assert_release(exchange.chunks.find(chunk_id) != exchange.chunks.end());
	exchange_chunk_t const& chunk = exchange.chunks[chunk_id];
	assert_release(chunk.world_pos_in.size() == width * height * chans);
	return &chunk.world_pos_in;
}

std::vector<f32> const* exchange_load_depth_out(
	exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height, u32 const chans) {
	assert_release(exchange.chunks.find(chunk_id) != exchange.chunks.end());
	exchange_chunk_t const& chunk = exchange.chunks[chunk_id];
	assert_release(chunk.depth_out.size() == width * height * chans);
	return &chunk.depth_out;
}

std::vector<f32> const* exchange_load_view_mat_in(exchange_t& exchange, u32 const chunk_id) {
	assert_release(exchange.chunks.find(chunk_id) != exchange.chunks.end());
	exchange_chunk_t const& chunk = exchange.chunks[chunk_id];
	assert_release(chunk.view_mat_in.size() == 16);
	return &chunk.view_mat_in;
}

std::vector<f32> const* exchange_load_proj_mat_in(exchange_t& exchange, u32 const chunk_id) {
	assert_release(exchange.chunks.find(chunk_id) != exchange.chunks.end());
	exchange_chunk_t const& chunk = exchange.chunks[chunk_id];
	assert_release(chunk.proj_mat_in.size() == 16);
	return &chunk.proj_mat_in;
}

std::vector<u8> const* exchange_load_eraser_2d_mask_in(
	exchange_t& exchange, u32 const chunk_id, u32 const width, u32 const height, u32 const chans) {
	assert_release(exchange.chunks.find(chunk_id) != exchange.chunks.end());
	exchange_chunk_t const& chunk = exchange.chunks[chunk_id];
	assert_release(chunk.eraser_2d_mask_in.size() == width * height * chans);
	return &chunk.eraser_2d_mask_in;
}

void exchange_run_inpainting(exchange_t& exchange, u32 const chunk_id, std::string const& prompt,
	std::string const& negative_prompt, u32 const seed, u32 const num_inference_steps, f32 const strength) {
	assert_release(exchange.chunks.find(chunk_id) != exchange.chunks.end());
	exchange_chunk_t& chunk = exchange.chunks[chunk_id];

	std::string colors_png_str;
	assert_release(chunk.colors_in.size() == chunk.width * chunk.height * 3);
	write_image_upload_item(colors_png_str, chunk.width, chunk.height, 3, chunk.colors_in.data(), chunk.width * 3);

	std::string mask_png_str;
	assert_release(chunk.mask_in.size() == chunk.width * chunk.height * 1);
	write_image_upload_item(mask_png_str, chunk.width, chunk.height, 1, chunk.mask_in.data(), chunk.width);

	httplib::UploadFormDataItems items = {
		{"init_image_file", colors_png_str, "init_image_file.png", "image/png"},
		{"mask_image_file", mask_png_str, "mask_image_file.png", "image/png"},
		{"prompt", prompt, "", "text/plain"},
		{"negative_prompt", negative_prompt, "", "text/plain"},
		{"seed", std::to_string(seed), "", "text/plain"},
		{"num_inference_steps", std::to_string(num_inference_steps), "", "text/plain"},
		{"strength", std::to_string(strength), "", "text/plain"},
	};

	std::string response_str;
	http_post(response_str, "http://127.0.0.1:8000", "/inpaint", items, 200);
	int out_w = 0, out_h = 0, out_chans = 0;
	u8* decoded = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(response_str.data()),
		static_cast<int>(response_str.size()), &out_w, &out_h, &out_chans, 3);
	assert_release(decoded != nullptr);
	assert_release(out_w == static_cast<int>(chunk.width));
	assert_release(out_h == static_cast<int>(chunk.height));
	assert_release(out_chans == 3);
	chunk.colors_out.assign(decoded, decoded + out_w * out_h * 3);
	stbi_image_free(decoded);
}

void exchange_run_depth_estimation(exchange_t& exchange, u32 const chunk_id) {
	assert_release(exchange.chunks.find(chunk_id) != exchange.chunks.end());
	exchange_chunk_t& chunk = exchange.chunks[chunk_id];

	std::string colors_png_str;
	assert_release(chunk.colors_out.size() == chunk.width * chunk.height * 3);
	write_image_upload_item(colors_png_str, chunk.width, chunk.height, 3, chunk.colors_out.data(), chunk.width * 3);

	httplib::UploadFormDataItems items = {
		{"image_file", colors_png_str, "image_file.png", "image/png"},
	};

	std::string response_str;
	http_post(response_str, "http://127.0.0.1:8000", "/depth", items, 200);
	chunk.depth_out.assign(reinterpret_cast<f32*>(response_str.data()),
		reinterpret_cast<f32*>(response_str.data()) + (response_str.size() / sizeof(f32)));
}
