#include "LightingSystem.hpp"
#include "VulkanUtility.hpp"
#include <algorithm>

constexpr uint32_t POINT_LIGHT_POSITION_BINDING = 1;
constexpr uint32_t POINT_LIGHT_BASE_COLOUR_BINDING = 2;
constexpr uint32_t POINT_LIGHT_ATTENUATION_FACTORS_BINDING = 3;
constexpr uint32_t DIRECTIONAL_LIGHT_DIRECTION_BINDING = 4;
constexpr uint32_t DIRECTIONAL_LIGHT_COLOUR_BINDING = 5;
constexpr uint32_t LIGHTING_INFO_BINDING = 6;

void LightingSystem::checkForFlagUpdates() {
	auto numberOfFrameOverlaps = this->bufferFlags.size();

	// Check point light flags for updates
	if (this->updateFlags & LightingSystemFlags::UpdatePointLightAttenuationFactorBuffer) {
		for (auto i = 0; i < numberOfFrameOverlaps; i++) {
			this->bufferFlags[i] |= LightingSystemFlags::UpdatePointLightAttenuationFactorBuffer;
		}
	}

	if (this->updateFlags & LightingSystemFlags::UpdatePointLightColourBuffer) {
		for (auto i = 0; i < numberOfFrameOverlaps; i++) {
			this->bufferFlags[i] |= LightingSystemFlags::UpdatePointLightColourBuffer;
		}
	}

	if (this->updateFlags & LightingSystemFlags::UpdatePointLightPositionBuffer) {
		for (auto i = 0; i < numberOfFrameOverlaps; i++) {
			this->bufferFlags[i] |= LightingSystemFlags::UpdatePointLightPositionBuffer;
		}
	}

	if (this->updateFlags & LightingSystemFlags::PointLightBufferResize) {
		for (auto i = 0; i < numberOfFrameOverlaps; i++) {
			this->bufferFlags[i] |= LightingSystemFlags::PointLightBufferResize;
		}
	}

	// Check for updates to directional Lights
	if (this->updateFlags & LightingSystemFlags::UpdateDirectionalLightColourBuffer) {
		for (auto i = 0; i < numberOfFrameOverlaps; i++) {
			this->bufferFlags[i] |= LightingSystemFlags::UpdateDirectionalLightColourBuffer;
		}
	}

	if (this->updateFlags & LightingSystemFlags::UpdateDirectionalLightDirectionBuffer) {
		for (auto i = 0; i < numberOfFrameOverlaps; i++) {
			this->bufferFlags[i] |= LightingSystemFlags::UpdateDirectionalLightDirectionBuffer;
		}
	}

	if (this->updateFlags & LightingSystemFlags::DirectionalLightBufferResize) {
		for (auto i = 0; i < numberOfFrameOverlaps; i++) {
			this->bufferFlags[i] |= LightingSystemFlags::DirectionalLightBufferResize;
		}
	}

	this->updateFlags = 0;
}

void LightingSystem::updatePointLightBuffers(VkDevice device, VkQueue graphicsQueue, UploadContext uploadContext, VmaAllocator vmaAllocator, size_t currentFrameIndex, VkDescriptorSet descriptor) {
	/*if (this->lightingInformation.numberPointLights == 0) {
		// Program will crash if a buffer of size zero is created
		return;
	}*/
	
	// Resize buffers if flagged
	if (this->bufferFlags[currentFrameIndex] & LightingSystemFlags::PointLightBufferResize) {
		if (this->lightingInformation.numberPointLights == 0) {
			// Program will crash if a buffer of size zero is created therefore fill with dummy data
			this->pointLights.attenuationFactors.push_back({ 0, 0, 0 });
			this->pointLights.baseColours.push_back({ 0, 0, 0, 0 });
			this->pointLights.positions.push_back({0, 0, 0, 0});
		}

		// Destroy old buffers
		vmaDestroyBuffer(vmaAllocator, this->pointLightAttenuationBuffers[currentFrameIndex].buffer, this->pointLightAttenuationBuffers[currentFrameIndex].allocation);
		vmaDestroyBuffer(vmaAllocator, this->pointLightColourBuffers[currentFrameIndex].buffer, this->pointLightColourBuffers[currentFrameIndex].allocation);
		vmaDestroyBuffer(vmaAllocator, this->pointLightPositionBuffers[currentFrameIndex].buffer, this->pointLightPositionBuffers[currentFrameIndex].allocation);

		size_t attenuationFactorSize = sizeof(AttenuationFactors) * this->pointLights.attenuationFactors.size();
		size_t positionSize = sizeof(glm::vec4) * this->pointLights.positions.size();
		size_t baseColourSize = sizeof(glm::vec4) * this->pointLights.baseColours.size();

		// Fill new buffers
		this->pointLightAttenuationBuffers[currentFrameIndex] = VulkanUtility::allocateGPUOnlyBuffer(device, graphicsQueue, uploadContext, vmaAllocator, this->pointLights.attenuationFactors.data(), attenuationFactorSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		this->pointLightColourBuffers[currentFrameIndex] = VulkanUtility::allocateGPUOnlyBuffer(device, graphicsQueue, uploadContext, vmaAllocator, this->pointLights.baseColours.data(), baseColourSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		this->pointLightPositionBuffers[currentFrameIndex] = VulkanUtility::allocateGPUOnlyBuffer(device, graphicsQueue, uploadContext, vmaAllocator, this->pointLights.positions.data(), positionSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

		this->bufferFlags[currentFrameIndex] &= ~(LightingSystemFlags::PointLightBufferResize);
	}
	
	std::vector<VkWriteDescriptorSet> descriptorSetWrites{};

	if (this->bufferFlags[currentFrameIndex] & LightingSystemFlags::UpdatePointLightAttenuationFactorBuffer) {
		size_t size = sizeof(AttenuationFactors) * this->pointLights.attenuationFactors.size();

		VkDescriptorBufferInfo attenuationFactorBufferInfo{};
		attenuationFactorBufferInfo.buffer = this->pointLightAttenuationBuffers[currentFrameIndex].buffer;
		attenuationFactorBufferInfo.offset = 0;
		attenuationFactorBufferInfo.range = size;

		VkWriteDescriptorSet attenuationFactorBufferWrite = VulkanUtility::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptor, &attenuationFactorBufferInfo, POINT_LIGHT_ATTENUATION_FACTORS_BINDING);
		descriptorSetWrites.push_back(attenuationFactorBufferWrite);
		
		this->bufferFlags[currentFrameIndex] &= ~LightingSystemFlags::UpdatePointLightAttenuationFactorBuffer;
	}

	if (this->bufferFlags[currentFrameIndex] & LightingSystemFlags::UpdatePointLightColourBuffer) {
		size_t size = sizeof(glm::vec4) * this->pointLights.baseColours.size();

		VkDescriptorBufferInfo colourBufferInfo{};
		colourBufferInfo.buffer = this->pointLightColourBuffers[currentFrameIndex].buffer;
		colourBufferInfo.offset = 0;
		colourBufferInfo.range = size;

		VkWriteDescriptorSet colourBufferWrite = VulkanUtility::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptor, &colourBufferInfo, POINT_LIGHT_BASE_COLOUR_BINDING);
		descriptorSetWrites.push_back(colourBufferWrite);

		this->bufferFlags[currentFrameIndex] &= ~LightingSystemFlags::UpdatePointLightColourBuffer;
	}

	if (this->bufferFlags[currentFrameIndex] & LightingSystemFlags::UpdatePointLightPositionBuffer) {
		size_t size = sizeof(glm::vec4) * this->pointLights.positions.size();

		VkDescriptorBufferInfo positionBufferInfo{};
		positionBufferInfo.buffer = this->pointLightPositionBuffers[currentFrameIndex].buffer;
		positionBufferInfo.offset = 0;
		positionBufferInfo.range = size;

		VkWriteDescriptorSet positionBufferWrite = VulkanUtility::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptor, &positionBufferInfo, POINT_LIGHT_POSITION_BINDING);
		descriptorSetWrites.push_back(positionBufferWrite);

		this->bufferFlags[currentFrameIndex] &= ~LightingSystemFlags::UpdatePointLightPositionBuffer;
	}

	vkUpdateDescriptorSets(device, descriptorSetWrites.size(), descriptorSetWrites.data(), 0, nullptr);
}

void LightingSystem::updateDirectionalLightBuffers(VkDevice device, VkQueue graphicsQueue, UploadContext uploadContext, VmaAllocator vmaAllocator, size_t currentFrameIndex, VkDescriptorSet descriptor) {
	// Resize buffers if flagged
	if (this->bufferFlags[currentFrameIndex] & LightingSystemFlags::DirectionalLightBufferResize) {
		if (this->lightingInformation.numberDirectionalLights == 0) {
			// Program will crash if a buffer of size zero is created therefore fill with dummy data
			this->directionalLights.baseColours.push_back({0, 0, 0, 0});
			this->directionalLights.directions.push_back({ 0, 0, 0, 0 });
		}

		// Destroy old buffers
		vmaDestroyBuffer(vmaAllocator, this->directionalLightColourBuffers[currentFrameIndex].buffer, this->directionalLightColourBuffers[currentFrameIndex].allocation);
		vmaDestroyBuffer(vmaAllocator, this->directionalLightDirectionBuffers[currentFrameIndex].buffer, this->directionalLightDirectionBuffers[currentFrameIndex].allocation);

		size_t directionSize = sizeof(glm::vec4) * this->directionalLights.directions.size();
		size_t baseColourSize = sizeof(glm::vec4) * this->directionalLights.baseColours.size();

		// Fill new buffers
		this->directionalLightColourBuffers[currentFrameIndex] = VulkanUtility::allocateGPUOnlyBuffer(device, graphicsQueue, uploadContext, vmaAllocator, this->directionalLights.baseColours.data(), baseColourSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		this->directionalLightDirectionBuffers[currentFrameIndex] = VulkanUtility::allocateGPUOnlyBuffer(device, graphicsQueue, uploadContext, vmaAllocator, this->directionalLights.directions.data(), directionSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		
		this->bufferFlags[currentFrameIndex] &= ~(LightingSystemFlags::DirectionalLightBufferResize);
	}

	std::vector<VkWriteDescriptorSet> descriptorSetWrites{};

	if (this->bufferFlags[currentFrameIndex] & LightingSystemFlags::UpdateDirectionalLightColourBuffer) {
		size_t size = sizeof(glm::vec4) * this->directionalLights.baseColours.size();

		VkDescriptorBufferInfo baseColourBufferInfo{};
		baseColourBufferInfo.buffer = this->directionalLightColourBuffers[currentFrameIndex].buffer;
		baseColourBufferInfo.offset = 0;
		baseColourBufferInfo.range = size;

		VkWriteDescriptorSet baseColourBufferWrite = VulkanUtility::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptor, &baseColourBufferInfo, DIRECTIONAL_LIGHT_COLOUR_BINDING);
		descriptorSetWrites.push_back(baseColourBufferWrite);

		this->bufferFlags[currentFrameIndex] &= ~LightingSystemFlags::UpdateDirectionalLightColourBuffer;
	}

	if (this->bufferFlags[currentFrameIndex] & LightingSystemFlags::UpdateDirectionalLightDirectionBuffer) {
		size_t size = sizeof(glm::vec4) * this->directionalLights.directions.size();

		VkDescriptorBufferInfo directionBufferInfo{};
		directionBufferInfo.buffer = this->directionalLightDirectionBuffers[currentFrameIndex].buffer;
		directionBufferInfo.offset = 0;
		directionBufferInfo.range = size;

		VkWriteDescriptorSet directionBufferWrite = VulkanUtility::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptor, &directionBufferInfo, DIRECTIONAL_LIGHT_DIRECTION_BINDING);
		descriptorSetWrites.push_back(directionBufferWrite);

		this->bufferFlags[currentFrameIndex] &= ~LightingSystemFlags::UpdateDirectionalLightDirectionBuffer;
	}

	vkUpdateDescriptorSets(device, descriptorSetWrites.size(), descriptorSetWrites.data(), 0, nullptr);
}

void LightingSystem::initialise(size_t frameOverlaps) {
	this->bufferFlags.resize(frameOverlaps);

	this->pointLightAttenuationBuffers.resize(frameOverlaps);
	this->pointLightColourBuffers.resize(frameOverlaps);
	this->pointLightPositionBuffers.resize(frameOverlaps);
	this->directionalLightColourBuffers.resize(frameOverlaps);
	this->directionalLightDirectionBuffers.resize(frameOverlaps);
	this->lightingInfoBuffers.resize(frameOverlaps);
}

size_t LightingSystem::addPointLight(PointLightCreateInfo pointLightCreateInfo) {
	size_t id = this->lightingInformation.numberPointLights;
	this->pointLights.positions.resize(id + 1);
	this->pointLights.attenuationFactors.resize(id + 1);
	this->pointLights.baseColours.resize(id + 1);

	this->pointLights.positions[id] = pointLightCreateInfo.position;
	this->pointLights.attenuationFactors[id] = pointLightCreateInfo.attenuationFactors;
	this->pointLights.baseColours[id] = pointLightCreateInfo.baseColour;

	this->lightingInformation.numberPointLights += 1;

	this->updateFlags |= LightingSystemFlags::PointLightBufferResize | LightingSystemFlags::UpdatePointLightAttenuationFactorBuffer
		| LightingSystemFlags::UpdatePointLightColourBuffer | LightingSystemFlags::UpdatePointLightPositionBuffer;

	return id;
}

size_t LightingSystem::addDirectionLight(DirectionalLightCreateInfo directionalLightCreateInfo) {
	size_t id = this->lightingInformation.numberDirectionalLights;
	this->directionalLights.directions.resize(id + 1);
	this->directionalLights.baseColours.resize(id + 1);

	this->directionalLights.directions[id] = directionalLightCreateInfo.direction;
	this->directionalLights.baseColours[id] = directionalLightCreateInfo.baseColour;

	this->lightingInformation.numberDirectionalLights += 1;

	this->updateFlags |= LightingSystemFlags::DirectionalLightBufferResize | LightingSystemFlags::UpdateDirectionalLightColourBuffer 
		| LightingSystemFlags::UpdateDirectionalLightDirectionBuffer;

	return id;
}

void LightingSystem::addLightingSystemToDescriptorSet(std::vector<VkDescriptorSetLayoutBinding>* bindings) {
	// Point Lights
	bindings->push_back(VulkanUtility::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, POINT_LIGHT_ATTENUATION_FACTORS_BINDING));
	bindings->push_back(VulkanUtility::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, POINT_LIGHT_BASE_COLOUR_BINDING));
	bindings->push_back(VulkanUtility::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, POINT_LIGHT_POSITION_BINDING));
	// Directional Lights
	bindings->push_back(VulkanUtility::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, DIRECTIONAL_LIGHT_COLOUR_BINDING));
	bindings->push_back(VulkanUtility::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, DIRECTIONAL_LIGHT_DIRECTION_BINDING));
	bindings->push_back(VulkanUtility::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, LIGHTING_INFO_BINDING));
}

void LightingSystem::updateLightingSystemBuffers(VkDevice device, VkQueue graphicsQueue, UploadContext uploadContext, VmaAllocator vmaAllocator, size_t currentFrameIndex,
												 VkDescriptorSet descriptor) {
	this->checkForFlagUpdates();

	// Check if either light has been resized. Resized = change in number of lights
	if (this->bufferFlags[currentFrameIndex] & (LightingSystemFlags::DirectionalLightBufferResize | LightingSystemFlags::PointLightBufferResize)) {
		// Change in number of lights
		vmaDestroyBuffer(vmaAllocator, this->lightingInfoBuffers[currentFrameIndex].buffer, this->lightingInfoBuffers[currentFrameIndex].allocation);
		size_t size = sizeof(LightingInformation);
		this->lightingInfoBuffers[currentFrameIndex] = VulkanUtility::allocateGPUOnlyBuffer(device, graphicsQueue, uploadContext, vmaAllocator, &this->lightingInformation, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		
		VkDescriptorBufferInfo lightingInfoBufferInfo{};
		lightingInfoBufferInfo.buffer = this->lightingInfoBuffers[currentFrameIndex].buffer;
		lightingInfoBufferInfo.offset = 0;
		lightingInfoBufferInfo.range = size;

		VkWriteDescriptorSet lightingInfoWrite = VulkanUtility::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptor, &lightingInfoBufferInfo, LIGHTING_INFO_BINDING);
		vkUpdateDescriptorSets(device, 1, &lightingInfoWrite, 0, nullptr);
	}

	// Update point light buffers
	this->updatePointLightBuffers(device, graphicsQueue, uploadContext, vmaAllocator, currentFrameIndex, descriptor);
	// Update directional light buffers
	this->updateDirectionalLightBuffers(device, graphicsQueue, uploadContext, vmaAllocator, currentFrameIndex, descriptor);
}
