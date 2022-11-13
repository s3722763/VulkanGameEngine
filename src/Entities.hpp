#pragma once
#include <vector>
#include "Components/ModelComponent.h"
#include <queue>

struct AddEntityInfo {
	ModelResource modelResource;
};

class Entities {
	// ID stuff
	std::queue<size_t> reusableIds;
	std::vector<size_t> usedIds;

	// Components
	std::vector<ModelResource> modelResources;
	
public:
	size_t addEntity(AddEntityInfo* info);

	std::vector<ModelResource>* getModelResourceIds();
};