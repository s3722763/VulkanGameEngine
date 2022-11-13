#version 460
#extension GL_KHR_vulkan_glsl : enable

struct AttenuationFactors {
	float constant;
	float linear;
	float quadratic;
};

// Point Lights
layout(std140, set = 0, binding = 1) readonly buffer PointLightPositions {
	vec4 positions[];
} pointLightPositions;

layout(std140, set = 0, binding = 2) readonly buffer PointLightBaseColours {
	vec4 baseColours[];
} pointLightBaseColours;

layout(std140, set = 0, binding = 3) readonly buffer PointLightAttenuationFactors {
	AttenuationFactors attenuationFactors[];
} pointLightAttenuationFactors;

// Directional Lights
layout(std140, set = 0, binding = 4) readonly buffer DirectionalLightDirections {
	vec4 directions[];
} directionalLightDirections;

layout(std140, set = 0, binding = 5) readonly buffer DirectionalLightBaseColours {
	vec4 colours[];
} directionalLightBaseColours;

layout(std140, set = 0, binding = 6) readonly buffer LightInformation {
	uint numberPointLights;
	uint numberDirectionalLights;
} lightingInfo;

layout (location = 0) in vec2 texCoord;

layout (location = 0) out vec4 outFragColour;

layout (set = 1, binding = 0) uniform sampler2D positionTexture;
layout (set = 1, binding = 1) uniform sampler2D normalTexture;
layout (set = 1, binding = 2) uniform sampler2D albedoTexture;

float calculateAttenuation(uint index, vec3 pos) {
	float len = length(pos - pointLightPositions.positions[index].xyz);
	AttenuationFactors factors = pointLightAttenuationFactors.attenuationFactors[index];

	return min(1 / (factors.constant + (factors.linear * len) + (factors.quadratic * len * len)), 1.0);
}

vec3 applyPointLights(vec3 baseColour, vec3 worldPos, vec3 normal) {
    vec3 result = vec3(0);

	for (uint i = 0; i < lightingInfo.numberPointLights; i++) {
		vec3 lightPos = pointLightPositions.positions[i].xyz;

		float attenuation = calculateAttenuation(i, lightPos);

		vec3 lightDir = normalize(worldPos - lightPos);
		float diff = max(dot(normal, -lightDir), 0.0);
		vec3 diffuse = diff * baseColour * pointLightBaseColours.baseColours[i].rgb * attenuation;

		result += diffuse;
	}

	return result;
}

vec3 applyDirectionalLights(vec3 baseColour, vec3 normal) {
	vec3 result = vec3(0);

	for (uint i = 0; i < lightingInfo.numberDirectionalLights; i++) {
		vec3 lightDir = normalize(directionalLightDirections.directions[i]).xyz;
		float diff = max(dot(normal, -lightDir), 0.0);
		
		result += diff * baseColour * directionalLightBaseColours.colours[i].rgb;
	}

	return result;
}

void main() {
	vec3 colour = texture(albedoTexture, texCoord).rgb;
	vec3 worldPos = texture(positionTexture, texCoord).rgb;
	vec3 normal = texture(normalTexture, texCoord).rgb;

	/*for (uint i = 0; i < lightingInfo.numberDirectionalLights; i++) {
		vec3 lightDir = normalize(directionalLightDirections.directions[i].xyz);
		float diff = max(dot(normal, -lightDir), 0.0);
		colour = colour * diff;
	}*/

	vec3 pointLightColour = applyPointLights(colour, worldPos, normal);
	vec3 directionalLightColour = applyDirectionalLights(colour, normal);

	outFragColour = vec4(pointLightColour + directionalLightColour + (colour * 0.01), 1.0f);
	//outFragColour = vec4(colour, 1.0);
}