#include "VulkanPipeline.hpp"
#include "VulkanPipeline.hpp"
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

Pipeline PipelineBuilder::buildPipeline(VkDevice device, VkRenderPass pass) {
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

	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &this->colorBlendAttachment;

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
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = this->pipelineLayout;
	pipelineInfo.renderPass = pass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDepthStencilState = &this->depthStencil;

	Pipeline pipeline;
	pipeline.name = "test";
	pipeline.readPipelineCacheFile(device);

	VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline.pipeline);
	if (result != VK_SUCCESS) {
		std::cout << "Failed to create pipeline: " << result << std::endl;
		abort();
	}

	// Cleanup shader modules
	for (auto& stage : this->shaderStages) {
		vkDestroyShaderModule(device, stage.module, nullptr);
	}

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
