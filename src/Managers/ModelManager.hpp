#pragma once
#include "../Components/ModelComponent.h"

#include <vk_mem_alloc.h>
#include <assimp/scene.h>
#include "../Systems/RenderSystem/VulkanTypes.hpp"
#include "../Components/RenderComponents/Material.hpp"

#include <iostream>
#include <vector>
#include <map>
#include <string>

enum LoadModelResultFlags {
	AlreadyLoaded = 1 << 0,
	ErrorLoading = 1 << 1
};

struct LoadModelResults {
	size_t id;
	uint64_t flags;
	std::string diffuseMaterial;
};

class ModelManager {
	std::map<std::string, size_t> nameToModelComponentId;
	std::map<std::string, size_t> nameToRenderComponentId;

	// TODO: Have these as shared pointers to enable multithreading with no race condition when vector is reallocatedz
	std::vector<ModelComponent> loadedModels;
	std::vector<ModelDetails> modelDetails;

	void processNode(aiNode* node, const aiScene* scene, ModelComponent* modelComponent, const std::string& directory, ModelDetails* details);
	void processMesh(aiMesh* mesh, const aiNode* node, const aiScene* scene, ModelComponent* modelComponent, ModelDetails* details);
	void processTexture(ModelComponent* modelComponent, aiString textureFile, TextureType type, const std::string& directory);
	void createBuffers(ModelRenderComponents* modelRenderComponents, ModelComponent* modelComponents);

public:
	ModelManager();

	LoadModelResults loadModel(const std::string& directory, const std::string& modelFileName, const std::string& identifier);
	std::vector<ModelComponent>* getModelComponents();
	std::vector<ModelDetails>* getModelDetails();
};