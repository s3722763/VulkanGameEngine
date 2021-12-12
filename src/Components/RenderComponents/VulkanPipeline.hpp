#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <string>
#include <shaderc/shaderc.hpp>

struct ShaderInfo {
	VkShaderStageFlags flags;
	const char* vertexShaderPath;
	const char* fragmentShaderPath;
};

class Pipeline {
public:
	VkPipeline pipeline;
	VkPipelineCache cache;

	const char* name = nullptr;

	void readPipelineCacheFile(VkDevice device);
	void writePipelineCacheFile(VkDevice device, bool destroyCache);
};

class PipelineBuilder {
private:
	std::string readFile(const char* filepath);
	std::vector<char> readCompiledFile(const char* filepath);
	std::vector<uint32_t> compileShader(const std::string& name, shaderc_shader_kind kind, const std::string& source, bool optimise);
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

	Pipeline buildPipeline(VkDevice device, VkRenderPass pass);
	void addShaders(VkDevice device, ShaderInfo* shaderInfo);
};
