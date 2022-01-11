#include "VulkanTypes.hpp"
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

VertexInputDescription ModelVertexInputDescription::getVertexDescription() {
	VertexInputDescription vertexInputDescription{};

	// Position attribute
	VkVertexInputBindingDescription positionBinding{};
	positionBinding.binding = 0;
	positionBinding.stride = sizeof(glm::vec3);
	positionBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	vertexInputDescription.bindings.push_back(positionBinding);
	
	VkVertexInputAttributeDescription positionAttribute{};
	positionAttribute.binding = 0;
	positionAttribute.location = 0;
	positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	positionAttribute.offset = 0;

	vertexInputDescription.attributes.push_back(positionAttribute);

	// Texture attribute
	VkVertexInputBindingDescription textureCoordBinding{};
	textureCoordBinding.binding = 1;
	textureCoordBinding.stride = sizeof(glm::vec2);
	textureCoordBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	vertexInputDescription.bindings.push_back(textureCoordBinding);

	VkVertexInputAttributeDescription textureCoordAttribute{};
	textureCoordAttribute.binding = 1;
	textureCoordAttribute.location = 1;
	textureCoordAttribute.format = VK_FORMAT_R32G32_SFLOAT;
	textureCoordAttribute.offset = 0;

	vertexInputDescription.attributes.push_back(textureCoordAttribute);

	// Normal Attribute
	VkVertexInputBindingDescription normalBinding{};
	normalBinding.binding = 2;
	normalBinding.stride = sizeof(glm::vec3);
	normalBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	vertexInputDescription.bindings.push_back(normalBinding);

	VkVertexInputAttributeDescription normalAttribute{};
	normalAttribute.binding = 2;
	normalAttribute.location = 2;
	normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	normalAttribute.offset = 0;

	vertexInputDescription.attributes.push_back(normalAttribute);

	return vertexInputDescription;
}
