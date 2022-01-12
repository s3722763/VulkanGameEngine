#pragma once
#include <glm/vec4.hpp>

struct AttenuationFactors {
	float constant;
	float linear;
	float quadratic;
	//float padding;
};

struct PointLights {
	std::vector<glm::vec4> positions;
	std::vector<glm::vec4> baseColours;
	std::vector<AttenuationFactors> attenuationFactors;
};

struct DirectionalLights {
	std::vector<glm::vec4> directions;
	std::vector<glm::vec4> baseColours;
};

struct PointLightCreateInfo {
	glm::vec4 position;
	glm::vec4 baseColour;
	AttenuationFactors attenuationFactors;
};

struct DirectionalLightCreateInfo {
	glm::vec4 direction;
	glm::vec4 baseColour;
};

struct LightingInformation {
	uint32_t numberPointLights = 0;
	uint32_t numberDirectionalLights = 0;
};