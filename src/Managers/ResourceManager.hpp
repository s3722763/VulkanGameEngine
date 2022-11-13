#pragma once
#include "VulkanResourceManager.hpp"
#include <vk_mem_alloc.h>
#include "ModelManager.hpp"
#include "../Components/RenderComponents/VulkanPipeline.hpp"
#include "../Systems/RenderSystem/VulkanTypes.hpp"
#include <memory>
#include <vector>
#include "../Components/RenderComponents/Material.hpp"

class ResourceManager {
private:
	std::unique_ptr<VulkanResourceManager> vulkanResourceManager;
	std::unique_ptr<ModelManager> modelManager;

public:
	ResourceManager();

	ModelResource loadModel(std::string& directory, std::string& modelFileName);

	void initialise(SDL_Window* window);
	void cleanup();

	std::vector<ModelRenderComponents>* getModelRenderBuffers();
	std::vector<ModelComponent>* getModelComponents();
	const VulkanDetails* getVulkanDetails();
	QueueDetails createGraphicsQueue();
	QueueDetails createTransferQueue();
};