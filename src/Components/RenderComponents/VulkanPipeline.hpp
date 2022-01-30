#pragma once
#include <vector>
#include <array>
#include <vulkan/vulkan.h>
#include <string>
#include <shaderc/shaderc.hpp>
#include "../../Systems/RenderSystem/VulkanTypes.hpp"
#include "Framebuffer.hpp"

struct FramebufferSetupData {
	uint32_t width, height;
};

struct ShaderInfo {
	VkShaderStageFlags flags;
	const char* vertexShaderPath;
	const char* fragmentShaderPath;
};

enum PipelineUsage {
	DeferredPipelineUsage,
	LightingPipelineUsage,
	ShadowPipelineUsage
};

class Pipeline {
public:
	VkPipeline pipeline;
	VkPipelineCache cache;
	Framebuffer framebuffer;

	// Descriptor Set Layout info for specific pipeline
	VkDescriptorSetLayout pipelineSetLayout;
	std::vector<VkDescriptorSetLayoutBinding> pipelineSetLayoutBindings;
	std::vector<std::vector<AllocatedBuffer>> pipelineSetLayoutBuffers;
	std::array<VkDescriptorSet, 3> pipelineDescriptors;
	//std::vector<std::vector<FramebufferAttachment>> pipelineAttachments;

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
	//std::vector<FramebufferAttachment> pipelineAttachments;
	VkShaderModule createShaderModule(VkDevice device, std::string name, shaderc_shader_kind kind, std::string code, std::string path);
	VkShaderModule createShaderModule(VkDevice device, std::string name, shaderc_shader_kind kind, std::vector<char>&& spirv);

	// Framebuffer infomation
	Framebuffer framebuffer;

	Pipeline buildDeferredPipeline(VkDevice device, size_t frameOverlap);
	Pipeline buildLightingPipeline(VkDevice device, size_t frameOverlap);
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

	Pipeline buildPipeline(VkDevice device, PipelineUsage pipelineUsage, size_t frameOverlap);
	void addShaders(VkDevice device, ShaderInfo* shaderInfo);
	void addPipelineDescriptorBinding(VkDescriptorType type, VkShaderStageFlagBits shaderStage);
	//void addPipelineDescriptorFramebufferImage(VkDevice device, size_t binding, const std::vector<VkImageView> imageViews, VkFormat format, VkSampler sampler);
	void allocatePipelineDescriptorUniformBuffer(VkDevice device, size_t binding, const std::vector<AllocatedBuffer> buffers, uint32_t frameOverlap);
	void setupFramebuffer(FramebufferSetupData framebufferSetupData);
	// If you add a depth buffer attachment, IT MUST BE THE LAST ATTACHMENT ADDED
	void addFramebufferAttachment(VkDevice device, VmaAllocator allocator, VkFormat format, VkImageUsageFlagBits usage, VkExtent3D extent, size_t frameOverlaps);
	// Used when framebuffer targets are already created (ie. Swapchain images)
	// Allocation of the image is the responsibility of the originator of this call
	void addFramebufferAttachment(VkDevice device, std::vector<VkImageView> attachmentImageViews, VkFormat format, VkImageUsageFlagBits usage, VkExtent3D extent, size_t frameOverlaps);
	VkDescriptorSetLayout createPipelineSetLayout(VkDevice device, uint32_t frameOverlap, VkDescriptorPool descriptorPool);
};
