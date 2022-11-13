#include "Entities.hpp"

size_t Entities::addEntity(AddEntityInfo* info) {
    size_t id = 0;

    if (!this->reusableIds.empty()) {
        id = this->reusableIds.front();
        this->reusableIds.pop();
    } else {
        id = this->usedIds.size();
        // Resize data structures to match new size
        this->modelResources.resize(id + 1);
    }

    this->usedIds.push_back(id);

    this->modelResources.at(id) = info->modelResource;

    return id;
}

std::vector<ModelResource>* Entities::getModelResourceIds() {
    return &this->modelResources;
}