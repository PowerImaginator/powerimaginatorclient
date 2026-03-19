#include "pch.h"

#include "exchange.h"
#include "io_utils.h"

exchange_t g_exchange;

namespace {
void require_colors(exchange_image_t const& image) {
	assert_release(image.width > 0);
	assert_release(image.height > 0);
	assert_release(image.colors.size() == static_cast<size_t>(image.width) * static_cast<size_t>(image.height) * 3);
}
}

void exchange_init(exchange_t& exchange, std::string const& inpaint_server_url,
	std::string const& vggt_server_url) {
	exchange.inpaint_server_url = inpaint_server_url;
	exchange.vggt_server_url = vggt_server_url;
	exchange.generated_outputs.clear();
}

void exchange_destroy(exchange_t& exchange) {
	exchange.generated_outputs.clear();
	exchange.bake_color = exchange_image_t();
	exchange.bake_mask.clear();
	exchange.paint_result = exchange_image_t();
}

void exchange_set_bake_inputs(exchange_t& exchange, u32 width, u32 height, std::vector<u8> const& color,
	std::vector<u8> const& mask) {
	assert_release(width > 0);
	assert_release(height > 0);
	assert_release(color.size() == static_cast<size_t>(width) * static_cast<size_t>(height) * 3);
	assert_release(mask.size() == static_cast<size_t>(width) * static_cast<size_t>(height));

	exchange.bake_color.width = width;
	exchange.bake_color.height = height;
	exchange.bake_color.colors = color;
	exchange.bake_mask = mask;
}

void exchange_run_inpainting(exchange_t& exchange, std::string const& prompt, std::string const& negative_prompt,
	u32 seed, u32 num_inference_steps, f32 strength) {
	require_colors(exchange.bake_color);
	assert_release(exchange.bake_mask.size() ==
		static_cast<size_t>(exchange.bake_color.width) * static_cast<size_t>(exchange.bake_color.height));

	std::string colors_png_str;
	write_image_upload_item(colors_png_str, static_cast<s32>(exchange.bake_color.width),
		static_cast<s32>(exchange.bake_color.height), 3, exchange.bake_color.colors.data(),
		static_cast<s32>(exchange.bake_color.width) * 3);

	std::string mask_png_str;
	write_image_upload_item(mask_png_str, static_cast<s32>(exchange.bake_color.width),
		static_cast<s32>(exchange.bake_color.height), 1, exchange.bake_mask.data(),
		static_cast<s32>(exchange.bake_color.width));

	std::string path = "/inpaint";

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
	http_post(response_str, exchange.inpaint_server_url, path, items, 200);

	int out_w = 0;
	int out_h = 0;
	int out_chans = 0;
	u8* decoded = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(response_str.data()),
		static_cast<int>(response_str.size()), &out_w, &out_h, &out_chans, 3);
	assert_release(decoded != nullptr);
	assert_release(out_w == static_cast<int>(exchange.bake_color.width));
	assert_release(out_h == static_cast<int>(exchange.bake_color.height));

	exchange.paint_result.width = static_cast<u32>(out_w);
	exchange.paint_result.height = static_cast<u32>(out_h);
	exchange.paint_result.colors.assign(decoded, decoded + out_w * out_h * 3);
	stbi_image_free(decoded);

	exchange.generated_outputs.push_back(exchange.paint_result);
}

void exchange_run_vggt(exchange_t& exchange, std::vector<u8>& output_bin) {
	assert_release(!exchange.generated_outputs.empty());

	httplib::UploadFormDataItems items;
	items.reserve(exchange.generated_outputs.size());
	for (size_t i = 0; i < exchange.generated_outputs.size(); ++i) {
		auto const& image = exchange.generated_outputs[i];
		require_colors(image);

		std::string image_png_str;
		write_image_upload_item(image_png_str, static_cast<s32>(image.width), static_cast<s32>(image.height), 3,
			image.colors.data(), static_cast<s32>(image.width) * 3);
		items.push_back({"image_files", image_png_str, "image_" + std::to_string(i) + ".png", "image/png"});
	}

	std::string path = "/vggt";

	std::string response_str;
	http_post(response_str, exchange.vggt_server_url, path, items, 200);
	output_bin.assign(response_str.begin(), response_str.end());
}
