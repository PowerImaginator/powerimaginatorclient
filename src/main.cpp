#include "pch.h"

#include "app.h"
#include "login_screen.h"

int main() {
	NFD_Init();

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	f32 const primary_monitor_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
	GLFWwindow* window = glfwCreateWindow(
		880.0f * primary_monitor_scale, 544.0f * primary_monitor_scale, "PowerImaginator", nullptr, nullptr);
	glfwSetWindowSizeLimits(
		window, 880.0f * primary_monitor_scale, 544.0f * primary_monitor_scale, GLFW_DONT_CARE, GLFW_DONT_CARE);
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	glfwSwapInterval(1);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(primary_monitor_scale);
	style.FontScaleDpi = primary_monitor_scale;
	style.WindowRounding = 0.0f;
	style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 410 core");

	if (!login_screen_main(window)) {
		return 1;
	}

	app_t app;
	app.window = window;
	app_init(app);
	glfwSetWindowUserPointer(window, &app);

	f64 prev_time = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		s32 display_width = 0, display_height = 0;
		glfwGetWindowSize(window, &display_width, &display_height);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		f64 const cur_time = glfwGetTime();
		f64 const dt = cur_time - prev_time;
		prev_time = cur_time;
		app_update(app, dt);

		app_imgui(app, dt);

		ImGui::Render();
		glViewport(0, 0, display_width, display_height);
		glClearColor(0.1f, 0.1f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	app_shutdown(app);

	glfwDestroyWindow(window);
	glfwTerminate();

	NFD_Quit();

	return 0;
}
