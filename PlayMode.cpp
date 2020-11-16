#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "Renderer.hpp"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "glm/ext/quaternion_geometric.hpp"
#include "glm/geometric.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

Load < Renderer > renderer_(LoadTagDefault, []() -> Renderer const * {
	return new Renderer();
});

PlayMode::PlayMode() : renderer(*renderer_) {

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	return false;
}

void PlayMode::update(float elapsed) {

	// move_force is in pixels/s^2 (it's actually acceleration, don't tell anybody)
	// max_velocity is in pixels/s
	static constexpr float MOVE_FORCE = 400.0f;
	static constexpr float MAX_VELOCITY = 200.0f;
	static constexpr float FRICTION = 0.95f;

	glm::vec2 force_vector = glm::vec2(0, 0);
	int num_keys;
	const uint8_t *keys = SDL_GetKeyboardState(&num_keys);

	if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) {
		force_vector.y += 1.0f;
	}
	if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
		force_vector.x -= 1.0f;
	}
	if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) {
		force_vector.y -= 1.0f;
	}
	if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
		force_vector.x += 1.0f;
	}

	// apply force
	if (glm::length(force_vector) > 0) {
		force_vector = glm::normalize(force_vector) * MOVE_FORCE;
	}
	char_velocity += force_vector * elapsed;

	// velocity cap
	if (glm::length(char_velocity) > MAX_VELOCITY) {
		char_velocity = glm::normalize(char_velocity) * MAX_VELOCITY;
	}

	//update position
	char_position += char_velocity * elapsed;
	char_velocity *= FRICTION;

	renderer.update_char_position(char_position, 0);

	// update camera to be out of dead space
	static const glm::vec2 MARGIN = glm::vec2(
			Renderer::ScreenWidth, 
			Renderer::ScreenHeight
	) * 1.0f / 3.0f;

	glm::vec2 space_from_camera_center = camera_position - char_position;

	if (space_from_camera_center.x > MARGIN.x) {
		camera_position.x = MARGIN.x + char_position.x;
	}
	if (-space_from_camera_center.x > MARGIN.x) {
		camera_position.x = char_position.x - MARGIN.x;
	}

	if (space_from_camera_center.y > MARGIN.y) {
		camera_position.y = MARGIN.y + char_position.y;
	}
	if (-space_from_camera_center.y > MARGIN.y) {
		camera_position.y = char_position.y - MARGIN.y;
	}

	renderer.update_camera_position(camera_position);
	renderer.update(elapsed);
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	renderer.draw(drawable_size);
	GL_ERRORS();
}
