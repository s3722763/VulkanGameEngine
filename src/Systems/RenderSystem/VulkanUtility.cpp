#include "VulkanUtility.hpp"
#include "VulkanUtility.hpp"
#include "VulkanUtility.hpp"
#include "VulkanUtility.hpp"
#include "VulkanUtility.hpp"
#include "VulkanUtility.hpp"
#include "VulkanUtility.hpp"
#include <iostream>

VkPipelineShaderStageCreateInfo VulkanUtility::pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule) {
	VkPipelineShaderStageCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.pNext = nullptr;

	info.stage = stage;
	info.module = shaderModule;
	// Entrypoint function name
	info.pName = "main";

	return info;
}

VkPipelineVertexInputStateCreateInfo VulkanUtility::vertexInputStateCreateInfo() {
	VkPipelineVertexInputStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	info.pNext = nullptr;

	info.vertexBindingDescriptionCount = 0;
	info.vertexAttributeDescriptionCount = 0;

	return info;
}

VkPipelineInputAssemblyStateCreateInfo VulkanUtility::inputAssemblyCreateInfo(VkPrimitiveTopology topology) {
	VkPipelineInputAssemblyStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	info.pNext = nullptr;

	info.topology = topology;
	// Not goigng to use primitive restart
	info.primitiveRestartEnable = VK_FALSE;

	return info;
}

VkPipelineRasterizationStateCreateInfo VulkanUtility::rasterizationStateCreateInfo(VkPolygonMode polygonMode) {
	VkPipelineRasterizationStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	info.pNext = nullptr;

	info.depthClampEnable = VK_FALSE;
	// Discards all primitives before rasterization stage if enabled 
	info.rasterizerDiscardEnable = VK_FALSE;

	info.polygonMode = polygonMode;
	info.lineWidth = 1.0f;
	// No backface culling
	info.cullMode = VK_CULL_MODE_NONE;
	info.frontFace = VK_FRONT_FACE_CLOCKWISE;
	info.depthBiasEnable = VK_FALSE;
	info.depthBiasConstantFactor = 0.0f;
	info.depthBiasClamp = 0.0f;
	info.depthBiasSlopeFactor = 0.0f;

	return info;
}

VkPipelineMultisampleStateCreateInfo VulkanUtility::multisamplingStateCreateInfo() {
	VkPipelineMultisampleStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	info.pNext = nullptr;

	info.sampleShadingEnable = VK_FALSE;
	info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	info.minSampleShading = 1.0f;
	info.pSampleMask = nullptr;
	info.alphaToCoverageEnable = VK_FALSE;
	info.alphaToOneEnable = VK_FALSE;

	return info;
}

VkPipelineColorBlendAttachmentState VulkanUtility::colorBlendAttachmentState() {
	VkPipelineColorBlendAttachmentState info{};
	info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	info.blendEnable = VK_FALSE;

	return info;
}

VkPipelineLayoutCreateInfo VulkanUtility::pipelineLayoutCreateInfo() {
	VkPipelineLayoutCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.pNext = nullptr;

	info.flags = 0;
	info.setLayoutCount = 0;
	info.pSetLayouts = nullptr;
	info.pushConstantRangeCount = 0;
	info.pPushConstantRanges = nullptr;

	return info;
}

VkImageCreateInfo VulkanUtility::imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent) {
	VkImageCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.pNext = nullptr;

	info.imageType = VK_IMAGE_TYPE_2D;
	info.format = format;
	info.extent = extent;

	info.mipLevels = 1;
	info.arrayLayers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = usageFlags;

	return info;
}

VkImageViewCreateInfo VulkanUtility::imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags flags) {
	VkImageViewCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.pNext = nullptr;
	
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.image = image;
	info.format = format;
	info.subresourceRange.baseMipLevel = 0;
	info.subresourceRange.levelCount = 1;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount = 1;
	info.subresourceRange.aspectMask = flags;

	return info;
}

VkPipelineDepthStencilStateCreateInfo VulkanUtility::depthStencilCreateInfo(bool depthTest, bool depthWrite, VkCompareOp compareOp) {
	VkPipelineDepthStencilStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	info.pNext = nullptr;

	info.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
	info.depthWriteEnable = depthTest ? VK_TRUE : VK_FALSE;

	info.depthCompareOp = depthTest ? compareOp : VK_COMPARE_OP_ALWAYS;
	info.depthBoundsTestEnable = VK_FALSE;
	info.minDepthBounds = 0.0f;
	info.maxDepthBounds = 1.0f;
	info.stencilTestEnable = VK_FALSE;

	return info;
}

VkDescriptorSetLayoutBinding VulkanUtility::descriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding) {
	VkDescriptorSetLayoutBinding set{};
	set.binding = binding;
	set.descriptorCount = 1;
	set.descriptorType = type;
	set.pImmutableSamplers = nullptr;
	set.stageFlags = stageFlags;

	return set;
}

VkWriteDescriptorSet VulkanUtility::writeDescriptorBuffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding) {
	VkWriteDescriptorSet set{};
	set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	set.pNext = nullptr;

	set.dstBinding = binding;
	set.dstSet = dstSet;
	set.descriptorCount = 1;
	set.descriptorType = type;
	set.pBufferInfo = bufferInfo;

	return set;
}

VkCommandPoolCreateInfo VulkanUtility::commandPoolCreateInfo(uint32_t queueFamily) {
	VkCommandPoolCreateInfo commandPoolInfo{};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.pNext = nullptr;

	// This command pool will be one that submits graphics commands
	commandPoolInfo.queueFamilyIndex = queueFamily;
	// Want the pool to allow for resetting of individual command buffers
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	return commandPoolInfo;
}

VkCommandBufferAllocateInfo VulkanUtility::commandBufferAllocateInfo(VkCommandPool pool, uint32_t count) {
	VkCommandBufferAllocateInfo cmdAllocInfo{};
	cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdAllocInfo.pNext = nullptr;

	// Allocate one command buffer
	cmdAllocInfo.commandBufferCount = count;
	// Command buffer level is primary
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdAllocInfo.commandPool = pool;

	return cmdAllocInfo;
}

VkCommandBufferBeginInfo VulkanUtility::commandBufferBeginInfo(VkCommandBufferUsageFlags flags) {
	VkCommandBufferBeginInfo cmdBeginInfo{};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.pNext = nullptr;
	// Used to connect secondary command buffers to the main buffer
	cmdBeginInfo.pInheritanceInfo = nullptr;
	// Only going to use this buffer once so going to tell Vulkan that
	cmdBeginInfo.flags = flags;
	return cmdBeginInfo;
}

VkSubmitInfo VulkanUtility::submitInfo(VkCommandBuffer* buffer) {
	VkSubmitInfo submit{};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;

	submit.commandBufferCount = 1;
	submit.pCommandBuffers = buffer;

	return submit;
}

VkSamplerCreateInfo VulkanUtility::samplerCreateInfo(VkFilter filters, VkSamplerAddressMode samplerAddressMode) {
	VkSamplerCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.pNext = nullptr;

	info.magFilter = filters;
	info.minFilter = filters;
	info.addressModeU = samplerAddressMode;
	info.addressModeV = samplerAddressMode;
	info.addressModeW = samplerAddressMode;

	return info;
}

VkWriteDescriptorSet VulkanUtility::writeDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding) {
	VkWriteDescriptorSet set{};
	set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	set.pNext = nullptr;

	set.dstBinding = binding;
	set.dstSet = dstSet;
	set.descriptorCount = 1;
	set.descriptorType = type;
	set.pImageInfo = imageInfo;

	return set;
}

AllocatedBuffer VulkanUtility::createBuffer(VmaAllocator allocator, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;

	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memoryUsage;

	AllocatedBuffer buffer{};

	VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, nullptr);

	if (result) {
		std::cout << "Detected Vulkan error while creating buffer: " << result << std::endl;
		abort();
	}

	return buffer;
}

VkPipelineColorBlendAttachmentState VulkanUtility::pipelineColorBlendAttachmentState(VkColorComponentFlags colorWriteMask, VkBool32 blendEnable) {
	VkPipelineColorBlendAttachmentState state{};

	state.colorWriteMask = colorWriteMask;
	state.blendEnable = blendEnable;

	return state;
}

VkDescriptorImageInfo VulkanUtility::descriptorimageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout layout) {
	VkDescriptorImageInfo result{};
	result.sampler = sampler;
	result.imageLayout = layout;
	result.imageView = imageView;

	return result;
}

void DeletionQueue::pushFunction(std::function<void()>&& function) {
	this->deletors.push_back(function);
}

void DeletionQueue::flush() {
	for (auto it = this->deletors.rbegin(); it != this->deletors.rend(); it++) {
		(*it)();
	}

	this->deletors.clear();
}

void VulkanUtility::immediateSubmit(VkDevice device, VkQueue graphicsQueue, UploadContext uploadContext, std::function<void(VkCommandBuffer cmd)>&& function) {
	VkCommandBufferAllocateInfo cmdAllocInfo = VulkanUtility::commandBufferAllocateInfo(uploadContext.commandPool, 1);
	VkCommandBuffer cmd;

	VkResult result = vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmd);

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

	result = vkQueueSubmit(graphicsQueue, 1, &submit, uploadContext.uploadFence);

	if (result) {
		std::cout << "Detected Vulkan error while beginning immedaite submit command buffer: " << result << std::endl;
		abort();
	}

	vkWaitForFences(device, 1, &uploadContext.uploadFence, true, 9999999999);
	vkResetFences(device, 1, &uploadContext.uploadFence);

	vkResetCommandPool(device, uploadContext.commandPool, 0);
}
