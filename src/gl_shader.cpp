#include "pch.h"

#include "gl_shader.h"
#include "io_utils.h"

GLuint gl_shader_compile(GLenum const type, std::string const& filename, std::string const& src) {
	GLuint const shader = glCreateShader(type);
	GLchar const* const src_ptr = src.data();
	glShaderSource(shader, 1, &src_ptr, nullptr);
	glCompileShader(shader);
	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar buf[512];
		glGetShaderInfoLog(shader, sizeof(buf), nullptr, buf);
		std::cerr << "shader_compile: \"" + filename + "\" compilation failed:\n" << buf << "\n";
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

void gl_shader_init(gl_shader_t& shader, std::string const& vert_filename, std::string const& frag_filename) {
	std::string full_vert_filename = get_resources_prefix() + vert_filename;
	std::string full_frag_filename = get_resources_prefix() + frag_filename;

	std::string vert_src, frag_src;
	load_file(vert_src, full_vert_filename);
	load_file(frag_src, full_frag_filename);
	GLuint const vert_shader = gl_shader_compile(GL_VERTEX_SHADER, full_vert_filename, vert_src);
	GLuint const frag_shader = gl_shader_compile(GL_FRAGMENT_SHADER, full_frag_filename, frag_src);
	assert_release(vert_shader && frag_shader);

	shader.program = glCreateProgram();
	glAttachShader(shader.program, vert_shader);
	glAttachShader(shader.program, frag_shader);
	glLinkProgram(shader.program);
	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);

	GLint success = 0;
	glGetProgramiv(shader.program, GL_LINK_STATUS, &success);
	if (!success) {
		GLchar buf[512];
		glGetProgramInfoLog(shader.program, sizeof(buf), nullptr, buf);
		std::cerr << "Error: Shader program \"" + full_vert_filename + "\" and \"" + full_frag_filename +
				"\" linking failed: \n"
			  << buf << "\n";
		glDeleteProgram(shader.program);
		assert_release(false);
	}
}
