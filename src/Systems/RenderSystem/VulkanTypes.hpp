#pragma once
#include <vma/vk_mem_alloc.h>
#include <vector>

struct AllocatedBuffer {
	VkBuffer buffer;
	VmaAllocation allocation;
	size_t size;
};

struct AllocatedImage {
	VkImage image{};
	VmaAllocation allocation{};
	VkImageView imageView{};
};

struct VertexInputDescription {
	std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;

	VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct VulkanDetails {
	VkDevice device;
	VmaAllocator allocator;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice chosenGPU;
	VkSurfaceKHR surface;
	VkPhysicalDeviceProperties gpuProperties;
};

struct QueueDetails {
	VkQueue queue;
	uint32_t family;
};

struct UploadContext {
	VkFence uploadFence;
	VkCommandPool commandPool;
};

struct ModelVertexInputDescription : VertexInputDescription {
	static VertexInputDescription getVertexDescription();
};
