#include "BackgroundFramebuffer.hpp"
#include "GL.hpp"
#include "data_path.hpp"
#include <vector>
#include "load_save_png.hpp"
#include "gl_errors.hpp"
#include "View.hpp"

BackgroundFramebuffer::BackgroundFramebuffer(GLuint empty_vao)
	: Framebuffer(
		data_path("shaders/fullscreen_quad_v.glsl"),
		data_path("shaders/bg_cloud_f.glsl"),
		empty_vao) {



	Time_float = glGetUniformLocation(program, "Time");
	Camera_vec2 = glGetUniformLocation(program, "Camera");
	Parallax_float = glGetUniformLocation(program, "Parallax");

	// generate blue noise texture
	glGenTextures(1, &blue_noise_tex);
	glBindTexture(GL_TEXTURE_2D, blue_noise_tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// generate the texture
	std::vector<glm::u8vec4> image_pixels;
	glm::uvec2 image_size;
	load_png(data_path("blue_noise.png"), &image_size, &image_pixels, LowerLeftOrigin);

	std::vector<uint8_t> pixels;
	pixels.reserve(image_size.x * image_size.y);

	for (size_t i = 0; i < image_size.x * image_size.y; i++) {
		pixels[i] = image_pixels[i].r;
	}

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_R8,
		image_size.x,
		image_size.y,
		0,
		GL_RED,
		GL_UNSIGNED_BYTE,
		pixels.data()
	);

	glBindTexture(GL_TEXTURE_2D, 0);
	GL_ERRORS();

}

GLuint BackgroundFramebuffer::draw(GLuint old_tex) {

	// draw clouds, which don't need texture
	Framebuffer::draw(blue_noise_tex);
	return tex;

}

void BackgroundFramebuffer::realloc(const glm::uvec2 &drawable_size) {
	Framebuffer::realloc(glm::uvec2(View::FieldWidth + 1, View::FieldHeight + 1));
}
