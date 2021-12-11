#pragma once
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "../Systems/RenderSystem/VulkanTypes.hpp"
#include "../Components/RenderComponents/Material.hpp"

#include <assimp/material.h>

enum TextureType {
	Diffuse,
	Specular,
	Height,
	Ambient
};

enum ModelResourceFlags {
	ErrorLoadingModel = 1 << 0,
	ErrorCreatingBuffers = 1 << 1
};

struct ModelResource {
	size_t modelComponentId;
	size_t modelRenderComponentId;
	size_t modelDetailsId;
	size_t materialGroupId;
	uint64_t flags;
};

struct ModelDetails {
	std::vector<glm::mat4> meshMatrices;
	std::string directory;
};

typedef struct MeshComponents {
	std::vector<std::vector<glm::vec3>> vertices;
	std::vector<std::vector<glm::vec3>> normals;
	std::vector<std::vector<glm::vec2>> texCoords;
	std::vector<std::vector<glm::vec3>> tangent;
	std::vector<std::vector<uint32_t>> indices;
	std::vector<std::vector<glm::vec3>> colours;
	std::vector<MaterialInfo> materials;
	// Assuming only one diffuse texture for a mesh
	// std::vector<TextureID> diffuseTextures;
	// std::vector<TextureID> specularTextures;
} MeshComponents;

struct ModelRenderComponents {
	std::vector<AllocatedBuffer> VertexPositionBuffers;
	std::vector<AllocatedBuffer> IndexBuffers;
	std::vector<AllocatedBuffer> TextureCoordBuffers;
	std::vector<AllocatedBuffer> NormalBuffers;
	std::vector<AllocatedBuffer> ColourBuffers;
};

struct ModelComponent {
	MeshComponents meshes;
};