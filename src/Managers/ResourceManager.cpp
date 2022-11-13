#include "ResourceManager.hpp"
#include <string>
#include <sdl2/SDL_video.h>

ResourceManager::ResourceManager() {
	this->modelManager = std::make_unique<ModelManager>();
	this->vulkanResourceManager = std::make_unique<VulkanResourceManager>();
}

ModelResource ResourceManager::loadModel(std::string& directory, std::string& modelFileName) {
	std::string identifier = directory;
	identifier = identifier.append("/").append(modelFileName);
	
	ModelResource resource{};

	LoadModelResults loadResult = this->modelManager->loadModel(directory, modelFileName, identifier);
	resource.modelComponentId = loadResult.id;
	
	if ((loadResult.flags & LoadModelResultFlags::ErrorLoading) == 0) {
		resource.modelRenderComponentId = this->vulkanResourceManager->loadModelComponentBuffers(identifier, this->modelManager->getModelComponents(), loadResult.id);
	} else {
		resource.flags |= ModelResourceFlags::ErrorLoadingModel;
	}

	return resource;
}

void ResourceManager::initialise(SDL_Window* window) {
	this->vulkanResourceManager->initialiseVulkan(window);
}

void ResourceManager::cleanup() {
	this->vulkanResourceManager->cleanupVulkanResources();
}

std::vector<ModelRenderComponents>* ResourceManager::getModelRenderBuffers() {
	return this->vulkanResourceManager->getModelRenderBuffers();
}

std::vector<ModelComponent>* ResourceManager::getModelComponents() {
	return this->modelManager->getModelComponents();
}

const VulkanDetails* ResourceManager::getVulkanDetails() {
	return this->vulkanResourceManager->getVulkanDetails();
}

QueueDetails ResourceManager::createGraphicsQueue() {
	return this->vulkanResourceManager->createGraphicsQueue();
}

QueueDetails ResourceManager::createTransferQueue() {
	return this->vulkanResourceManager->createTransferQueue();
}
