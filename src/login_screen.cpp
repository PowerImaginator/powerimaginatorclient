#include "pch.h"

#include "exchange.h"
#include "login_screen.h"

bool login_screen_update(f64 const dt) {
	UNUSED(dt);

	static std::string server_url = "http://localhost:8000";
	static std::string api_token = "";

	f32 const margin = 16.0f;
	ImGui::SetNextWindowPos(ImVec2(margin, margin), ImGuiCond_Always);
	ImGui::SetNextWindowSize(
		ImVec2(ImGui::GetIO().DisplaySize.x - margin * 2.0f, ImGui::GetIO().DisplaySize.y - margin * 2.0f),
		ImGuiCond_Always);

	ImGui::Begin("Login", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse);

	ImGui::InputText("Server URL", &server_url);
	ImGui::InputText("API Token", &api_token, ImGuiInputTextFlags_Password);

	bool connected = false;
	if (ImGui::Button("Connect")) {
		while (!server_url.empty() && server_url.back() == '/') {
			server_url.pop_back();
		}

		exchange_init(g_exchange, server_url, api_token);

		connected = true;
	}

	ImGui::End();

	return connected;
}

bool login_screen_main(GLFWwindow* window) {
	bool logged_in = false;

	f64 prev_time = glfwGetTime();
	while (!logged_in && !glfwWindowShouldClose(window)) {
		glfwPollEvents();

		s32 display_width = 0, display_height = 0;
		glfwGetWindowSize(window, &display_width, &display_height);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		f64 const cur_time = glfwGetTime();
		f64 const dt = cur_time - prev_time;
		prev_time = cur_time;
		logged_in = login_screen_update(dt);

		ImGui::Render();
		glViewport(0, 0, display_width, display_height);
		glClearColor(0.1f, 0.1f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	return logged_in;
}
