#include "ModelManager.hpp"

#include <array>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <stb/stb_image.h>

LoadModelResults ModelManager::loadModel(const std::string& directory, const std::string& modelFileName, const std::string& identifier) {
	LoadModelResults result{};

	if (this->nameToModelComponentId.find(identifier) != this->nameToModelComponentId.end()) {
		result.id = this->nameToModelComponentId[identifier];
		result.flags = LoadModelResultFlags::AlreadyLoaded;
	} else {
		Assimp::Importer importer;
		std::string modelFilepath = directory;
		modelFilepath = modelFilepath.append("/").append(modelFileName);

		const aiScene* scene = importer.ReadFile(modelFilepath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

		ModelComponent modelComponent;
		ModelDetails modelDetails;

		modelDetails.directory = directory;

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::cerr << "Error : Assimp : " << importer.GetErrorString() << "\n";
			result.flags = LoadModelResultFlags::ErrorLoading;
		}

		//std::map<std::string, GLuint> loadedTextures;

		this->processNode(scene->mRootNode, scene, &modelComponent, directory, &modelDetails);

		size_t id = this->loadedModels.size();
		this->loadedModels.push_back(modelComponent);
		this->nameToModelComponentId[identifier] = id;

		result.id = id;
	}

	return result;
}

std::vector<ModelComponent>* ModelManager::getModelComponents() {
	return &this->loadedModels;
}

std::vector<ModelDetails>* ModelManager::getModelDetails() {
	return &this->modelDetails;
}

void ModelManager::processNode(aiNode* node, const aiScene* scene, ModelComponent* modelComponent, const std::string& directory, ModelDetails* details) {
	for (auto i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		this->processMesh(mesh, node, scene, modelComponent, details);
	}

	for (auto i = 0; i < node->mNumChildren; i++) {
		this->processNode(node->mChildren[i], scene, modelComponent, directory, details);
	}
}

void ModelManager::processMesh(aiMesh* mesh, const aiNode* node, const aiScene* scene, ModelComponent* modelComponent, ModelDetails* details) {
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec3> tangents;
	std::vector<glm::vec2> texCoords;
	//std::vector<glm::vec3> colours;
	std::vector<uint32_t> indices;

	vertices.resize(mesh->mNumVertices);
	normals.resize(mesh->mNumVertices);
	tangents.resize(mesh->mNumVertices);
	texCoords.resize(mesh->mNumVertices);
	//colours.resize(mesh->mNumVertices);
	// ASSUME: each face is a triangle
	indices.resize(3 * mesh->mNumFaces);

	for (auto i = 0; i < mesh->mNumVertices; i++) {
		glm::vec3 vertex = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
		glm::vec3 normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
		glm::vec3 tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
		//glm::vec3 colour = { mesh->mColors[i]->r, mesh->mColors[i]->g, mesh->mColors[i]->b };
		// 1 - for vulkan
		glm::vec2 texCoord = { mesh->mTextureCoords[0][i].x, 1 - mesh->mTextureCoords[0][i].y };

		// TODO: Texcoords
		vertices[i] = std::move(vertex);
		normals[i] = std::move(normal);
		tangents[i] = std::move(tangent);
		texCoords[i] = std::move(texCoord);
		//colours[i] = std::move(colour);
	}

	for (auto i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];

		for (auto j = 0; j < face.mNumIndices; j++) {
			auto index = (3 * i) + j;
			indices[index] = face.mIndices[j];
		}
	}

	size_t id = modelComponent->meshes.vertices.size();

	auto* material = scene->mMaterials[mesh->mMaterialIndex];

	aiString a;
	material->GetTexture(aiTextureType_DIFFUSE, 0, &a);

	MaterialInfo materialInfo{};
	materialInfo.diffusePath = "./" + details->directory + '/' + a.C_Str();

	modelComponent->meshes.indices.push_back(std::move(indices));
	modelComponent->meshes.vertices.push_back(std::move(vertices));
	modelComponent->meshes.normals.push_back(std::move(normals));
	modelComponent->meshes.tangent.push_back(std::move(tangents));
	modelComponent->meshes.texCoords.push_back(std::move(texCoords));
	modelComponent->meshes.materials.push_back(materialInfo);
	//modelComponent->meshes.colours.push_back(std::move(colours));

	// Material processing
	/*aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	
	// Diffuse texture
	if (material->GetTextureCount(aiTextureType_DIFFUSE) > 1 && material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
		std::cerr << "Material " << material->GetName().C_Str() << " contains more than 1 diffuse texture." << std::endl;
	}

	// 
	/*for (int i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++) {
		aiString a;
		material->GetTexture(aiTextureType_DIFFUSE, 0, &a);
		std::cout << a.C_Str() << std::endl;
	}*/

	/*aiString a;
	material->GetTexture(aiTextureType_DIFFUSE, 0, &a);
	

	aiString path;
	material->GetTexture(aiTextureType_DIFFUSE, 0, &path);
	//this->processTexture(modelComponent, path, TextureType::Diffuse, directory);

	// Specular texture
	if (material->GetTextureCount(aiTextureType_SPECULAR) > 1 && material->GetTextureCount(aiTextureType_SPECULAR) != 0) {
		std::cerr << "Material " << material->GetName().C_Str() << " contains more than 1 specular texture." << std::endl;
	}

	material->GetTexture(aiTextureType_SPECULAR, 0, &path);*/
	//this->processTexture(modelComponent, path, TextureType::Specular, directory, loadedTextures);

	// Store model matrix
	auto mat = node->mTransformation;

	glm::mat4 matrix = {
		mat.a1, mat.a2, mat.a3, mat.a4,
		mat.b1, mat.b2, mat.b3, mat.b4,
		mat.c1, mat.c2, mat.c3, mat.c4,
		mat.d1, mat.d2, mat.d3, mat.d4
	};

	matrix = glm::transpose(matrix);

	//float factor = 0;

	/*for (int i = 0; i < scene->mMetaData->mNumProperties; i++) {
		std::cout << scene->mMetaData->mKeys[i].C_Str() << std::endl;
	}*/

	//scene->mMetaData->Get("OriginalUnitScaleFactor", factor);

	//modelComponent->meshes.modelMatrixes.push_back(matrix);
	details->meshMatrices.push_back(matrix);
}

/*void ModelSystem::processTexture(ModelComponent* modelComponent, aiString path, TextureType type, const std::string& directory, std::map<std::string, GLuint>* loadedTextures) {
	std::string fullPath = directory;
	fullPath = fullPath.append("/").append(path.C_Str());

	GLuint textureID; 
	auto stored	= loadedTextures->find(fullPath);

	if (stored != loadedTextures->end()) {
		textureID = stored->second;
	} else {
		glGenTextures(1, &textureID);

		int width, height, components;
		unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &components, 0);

		if (data) {
			GLenum format;

			if (components == 1) {
				format = GL_RED;
			} else if (components == 3) {
				format = GL_RGB;
			} else if (components == 4) {
				format = GL_RGBA;
			}

			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			loadedTextures->insert({fullPath, textureID});
			stbi_image_free(data);
		} else {
			std::cerr << "Texture failed to load from path " << fullPath << std::endl;
			stbi_image_free(data);
		}
	}

	switch (type) {
	case TextureType::Diffuse:
		modelComponent->meshes.diffuseTextures.push_back(textureID);
		break;
	case TextureType::Specular:
		modelComponent->meshes.specularTextures.push_back(textureID);
		break;
	}
	
}*/

ModelManager::ModelManager() {
	stbi_set_flip_vertically_on_load(true);
}

