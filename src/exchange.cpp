#include "pch.h"

#include "exchange.h"
#include "io_utils.h"

#include <thread>

exchange_t g_exchange;

namespace {
void require_colors(exchange_image_t const& image) {
	assert_release(image.width > 0);
	assert_release(image.height > 0);
	assert_release(image.colors.size() == static_cast<size_t>(image.width) * static_cast<size_t>(image.height) * 3);
}

std::string get_fal_endpoint(exchange_edit_model_t const model) {
	switch (model) {
	case exchange_edit_model_t::QwenImageEdit2511:
		return "fal-ai/qwen-image-edit-2511";
	case exchange_edit_model_t::FluxKlein4B:
		return "fal-ai/flux-2/klein/4b/edit/lora";
	case exchange_edit_model_t::FluxKlein9B:
		return "fal-ai/flux-2/klein/9b/edit/lora";
	default:
		assert_release(false);
		return "";
	}
}

std::string get_fal_first_generation_endpoint(std::string endpoint) {
	std::string const edit_suffix = "/edit/lora";
	if (endpoint.size() >= edit_suffix.size() &&
		endpoint.compare(endpoint.size() - edit_suffix.size(), edit_suffix.size(), edit_suffix) == 0) {
		endpoint.resize(endpoint.size() - edit_suffix.size());
	}
	return endpoint;
}

std::string get_qwen_acceleration(exchange_qwen_acceleration_t const acceleration) {
	switch (acceleration) {
	case exchange_qwen_acceleration_t::None:
		return "none";
	case exchange_qwen_acceleration_t::Regular:
		return "regular";
	case exchange_qwen_acceleration_t::High:
		return "high";
	default:
		assert_release(false);
		return "regular";
	}
}

std::string base64_encode(std::string const& input) {
	static char const* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string output;
	output.reserve(((input.size() + 2) / 3) * 4);

	size_t i = 0;
	while (i + 3 <= input.size()) {
		u32 const octet_a = static_cast<u8>(input[i + 0]);
		u32 const octet_b = static_cast<u8>(input[i + 1]);
		u32 const octet_c = static_cast<u8>(input[i + 2]);
		u32 const triple = (octet_a << 16) | (octet_b << 8) | octet_c;

		output.push_back(chars[(triple >> 18) & 0x3F]);
		output.push_back(chars[(triple >> 12) & 0x3F]);
		output.push_back(chars[(triple >> 6) & 0x3F]);
		output.push_back(chars[(triple >> 0) & 0x3F]);
		i += 3;
	}

	size_t const remaining = input.size() - i;
	if (remaining == 1) {
		u32 const octet_a = static_cast<u8>(input[i + 0]);
		u32 const triple = octet_a << 16;
		output.push_back(chars[(triple >> 18) & 0x3F]);
		output.push_back(chars[(triple >> 12) & 0x3F]);
		output.push_back('=');
		output.push_back('=');
	} else if (remaining == 2) {
		u32 const octet_a = static_cast<u8>(input[i + 0]);
		u32 const octet_b = static_cast<u8>(input[i + 1]);
		u32 const triple = (octet_a << 16) | (octet_b << 8);
		output.push_back(chars[(triple >> 18) & 0x3F]);
		output.push_back(chars[(triple >> 12) & 0x3F]);
		output.push_back(chars[(triple >> 6) & 0x3F]);
		output.push_back('=');
	}

	return output;
}

std::string make_image_data_uri(exchange_image_t const& image) {
	std::string image_png_str;
	write_image_upload_item(image_png_str, static_cast<s32>(image.width), static_cast<s32>(image.height), 3,
		image.colors.data(), static_cast<s32>(image.width) * 3);
	return "data:image/png;base64," + base64_encode(image_png_str);
}

httplib::Headers make_fal_headers() {
	assert_release(std::string(ENV_FAL_API_KEY).size() > 0);
	std::string const authorization = "Key " + std::string(ENV_FAL_API_KEY);
	return {{"Authorization", authorization}};
}

void split_url(std::string const& url, std::string& host, std::string& path) {
	size_t const scheme_end = url.find("://");
	assert_release(scheme_end != std::string::npos);

	size_t const host_start = scheme_end + 3;
	size_t const path_start = url.find('/', host_start);
	if (path_start == std::string::npos) {
		host = url;
		path = "/";
	} else {
		host = url.substr(0, path_start);
		path = url.substr(path_start);
	}
}
} // namespace

void exchange_init(exchange_t& exchange, std::string const& fal_queue_server_url, std::string const& vggt_server_url) {
	exchange.fal_queue_server_url = fal_queue_server_url;
	exchange.vggt_server_url = vggt_server_url;
	exchange.generated_outputs.clear();
}

void exchange_destroy(exchange_t& exchange) {
	exchange.generated_outputs.clear();
	exchange.bake_color = exchange_image_t();
	exchange.bake_mask.clear();
	exchange.paint_result = exchange_image_t();
}

void exchange_set_bake_inputs(
	exchange_t& exchange, u32 width, u32 height, std::vector<u8> const& color, std::vector<u8> const& mask) {
	assert_release(width > 0);
	assert_release(height > 0);
	assert_release(color.size() == static_cast<size_t>(width) * static_cast<size_t>(height) * 3);
	assert_release(mask.size() == static_cast<size_t>(width) * static_cast<size_t>(height));

	exchange.bake_color.width = width;
	exchange.bake_color.height = height;
	exchange.bake_color.colors = color;
	exchange.bake_mask = mask;
}

void exchange_run_image_edit(exchange_t& exchange, exchange_edit_model_t model, std::string const& prompt,
	std::string const& negative_prompt, u32 seed, u32 num_inference_steps, f32 guidance_scale,
	exchange_qwen_acceleration_t qwen_acceleration, bool enable_safety_checker) {
	require_colors(exchange.bake_color);
	bool const is_first_generation = exchange.generated_outputs.empty();
	std::string endpoint = get_fal_endpoint(model);
	if (is_first_generation) {
		endpoint = get_fal_first_generation_endpoint(endpoint);
	}
	httplib::Headers const headers = make_fal_headers();

	nlohmann::json input = {
		{"prompt", prompt},
		{"seed", seed},
		{"num_inference_steps", num_inference_steps},
		{"num_images", 1},
		{"enable_safety_checker", enable_safety_checker},
		{"output_format", "png"},
	};
	if (!is_first_generation) {
		std::string const image_data_uri = make_image_data_uri(exchange.bake_color);
		input["image_urls"] = nlohmann::json::array({image_data_uri});
	}

	if (model == exchange_edit_model_t::QwenImageEdit2511) {
		input["negative_prompt"] = negative_prompt;
		input["guidance_scale"] = std::clamp(guidance_scale, 1.0f, 20.0f);
		input["acceleration"] = get_qwen_acceleration(qwen_acceleration);
	}

	nlohmann::json submit_result;
	http_post(submit_result, exchange.fal_queue_server_url, "/" + endpoint, input, 200, headers);
	assert_release(submit_result.contains("request_id"));
	std::string const request_id = submit_result["request_id"].get<std::string>();
	std::string status_host = exchange.fal_queue_server_url;
	std::string status_path = "/" + endpoint + "/requests/" + request_id + "/status";
	if (submit_result.contains("status_url") && submit_result["status_url"].is_string()) {
		split_url(submit_result["status_url"].get<std::string>(), status_host, status_path);
	}
	std::string response_host = exchange.fal_queue_server_url;
	std::string response_path = "/" + endpoint + "/requests/" + request_id;
	if (submit_result.contains("response_url") && submit_result["response_url"].is_string()) {
		split_url(submit_result["response_url"].get<std::string>(), response_host, response_path);
	}
	std::cout << "FAL request submitted endpoint=" << endpoint << " request_id=" << request_id
			  << " mode=" << (is_first_generation ? "text-to-image" : "image-edit") << std::endl;

	bool completed = false;
	for (u32 i = 0; i < 500; ++i) {
		std::cout << "Polling generation status endpoint=" << endpoint << " request_id=" << request_id
				  << " attempt=" << (i + 1) << std::endl;

		nlohmann::json status_result;
		http_get(status_result, status_host, status_path, 200, headers, true);
		assert_release(status_result.contains("status"));

		std::string const status = status_result["status"].get<std::string>();
		std::cout << "FAL status=" << status << std::endl;
		if (status == "COMPLETED") {
			completed = true;
			break;
		}
		if (status == "FAILED" || status == "CANCELLED") {
			std::cerr << "FAL request ended with non-success status: " << status;
			if (status_result.contains("error")) {
				std::cerr << " error=" << status_result["error"].dump();
			}
			std::cerr << std::endl;
			assert_release(false);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	assert_release(completed);

	nlohmann::json generation_result;
	http_get(generation_result, response_host, response_path, 200, headers);
	assert_release(generation_result.contains("images"));
	auto const& images = generation_result["images"];
	assert_release(images.is_array());
	assert_release(!images.empty());
	assert_release(images[0].contains("url"));
	std::string const output_image_url = images[0]["url"].get<std::string>();

	std::string image_host;
	std::string image_path;
	split_url(output_image_url, image_host, image_path);

	std::string response_str;
	http_get(response_str, image_host, image_path, 200);

	int out_w = 0;
	int out_h = 0;
	int out_chans = 0;
	u8* decoded = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(response_str.data()),
		static_cast<int>(response_str.size()), &out_w, &out_h, &out_chans, 3);
	assert_release(decoded != nullptr);
	if (out_w != static_cast<int>(exchange.bake_color.width) || out_h != static_cast<int>(exchange.bake_color.height)) {
		std::cout << "Generated image size differs from bake size generated=" << out_w << "x" << out_h
				  << " bake=" << exchange.bake_color.width << "x" << exchange.bake_color.height << std::endl;
	}

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
