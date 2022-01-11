#pragma once
#include <vector>
#include <array>
#include <vulkan/vulkan.h>
#include <string>
#include <shaderc/shaderc.hpp>
#include "../../Systems/RenderSystem/VulkanTypes.hpp"

struct ShaderInfo {
	VkShaderStageFlags flags;
	const char* vertexShaderPath;
	const char* fragmentShaderPath;
};

class Pipeline {
public:
	VkPipeline pipeline;
	VkPipelineCache cache;

	// Descriptor Set Layout info for specific pipeline
	VkDescriptorSetLayout pipelineSetLayout;
	std::vector<VkDescriptorSetLayoutBinding> pipelineSetLayoutBindings;
	std::vector<std::vector<AllocatedBuffer>> pipelineSetLayoutBuffers;

	const char* name = nullptr;

	void readPipelineCacheFile(VkDevice device);
	void writePipelineCacheFile(VkDevice device, bool destroyCache);
};

class PipelineBuilder {
private:
	std::string readFile(const char* filepath);
	std::vector<char> readCompiledFile(const char* filepath);
	std::vector<uint32_t> compileShader(const std::string& name, shaderc_shader_kind kind, const std::string& source, bool optimise);
	std::array<VkDescriptorSet, 3> pipelineDescriptors;
	VkShaderModule createShaderModule(VkDevice device, std::string name, shaderc_shader_kind kind, std::string code, std::string path);
	VkShaderModule createShaderModule(VkDevice device, std::string name, shaderc_shader_kind kind, std::vector<char>&& spirv);

public:
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkViewport viewport;
	VkRect2D scissor;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo multisampling;
	VkPipelineDepthStencilStateCreateInfo depthStencil;
	VkPipelineLayout pipelineLayout;
	VkDescriptorSetLayout pipelineSetLayout;

	// Descriptor Set Layout bindings for specific layout
	std::vector<VkDescriptorSetLayoutBinding> pipelineSetLayoutBindings;
	std::vector<std::vector<AllocatedBuffer>> pipelineSetLayoutBuffers;

	Pipeline buildPipeline(VkDevice device, VkRenderPass pass);
	void addShaders(VkDevice device, ShaderInfo* shaderInfo);
	void addPipelineDescriptorBinding(VkDescriptorType type, VkShaderStageFlagBits shaderStage);
	void allocatePipelineDescriptorUniformBuffer(VkDevice device, size_t binding, const std::vector<AllocatedBuffer> buffers, uint32_t frameOverlap);
	VkDescriptorSetLayout createPipelineSetLayout(VkDevice device, uint32_t frameOverlap, VkDescriptorPool descriptorPool);
};
