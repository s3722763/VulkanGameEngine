#include "RenderSystem.hpp"
void RenderSystem::initialise(const VulkanDetails* vulkanDetails, QueueDetails graphicsQueue, QueueDetails transferQueue, QueueDetails imageTransferQueue, SDL_Window* window) {
	return this->vulkanRenderer.initialise(vulkanDetails, graphicsQueue, transferQueue, imageTransferQueue, window);
}

void RenderSystem::render(std::vector<ModelRenderComponents>* models, std::vector<ModelResource>* modelResourceIds, Camera* camera) {
	vulkanRenderer.draw(models, modelResourceIds, &this->renderableIds, camera);
}

void RenderSystem::cleanup() {
	vulkanRenderer.cleanup();
}

void RenderSystem::addEntity(size_t id) {
	this->renderableIds.push_back(id);
}

void RenderSystem::uploadModel(ModelComponent model) {

}

size_t RenderSystem::uploadModelMaterials(std::vector<MaterialInfo>* materials) {
	return this->vulkanRenderer.uploadModelMaterials(materials);
}

VmaAllocator RenderSystem::getVmaAllocator() {
	return this->vulkanRenderer.getVmaAllocator();
}
