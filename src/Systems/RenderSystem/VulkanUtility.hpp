#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <deque>
#include <functional>
#include <iostream>

#include "VulkanTypes.hpp"

struct DeletionQueue {
	std::deque<std::function<void()>> deletors;

	void pushFunction(std::function<void()>&& function);
	void flush();
};

namespace VulkanUtility {
	VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo();
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo(VkPrimitiveTopology topology);
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo(VkPolygonMode polygonMode);
	VkPipelineMultisampleStateCreateInfo multisamplingStateCreateInfo();
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState();
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo();
	VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
	VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags flags);
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo(bool depthTest, bool depthWrite, VkCompareOp compareOp);
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
	VkWriteDescriptorSet writeDescriptorBuffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
	VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamily);
	VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count);
	VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags);
	VkSubmitInfo submitInfo(VkCommandBuffer* buffer);
	VkSamplerCreateInfo samplerCreateInfo(VkFilter filters, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
	VkWriteDescriptorSet writeDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding);
	AllocatedBuffer createBuffer(VmaAllocator allocator, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void immediateSubmit(VkDevice device, VkQueue graphicsQueue, UploadContext uploadContext, std::function<void(VkCommandBuffer cmd)>&& function);

	template<typename T>
	void copyToBuffer(VmaAllocator allocator, AllocatedBuffer* buffer, T* data, size_t size) {
		void* memoryPointer{};
		vmaMapMemory(allocator, buffer->allocation, &memoryPointer);
		memcpy(memoryPointer, data, size);
		vmaUnmapMemory(allocator, buffer->allocation);
	}

	template<typename T>
	AllocatedBuffer allocateGPUOnlyBuffer(VkDevice device, VkQueue graphicsQueue, UploadContext uploadContext, VmaAllocator allocator, 
										  T* buffer, size_t size, VkBufferUsageFlags usageFlags) {
		AllocatedBuffer allocatedBuffer{};
		AllocatedBuffer stagingBuffer{};
		size_t bufferSize = size;

		VkBufferCreateInfo stagingBufferInfo{};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;

		stagingBufferInfo.size = bufferSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		// Let VMA know this data should be in CPU RAM
		VmaAllocationCreateInfo vmaAllocInfo{};
		vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		VkResult result = vmaCreateBuffer(allocator, &stagingBufferInfo, &vmaAllocInfo, &stagingBuffer.buffer, &stagingBuffer.allocation, nullptr);

		if (result) {
			std::cout << "Couldn't create staging buffer: " << result << std::endl;
			abort();
		}

		VulkanUtility::copyToBuffer<T>(allocator, &stagingBuffer, buffer, size);

		VkBufferCreateInfo vertexBufferInfo{};
		vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.pNext = nullptr;

		vertexBufferInfo.size = bufferSize;
		vertexBufferInfo.usage = usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		result = vmaCreateBuffer(allocator, &vertexBufferInfo, &vmaAllocInfo, &allocatedBuffer.buffer, &allocatedBuffer.allocation, nullptr);

		if (result) {
			std::cout << "Couldn't create vertex buffer: " << result << std::endl;
			abort();
		}

		VulkanUtility::immediateSubmit(device, graphicsQueue, uploadContext, [=](VkCommandBuffer cmd) {
			VkBufferCopy copy{};
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer.buffer, allocatedBuffer.buffer, 1, &copy);
		});

		vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);

		allocatedBuffer.size = size;

		return allocatedBuffer;
	}
};