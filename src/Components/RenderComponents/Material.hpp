#pragma once
#include <vulkan/vulkan.h>
#include <string>

struct Material {
	size_t diffuseTextureId;
	VkDescriptorSet materialDescriptorSet;
	VkDescriptorImageInfo materialDescriptorImage;
};

struct MaterialInfo {
	std::string diffusePath;
};