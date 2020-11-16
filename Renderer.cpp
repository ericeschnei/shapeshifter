#include "Renderer.hpp"
#include "GL.hpp"
#include "Load.hpp"
#include "data_path.hpp"
#include "gl_compile_program.hpp"
#include "gl_errors.hpp"
#include <algorithm>
#include <iterator>
#include <vector>
#include <fstream>

Load < GLuint > texture_program(LoadTagDefault, []() -> GLuint *{

	std::ifstream vfs(data_path("shaders/texture_vertex.glsl"));
	std::string vertex_shader(
		(std::istreambuf_iterator<char>(vfs)),
		(std::istreambuf_iterator<char>())
	);

	std::ifstream ffs(data_path("shaders/texture_fragment.glsl"));
	std::string fragment_shader(
		(std::istreambuf_iterator<char>(ffs)),
		(std::istreambuf_iterator<char>())
	);

	GLuint program = gl_compile_program(
		vertex_shader,
		fragment_shader
	);

	GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");

	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now
	glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0
	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now

	return new GLuint(program);

});

struct BackgroundProgram {
	GLuint program;
	GLint Time_float;
};

// program that simply draws the background.
Load < BackgroundProgram > background_program(LoadTagDefault, []() -> BackgroundProgram *{

	std::ifstream vfs(data_path("shaders/background_vertex.glsl"));
	std::string vertex_shader(
		(std::istreambuf_iterator<char>(vfs)),
		(std::istreambuf_iterator<char>())
	);

	std::ifstream ffs(data_path("shaders/background_fragment.glsl"));
	std::string fragment_shader(
		(std::istreambuf_iterator<char>(ffs)),
		(std::istreambuf_iterator<char>())
	);

	GLuint program = gl_compile_program(
		vertex_shader,
		fragment_shader
	);

	return new BackgroundProgram {
		program,
		glGetUniformLocation(program, "Time")
	};

});

float quad_verts[] = {
	-1.0f,  1.0f,  0.0f,  1.0f,
	-1.0f, -1.0f,  0.0f,  0.0f,
	 1.0f, -1.0f,  1.0f,  0.0f,
	-1.0f,  1.0f,  0.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,  0.0f,
	 1.0f,  1.0f,  1.0f,  1.0f
};

Renderer::Renderer() {

	// construct OpenGL stuff

	auto make_fb = [](
		GLuint *fbo,
		GLuint *tex,
		GLuint *rbo,
		GLint  scaling_type,
		bool   depth,
		GLuint x,
		GLuint y
	) {

		// framebuffer
		glGenFramebuffers(1, fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, *fbo);

		// texture
		glGenTextures(1, tex);
		glBindTexture(GL_TEXTURE_2D, *tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, scaling_type); // TODO experiment with GL_NEAREST
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, scaling_type); // TODO experiment with GL_NEAREST
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glBindTexture(GL_TEXTURE_2D, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex, 0);

		if (depth) {
			// renderbuffer (for depth/stencil)
			glGenRenderbuffers(1, rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, *rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, x, y);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *rbo);
		}


		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		GL_ERRORS();

	};

	auto make_vb = [](
		GLuint *vbo,
		GLuint *vao,
		const std::vector<size_t> &sizes,
		size_t total_size
	) {

		glGenVertexArrays(1, vao);
		glGenBuffers(1, vbo);

		GLuint offset = 0;

		glBindVertexArray(*vao);
		glBindBuffer(GL_ARRAY_BUFFER, *vbo);

		for (size_t i = 0; i < sizes.size(); i++) {

			glVertexAttribPointer(
				i,
				sizes[i],
				GL_FLOAT,
				GL_FALSE,
				total_size * sizeof(float),
				(GLbyte *)0 + offset * sizeof(float)
			);
			glEnableVertexAttribArray(i);

		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		GL_ERRORS();

	};

	make_fb(&tiny_fbo, &tiny_tex, &tiny_rbo, GL_NEAREST, true, ScreenWidth, ScreenHeight);
	make_vb(&quad_vbo, &quad_vao, {2, 2}, 4);

	// load single quad onto vb
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(quad_verts),
		quad_verts,
		GL_STATIC_DRAW
	);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

}

Renderer::~Renderer() {}

void Renderer::draw(const glm::uvec2 &drawable_size) {

	// first pass
	glBindFramebuffer(GL_FRAMEBUFFER, tiny_fbo);
	glViewport(0, 0, ScreenWidth, ScreenHeight);
	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	// draw all pixel things here
	glUseProgram(background_program->program);

	glUniform1f(background_program->Time_float, total_elapsed);

	glBindVertexArray(quad_vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	glUseProgram(0);

	// clear entire screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, drawable_size.x, drawable_size.y);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	// get real drawable size
	size_t real_drawable_scale = std::min(
		drawable_size.x * ScreenHeight,
		drawable_size.y * ScreenWidth
	);
	glm::uvec2 actual_drawable_size = glm::uvec2(
		real_drawable_scale / ScreenHeight,
		real_drawable_scale / ScreenWidth
	);
	glViewport(
		(drawable_size.x - actual_drawable_size.x) / 2,
		(drawable_size.y - actual_drawable_size.y) / 2,
		actual_drawable_size.x,
		actual_drawable_size.y
	);

	glUseProgram(*texture_program);
	glBindVertexArray(quad_vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tiny_tex);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);

	GL_ERRORS();

}

void Renderer::update(float elapsed) {
	total_elapsed += elapsed;
}
