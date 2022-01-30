#include "World.hpp"
#include <sdl/SDL_vulkan.h>
#include <chrono>
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui.h"



size_t World::addEntity(EntityCreateInfo* info) {
	AddEntityInfo addEntityInfo{};

    // Create attributes
    if ((info->flags & EntityCreateInfoFlags::HasModel) != 0) {
        addEntityInfo.modelResource = this->resourceManager->loadModel(info->directory, info->model);

		auto* materials = &this->resourceManager->getModelComponents()->at(addEntityInfo.modelResource.modelComponentId).meshes.materials;
		addEntityInfo.modelResource.materialGroupId = this->renderSystem->uploadModelMaterials(materials);
    }

    // Store attributes
    size_t id = this->entities.addEntity(&addEntityInfo);

    // Pass ids for atttributes
    if ((info->flags & EntityCreateInfoFlags::Renderable) != 0) {
        this->renderSystem->addEntity(id);
    }

    return id;
}

void World::initialise() {
	this->renderSystem = std::make_unique<RenderSystem>();
	this->resourceManager = std::make_unique<ResourceManager>();

	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN;

	this->window = SDL_CreateWindow("Game Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, windowFlags);
	this->resourceManager->initialise(window);

	// Initialise SDL

	auto* vulkanDetails = this->resourceManager->getVulkanDetails();
	auto graphicsQueueDetails = this->resourceManager->createGraphicsQueue();
	auto transferQueueDetails = this->resourceManager->createTransferQueue();
	auto graphicsTransferQueueDetails = this->resourceManager->createGraphicsQueue();

	this->renderSystem->initialise(vulkanDetails, graphicsQueueDetails, transferQueueDetails, graphicsTransferQueueDetails, this->window);

	EntityCreateInfo info{};
	info.directory = "resources/models/backpack";
	info.model = "backpack.obj";
	info.flags |= EntityCreateInfoFlags::HasModel | EntityCreateInfoFlags::Renderable;

	this->addEntity(&info);

	// Eat mouse
	SDL_SetRelativeMouseMode(SDL_TRUE);
}

void World::run() {
	SDL_Event e;
	bool exit = false;

	Camera camera{};
	camera.setPosition({ 0.0, 2.0, 10.0 });

	while (!exit) {
		// Delta seconds
		std::chrono::steady_clock::time_point static time = std::chrono::steady_clock::now();
		auto current_time = std::chrono::steady_clock::now();
		float duration_seconds = std::chrono::duration<float>(current_time - time).count();
		time = current_time;

		const uint8_t* currentKeyStates = SDL_GetKeyboardState(NULL);

		while (SDL_PollEvent(&e) != 0) {
			//ImGui_ImplSDL2_ProcessEvent(&e);

			if (e.type == SDL_QUIT) {
				exit = true;
			} else if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.scancode == SDL_SCANCODE_Q) {
					exit = true;
				} else if (e.key.keysym.scancode == SDL_SCANCODE_LALT) {
					if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
						SDL_SetRelativeMouseMode(SDL_FALSE);
					} else {
						SDL_SetRelativeMouseMode(SDL_TRUE);
					}
				}
			} else if (e.type == SDL_MOUSEMOTION) {
				// Only move the camera when the window has eaten the mouse
				if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
					const float sensitivity = 0.02f;

					float yawChange = sensitivity * e.motion.xrel;
					float pitchChange = sensitivity * e.motion.yrel;
					//std::cout << e.motion.xrel << " - " << e.motion.yrel << "\n";
					//auto rotation_matrix = glm::eulerAngleXY(glm::radians(this->camera_pitch), glm::radians(this->camera_yaw));
					camera.updateLookDirection(pitchChange, yawChange);
				}
			}
		}

		camera.updateCamera(duration_seconds, currentKeyStates);

		//auto cameraPos = camera.getPosition();
		//std::cout << "Camera pos X: " << cameraPos.x << " Y: " << cameraPos.y << " Z: " << cameraPos.z << std::endl;
		/*ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame(this->window);
		ImGui::NewFrame();*/

		//ImGui::ShowDemoWindow();

		this->renderSystem->render(this->resourceManager->getModelRenderBuffers(), this->entities.getModelResourceIds(), &camera);
	}

	renderSystem->cleanup();
	resourceManager->cleanup();
}
 