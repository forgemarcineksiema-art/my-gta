#pragma once

#include "bs3d/core/Types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace bs3d {

struct SceneObject {
    EntityId id = InvalidEntity;
    std::string name;
    Transform transform{};
};

class Scene {
public:
    EntityId createObject(std::string name, Transform transform);
    void clear();

    Transform* findTransform(EntityId id);
    const Transform* findTransform(EntityId id) const;
    const std::vector<SceneObject>& objects() const;
    std::size_t objectCount() const;

private:
    EntityId nextId_ = 1;
    std::vector<SceneObject> objects_;
};

} // namespace bs3d
