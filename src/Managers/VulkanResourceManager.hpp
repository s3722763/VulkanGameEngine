#pragma once
#include <vk_mem_alloc.h>
#include "../Components/ModelComponent.h"
#include "../Systems/RenderSystem/VulkanTypes.hpp"
#include "../Systems/RenderSystem/VulkanUtility.hpp"
#include "../Systems/RenderSystem/VkBootstrap.h"
#include <iostream>
#include <vector>
#include <sdl2/SDL_video.h>

class VulkanResourceManager {
	std::vector<ModelRenderComponents> modelRenderBuffers;

	// VMA is threadsafe
	VulkanDetails vulkanDetails;
	UploadContext uploadContext;
	vkb::Device vkbDevice;

	VkQueue transferQueue;
	uint32_t transferQueueFamily;

	template<typename T>
	AllocatedBuffer generateNewVertexBuffer(std::vector<T>* buffer, VkBufferUsageFlags usageFlags) {
		AllocatedBuffer allocatedBuffer{};
		AllocatedBuffer stagingBuffer{};
		size_t bufferSize = buffer->size() * sizeof(T);

		VkBufferCreateInfo stagingBufferInfo{};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;

		stagingBufferInfo.size = bufferSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		// Let VMA know this data should be in CPU RAM
		VmaAllocationCreateInfo vmaAllocInfo{};
		vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		VkResult result = vmaCreateBuffer(this->vulkanDetails.allocator, &stagingBufferInfo, &vmaAllocInfo, &stagingBuffer.buffer, &stagingBuffer.allocation, nullptr);

		if (result) {
			std::cout << "Couldn't create staging buffer: " << result << std::endl;
			abort();
		}

		this->copyToVertexBuffer<T>(&stagingBuffer, buffer);
		
		VkBufferCreateInfo vertexBufferInfo{};
		vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.pNext = nullptr;

		vertexBufferInfo.size = bufferSize;
		vertexBufferInfo.usage = usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		result = vmaCreateBuffer(this->vulkanDetails.allocator, &vertexBufferInfo, &vmaAllocInfo, &allocatedBuffer.buffer, &allocatedBuffer.allocation, nullptr);

		if (result) {
			std::cout << "Couldn't create vertex buffer: " << result << std::endl;
			abort();
		}

		this->immediateSubmit([=](VkCommandBuffer cmd) {
			VkBufferCopy copy{};
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer.buffer, allocatedBuffer.buffer, 1, &copy);
		});

		vmaDestroyBuffer(this->vulkanDetails.allocator, stagingBuffer.buffer, stagingBuffer.allocation);

		allocatedBuffer.size = buffer->size();

		return allocatedBuffer;
	}

	template<typename T>
	void copyToVertexBuffer(AllocatedBuffer* buffer, std::vector<T>* data) {
		void* memoryPointer{};
		vmaMapMemory(this->vulkanDetails.allocator, buffer->allocation, &memoryPointer);
		memcpy(memoryPointer, data->data(), data->size() * sizeof(T));
		vmaUnmapMemory(this->vulkanDetails.allocator, buffer->allocation);
	}

	void cleanupModelBuffers();
	void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

public:
	void initialiseVulkan(SDL_Window* window);
	
	void cleanupVulkanResources();
	size_t loadModelComponentBuffers(std::string identifier, std::vector<ModelComponent>* modelComponents, size_t modelId);
	std::vector<ModelRenderComponents>* getModelRenderBuffers();
	const VulkanDetails* getVulkanDetails();
	QueueDetails createGraphicsQueue();
	QueueDetails createTransferQueue();
};