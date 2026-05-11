#include "bs3d/core/Scene.h"

#include <utility>

namespace bs3d {

EntityId Scene::createObject(std::string name, Transform transform) {
    const EntityId id = nextId_++;
    objects_.push_back({id, std::move(name), transform});
    return id;
}

void Scene::clear() {
    objects_.clear();
    nextId_ = 1;
}

Transform* Scene::findTransform(EntityId id) {
    for (SceneObject& object : objects_) {
        if (object.id == id) {
            return &object.transform;
        }
    }

    return nullptr;
}

const Transform* Scene::findTransform(EntityId id) const {
    for (const SceneObject& object : objects_) {
        if (object.id == id) {
            return &object.transform;
        }
    }

    return nullptr;
}

const std::vector<SceneObject>& Scene::objects() const {
    return objects_;
}

std::size_t Scene::objectCount() const {
    return objects_.size();
}

} // namespace bs3d
