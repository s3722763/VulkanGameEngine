#include "Camera.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <sdl/SDL_scancode.h>

void Camera::updateCamera(float deltaS, const uint8_t* currentKeyStates) {
    const float speed = 10 * deltaS;

    if (currentKeyStates[SDL_SCANCODE_W] == 1) {
        this->position += speed * this->lookAt;
    } else if (currentKeyStates[SDL_SCANCODE_S] == 1) {
        this->position -= speed * this->lookAt;
    }

    if (currentKeyStates[SDL_SCANCODE_A] == 1) {
        this->position -= glm::normalize(glm::cross(this->lookAt, this->up)) * speed;
    } else if (currentKeyStates[SDL_SCANCODE_D] == 1) {
        this->position += glm::normalize(glm::cross(this->lookAt, this->up)) * speed;
    }
}

void Camera::updateLookDirection(float pitchChange, float yawChange) {
    this->pitch -= pitchChange;
    this->yaw += yawChange;

    auto new_look_direction = glm::vec3(std::cosf(glm::radians(this->yaw)) * std::cosf(glm::radians(this->pitch)),
                                        std::sinf(glm::radians(this->pitch)),
                                        std::sin(glm::radians(this->yaw)) * std::cos(glm::radians(this->pitch)));

    this->lookAt = glm::normalize(new_look_direction);
}

glm::mat4 Camera::generateView() {
	return glm::lookAt(this->position, this->position + this->lookAt, this->up);
}

glm::vec3 Camera::getPosition() {
	return this->position;
}

void Camera::setPosition(glm::vec3 pos) {
    this->position = pos;
}
