#pragma once

#include "WorldArtTypes.h"

#include "bs3d/core/WorldCollision.h"

#include <string>
#include <vector>

namespace bs3d {

enum class PropPhysicsBehavior {
    Static,
    FakeDynamic,
    DynamicLite,
    BreakableLite
};

struct PropSimulationDefinition {
    std::string id;
    Vec3 position{};
    Vec3 size{1.0f, 1.0f, 1.0f};
    float yawRadians = 0.0f;
    PropPhysicsBehavior behavior = PropPhysicsBehavior::Static;
    CollisionProfile collisionProfile = CollisionProfile::playerBlocker();
    bool walkableTop = false;
};

struct PropSimulationState {
    std::string id;
    Vec3 basePosition{};
    Vec3 position{};
    Vec3 size{1.0f, 1.0f, 1.0f};
    Vec3 velocity{};
    float yawRadians = 0.0f;
    float angularVelocity = 0.0f;
    float impulseAmount = 0.0f;
    PropPhysicsBehavior behavior = PropPhysicsBehavior::Static;
    CollisionProfile collisionProfile = CollisionProfile::playerBlocker();
    bool walkableTop = false;
};

struct PropCarryResult {
    bool started = false;
    std::string propId;
};

struct PropSyncStats {
    std::size_t objectsVisited = 0;
    std::size_t indexedLookups = 0;
    std::size_t linearFindFallbacks = 0;
    std::size_t updatedObjects = 0;
};

struct PropSimulationSnapshot {
    std::vector<PropSimulationState> props;
    std::string carriedId;
};

class PropSimulationSystem {
public:
    void clear();
    void addProp(const PropSimulationDefinition& definition);
    void addPropsFromWorld(const std::vector<WorldObject>& objects);
    void update(float deltaSeconds);
    void update(float deltaSeconds, const WorldCollision& staticCollision);
    bool applyImpulseNear(Vec3 position, float radius, Vec3 impulse);
    bool applyPlayerContact(Vec3 playerPosition, Vec3 playerVelocity, float playerRadius);
    bool hasMovablePropNear(Vec3 holderPosition, Vec3 holderForward, float reach) const;
    PropCarryResult tryBeginCarry(Vec3 holderPosition, Vec3 holderForward, float reach);
    void updateCarried(Vec3 holderPosition, Vec3 holderForward);
    void dropCarried(Vec3 releaseVelocity = {});
    bool carrying() const;
    const std::string& carriedId() const;
    void publishCollision(WorldCollision& collision) const;
    PropSyncStats syncWorldObjects(std::vector<WorldObject>& objects) const;
    PropSimulationSnapshot snapshot() const;
    void restore(const PropSimulationSnapshot& snapshot);

    const PropSimulationState* find(const std::string& id) const;
    std::size_t count() const;

private:
    void updateInternal(float deltaSeconds, const WorldCollision* staticCollision);

    std::vector<PropSimulationState> props_{};
    std::string carriedId_;
};

} // namespace bs3d
