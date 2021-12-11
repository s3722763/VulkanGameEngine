#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

struct Camera {
	glm::vec3 position = {0, 0, 0};
	glm::vec3 lookAt = { 0, 0, -1 };
	glm::vec3 up = { 0, 1, 0 };
	float yaw = -90;
	float pitch = 0;

	void updateCamera(float deltaS, const uint8_t* currentKeyStates);
	void updateLookDirection(float pitchChange, float yawChange);
	glm::mat4 generateView();
	glm::vec3 getPosition();
	void setPosition(glm::vec3 pos);
};