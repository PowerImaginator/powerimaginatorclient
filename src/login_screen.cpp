#include "pch.h"

#include "exchange.h"
#include "login_screen.h"

bool login_screen_update(f64 const dt) {
	UNUSED(dt);
	exchange_init(g_exchange, "http://localhost:8000", "http://localhost:8001", "password1");
	return true;
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
