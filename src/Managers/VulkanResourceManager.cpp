#include "VulkanResourceManager.hpp"
#include "VulkanResourceManager.hpp"
#include "VulkanResourceManager.hpp"
#include <sdl/SDL_vulkan.h>

void VulkanResourceManager::initialiseVulkan(SDL_Window* window) {
	vkb::InstanceBuilder instanceBuilder{};
	// Initialise vulkan instance with basic debug features
	auto instanceReturned = instanceBuilder.set_app_name("Game Engine")
		.request_validation_layers(true)
		.require_api_version(1, 1, 0)
		.use_default_debug_messenger()
		.build();

	vkb::Instance vkbInstance = instanceReturned.value();

	this->vulkanDetails.instance = vkbInstance.instance;
	this->vulkanDetails.debugMessenger = vkbInstance.debug_messenger;

	SDL_Vulkan_CreateSurface(window, this->vulkanDetails.instance, &this->vulkanDetails.surface);

	// Use vkbootstrap to select the best GPU
	vkb::PhysicalDeviceSelector selector{ vkbInstance };
	vkb::PhysicalDevice vkbPhysicalDevice = selector
		.set_minimum_version(1, 1)
		.set_surface(this->vulkanDetails.surface)
		.select()
		.value();

	// Create final device to be used
	vkb::DeviceBuilder vkbDeviceBuilder{ vkbPhysicalDevice };
	vkb::Device vkbDevice = vkbDeviceBuilder.build().value();
	this->vkbDevice = vkbDevice;
	this->vulkanDetails.device = vkbDevice.device;
	this->vulkanDetails.chosenGPU = vkbPhysicalDevice.physical_device;

	this->transferQueue = vkbDevice.get_queue(vkb::QueueType::transfer).value();
	this->transferQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();

	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.physicalDevice = this->vulkanDetails.chosenGPU;
	allocatorInfo.device = this->vulkanDetails.device;
	allocatorInfo.instance = this->vulkanDetails.instance;
	vmaCreateAllocator(&allocatorInfo, &this->vulkanDetails.allocator);

	vkGetPhysicalDeviceProperties(this->vulkanDetails.chosenGPU, &this->vulkanDetails.gpuProperties);

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;

	VkResult result = vkCreateFence(this->vkbDevice.device, &fenceCreateInfo, nullptr, &this->uploadContext.uploadFence);

	if (result) {
		std::cout << "Error creating upload context fence: " << result << std::endl;
		abort();
	}

	auto commandPoolCreateInfo = VulkanUtility::commandPoolCreateInfo(this->transferQueueFamily);

	result = vkCreateCommandPool(this->vkbDevice.device, &commandPoolCreateInfo, nullptr, &this->uploadContext.commandPool);

	if (result) {
		std::cout << "Error creating upload context command pool: " << result << std::endl;
		abort();
	}
}

void VulkanResourceManager::cleanupModelBuffers() {
	for (auto& model : this->modelRenderBuffers) {
		for (auto& vertexPositionBuffer : model.VertexPositionBuffers) {
			vmaDestroyBuffer(this->vulkanDetails.allocator, vertexPositionBuffer.buffer, vertexPositionBuffer.allocation);
		}

		for (auto& texCoordBuffer : model.TextureCoordBuffers) {
			vmaDestroyBuffer(this->vulkanDetails.allocator, texCoordBuffer.buffer, texCoordBuffer.allocation);
		}
	}

	vkDestroyCommandPool(this->vulkanDetails.device, this->uploadContext.commandPool, nullptr);
	vkDestroyFence(this->vulkanDetails.device, this->uploadContext.uploadFence, nullptr);
}

void VulkanResourceManager::cleanupVulkanResources() {
	this->cleanupModelBuffers();
}

size_t VulkanResourceManager::loadModelComponentBuffers(std::string identifier, std::vector<ModelComponent>* modelComponents, size_t modelId) {
	size_t bufferId = this->modelRenderBuffers.size();
	ModelRenderComponents modelRenderComponents{};
	MeshComponents* meshComponents = &modelComponents->at(modelId).meshes;

	size_t numberOfMeshes = meshComponents->vertices.size();
	modelRenderComponents.VertexPositionBuffers.resize(numberOfMeshes);
	//modelRenderComponents.ColourBuffers.resize(numberOfMeshes);
	modelRenderComponents.IndexBuffers.resize(numberOfMeshes);
	modelRenderComponents.TextureCoordBuffers.resize(numberOfMeshes);
	modelRenderComponents.NormalBuffers.resize(numberOfMeshes);

	auto* meshVertices = &meshComponents->vertices;
	
	for (auto i = 0; i < numberOfMeshes; i++) {
		auto* vertices = &meshVertices->at(i);

		AllocatedBuffer newBuffer = this->generateNewVertexBuffer(vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

		modelRenderComponents.VertexPositionBuffers.at(i) = newBuffer;
	}

	auto* meshTexCoords = &meshComponents->texCoords;

	for (auto i = 0; i < numberOfMeshes; i++) {
		auto* texCoords = &meshTexCoords->at(i);
		AllocatedBuffer newBuffer = this->generateNewVertexBuffer(texCoords, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		modelRenderComponents.TextureCoordBuffers.at(i) = newBuffer;
	}

	auto* meshIndices = &meshComponents->indices;

	for (auto i = 0; i < numberOfMeshes; i++) {
		auto* indices = &meshIndices->at(i);
		AllocatedBuffer newBuffer = this->generateNewVertexBuffer(indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		modelRenderComponents.IndexBuffers.at(i) = newBuffer;
	}

	modelRenderBuffers.push_back(modelRenderComponents);

	return bufferId;
}

void VulkanResourceManager::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) {
	VkCommandBufferAllocateInfo cmdAllocInfo = VulkanUtility::commandBufferAllocateInfo(this->uploadContext.commandPool, 1);
	VkCommandBuffer cmd;

	VkResult result = vkAllocateCommandBuffers(this->vulkanDetails.device, &cmdAllocInfo, &cmd);

	if (result) {
		std::cout << "Detected Vulkan error while allocating immedaite submit command buffer: " << result << std::endl;
		abort();
	}

	VkCommandBufferBeginInfo cmdBeginInfo = VulkanUtility::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	result = vkBeginCommandBuffer(cmd, &cmdBeginInfo);

	if (result) {
		std::cout << "Detected Vulkan error while beginning immedaite submit command buffer: " << result << std::endl;
		abort();
	}

	// execute function
	function(cmd);

	result = vkEndCommandBuffer(cmd);

	if (result) {
		std::cout << "Detected Vulkan error while ending immedaite submit command buffer: " << result << std::endl;
		abort();
	}

	VkSubmitInfo submit = VulkanUtility::submitInfo(&cmd);

	result = vkQueueSubmit(this->transferQueue, 1, &submit, this->uploadContext.uploadFence);

	if (result) {
		std::cout << "Detected Vulkan error while beginning immedaite submit command buffer: " << result << std::endl;
		abort();
	}

	vkWaitForFences(this->vulkanDetails.device, 1, &this->uploadContext.uploadFence, true, 9999999999);
	vkResetFences(this->vulkanDetails.device, 1, &this->uploadContext.uploadFence);

	vkResetCommandPool(this->vulkanDetails.device, this->uploadContext.commandPool, 0);
}

std::vector<ModelRenderComponents>* VulkanResourceManager::getModelRenderBuffers() {
	return &this->modelRenderBuffers;
}

const VulkanDetails* VulkanResourceManager::getVulkanDetails() {
	return &this->vulkanDetails;
}

QueueDetails VulkanResourceManager::createGraphicsQueue() {
	QueueDetails details{};

	details.queue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	details.family = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
	return details;
}

QueueDetails VulkanResourceManager::createTransferQueue() {
	QueueDetails details{};

	details.queue = vkbDevice.get_queue(vkb::QueueType::transfer).value();
	details.family = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();

	return details;
}
