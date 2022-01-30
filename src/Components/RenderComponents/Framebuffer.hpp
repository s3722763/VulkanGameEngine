#pragma once
#include <vulkan/vulkan.h>

struct FramebufferAttachment {
	AllocatedImage image;
	VkFormat format;
	uint32_t bindingPoint;
	VkImageAspectFlags aspectMask;
};

struct Framebuffer {
	uint32_t width, height;
	std::vector<VkFramebuffer> framebuffer;
	std::vector<std::vector<FramebufferAttachment>> framebufferAttachments;
	std::vector<VkAttachmentDescription> framebufferAttachmentDescriptions;
	std::vector<VkAttachmentReference> framebufferAttachmentReferences;
	VkAttachmentReference depthAttachmentReference;
	VkRenderPass renderPass;
	VkSampler attachmentSampler;
};