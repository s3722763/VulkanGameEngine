#include "VulkanRenderer.hpp"
#include "VulkanRenderer.hpp"

#include "VulkanUtility.hpp"
#include <iostream>
#include <functional>
#include <string>
#include <sstream>
#include <fstream>
#include <array>
#include <VulkanTypes.hpp>
#include "../../Components/ModelComponent.h"
#include <glm/gtx/transform.hpp>
#include <stb_image.h>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>

constexpr int WIDTH = 1920;
constexpr int HEIGHT = 1080;
constexpr size_t POSITION_ATTACHMENT_INDEX = 0;
constexpr size_t NORMAL_ATTACHMENT_INDEX = 1;
constexpr size_t ALBEDO_ATTACHMENT_INDEX = 2;
constexpr size_t DEPTH_ATTACHMENT_INDEX = 3;

void VulkanRenderer::initialiseFramedataStructures() {
	this->framedata.commandPools.resize(FRAME_OVERLAP);
	this->framedata.deferredMainCommandBuffers.resize(FRAME_OVERLAP);
	this->framedata.lightingMainCommandBuffers.resize(FRAME_OVERLAP);
	this->framedata.presentSemaphores.resize(FRAME_OVERLAP);
	this->framedata.renderSemaphores.resize(FRAME_OVERLAP);
	this->framedata.deferredSemaphores.resize(FRAME_OVERLAP);
	this->framedata.renderFences.resize(FRAME_OVERLAP);
	this->framedata.depthImages.resize(FRAME_OVERLAP);
	this->framedata.depthImageViews.resize(FRAME_OVERLAP);
	this->framedata.cameraBuffers.resize(FRAME_OVERLAP);
	this->framedata.globalDescriptors.resize(FRAME_OVERLAP);
}

void VulkanRenderer::initialiseSwapchain() {
	vkb::SwapchainBuilder swapchainBuilder{ this->chosenGPU, this->device, this->surface };
	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection()
		// Use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(WIDTH, HEIGHT)
		.build()
		.value();

	// Store swapchain and its images
	this->swapchain = vkbSwapchain.swapchain;
	this->swapchainImages = vkbSwapchain.get_images().value();
	this->swapchainImageViews = vkbSwapchain.get_image_views().value();
	this->swapchainImageFormat = vkbSwapchain.image_format;

	this->mainDeletionQueue.pushFunction([=]() {
		vkDestroySwapchainKHR(this->device, this->swapchain, nullptr);
	});

	// Initialise depth buffer
	VkExtent3D depthImageExtent = { WIDTH, HEIGHT, 1 };
	this->depthFormat = VK_FORMAT_D32_SFLOAT;

	VkImageCreateInfo depthImageInfo = VulkanUtility::imageCreateInfo(this->depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);
	VmaAllocationCreateInfo depthImageAllocInfo{};
	depthImageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	depthImageAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	for (auto i = 0; i < FRAME_OVERLAP; i++) {
		vmaCreateImage(this->allocator, &depthImageInfo, &depthImageAllocInfo, &this->framedata.depthImages[i].image, &this->framedata.depthImages[i].allocation, nullptr);

		VkImageViewCreateInfo depthImageViewInfo = VulkanUtility::imageViewCreateInfo(this->depthFormat, this->framedata.depthImages[i].image, VK_IMAGE_ASPECT_DEPTH_BIT);

		VkResult result = vkCreateImageView(this->device, &depthImageViewInfo, nullptr, &this->framedata.depthImageViews[i]);

		if (result) {
			std::cout << "Dectected Vulkan error while creating depth image view: " << result << std::endl;
			abort();
		}

		this->mainDeletionQueue.pushFunction([=]() {
			vkDestroyImageView(this->device, this->framedata.depthImageViews[i],  nullptr);
			vmaDestroyImage(this->allocator, this->framedata.depthImages[i].image, this->framedata.depthImages[i].allocation);
		});
	}
}

void VulkanRenderer::initialiseCommands() {
	VkCommandPoolCreateInfo commandPoolInfo = VulkanUtility::commandPoolCreateInfo(this->graphicsQueueFamily);

	for (auto i = 0; i < FRAME_OVERLAP; i++) {
		VkResult result = vkCreateCommandPool(this->device, &commandPoolInfo, nullptr, &this->framedata.commandPools[i]);

		if (result) {
			std::cout << "Dectected Vulkan error while creating main graphics queue: " << result << std::endl;
			abort();
		}

		// Commands will come from the main graphics queue
		VkCommandBufferAllocateInfo cmdAllocInfo = VulkanUtility::commandBufferAllocateInfo(this->framedata.commandPools[i], 1);

		result = vkAllocateCommandBuffers(this->device, &cmdAllocInfo, &this->framedata.deferredMainCommandBuffers[i]);

		if (result) {
			std::cout << "Detected Vulkan error while creating main graphics command buffer: " << result << std::endl;
			abort();
		}

		result = vkAllocateCommandBuffers(this->device, &cmdAllocInfo, &this->framedata.lightingMainCommandBuffers[i]);

		if (result) {
			std::cout << "Detected Vulkan error while creating main graphics command buffer: " << result << std::endl;
			abort();
		}

		this->mainDeletionQueue.pushFunction([=]() {
			vkDestroyCommandPool(this->device, this->framedata.commandPools[i], nullptr);
		});
	}

	VkCommandPoolCreateInfo uploadCommandPoolInfo = VulkanUtility::commandPoolCreateInfo(transferQueueFamily);
	VkResult result = vkCreateCommandPool(this->device, &uploadCommandPoolInfo, nullptr, &this->uploadContext.commandPool);

	if (result) {
		std::cout << "Detected Vulkan error while creating transfer command buffer: " << result << std::endl;
		abort();
	}

	VkCommandPoolCreateInfo imageTransfercommandPoolInfo = VulkanUtility::commandPoolCreateInfo(this->imageTransferQueueFamily);
	result = vkCreateCommandPool(this->device, &imageTransfercommandPoolInfo, nullptr, &this->imageTransferContext.commandPool);

	if (result) {
		std::cout << "Detected Vulkan error while creating transfer command buffer: " << result << std::endl;
		abort();
	}

	this->mainDeletionQueue.pushFunction([=]() {
		vkDestroyCommandPool(this->device, this->uploadContext.commandPool, nullptr);
		vkDestroyCommandPool(this->device, this->imageTransferContext.commandPool, nullptr);
	});
}

/*void VulkanRenderer::initialiseDefaultRenderpass() {
	// I think each attachment is the input textures for like deferred rendering

	// Render pass will use this colour attachment
	VkAttachmentDescription colourAttachment{};
	// Needs to be the same format as the swapchain
	colourAttachment.format = this->swapchainImageFormat;
	colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// Clear attachment on load
	colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// Store it when the render pass is done
	colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Dont care about the starting layout of the attachment
	colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// Want the image ot be ready for display when the renderpass finishes
	colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colourAttachmentReference;
	colourAttachmentReference.attachment = 0;
	colourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.flags = 0;
	depthAttachment.format = this->depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference{};
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Going to create 1 subpass
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourAttachmentReference;
	subpass.pDepthStencilAttachment = &depthAttachmentReference;

	std::array<VkAttachmentDescription, 2> attachments = {
		colourAttachment,
		depthAttachment
	};

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.attachmentCount = attachments.size();
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	/*VkSubpassDependency dependency{};
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkResult result = vkCreateRenderPass(this->device, &renderPassInfo, nullptr, &this->renderPass);

	if (result) {
		std::cout << "Detected Vulkan error while creating default renderpass: " << result << std::endl;
		abort();
	}

	this->mainDeletionQueue.pushFunction([=]() {
		vkDestroyRenderPass(this->device, this->renderPass, nullptr);
	});
}*/

/*void VulkanRenderer::initialiseFramebuffers() {
	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.pNext = nullptr;
	framebufferInfo.renderPass = this->renderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.width = WIDTH;
	framebufferInfo.height = HEIGHT;
	framebufferInfo.layers = 1;

	// Get how many images we have in the swap chain
	const size_t swapchainImageCount = this->swapchainImages.size();
	this->framebuffers = std::vector<VkFramebuffer>(FRAME_OVERLAP);

	// FRAME_OVERLAP;

	for (auto i = 0; i < FRAME_OVERLAP; i++) {
		std::array<VkImageView, 2> attachments;
		attachments[0] = this->swapchainImageViews[i];
		attachments[1] = this->framedata.depthImageViews[i];

		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.attachmentCount = attachments.size();

		VkResult result = vkCreateFramebuffer(this->device, &framebufferInfo, nullptr, &this->framebuffers[i]);

		if (result) {
			std::cout << "Detected Vulkan error while creating framebuffer number " << std::to_string(i) << ": " << result << std::endl;
			abort();
		}

		this->mainDeletionQueue.pushFunction([=]() {
			vkDestroyFramebuffer(this->device, this->framebuffers[i], nullptr);
			vkDestroyImageView(this->device, this->swapchainImageViews[i], nullptr);
		});
	}
}*/

void VulkanRenderer::initialiseSyncStructures() {
	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;

	// Want it to start with the signaled bit so we can call it before the GPU uses it (first the first frame)
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	VkResult result{};

	for (auto i = 0; i < FRAME_OVERLAP; i++) {
		result = vkCreateFence(this->device, &fenceCreateInfo, nullptr, &this->framedata.renderFences[i]);

		if (result) {
			std::cout << "Detected Vulkan error while creating render fence: " << result << std::endl;
			abort();
		}

		this->mainDeletionQueue.pushFunction([=]() {
			vkDestroyFence(this->device, this->framedata.renderFences[i], nullptr);
		});

		result = vkCreateSemaphore(this->device, &semaphoreCreateInfo, nullptr, &this->framedata.presentSemaphores[i]);

		if (result) {
			std::cout << "Detected Vulkan error while creating present semaphore: " << result << std::endl;
			abort();
		}

		result = vkCreateSemaphore(this->device, &semaphoreCreateInfo, nullptr, &this->framedata.renderSemaphores[i]);

		if (result) {
			std::cout << "Detected Vulkan error while creating render semaphore: " << result << std::endl;
			abort();
		}

		result = vkCreateSemaphore(this->device, &semaphoreCreateInfo, nullptr, &this->framedata.deferredSemaphores[i]);

		if (result) {
			std::cout << "Detected Vulkan error while creating deferred semaphore: " << result << std::endl;
			abort();
		}

		this->mainDeletionQueue.pushFunction([=]() {
			vkDestroySemaphore(this->device, this->framedata.deferredSemaphores[i], nullptr);
			vkDestroySemaphore(this->device, this->framedata.presentSemaphores[i], nullptr);
			vkDestroySemaphore(this->device, this->framedata.renderSemaphores[i], nullptr);
		});
	}

	fenceCreateInfo.flags &= ~(VK_FENCE_CREATE_SIGNALED_BIT);
	result = vkCreateFence(this->device, &fenceCreateInfo, nullptr, &this->uploadContext.uploadFence);

	if (result) {
		std::cout << "Detected Vulkan error while creating render fence: " << result << std::endl;
		abort();
	}

	result = vkCreateFence(this->device, &fenceCreateInfo, nullptr, &this->imageTransferContext.uploadFence);

	if (result) {
		std::cout << "Detected Vulkan error while creating image transfer fence: " << result << std::endl;
		abort();
	}

	this->mainDeletionQueue.pushFunction([=]() {
		vkDestroyFence(this->device, this->imageTransferContext.uploadFence, nullptr);
		vkDestroyFence(this->device, this->uploadContext.uploadFence, nullptr);
	});
}

void VulkanRenderer::initialisePipelines() {
	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.pNext = nullptr;
	samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.maxAnisotropy = 1.0f;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 1.0f;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	vkCreateSampler(device, &samplerCreateInfo, nullptr, &this->framebufferAttachmentSampler);

	this->initialiseDeferredPipeline();
	this->initialisePhongPipeline();

	// Create default sampler
	VkSamplerCreateInfo samplerInfo = VulkanUtility::samplerCreateInfo(VK_FILTER_LINEAR);
	vkCreateSampler(this->device, &samplerInfo, nullptr, &this->defaultSampler);

	/*VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.attachmentCount = 0;
	renderPassInfo.pAttachments = nullptr;
	renderPassInfo.subpassCount = 0;
	renderPassInfo.pSubpasses = nullptr;
	renderPassInfo.dependencyCount = 0;
	renderPassInfo.pDependencies = nullptr;

	vkCreateRenderPass(device, &renderPassInfo, nullptr, &this->imguiRenderPass);*/
}

void VulkanRenderer::initialiseImgui() {
	VkDescriptorPoolSize poolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.pNext = nullptr;
	poolInfo.maxSets = 1000;
	poolInfo.poolSizeCount = std::size(poolSizes);
	poolInfo.pPoolSizes = poolSizes;

	VkDescriptorPool imguiPool;
	VkResult result = vkCreateDescriptorPool(this->device, &poolInfo, nullptr, &imguiPool);

	if (result) {
		std::cout << "Failed to create descriptor pool for imgui: " << result << std::endl;
		abort();
	}

	ImGui::CreateContext();

	/*int width, height;
	SDL_GetWindowSize(this->window, &width, &height);

	auto& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(width, height);*/

	ImGui_ImplSDL2_InitForVulkan(this->window);

	ImGui_ImplVulkan_InitInfo initInfo{};
	initInfo.Instance = this->instance;
	initInfo.PhysicalDevice = this->chosenGPU;
	initInfo.Device = this->device;
	initInfo.Queue = this->graphicsQueue;
	initInfo.DescriptorPool = imguiPool;
	initInfo.MinImageCount = 3;
	initInfo.ImageCount = 3;
	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&initInfo, this->imguiRenderPass);

	VulkanUtility::immediateSubmit(this->device, this->imageTransferQueue, this->imageTransferContext, [&](VkCommandBuffer cmd) {
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
	});

	// Clear textures from CPU
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	this->mainDeletionQueue.pushFunction([=] {
		vkDestroyDescriptorPool(this->device, imguiPool, nullptr);
		ImGui_ImplVulkan_Shutdown();
	});
}

void VulkanRenderer::initialiseGlobalDescriptors() {
	std::vector<VkDescriptorSetLayoutBinding> globalDescriptorSetLayoutBindings{};
	// Camera Buffer binding
	globalDescriptorSetLayoutBindings.push_back(VulkanUtility::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0));
	// Add lighting system sets to global layout
	this->lightingSystem.addLightingSystemToDescriptorSet(&globalDescriptorSetLayoutBindings);

	VkDescriptorSetLayoutCreateInfo setInfo{};
	setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setInfo.pNext = nullptr;

	setInfo.bindingCount = globalDescriptorSetLayoutBindings.size();
	setInfo.flags = 0;
	setInfo.pBindings = globalDescriptorSetLayoutBindings.data();

	vkCreateDescriptorSetLayout(this->device, &setInfo, nullptr, &this->sceneSetLayout);

	std::vector<VkDescriptorPoolSize> sizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 }
	};

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.pNext = nullptr;

	poolInfo.flags = 0;
	poolInfo.maxSets = 100;
	poolInfo.poolSizeCount = static_cast<uint32_t>(sizes.size());
	poolInfo.pPoolSizes = sizes.data();

	vkCreateDescriptorPool(this->device, &poolInfo, nullptr, &this->descriptorPool);

	this->mainDeletionQueue.pushFunction([=]() {
		vkDestroyDescriptorSetLayout(this->device, this->sceneSetLayout, nullptr);
		//vkDestroyDescriptorSetLayout(this->device, this->phongTextureSetLayout, nullptr);
		vkDestroyDescriptorPool(this->device, this->descriptorPool, nullptr);
	});

	// Create lighting buffers


	for (auto i = 0; i < FRAME_OVERLAP; i++) {
		this->framedata.cameraBuffers[i] = VulkanUtility::createBuffer(this->allocator, sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;

		allocInfo.descriptorPool = this->descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &this->sceneSetLayout;

		vkAllocateDescriptorSets(this->device, &allocInfo, &this->framedata.globalDescriptors[i]);

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = this->framedata.cameraBuffers[i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(GPUCameraData);

		VkWriteDescriptorSet cameraSetWrite = VulkanUtility::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, this->framedata.globalDescriptors[i], &bufferInfo, 0);

		vkUpdateDescriptorSets(this->device, 1, &cameraSetWrite, 0, nullptr);

		this->mainDeletionQueue.pushFunction([=]() {
			vmaDestroyBuffer(this->allocator, this->framedata.cameraBuffers[i].buffer, this->framedata.cameraBuffers[i].allocation);
		});
	}
}

void VulkanRenderer::drawObjects(VkCommandBuffer cmd, std::vector<ModelRenderComponents>* modelRenderComponents, std::vector<ModelResource>* modelResourceIds, 
								 std::vector<size_t>* ids, Camera* camera) {
	static float count = 0;

	glm::mat4 view = camera->generateView();
	glm::mat4 proj = glm::perspective(glm::radians(70.0f), static_cast<float>(WIDTH) / static_cast<float>(HEIGHT), 0.1f, 200.0f);
	proj[1][1] *= -1;

	GPUCameraData cameraData{};
	cameraData.proj = proj;
	cameraData.view = view;
	cameraData.viewProj = proj * view;

	void* data;
	vmaMapMemory(this->allocator, this->framedata.cameraBuffers[this->getCurrentFrameIndex()].allocation, &data);
	memcpy(data, &cameraData, sizeof(GPUCameraData));
	vmaUnmapMemory(this->allocator, this->framedata.cameraBuffers[this->getCurrentFrameIndex()].allocation);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->deferredPipeline.pipeline);
	
	std::array<VkDescriptorSet, 1> sceneDescriptorSets = {
		this->framedata.globalDescriptors[this->getCurrentFrameIndex()]
	};

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->deferredPipelineLayout, 0, sceneDescriptorSets.size(), sceneDescriptorSets.data(), 0, nullptr);

	glm::vec3 modelPos = glm::vec3{ 0, 0, 0};
	glm::mat4 meshMatrix = glm::translate(glm::mat4{1.0f}, modelPos);

	PushConstants pushConstants{};
	pushConstants.renderMatrix = meshMatrix;

	vkCmdPushConstants(cmd, this->deferredPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

	VkDeviceSize offset = 0;
	std::array<VkDeviceSize, 3> offsets = {
		offset,
		offset,
		offset
	};

	for (auto id : *ids) {
		auto& resourceId = modelResourceIds->at(id);

		auto& model = modelRenderComponents->at(resourceId.modelComponentId);

		auto* materialIds = &this->modelMaterials.at(resourceId.materialGroupId);

		for (size_t i = 0; i < model.VertexPositionBuffers.size(); i++) {
			std::array<VkBuffer, 3> vertexBuffers = {
				model.VertexPositionBuffers[i].buffer,
				model.TextureCoordBuffers[i].buffer,
				model.NormalBuffers[i].buffer
			};

			auto* material = &this->materials.at(materialIds->at(i));

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->deferredPipelineLayout, 1, 1, &material->materialDescriptorSet, 0, nullptr);

			vkCmdBindIndexBuffer(cmd, model.IndexBuffers[i].buffer, offset, VK_INDEX_TYPE_UINT32);
			vkCmdBindVertexBuffers(cmd, 0, vertexBuffers.size(), vertexBuffers.data(), offsets.data());
			vkCmdDrawIndexed(cmd, model.IndexBuffers[i].size, 1, 0, 0, 0);
		}
	}
}

size_t VulkanRenderer::addMaterial(Material&& material, std::vector<AllocatedImage>* images) {
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = this->descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &this->deferredPipeline.pipelineSetLayout;

	VkResult result = vkAllocateDescriptorSets(this->device, &allocInfo, &material.materialDescriptorSet);

	if (result) {
		std::cout << "Detected Vulkan error while allocating descriptor set to material: " << result << std::endl;
		abort();
	}

	VkDescriptorImageInfo imageBufferInfo{};
	imageBufferInfo.sampler = this->defaultSampler;
	imageBufferInfo.imageView = images->at(material.diffuseTextureId).imageView;
	imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet diffuseTexture = VulkanUtility::writeDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, material.materialDescriptorSet, &imageBufferInfo, 0);
	vkUpdateDescriptorSets(this->device, 1, &diffuseTexture, 0, nullptr);

	size_t id = this->materials.size();
	this->materials.push_back(material);

	return id;
}

size_t VulkanRenderer::createImageFromFile(std::string& file) {
	auto material = this->materialMap.find(file);

	if (material == this->materialMap.end()) {
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(file.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		if (!pixels) {
			std::cout << "Failed to load texture file: " << file << " -> " << stbi_failure_reason() << std::endl;
			return -1;
		}

		void* pixel_ptr = pixels;
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		AllocatedBuffer stagingBuffer{};
		size_t bufferSize = imageSize;

		VkBufferCreateInfo stagingBufferInfo{};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.pNext = nullptr;

		stagingBufferInfo.size = bufferSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		// Let VMA know this data should be in CPU RAM
		VmaAllocationCreateInfo vmaAllocInfo{};
		vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		VkResult result = vmaCreateBuffer(this->allocator, &stagingBufferInfo, &vmaAllocInfo, &stagingBuffer.buffer, &stagingBuffer.allocation, nullptr);

		if (result) {
			std::cout << "Couldn't create staging buffer: " << result << std::endl;
			abort();
		}

		void* data{};
		vmaMapMemory(this->allocator, stagingBuffer.allocation, &data);
		memcpy(data, pixel_ptr, static_cast<size_t>(imageSize));
		vmaUnmapMemory(this->allocator, stagingBuffer.allocation);

		stbi_image_free(pixels);

		VkExtent3D imageExtent{};
		imageExtent.width = static_cast<uint32_t>(texWidth);
		imageExtent.height = static_cast<uint32_t>(texHeight);
		imageExtent.depth = 1;

		VkImageCreateInfo imageInfo = VulkanUtility::imageCreateInfo(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
		AllocatedImage image{};

		VmaAllocationCreateInfo imageAllocInfo{};
		imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		vmaCreateImage(this->allocator, &imageInfo, &imageAllocInfo, &image.image, &image.allocation, nullptr);

		VulkanUtility::immediateSubmit(this->device, this->imageTransferQueue, this->imageTransferContext, [=](VkCommandBuffer cmd) {
			VkImageSubresourceRange range{};
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel = 0;
			range.levelCount = 1;
			range.baseArrayLayer = 0;
			range.layerCount = 1;

			VkImageMemoryBarrier imageBarrierToTransfer{};
			imageBarrierToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrierToTransfer.pNext = nullptr;

			imageBarrierToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageBarrierToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrierToTransfer.image = image.image;
			imageBarrierToTransfer.subresourceRange = range;

			imageBarrierToTransfer.srcAccessMask = 0;
			imageBarrierToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrierToTransfer);

			VkBufferImageCopy copyRegion{};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;

			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = imageExtent;

			vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			VkImageMemoryBarrier imageBarrierToReadable = imageBarrierToTransfer;
			imageBarrierToReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrierToReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageBarrierToReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageBarrierToReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrierToReadable);
		});

		VkImageViewCreateInfo imageViewInfo = VulkanUtility::imageViewCreateInfo(VK_FORMAT_R8G8B8A8_SRGB, image.image, VK_IMAGE_ASPECT_COLOR_BIT);
		vkCreateImageView(this->device, &imageViewInfo, nullptr, &image.imageView);

		size_t id = this->materialImages.size();
		this->materialImages.push_back(image);

		this->materialMap[file] = id;

		return id;
	} else {
		return material->second;
	}
}

size_t VulkanRenderer::getCurrentFrameIndex() {
	return this->framenumber % FRAME_OVERLAP;
}

void VulkanRenderer::initialise(const VulkanDetails* vulkanDetails, QueueDetails graphicsQueue, QueueDetails transferQueue, QueueDetails imageTransferQueue, SDL_Window* window) {
	this->device = vulkanDetails->device;
	this->instance = vulkanDetails->instance;
	this->debugMessenger = vulkanDetails->debugMessenger;
	this->graphicsQueue = graphicsQueue.queue;
	this->graphicsQueueFamily = graphicsQueue.family;
	this->allocator = vulkanDetails->allocator;
	this->surface = vulkanDetails->surface;
	this->chosenGPU = vulkanDetails->chosenGPU;
	this->transferQueue = transferQueue.queue;
	this->transferQueueFamily = transferQueue.family;
	this->imageTransferQueue = imageTransferQueue.queue;
	this->imageTransferQueueFamily = imageTransferQueue.family;
	this->window = window;

	this->initialiseFramedataStructures();
	this->initialiseSwapchain();
	this->initialiseCommands();
	//this->initialiseDefaultRenderpass();
	//this->initialiseFramebuffers();
	this->initialiseSyncStructures();

	this->lightingSystem.initialise(FRAME_OVERLAP);

	// Add light
	PointLightCreateInfo pointLightCreateInfo{};
	pointLightCreateInfo.attenuationFactors = { 0, 0, 0 };
	pointLightCreateInfo.baseColour = { 1.0, 1.0, 1.0, 0.0 };
	pointLightCreateInfo.position = { -10, 0, 0, 0};

	this->lightingSystem.addPointLight(pointLightCreateInfo);

	DirectionalLightCreateInfo directionalLightCreateInfo{};
	directionalLightCreateInfo.baseColour = { 0.8, 0.8, 0.8, 0.0 };
	directionalLightCreateInfo.direction = { 0.0, 0.0, 1.0, 0.0 }; 
	this->lightingSystem.addDirectionLight(directionalLightCreateInfo);

	this->initialiseGlobalDescriptors();
	this->initialisePipelines();
	//this->initialiseImgui();
}

void VulkanRenderer::draw(std::vector<ModelRenderComponents>* modelRenderComponents, std::vector<ModelResource>* modelResourceIds, std::vector<size_t>* ids, Camera* camera) {
	size_t index = this->getCurrentFrameIndex();

	// Wait for GPU to finish rendering the last frame. Timeout after 1 second
	VkResult result = vkWaitForFences(this->device, 1, &this->framedata.renderFences[index], true, 1000000000);

	if (result) {
		std::cout << "Detected Vulkan error while waiting for render fence in draw function: " << result << std::endl;
		abort();
	}

	result = vkResetFences(this->device, 1, &this->framedata.renderFences[index]);

	if (result) {
		std::cout << "Detected Vulkan error while resetting render fence in draw function: " << result << std::endl;
		abort();
	}

	// Update light system
	this->lightingSystem.updateLightingSystemBuffers(this->device, this->transferQueue, this->uploadContext, this->allocator, index, this->framedata.globalDescriptors[index]);

	uint32_t swapchainImageIndex;
	result = vkAcquireNextImageKHR(this->device, this->swapchain, 1000000000, this->framedata.presentSemaphores[index], nullptr, &swapchainImageIndex);

	if (result) {
		std::cout << "Detected Vulkan error while acquiring swapchain image: " << result << std::endl;
		abort();
	}

	// We are sure command buffer is finished executing as previous frame finished processing. We can result the command buffer
	result = vkResetCommandBuffer(this->framedata.deferredMainCommandBuffers[index], 0);

	if (result) {
		std::cout << "Detected Vulkan error while resetting the main command buffer: " << result << std::endl;
		abort();
	}

	//ImGui::Render();

	VkCommandBuffer deferredCmd = this->framedata.deferredMainCommandBuffers[index];
	VkCommandBufferBeginInfo cmdBeginInfo = VulkanUtility::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	
	result = vkBeginCommandBuffer(deferredCmd, &cmdBeginInfo);

	if (result) {
		std::cout << "Detected Vulkan error while beginning command buffer in draw function: " << result << std::endl;
		abort();
	}

	VkClearValue clearValue{};
	//float flash = std::abs(std::sin(static_cast<float>(this->framenumber) / 120.0f));
	clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };

	VkClearValue depthClear{};
	depthClear.depthStencil.depth = 1.0f;
	depthClear.depthStencil.stencil = 1;

	// Begin main render pass
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.pNext = nullptr;

	renderPassInfo.renderPass = this->deferredPipeline.framebuffer.renderPass;
	// Offset of x = 0 & y = 0
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = { this->deferredPipeline.framebuffer.width, this->deferredPipeline.framebuffer.height };
	renderPassInfo.framebuffer = this->deferredPipeline.framebuffer.framebuffer[index];

	std::array<VkClearValue, 4> clearValues = {
		clearValue, clearValue, clearValue, depthClear
	};

	// Connect clear values
	renderPassInfo.clearValueCount = clearValues.size();
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(deferredCmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	this->drawObjects(deferredCmd, modelRenderComponents, modelResourceIds, ids, camera);

	/*glm::vec3 camPos = {0.0f, 0.0f, -2.0f};
	glm::mat4 view = glm::translate(glm::mat4(1.0f), camPos);
	glm::mat4 projection = glm::perspective(glm::radians(70.0f), 1920.0f / 1080.0f, 0.1f, 200.0f);
	projection[1][1] *= -1;*/

	//ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRenderPass(deferredCmd);

	result = vkEndCommandBuffer(deferredCmd);

	if (result) {
		std::cout << "Detected Vulkan error while ending command buffer in draw function: " << result << std::endl;
		abort();
	}

	// Prepare to submit the command buffer to the graphcis queue
	VkSubmitInfo submit = VulkanUtility::submitInfo(&deferredCmd);
	VkPipelineStageFlags waitState = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit.pWaitDstStageMask = &waitState;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &this->framedata.presentSemaphores[index];
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &this->framedata.deferredSemaphores[index];
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &deferredCmd;
	vkQueueSubmit(this->graphicsQueue, 1, &submit, VK_NULL_HANDLE);

	// Draw to actual framebuffers
	result = vkResetCommandBuffer(this->framedata.lightingMainCommandBuffers[index], 0);

	if (result) {
		std::cout << "Detected Vulkan error while resetting the main command buffer: " << result << std::endl;
		abort();
	}

	//ImGui::Render();

	VkCommandBuffer lightingCmd = this->framedata.lightingMainCommandBuffers[index];
	cmdBeginInfo = VulkanUtility::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	result = vkBeginCommandBuffer(lightingCmd, &cmdBeginInfo);

	if (result) {
		std::cout << "Detected Vulkan error while beginning command buffer in draw function: " << result << std::endl;
		abort();
	}
	
	VkRenderPassBeginInfo lightingRenderPassInfo{};
	lightingRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	lightingRenderPassInfo.pNext = nullptr;

	lightingRenderPassInfo.renderPass = this->phongPipeline.framebuffer.renderPass;
	// Offset of x = 0 & y = 0
	lightingRenderPassInfo.renderArea.offset = { 0, 0 };
	lightingRenderPassInfo.renderArea.extent = { this->phongPipeline.framebuffer.width, this->phongPipeline.framebuffer.height };
	lightingRenderPassInfo.framebuffer = this->phongPipeline.framebuffer.framebuffer[index];

	std::array<VkClearValue, 2> lightingClearValues = {
		clearValue, depthClear
	};

	// Connect clear values
	lightingRenderPassInfo.clearValueCount = lightingClearValues.size();
	lightingRenderPassInfo.pClearValues = lightingClearValues.data();

	std::array<VkDescriptorSet, 1> sceneDescriptorSets = {
		this->framedata.globalDescriptors[this->getCurrentFrameIndex()]
	};

	vkCmdBeginRenderPass(lightingCmd, &lightingRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindDescriptorSets(lightingCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->deferredPipelineLayout, 0, sceneDescriptorSets.size(), sceneDescriptorSets.data(), 0, nullptr);
	vkCmdBindDescriptorSets(lightingCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->phongPipelineLayout, 1, 1, &this->phongPipeline.pipelineDescriptors[index], 0, nullptr);
	vkCmdBindPipeline(lightingCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->phongPipeline.pipeline);
	vkCmdDraw(lightingCmd, 3, 1, 0, 0);

	vkCmdEndRenderPass(lightingCmd);

	result = vkEndCommandBuffer(lightingCmd);

	if (result) {
		std::cout << "Detected Vulkan error while ending command buffer in draw function: " << result << std::endl;
		abort();
	}

	// Prepare to submit the command buffer to the graphcis queue
	submit = VulkanUtility::submitInfo(&lightingCmd);

	waitState = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit.pWaitDstStageMask = &waitState;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &this->framedata.deferredSemaphores[index];
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &this->framedata.renderSemaphores[index];

	// Submit the command buffer to the queue
	result = vkQueueSubmit(this->graphicsQueue, 1, &submit, this->framedata.renderFences[index]);

	if (result) {
		std::cout << "Detected Vulkan error while submitting command buffer to graphics queue: " << result << std::endl;
		abort();
	}

	// Need to wait for the render semaphore to be set to make the image visible on screen
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &this->swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pWaitSemaphores = &this->framedata.renderSemaphores[index];
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &swapchainImageIndex;
	
	result = vkQueuePresentKHR(this->graphicsQueue, &presentInfo);

	if (result) {
		std::cout << "Detected Vulkan error while queuing a present image: " << result << std::endl;
		abort();
	}

	this->framenumber += 1;
}

// ASSUME ONLY ONE TEXTURE OF EACH TYPE
size_t VulkanRenderer::uploadMaterial(MaterialInfo materialInfo) {
	Material material{};

	aiString a;
	std::string path = materialInfo.diffusePath;
	std::cout << "Loading material: " + path << std::endl;
	material.diffuseTextureId = this->createImageFromFile(path);

	auto id = this->addMaterial(std::move(material), &this->materialImages);

	return id;
}

size_t VulkanRenderer::uploadModelMaterials(std::vector<MaterialInfo>* materials) {
	std::vector<size_t> materialIds;
	materialIds.resize(materials->size());

	for (size_t i = 0; i < materials->size(); i++) {
		materialIds[i] = this->uploadMaterial(materials->at(i));
	}
	
	size_t id = modelMaterials.size();
	modelMaterials.push_back(std::move(materialIds));

	return id;
}

void VulkanRenderer::cleanup() {
	vkWaitForFences(this->device, this->framedata.renderFences.size(), this->framedata.renderFences.data(), true, 1000000000);
	//phongPipeline.writePipelineCacheFile(this->device, true);

	this->mainDeletionQueue.flush();
}

void VulkanRenderer::initialisePhongPipeline() {
	// Setup shader information
	ShaderInfo shaderInfo{};
	shaderInfo.flags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderInfo.vertexShaderPath = "resources/shaders/phong.vert";
	shaderInfo.fragmentShaderPath = "resources/shaders/phong.frag";

	PipelineBuilder pipelineBuilder;
	pipelineBuilder.addShaders(this->device, &shaderInfo);

	pipelineBuilder.vertexInputInfo = VulkanUtility::vertexInputStateCreateInfo();
	pipelineBuilder.inputAssembly = VulkanUtility::inputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.viewport.x = 0.0f;
	pipelineBuilder.viewport.y = 0.0f;
	pipelineBuilder.viewport.width = WIDTH;
	pipelineBuilder.viewport.height = HEIGHT;
	pipelineBuilder.viewport.minDepth = 0.0f;
	pipelineBuilder.viewport.maxDepth = 1.0f;

	pipelineBuilder.scissor.offset = { 0, 0 };
	pipelineBuilder.scissor.extent = { WIDTH, HEIGHT };

	pipelineBuilder.rasterizer = VulkanUtility::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
	pipelineBuilder.multisampling = VulkanUtility::multisamplingStateCreateInfo();
	pipelineBuilder.colorBlendAttachment = VulkanUtility::colorBlendAttachmentState();

	pipelineBuilder.setupFramebuffer({ .width = WIDTH, .height = HEIGHT });

	// Setup descriptor sets and push constrants
	// Must create pipeline set layout with builder before creating pipeline layout
	VkExtent3D extent{};
	extent.depth = 1;
	extent.width = WIDTH;
	extent.height = HEIGHT;

	// Setup framebuffer and depth buffer
	pipelineBuilder.addFramebufferAttachment(this->device, this->swapchainImageViews, this->swapchainImageFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, extent, FRAME_OVERLAP);
	pipelineBuilder.addFramebufferAttachment(this->device, this->allocator, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, extent, FRAME_OVERLAP);

	// Position texture
	pipelineBuilder.addPipelineDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	// Normal texture
	pipelineBuilder.addPipelineDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	// Albedo texture
	pipelineBuilder.addPipelineDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	auto pipelineSetLayout = pipelineBuilder.createPipelineSetLayout(this->device, FRAME_OVERLAP, this->descriptorPool);
	
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = VulkanUtility::pipelineLayoutCreateInfo();

	std::array<VkDescriptorSetLayout, 2> setLayouts = {
		this->sceneSetLayout,
		pipelineSetLayout
	};

	VkPushConstantRange pushConstant{};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(PushConstants);
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstant;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;

	pipelineLayoutCreateInfo.pSetLayouts = setLayouts.data();
	pipelineLayoutCreateInfo.setLayoutCount = setLayouts.size();

	VkResult result = vkCreatePipelineLayout(this->device, &pipelineLayoutCreateInfo, nullptr, &this->phongPipelineLayout);
	if (result) {
		std::cout << "Detected Vulkan error while creating pipeline layout: " << result << std::endl;
		abort();
	}

	pipelineBuilder.pipelineLayout = this->phongPipelineLayout;

	pipelineBuilder.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	pipelineBuilder.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	VkPipelineDepthStencilStateCreateInfo depthStencil = VulkanUtility::depthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
	pipelineBuilder.depthStencil = depthStencil;

	this->phongPipeline = pipelineBuilder.buildPipeline(this->device, PipelineUsage::LightingPipelineUsage, FRAME_OVERLAP);

	this->mainDeletionQueue.pushFunction([=]() {
		vkDestroyPipeline(this->device, this->phongPipeline.pipeline, nullptr);
		vkDestroyPipelineLayout(this->device, this->phongPipelineLayout, nullptr);
	});

	// Update image descriptor sets to connect deferred framebuffer images to input

	for (auto i = 0; i < FRAME_OVERLAP; i++) {
		// Position
		auto* positionImage = &this->deferredPipeline.framebuffer.framebufferAttachments[POSITION_ATTACHMENT_INDEX][i].image;
		VkDescriptorImageInfo positionImageInfo = VulkanUtility::descriptorimageInfo(this->framebufferAttachmentSampler, positionImage->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		// Normal
		auto* normalImage = &this->deferredPipeline.framebuffer.framebufferAttachments[NORMAL_ATTACHMENT_INDEX][i].image;
		VkDescriptorImageInfo normalImageInfo = VulkanUtility::descriptorimageInfo(this->framebufferAttachmentSampler, normalImage->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		// Albedo
		auto* albedoImage = &this->deferredPipeline.framebuffer.framebufferAttachments[ALBEDO_ATTACHMENT_INDEX][i].image;
		VkDescriptorImageInfo albedoImageInfo = VulkanUtility::descriptorimageInfo(this->framebufferAttachmentSampler, albedoImage->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	
		auto descriptor = this->phongPipeline.pipelineDescriptors[i];

		std::array<VkWriteDescriptorSet, 3> writes = {
			VulkanUtility::writeDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptor, &positionImageInfo, POSITION_ATTACHMENT_INDEX),
			VulkanUtility::writeDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptor, &normalImageInfo, NORMAL_ATTACHMENT_INDEX),
			VulkanUtility::writeDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptor, &albedoImageInfo, ALBEDO_ATTACHMENT_INDEX)
		};

		vkUpdateDescriptorSets(this->device, writes.size(), writes.data(), 0, nullptr);
	}
}

void VulkanRenderer::initialiseDeferredPipeline() {
	// Setup shader information
	ShaderInfo shaderInfo{};
	shaderInfo.flags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderInfo.vertexShaderPath = "resources/shaders/deferred.vert";
	shaderInfo.fragmentShaderPath = "resources/shaders/deferred.frag";

	PipelineBuilder pipelineBuilder;
	pipelineBuilder.addShaders(this->device, &shaderInfo);

	pipelineBuilder.vertexInputInfo = VulkanUtility::vertexInputStateCreateInfo();
	pipelineBuilder.inputAssembly = VulkanUtility::inputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.viewport.x = 0.0f;
	pipelineBuilder.viewport.y = 0.0f;
	pipelineBuilder.viewport.width = WIDTH;
	pipelineBuilder.viewport.height = HEIGHT;
	pipelineBuilder.viewport.minDepth = 0.0f;
	pipelineBuilder.viewport.maxDepth = 1.0f;

	pipelineBuilder.scissor.offset = { 0, 0 };
	pipelineBuilder.scissor.extent = { WIDTH, HEIGHT };

	pipelineBuilder.rasterizer = VulkanUtility::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
	pipelineBuilder.multisampling = VulkanUtility::multisamplingStateCreateInfo();
	pipelineBuilder.colorBlendAttachment = VulkanUtility::colorBlendAttachmentState();

	pipelineBuilder.setupFramebuffer({ .width = WIDTH, .height = HEIGHT});

	// Setup descriptor sets and push constrants
	// Must create pipeline set layout with builder before creating pipeline layout
	VkExtent3D extent{};
	extent.depth = 1;
	extent.width = WIDTH;
	extent.height = HEIGHT;

	// Position
	pipelineBuilder.addFramebufferAttachment(this->device, this->allocator, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, extent, FRAME_OVERLAP);
	// Normals
	pipelineBuilder.addFramebufferAttachment(this->device, this->allocator, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, extent, FRAME_OVERLAP);
	// Albedo
	pipelineBuilder.addFramebufferAttachment(this->device, this->allocator, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, extent, FRAME_OVERLAP);
	// Depth
	pipelineBuilder.addFramebufferAttachment(this->device, this->allocator, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, extent, FRAME_OVERLAP);

	// Albedo texture from model
	pipelineBuilder.addPipelineDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	auto pipelineSetLayout = pipelineBuilder.createPipelineSetLayout(this->device, FRAME_OVERLAP, this->descriptorPool);

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = VulkanUtility::pipelineLayoutCreateInfo();

	std::array<VkDescriptorSetLayout, 2> setLayouts = {
		this->sceneSetLayout,
		pipelineSetLayout
	};

	VkPushConstantRange pushConstant{};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(PushConstants);
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstant;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;

	pipelineLayoutCreateInfo.pSetLayouts = setLayouts.data();
	pipelineLayoutCreateInfo.setLayoutCount = setLayouts.size();

	VkResult result = vkCreatePipelineLayout(this->device, &pipelineLayoutCreateInfo, nullptr, &this->deferredPipelineLayout);
	if (result) {
		std::cout << "Detected Vulkan error while creating deferred pipeline layout: " << result << std::endl;
		abort();
	}

	pipelineBuilder.pipelineLayout = this->deferredPipelineLayout;

	// Setup vertex inputs
	VertexInputDescription vertexDescription = ModelVertexInputDescription::getVertexDescription();
	pipelineBuilder.vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();
	pipelineBuilder.vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();

	pipelineBuilder.vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
	pipelineBuilder.vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();

	pipelineBuilder.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	pipelineBuilder.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	VkPipelineDepthStencilStateCreateInfo depthStencil = VulkanUtility::depthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
	pipelineBuilder.depthStencil = depthStencil;

	this->deferredPipeline = pipelineBuilder.buildPipeline(this->device, PipelineUsage::DeferredPipelineUsage, FRAME_OVERLAP);

	this->mainDeletionQueue.pushFunction([=]() {
		vkDestroyPipeline(this->device, this->deferredPipeline.pipeline, nullptr);
		vkDestroyPipelineLayout(this->device, this->deferredPipelineLayout, nullptr);
	});
}

VmaAllocator VulkanRenderer::getVmaAllocator() {
	return this->allocator;
}