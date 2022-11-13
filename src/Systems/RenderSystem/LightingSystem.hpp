#pragma once
#include <vector>
#include <glm/vec3.hpp>
#include "../../Components/RenderComponents/LightComponent.hpp"
#include <vulkan/vulkan_core.h>
#include "VulkanTypes.hpp"

enum LightingSystemFlags {
	UpdatePointLightPositionBuffer = 1 << 0,
	UpdatePointLightColourBuffer = 1 << 1,
	UpdatePointLightAttenuationFactorBuffer = 1 << 2,
	UpdateDirectionalLightColourBuffer = 1 << 3,
	UpdateDirectionalLightDirectionBuffer = 1 << 4,
	PointLightBufferResize = 1 << 5,
	DirectionalLightBufferResize = 1 << 6
};

class LightingSystem {
private:
	PointLights pointLights;
	DirectionalLights directionalLights;
	LightingInformation lightingInformation;
	// Default to updating first
	uint64_t updateFlags = LightingSystemFlags::UpdatePointLightPositionBuffer | LightingSystemFlags::UpdatePointLightColourBuffer 
		| LightingSystemFlags::UpdatePointLightAttenuationFactorBuffer | LightingSystemFlags::UpdateDirectionalLightColourBuffer
		| LightingSystemFlags::UpdateDirectionalLightDirectionBuffer | LightingSystemFlags::PointLightBufferResize | LightingSystemFlags::DirectionalLightBufferResize;
	std::vector<uint64_t> bufferFlags;
	std::vector<AllocatedBuffer> pointLightPositionBuffers;
	std::vector<AllocatedBuffer> pointLightColourBuffers;
	std::vector<AllocatedBuffer> pointLightAttenuationBuffers;
	std::vector<AllocatedBuffer> directionalLightColourBuffers;
	std::vector<AllocatedBuffer> directionalLightDirectionBuffers;
	std::vector<AllocatedBuffer> lightingInfoBuffers;

	void checkForFlagUpdates();
	void updatePointLightBuffers(VkDevice device, VkQueue graphicsQueue, UploadContext uploadContext, VmaAllocator vmaAllocator, size_t currentFrameIndex,
								 VkDescriptorSet descriptor);
	void updateDirectionalLightBuffers(VkDevice device, VkQueue graphicsQueue, UploadContext uploadContext, VmaAllocator vmaAllocator, size_t currentFrameIndex,
									   VkDescriptorSet descriptor);
public:
	void initialise(size_t frameOverlaps);
	size_t addPointLight(PointLightCreateInfo pointLightCreateInfo);
	size_t addDirectionLight(DirectionalLightCreateInfo directionalLightCreateInfo);

	void addLightingSystemToDescriptorSet(std::vector<VkDescriptorSetLayoutBinding>* bindings);
	void updateLightingSystemBuffers(VkDevice device, VkQueue graphicsQueue, UploadContext uploadContext, VmaAllocator vmaAllocator, size_t currentFrameIndex,
									 VkDescriptorSet descriptor);
};