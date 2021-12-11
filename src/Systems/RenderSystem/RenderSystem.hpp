#pragma once
#include "VulkanRenderer.hpp"
#include "../../Components/ModelComponent.h"
#include "../../Components/RenderComponents/Material.hpp"
#include "../../Components/RenderComponents/Camera.hpp"
#include <vector>

class RenderSystem {
	VulkanRenderer vulkanRenderer;

	std::vector<size_t> renderableIds;

public:
	void initialise(const VulkanDetails* vulkanDetails, QueueDetails graphicsQueue, QueueDetails transferQueue, QueueDetails imageTransferQueue);
	void render(std::vector<ModelRenderComponents>* models, std::vector<ModelResource>* modelResourceIds, Camera* camera);
	void cleanup();
	void addEntity(size_t id);
	void uploadModel(ModelComponent model);
	size_t uploadModelMaterials(std::vector<MaterialInfo>* materials);
	VmaAllocator getVmaAllocator();
};