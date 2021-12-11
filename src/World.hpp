#pragma once
#include "Systems/RenderSystem/RenderSystem.hpp"
#include "Components/ModelComponent.h"
#include "Entities.hpp"

#include <memory>
#include <Managers/ResourceManager.hpp>

constexpr int WIDTH = 1920;
constexpr int HEIGHT = 1080;

typedef enum EntityCreateInfoFlags {
	Renderable = 1 << 0,
	HasModel = 1 << 1,
	HasPosition = 1 << 2,
	Moves = 1 << 3,
	Physicalised = 1 << 4
} EntityCreateInfoFlags;

struct EntityCreateInfo {
	uint64_t flags;
	uint64_t physicaliseFlags;
	std::string directory;
	std::string model;
};

class World {
	std::unique_ptr<RenderSystem> renderSystem;
	std::unique_ptr<ResourceManager> resourceManager;

	Entities entities;
	
	size_t addEntity(EntityCreateInfo* info);

public:
	void initialise();
	void run();
};