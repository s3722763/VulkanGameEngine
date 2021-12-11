#pragma once
#include "VkBootstrap.h"
#include <sdl/SDL.h>
#include <sdl/SDL_vulkan.h>
#include <sdl/SDL_video.h>
#include <vector>
#include <map>
#include "../../Components/RenderComponents/VulkanPipeline.hpp"
#include "VulkanUtility.hpp"
#include <vma/vk_mem_alloc.h>
#include "../../Components/ModelComponent.h"
#include "../../Components/RenderComponents/Material.hpp"
#include "../../Components/RenderComponents/Camera.hpp"

struct PushConstants {
	glm::vec4 data;
	glm::mat4 renderMatrix;
};

struct GPUCameraData {
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewProj;
};

struct Framedata {
	std::vector<VkSemaphore> presentSemaphores;
	std::vector<VkSemaphore> renderSemaphores;
	std::vector<VkCommandPool> commandPools;
	std::vector<VkCommandBuffer> mainCommandBuffers;
	std::vector<VkFence> renderFences;
	std::vector<VkImageView> depthImageViews;
	std::vector<AllocatedImage> depthImages;
	std::vector<AllocatedBuffer> cameraBuffers;
	std::vector<VkDescriptorSet> globalDescriptors;
};

constexpr size_t FRAME_OVERLAP = 3;

class VulkanRenderer {
	// Vulkan device variables
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice chosenGPU;
	VkDevice device;
	VkSurfaceKHR surface;

	// Vulkan swapchain
	VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;

	// Queues
	VkQueue graphicsQueue;
	uint32_t graphicsQueueFamily;

	// Command Pool
	//VkCommandPool graphicsCommandPool;
	//VkCommandBuffer mainCommandBuffer;
	Framedata framedata;

	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;

	//VkSemaphore presentSemaphore, renderSemaphore;
	//VkFence renderFence;

	VkPipelineLayout trianglePipelineLayout;
	Pipeline trianglePipeline;

	// VMA
	VmaAllocator allocator;

	// Depth buffer
	//VkImageView depthImageView;
	//AllocatedImage depthImage;
	VkFormat depthFormat;

	VkDescriptorSetLayout globalSetLayout;
	VkDescriptorSetLayout singleTextureSetLayout;
	VkDescriptorPool descriptorPool;

	DeletionQueue mainDeletionQueue;

	VkPhysicalDeviceProperties gpuProperties;

	// Materials
	std::vector<Material> materials;
	std::vector<AllocatedImage> materialImages;
	std::vector<std::vector<size_t>> modelMaterials;
	std::map<std::string, size_t> materialMap;
	VkSampler defaultSampler;
	UploadContext imageTransferContext;
	VkQueue imageTransferQueue;
	uint32_t imageTransferQueueFamily;

	// Upload to GPU
	UploadContext uploadContext;
	VkQueue transferQueue;
	uint32_t transferQueueFamily;

	// SDL
	SDL_Window* window;

	size_t framenumber = 0;

	void initialiseFramedataStructures();
	void initialiseSwapchain();
	void initialiseCommands();
	void initialiseDefaultRenderpass();
	void initialiseFramebuffers();
	void initialiseSyncStructures();
	void initialisePipelines();
	void initialiseDescriptors();

	void drawObjects(VkCommandBuffer cmd, std::vector<ModelRenderComponents>* modelRenderComponents, std::vector<ModelResource>* modelResourceIds, std::vector<size_t>* ids, Camera* camera);

	size_t addMaterial(Material&& material, std::vector<AllocatedImage>* images);

	AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	size_t createImageFromFile(std::string& file);
	void immediateSubmit(UploadContext uploadContext, std::function<void(VkCommandBuffer cmd)>&& function);

	size_t getCurrentFrameIndex();
public:
	void initialise(const VulkanDetails* vulkanDetails, QueueDetails graphicsQueue, QueueDetails transferQueue, QueueDetails imageTransferQueue);
	void draw(std::vector<ModelRenderComponents>* modelRenderComponents, std::vector<ModelResource>* modelResourceIds, std::vector<size_t>* ids, Camera* camera);
	void cleanup();
	size_t uploadMaterial(MaterialInfo model);
	size_t uploadModelMaterials(std::vector<MaterialInfo>* materials);
	VmaAllocator getVmaAllocator();
};