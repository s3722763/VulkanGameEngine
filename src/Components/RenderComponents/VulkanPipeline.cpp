#include "VulkanPipeline.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanPipeline.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "../../Systems/RenderSystem/VulkanUtility.hpp"

const char* shaderCacheLocation = "resources/shaders/cache/";

std::string PipelineBuilder::readFile(const char* filepath) {
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);

	size_t filesize = (size_t)file.tellg();

	std::string buffer;
	buffer.resize(filesize);

	file.seekg(0);
	file.read(buffer.data(), filesize);
	
	file.close();

	return buffer;
}

std::vector<char> PipelineBuilder::readCompiledFile(const char* filepath) {
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		std::cout << "Could not open file: " << filepath << std::endl;
		abort();
	}

	size_t filesize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(filesize);

	file.seekg(0);
	file.read(buffer.data(), filesize);
	file.close();

	return buffer;
}

std::vector<uint32_t> PipelineBuilder::compileShader(const std::string& name, shaderc_shader_kind kind, const std::string& source, bool optimise) {
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	if (optimise) {
		options.SetOptimizationLevel(shaderc_optimization_level_performance);
	}

	shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, kind, name.c_str(), options);
	
	if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
		std::cout << module.GetErrorMessage();
		abort();
	}

	return { module.cbegin(), module.cend() };
}


VkShaderModule PipelineBuilder::createShaderModule(VkDevice device, std::string name, shaderc_shader_kind kind, std::string code, std::string path) {
	auto spirv = this->compileShader(name, kind, code, false);

	// As compiling again, delete old file and save new one
	std::ofstream file(path, std::ios::trunc | std::ios::binary);
	file.write(reinterpret_cast<const char*>(spirv.data()), spirv.size() * sizeof(uint32_t));
	file.close();

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;
	// Size in bytes
	createInfo.codeSize = spirv.size() * sizeof(uint32_t);
	createInfo.pCode = spirv.data();

	VkShaderModule shaderModule;

	vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);

	return shaderModule;
}

VkShaderModule PipelineBuilder::createShaderModule(VkDevice device, std::string name, shaderc_shader_kind kind, std::vector<char>&& spirv) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;
	// Size in bytes
	createInfo.codeSize = spirv.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(spirv.data());

	VkShaderModule shaderModule;

	vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);

	return shaderModule;
}

void PipelineBuilder::addPipelineDescriptorBinding(VkDescriptorType type, VkShaderStageFlagBits shaderStage) {
	auto bufferBinding = VulkanUtility::descriptorSetLayoutBinding(type, shaderStage, static_cast<uint32_t>(this->pipelineSetLayoutBindings.size()));
	
	this->pipelineSetLayoutBindings.push_back(bufferBinding);
}

void PipelineBuilder::allocatePipelineDescriptorUniformBuffer(VkDevice device, size_t binding, std::vector<AllocatedBuffer> buffers, uint32_t frameOverlap) {
	std::vector<VkWriteDescriptorSet> writes{};
	writes.resize(buffers.size());

	for (auto i = 0; i < frameOverlap; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = buffers[i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = buffers[i].size;

		writes[i] = VulkanUtility::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->pipelineDescriptors[i], &bufferInfo, binding);
	}

	vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
	this->pipelineSetLayoutBuffers.push_back(buffers);
}

void PipelineBuilder::setupFramebuffer(FramebufferSetupData framebufferSetupData) {
	this->framebuffer.width = framebufferSetupData.width;
	this->framebuffer.height = framebufferSetupData.height;
}

void PipelineBuilder::addFramebufferAttachment(VkDevice device, VmaAllocator allocator, VkFormat format, VkImageUsageFlagBits usage, VkExtent3D extent, size_t frameOverlaps) {
	FramebufferAttachment attachment{};
	
	VkImageAspectFlags aspectMask = 0;
	VkImageLayout imageLayout;

	
	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	} else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	assert(aspectMask > 0);

	VkImageCreateInfo framebufferAttachmentImageInfo = VulkanUtility::imageCreateInfo(format, usage | VK_IMAGE_USAGE_SAMPLED_BIT, extent);
	VmaAllocationCreateInfo vmaAllocInfo{};
	vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	vmaAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	std::vector<FramebufferAttachment> newFramebufferAttachments{};
	newFramebufferAttachments.resize(frameOverlaps);

	for (auto i = 0; i < frameOverlaps; i++) {
		vmaCreateImage(allocator, &framebufferAttachmentImageInfo, &vmaAllocInfo, &newFramebufferAttachments[i].image.image, &newFramebufferAttachments[i].image.allocation, nullptr);

		VkImageViewCreateInfo framebufferAttachmentImageInfo = VulkanUtility::imageViewCreateInfo(format, newFramebufferAttachments[i].image.image, aspectMask);
		
		VkResult result = vkCreateImageView(device, &framebufferAttachmentImageInfo, nullptr, &newFramebufferAttachments[i].image.imageView);

		if (result) {
			std::cout << "Detected Vulkan error while creating image view for framebuffer attachment: " << result << std::endl;
			abort();
		}

		newFramebufferAttachments[i].format = format;
		newFramebufferAttachments[i].aspectMask = aspectMask;
	}

	this->framebuffer.framebufferAttachments.push_back(newFramebufferAttachments);

	VkAttachmentDescription framebufferAttachmentDescription{};
	framebufferAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	framebufferAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	framebufferAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	framebufferAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	framebufferAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
		framebufferAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		framebufferAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		this->framebuffer.framebufferAttachmentReferences.push_back({ static_cast<uint32_t>(this->framebuffer.framebufferAttachmentReferences.size()), imageLayout });
	} else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		framebufferAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		framebufferAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		// ASSUMES DEPTH ATTACHMENT IS THE LAST ATTACHMENT ADDED AS THIS HAS TO BE THE CASE
		this->framebuffer.depthAttachmentReference = { static_cast<uint32_t>(this->framebuffer.framebufferAttachmentReferences.size()), imageLayout };
	}

	framebufferAttachmentDescription.format = format;

	this->framebuffer.framebufferAttachmentDescriptions.push_back(framebufferAttachmentDescription);
}

void PipelineBuilder::addShaders(VkDevice device, ShaderInfo* shaderInfo) {
	this->shaderStages.clear();

	if (shaderInfo->flags & VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT) {
		std::string shaderVertexPath = shaderInfo->vertexShaderPath;
		auto shaderCompiledPath = shaderVertexPath + ".spv";

		if (!std::filesystem::exists(shaderCompiledPath) || std::filesystem::last_write_time(shaderInfo->vertexShaderPath) > std::filesystem::last_write_time(shaderCompiledPath)) {
			// Need to compile
			std::cout << "Compiling vertex shader: " << shaderVertexPath << std::endl;
			auto vertexCode = this->readFile(shaderInfo->vertexShaderPath);

			auto vertexShaderModule = this->createShaderModule(device, "vertexShader", shaderc_vertex_shader, vertexCode, shaderCompiledPath);
			auto shaderStageCreateInfo = VulkanUtility::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule);
			this->shaderStages.push_back(shaderStageCreateInfo);
		} else {
			std::cout << "Reading compiled vertex shader: " << shaderCompiledPath << std::endl;
			auto vertexCode = this->readCompiledFile(shaderCompiledPath.c_str());
			auto vertexShaderModule = this->createShaderModule(device, "vertexShader", shaderc_vertex_shader, std::move(vertexCode));
			auto shaderStageCreateInfo = VulkanUtility::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule);
			this->shaderStages.push_back(shaderStageCreateInfo);
		}
	}

	if (shaderInfo->flags & VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT) {
		std::string shaderFragmentPath = shaderInfo->fragmentShaderPath;
		auto shaderCompiledPath = shaderFragmentPath + ".spv";

		if (!std::filesystem::exists(shaderCompiledPath) || std::filesystem::last_write_time(shaderInfo->fragmentShaderPath) > std::filesystem::last_write_time(shaderCompiledPath)) {
			std::cout << "Compiling fragment shader: " << shaderFragmentPath << std::endl;
			auto fragmentCode = this->readFile(shaderInfo->fragmentShaderPath);
			auto fragmentShaderModule = this->createShaderModule(device, "fragmentShader", shaderc_fragment_shader, fragmentCode, shaderCompiledPath);
			auto shaderStageCreateInfo = VulkanUtility::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule);
			this->shaderStages.push_back(shaderStageCreateInfo);
		} else {
			std::cout << "Reading compiled fragment shader: " << shaderCompiledPath << std::endl;
			auto fragmentCode = this->readCompiledFile(shaderCompiledPath.c_str());
			auto fragmentShaderModule = this->createShaderModule(device, "fragmentShader", shaderc_fragment_shader, std::move(fragmentCode));
			auto shaderStageCreateInfo = VulkanUtility::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule);
			this->shaderStages.push_back(shaderStageCreateInfo);
		}
	}
}


void PipelineBuilder::addFramebufferAttachment(VkDevice device, std::vector<VkImageView> attachmentImageViews, VkFormat format, VkImageUsageFlagBits usage, VkExtent3D extent, size_t frameOverlaps) {
	FramebufferAttachment attachment{};

	VkImageAspectFlags aspectMask = 0;
	VkImageLayout imageLayout;

	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	} else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	assert(aspectMask > 0);

	VkImageCreateInfo framebufferAttachmentImageInfo = VulkanUtility::imageCreateInfo(format, usage | VK_IMAGE_USAGE_SAMPLED_BIT, extent);
	VmaAllocationCreateInfo vmaAllocInfo{};
	vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	vmaAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	std::vector<FramebufferAttachment> newFramebufferAttachments{};
	newFramebufferAttachments.resize(frameOverlaps);

	for (auto i = 0; i < frameOverlaps; i++) {
		newFramebufferAttachments[i].image.imageView = attachmentImageViews[i];
		newFramebufferAttachments[i].format = format;
		newFramebufferAttachments[i].aspectMask = aspectMask;
	}

	this->framebuffer.framebufferAttachments.push_back(newFramebufferAttachments);

	VkAttachmentDescription framebufferAttachmentDescription{};
	framebufferAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	framebufferAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	framebufferAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	framebufferAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	framebufferAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// TODO: Make flags for what type of initial and final layouts are going to be used. Maybe use templates.
	framebufferAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	framebufferAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	this->framebuffer.framebufferAttachmentReferences.push_back({ static_cast<uint32_t>(this->framebuffer.framebufferAttachmentReferences.size()), imageLayout });

	framebufferAttachmentDescription.format = format;

	this->framebuffer.framebufferAttachmentDescriptions.push_back(framebufferAttachmentDescription);
}

VkDescriptorSetLayout PipelineBuilder::createPipelineSetLayout(VkDevice device, uint32_t frameOverlap, VkDescriptorPool descriptorPool) {
	// Create vulkan objects for set layout specific for pipeline
	VkDescriptorSetLayoutCreateInfo setInfo{};
	setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setInfo.pNext = nullptr;

	setInfo.bindingCount = this->pipelineSetLayoutBindings.size();
	setInfo.flags = 0;
	setInfo.pBindings = this->pipelineSetLayoutBindings.data();

	// TODO: Add destroy for deconstruction
	vkCreateDescriptorSetLayout(device, &setInfo, nullptr, &this->pipelineSetLayout);

	for (auto i = 0; i < frameOverlap; i++) {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;

		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &this->pipelineSetLayout;

		vkAllocateDescriptorSets(device, &allocInfo, &this->pipelineDescriptors[i]);
	}
	
	return this->pipelineSetLayout;
}

// TODO: Write and update descriptor sets

Pipeline PipelineBuilder::buildPipeline(VkDevice device, PipelineUsage pipelineUsage, size_t frameOverlap) {
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;

	viewportState.viewportCount = 1;
	viewportState.pViewports = &this->viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &this->scissor;

	// Setup dummy blending. Dont have transparent objects yet
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.pNext = nullptr;

	// Build render subpass
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pColorAttachments = this->framebuffer.framebufferAttachmentReferences.data();
	subpass.colorAttachmentCount = this->framebuffer.framebufferAttachmentReferences.size();
	subpass.pDepthStencilAttachment = &this->framebuffer.depthAttachmentReference;

	// Subpass dependencies
	std::array<VkSubpassDependency, 2> dependencies{};
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].dstSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.attachmentCount = this->framebuffer.framebufferAttachmentDescriptions.size();
	renderPassInfo.pAttachments = this->framebuffer.framebufferAttachmentDescriptions.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();

	auto result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &this->framebuffer.renderPass);

	if (result) {
		std::cout << "Failed to create deferred render pass: " << result << std::endl;
		abort();
	}

	for (auto i = 0; i < frameOverlap; i++) {
		size_t numberAttachments = this->framebuffer.framebufferAttachments.size();

		std::vector<VkImageView> attachments{};
		attachments.resize(numberAttachments);

		for (auto j = 0; j < numberAttachments; j++) {
			attachments[j] = this->framebuffer.framebufferAttachments[j][i].image.imageView;
		}

		VkFramebufferCreateInfo fbCreateInfo{};
		fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbCreateInfo.pNext = nullptr;
		fbCreateInfo.renderPass = this->framebuffer.renderPass;
		fbCreateInfo.attachmentCount = attachments.size();
		fbCreateInfo.pAttachments = attachments.data();
		fbCreateInfo.width = this->framebuffer.width;
		fbCreateInfo.height = this->framebuffer.height;
		fbCreateInfo.layers = 1;

		VkFramebuffer newFramebuffer;
		auto result = vkCreateFramebuffer(device, &fbCreateInfo, nullptr, &newFramebuffer);

		if (result) {
			std::cout << "Failed to create framebuffer: " << result << std::endl;
			abort();
		}

		this->framebuffer.framebuffer.push_back(newFramebuffer);
	}

	if (result) {
		std::cout << "Failed to create sampler: " << result << std::endl;
		abort();
	}

	// Colour blend states for attachments
	std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates{};
	size_t numberColourAttachments = this->framebuffer.framebufferAttachmentReferences.size();
	blendAttachmentStates.resize(numberColourAttachments);

	// ASSUMES IF THERE IS A DEPTH ATTACHMENT AT END AND ONLY ONE DEPTH ATTACHMENT
	for (auto i = 0; i < numberColourAttachments; i++) {
		blendAttachmentStates[i] = VulkanUtility::pipelineColorBlendAttachmentState(0xF, VK_FALSE);
	}

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.pNext = nullptr;
	colorBlendStateCreateInfo.attachmentCount = numberColourAttachments;
	colorBlendStateCreateInfo.pAttachments = blendAttachmentStates.data();

	// Build the pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;

	pipelineInfo.stageCount = this->shaderStages.size();
	pipelineInfo.pStages = this->shaderStages.data();
	pipelineInfo.pVertexInputState = &this->vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &this->inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &this->rasterizer;
	pipelineInfo.pMultisampleState = &this->multisampling;
	pipelineInfo.pColorBlendState = &colorBlendStateCreateInfo;
	pipelineInfo.layout = this->pipelineLayout;
	pipelineInfo.renderPass = this->framebuffer.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDepthStencilState = &this->depthStencil;

	Pipeline pipeline;
	pipeline.name = "test";
	pipeline.readPipelineCacheFile(device);
	pipeline.pipelineSetLayout = this->pipelineSetLayout;

	result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline.pipeline);
	if (result != VK_SUCCESS) {
		std::cout << "Failed to create pipeline: " << result << std::endl;
		abort();
	}

	// Cleanup shader modules
	for (auto& stage : this->shaderStages) {
		vkDestroyShaderModule(device, stage.module, nullptr);
	}

	pipeline.pipelineSetLayoutBindings = std::move(this->pipelineSetLayoutBindings);
	pipeline.pipelineSetLayoutBuffers = std::move(this->pipelineSetLayoutBuffers);
	pipeline.framebuffer = std::move(this->framebuffer);
	pipeline.pipelineDescriptors = this->pipelineDescriptors;
	return pipeline;
}

void Pipeline::readPipelineCacheFile(VkDevice device) {
	std::vector<uint8_t> pipelineData{};

	auto path = this->name;
	
	if (std::filesystem::exists(path)) {
		std::ifstream file(path, std::ios::in | std::ios::binary);

		size_t filesize = (size_t)file.tellg();
		pipelineData.resize(filesize);

		file.seekg(0);
		file.read(reinterpret_cast<char*>(pipelineData.data()), pipelineData.size());
		file.close();
	}

	VkPipelineCacheCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	createInfo.pNext = nullptr;

	createInfo.initialDataSize = pipelineData.size();
	createInfo.pInitialData = pipelineData.data();

	VkResult result = vkCreatePipelineCache(device, &createInfo, nullptr, &this->cache);

	if (result) {
		std::cout << "Error reading pipeline cache file " << this->name << ": " << result << std::endl;
		abort();
	}
}

void Pipeline::writePipelineCacheFile(VkDevice device, bool destroyCache) {
	size_t size{};

	VkResult result = vkGetPipelineCacheData(device, this->cache, &size, nullptr);

	if (result) {
		std::cout << "Error reading pipeline cache data size " << this->name << ": " << result << std::endl;
		abort();
	}

	std::vector<uint8_t> data(size);
	
	result = vkGetPipelineCacheData(device, this->cache, &size, data.data());

	if (result) {
		std::cout << "Error reading pipeline cache data " << this->name << ": " << result << std::endl;
		abort();
	}

	auto path = this->name;

	std::ofstream file;
	file.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
	
	file.write(reinterpret_cast<const char*>(data.data()), data.size());

	file.close();

	if (destroyCache) {
		vkDestroyPipelineCache(device, this->cache, nullptr);
	}
}
