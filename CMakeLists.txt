cmake_minimum_required(VERSION 3.25)

if(WIN32)
	set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "")
	set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows" ${CMAKE_PREFIX_PATH})
	set(X_VCPKG_APPLOCAL_DEPS_INSTALL TRUE CACHE BOOL "")
endif()

project(PowerImaginator VERSION 0.1.0)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.4 REQUIRED)
find_package(
	VTK REQUIRED COMPONENTS
	CommonCore
	CommonDataModel
	CommonMath
	CommonTransforms
	FiltersGeneral
	FiltersSources
	FiltersPoints
	IOPLY
)

add_subdirectory(extern/nativefiledialog-extended)

set(
	PROJECT_HEADERS
	"src/alignment_tool.h"
	"src/app.h"
	"src/autoeraser.h"
	"src/camera_tool.h"
	"src/eraser_2d_tool.h"
	"src/exchange.h"
	"src/fly_camera.h"
	"src/gl_render_pass.h"
	"src/gl_shader.h"
	"src/gl_vertex_buffers.h"
	"src/io_utils.h"
	"src/login_screen.h"
	"src/meshgrid_utils.h"
	"src/paint_tool.h"
	"src/paint_result_tool.h"
	"src/points_renderer.h"
)
set(
	PROJECT_SOURCES
	"src/main.cpp"
	"src/alignment_tool.cpp"
	"src/app.cpp"
	"src/autoeraser.cpp"
	"src/camera_tool.cpp"
	"src/eraser_2d_tool.cpp"
	"src/exchange.cpp"
	"src/fly_camera.cpp"
	"src/gl_render_pass.cpp"
	"src/gl_shader.cpp"
	"src/gl_vertex_buffers.cpp"
	"src/io_utils.cpp"
	"src/login_screen.cpp"
	"src/meshgrid_utils.cpp"
	"src/paint_tool.cpp"
	"src/paint_result_tool.cpp"
	"src/points_renderer.cpp"
)

set(
  EXTERN_HEADERS
  extern/cpp-httplib
  extern/glad/include
  extern/glm
  extern/imgui
  extern/json/include
  extern/stb
  extern
)
file(
  GLOB EXTERN_SOURCES
  extern/glad/src/glad.c
  extern/imgui/imgui.cpp
  extern/imgui/imgui_demo.cpp
  extern/imgui/imgui_draw.cpp
  extern/imgui/imgui_tables.cpp
  extern/imgui/imgui_widgets.cpp
  extern/imgui/misc/cpp/imgui_stdlib.cpp
  extern/imgui/backends/imgui_impl_glfw.cpp
  extern/imgui/backends/imgui_impl_opengl3.cpp
  extern/implementation.c
)

file(GLOB PROJECT_CONFIGS CMakeLists.txt .gitattributes .gitignore .gitmodules)
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${EXTERN_SOURCES} ${PROJECT_CONFIGS})
target_precompile_headers(${PROJECT_NAME} PRIVATE "src/pch.h")
set_source_files_properties(${EXTERN_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS true)
target_compile_definitions(${PROJECT_NAME} PRIVATE IMGUI_DEFINE_MATH_OPERATORS IMGUI_IMPL_OPENGL_LOADER_GLAD)
set_source_files_properties(${EXTERN_SOURCES} PROPERTIES COMPILE_WARNING_AS_ERROR OFF)
set_target_properties(
	${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	COMPILE_WARNING_AS_ERROR ON
)
if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
	src
	${EXTERN_HEADERS}
)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL glfw ${VTK_LIBRARIES} nfd)

vtk_module_autoinit(
	TARGETS ${PROJECT_NAME}
	MODULES ${VTK_LIBRARIES}
)

if(APPLE)
	# Configure Info.plist
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/branding/Info.plist
		${CMAKE_CURRENT_BINARY_DIR}/Info.plist
		COPYONLY
	)

	# Configure the bundle fixup script
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/cmake/FixBundleMac.cmake.in
		${CMAKE_CURRENT_BINARY_DIR}/FixBundleMac.cmake
		@ONLY
	)

	# Create proper macOS bundle structure manually
	install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${PROJECT_NAME}.app/Contents/MacOS
	)

	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
		DESTINATION ${PROJECT_NAME}.app/Contents
	)

	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/branding/icon.icns"
		DESTINATION ${PROJECT_NAME}.app/Contents/Resources
	)

	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders
		DESTINATION ${PROJECT_NAME}.app/Contents/Resources
	)

	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/branding/Assets.car"
		DESTINATION ${PROJECT_NAME}.app/Contents/Resources
	)

	# Set RPATH for bundle
	set_target_properties(${PROJECT_NAME} PROPERTIES
		INSTALL_RPATH "@executable_path/../Frameworks"
		BUILD_WITH_INSTALL_RPATH TRUE
	)

	# Use DragNDrop generator to create DMG from the .app bundle
	set(CPACK_GENERATOR "DragNDrop")
	set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME}")
	set(CPACK_DMG_FORMAT "UDBZ")

	# Run the bundle fixup script after installation
	install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/FixBundleMac.cmake")

	include(CPack)
elseif(WIN32)
	# Configure the bundle fixup script
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/cmake/FixBundleWindows.cmake.in
		${CMAKE_CURRENT_BINARY_DIR}/FixBundleWindows.cmake
		@ONLY
	)

	# Install executable
	install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION .
	)

	# Install shaders
	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders
		DESTINATION .
	)

	# Use ZIP generator for Windows
	set(CPACK_GENERATOR "ZIP")
	set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-windows-x64")

	# Run the bundle fixup script after installation
	install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/FixBundleWindows.cmake")

	include(CPack)
endif()
