#include "PropSimulationSystem.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace bs3d {

namespace {

bool hasTag(const WorldObject& object, const std::string& tag) {
    for (const std::string& objectTag : object.gameplayTags) {
        if (objectTag == tag) {
            return true;
        }
    }
    return false;
}

bool canMove(PropPhysicsBehavior behavior) {
    return behavior == PropPhysicsBehavior::DynamicLite ||
           behavior == PropPhysicsBehavior::FakeDynamic ||
           behavior == PropPhysicsBehavior::BreakableLite;
}

PropPhysicsBehavior behaviorForObject(const WorldObject& object) {
    if (hasTag(object, "fake_dynamic")) {
        return PropPhysicsBehavior::FakeDynamic;
    }
    if (hasTag(object, "breakable_lite")) {
        return PropPhysicsBehavior::BreakableLite;
    }
    if (hasTag(object, "dynamic_prop")) {
        return PropPhysicsBehavior::DynamicLite;
    }
    return PropPhysicsBehavior::Static;
}

float lengthXZ(Vec3 value) {
    return std::sqrt(value.x * value.x + value.z * value.z);
}

Vec3 normalizeXZSafe(Vec3 value) {
    const float len = lengthXZ(value);
    if (len <= 0.0001f) {
        return {};
    }
    return {value.x / len, 0.0f, value.z / len};
}

} // namespace

void PropSimulationSystem::clear() {
    props_.clear();
    carriedId_.clear();
}

void PropSimulationSystem::addProp(const PropSimulationDefinition& definition) {
    PropSimulationState state;
    state.id = definition.id;
    state.basePosition = definition.position;
    state.position = definition.position;
    state.size = definition.size;
    state.yawRadians = definition.yawRadians;
    state.behavior = definition.behavior;
    state.collisionProfile = definition.collisionProfile;
    state.walkableTop = definition.walkableTop;
    props_.push_back(state);
}

void PropSimulationSystem::addPropsFromWorld(const std::vector<WorldObject>& objects) {
    clear();
    for (const WorldObject& object : objects) {
        const bool physical = hasTag(object, "physical_prop") ||
                              hasTag(object, "dynamic_prop") ||
                              hasTag(object, "collision_toy") ||
                              object.id == "bogus_bench";
        if (!physical || object.collision.kind == WorldCollisionShapeKind::None) {
            continue;
        }

        addProp({object.id,
                 object.position,
                 object.collision.size.x > 0.0f ? object.collision.size : object.scale,
                 object.yawRadians + object.collision.yawRadians,
                 behaviorForObject(object),
                 object.collisionProfile,
                 hasTag(object, "walkable_prop_top")});
    }
}

void PropSimulationSystem::update(float deltaSeconds) {
    updateInternal(deltaSeconds, nullptr);
}

void PropSimulationSystem::update(float deltaSeconds, const WorldCollision& staticCollision) {
    updateInternal(deltaSeconds, &staticCollision);
}

void PropSimulationSystem::updateInternal(float deltaSeconds, const WorldCollision* staticCollision) {
    // Clamp dt to prevent large jumps during frame spikes or paused state.
    const float dt = std::clamp(deltaSeconds, 0.0f, 0.10f);
    for (PropSimulationState& prop : props_) {
        if (prop.id == carriedId_) {
            continue;
        }
        if (!canMove(prop.behavior)) {
            continue;
        }

        if (prop.behavior == PropPhysicsBehavior::FakeDynamic) {
            const Vec3 previous = prop.position;
            prop.position = prop.position + prop.velocity * dt;
            if (staticCollision != nullptr) {
                const float radius = std::max(prop.size.x, prop.size.z) * 0.5f;
                if (staticCollision->isCircleBlocked(prop.position,
                                                     radius,
                                                     collisionLayerMask(CollisionLayer::WorldStatic))) {
                    prop.position = previous;
                    prop.velocity.x = 0.0f;
                    prop.velocity.z = 0.0f;
                }
            }
            prop.position = prop.basePosition + (prop.position - prop.basePosition) * std::pow(0.10f, dt);
            prop.velocity = prop.velocity * std::pow(0.18f, dt);
        } else {
            const Vec3 previous = prop.position;
            prop.position = prop.position + prop.velocity * dt;
            if (staticCollision != nullptr) {
                const float radius = std::max(prop.size.x, prop.size.z) * 0.5f;
                if (staticCollision->isCircleBlocked(prop.position,
                                                     radius,
                                                     collisionLayerMask(CollisionLayer::WorldStatic))) {
                    prop.position = previous;
                    prop.velocity.x = 0.0f;
                    prop.velocity.z = 0.0f;
                }
            }
            prop.velocity = prop.velocity * std::pow(0.20f, dt);
        }
        prop.yawRadians += prop.angularVelocity * dt;
        prop.angularVelocity *= std::pow(0.18f, dt);
        prop.impulseAmount = std::max(0.0f, prop.impulseAmount - dt * 1.8f);

        if (lengthXZ(prop.velocity) < 0.02f) {
            prop.velocity = {};
        }
    }
}

bool PropSimulationSystem::applyImpulseNear(Vec3 position, float radius, Vec3 impulse) {
    bool applied = false;
    for (PropSimulationState& prop : props_) {
        if (prop.id == carriedId_) {
            continue;
        }
        if (!canMove(prop.behavior)) {
            continue;
        }

        const float reach = radius + std::max(prop.size.x, prop.size.z) * 0.65f;
        if (distanceSquaredXZ(position, prop.position) > reach * reach) {
            continue;
        }

        const Vec3 impulseDirection = normalizeXZSafe(impulse);
        const Vec3 awayFromHitDirection = normalizeXZSafe(prop.position - position);
        const Vec3 pushDirection = lengthXZ(impulseDirection) > 0.0f ? impulseDirection : awayFromHitDirection;
        const float strength = std::clamp(lengthXZ(impulse), 0.0f, 9.0f);
        prop.velocity = prop.velocity + pushDirection * (strength * 0.65f);
        prop.angularVelocity += (pushDirection.x - pushDirection.z) * 0.75f;
        prop.impulseAmount = std::min(1.0f, prop.impulseAmount + strength / 9.0f);
        applied = true;
    }
    return applied;
}

bool PropSimulationSystem::applyPlayerContact(Vec3 playerPosition, Vec3 playerVelocity, float playerRadius) {
    const float speed = lengthXZ(playerVelocity);
    if (speed <= 0.15f) {
        return false;
    }

    bool applied = false;
    const Vec3 direction = normalizeXZSafe(playerVelocity);
    for (PropSimulationState& prop : props_) {
        if (prop.id == carriedId_) {
            continue;
        }
        if (!canMove(prop.behavior)) {
            continue;
        }

        const float reach = playerRadius + std::max(prop.size.x, prop.size.z) * 0.55f;
        if (distanceSquaredXZ(playerPosition, prop.position) > reach * reach) {
            continue;
        }

        const bool hardPlayerBlocker = hasCollisionLayer(prop.collisionProfile.layers, CollisionLayer::PlayerBlocker);
        const float softness = hardPlayerBlocker ? 0.18f : 0.58f;
        const float maxImpulse = hardPlayerBlocker ? 1.2f : 3.8f;
        const float strength = std::clamp(speed * softness, 0.0f, maxImpulse);
        prop.velocity = prop.velocity + direction * strength;
        prop.angularVelocity += (direction.x - direction.z) * (hardPlayerBlocker ? 0.20f : 0.55f);
        prop.impulseAmount = std::min(1.0f, prop.impulseAmount + strength / std::max(maxImpulse, 0.001f));
        applied = true;
    }
    return applied;
}

bool PropSimulationSystem::hasMovablePropNear(Vec3 holderPosition, Vec3 holderForward, float reach) const {
    const Vec3 forward = normalizeXZSafe(holderForward);
    for (const PropSimulationState& prop : props_) {
        if (prop.id == carriedId_) {
            continue;
        }
        if (!canMove(prop.behavior)) {
            continue;
        }
        const float maxSize = std::max({prop.size.x, prop.size.y, prop.size.z});
        if (maxSize > 1.20f) {
            continue;
        }
        const Vec3 toProp = prop.position - holderPosition;
        if (lengthXZ(forward) > 0.0f) {
            const float frontDot = toProp.x * forward.x + toProp.z * forward.z;
            if (frontDot < -0.15f) {
                continue;
            }
        }
        const float effectiveReach = reach + std::max(prop.size.x, prop.size.z) * 0.55f;
        if (distanceSquaredXZ(holderPosition, prop.position) <= effectiveReach * effectiveReach) {
            return true;
        }
    }
    return false;
}

PropCarryResult PropSimulationSystem::tryBeginCarry(Vec3 holderPosition, Vec3 holderForward, float reach) {
    PropCarryResult result;
    if (!carriedId_.empty()) {
        return result;
    }

    const Vec3 forward = normalizeXZSafe(holderForward);
    PropSimulationState* best = nullptr;
    float bestDistanceSq = 0.0f;
    for (PropSimulationState& prop : props_) {
        if (!canMove(prop.behavior)) {
            continue;
        }
        const float maxSize = std::max({prop.size.x, prop.size.y, prop.size.z});
        if (maxSize > 1.10f) {
            continue;
        }
        const Vec3 toProp = prop.position - holderPosition;
        if (lengthXZ(forward) > 0.0f) {
            const float frontDot = toProp.x * forward.x + toProp.z * forward.z;
            if (frontDot < -0.20f) {
                continue;
            }
        }
        const float effectiveReach = reach + std::max(prop.size.x, prop.size.z) * 0.45f;
        const float distanceSq = distanceSquaredXZ(holderPosition, prop.position);
        if (distanceSq > effectiveReach * effectiveReach) {
            continue;
        }
        if (best == nullptr || distanceSq < bestDistanceSq) {
            best = &prop;
            bestDistanceSq = distanceSq;
        }
    }

    if (best == nullptr) {
        return result;
    }

    carriedId_ = best->id;
    best->velocity = {};
    best->angularVelocity = 0.0f;
    best->impulseAmount = 0.20f;
    updateCarried(holderPosition, lengthXZ(forward) > 0.0f ? forward : Vec3{0.0f, 0.0f, 1.0f});
    result.started = true;
    result.propId = carriedId_;
    return result;
}

void PropSimulationSystem::updateCarried(Vec3 holderPosition, Vec3 holderForward) {
    if (carriedId_.empty()) {
        return;
    }
    const Vec3 forward = normalizeXZSafe(holderForward);
    const Vec3 holdForward = lengthXZ(forward) > 0.0f ? forward : Vec3{0.0f, 0.0f, 1.0f};
    for (PropSimulationState& prop : props_) {
        if (prop.id != carriedId_) {
            continue;
        }
        prop.position = holderPosition + holdForward * 0.90f + Vec3{0.0f, 0.58f, 0.0f};
        prop.velocity = {};
        prop.angularVelocity = 0.0f;
        prop.yawRadians = yawFromDirection(holdForward);
        prop.impulseAmount = std::max(prop.impulseAmount, 0.12f);
        return;
    }
    carriedId_.clear();
}

void PropSimulationSystem::dropCarried(Vec3 releaseVelocity) {
    if (carriedId_.empty()) {
        return;
    }
    for (PropSimulationState& prop : props_) {
        if (prop.id == carriedId_) {
            prop.position.y = prop.basePosition.y;
            prop.velocity = releaseVelocity;
            prop.velocity.y = 0.0f;
            prop.angularVelocity += (releaseVelocity.x - releaseVelocity.z) * 0.18f;
            prop.impulseAmount = std::min(1.0f, prop.impulseAmount + lengthXZ(releaseVelocity) / 8.0f);
            break;
        }
    }
    carriedId_.clear();
}

bool PropSimulationSystem::carrying() const {
    return !carriedId_.empty();
}

const std::string& PropSimulationSystem::carriedId() const {
    return carriedId_;
}

void PropSimulationSystem::publishCollision(WorldCollision& collision) const {
    for (const PropSimulationState& prop : props_) {
        if (prop.id == carriedId_) {
            continue;
        }
        if (prop.collisionProfile.responseKind == CollisionResponseKind::None) {
            continue;
        }
        collision.addDynamicOrientedBox(prop.position + Vec3{0.0f, prop.size.y * 0.5f, 0.0f},
                                       prop.size,
                                       prop.yawRadians,
                                       prop.collisionProfile);
        if (prop.walkableTop) {
            const Vec3 topSize{prop.size.x * 0.82f, 0.06f, prop.size.z * 0.82f};
            collision.addDynamicGroundBox(prop.position + Vec3{0.0f, prop.size.y - topSize.y, 0.0f},
                                          topSize,
                                          prop.yawRadians,
                                          CollisionProfile::walkableSurface());
        }
    }
}

PropSyncStats PropSimulationSystem::syncWorldObjects(std::vector<WorldObject>& objects) const {
    PropSyncStats stats;
    std::unordered_map<std::string, const PropSimulationState*> index;
    index.reserve(props_.size());
    for (const PropSimulationState& prop : props_) {
        index.emplace(prop.id, &prop);
    }

    for (WorldObject& object : objects) {
        ++stats.objectsVisited;
        ++stats.indexedLookups;
        const auto found = index.find(object.id);
        if (found != index.end()) {
            const PropSimulationState* state = found->second;
            object.position = state->position;
            object.yawRadians = state->yawRadians;
            ++stats.updatedObjects;
        }
    }
    return stats;
}

PropSimulationSnapshot PropSimulationSystem::snapshot() const {
    return {props_, carriedId_};
}

void PropSimulationSystem::restore(const PropSimulationSnapshot& snapshot) {
    props_ = snapshot.props;
    carriedId_ = snapshot.carriedId;
}

const PropSimulationState* PropSimulationSystem::find(const std::string& id) const {
    for (const PropSimulationState& prop : props_) {
        if (prop.id == id) {
            return &prop;
        }
    }
    return nullptr;
}

std::size_t PropSimulationSystem::count() const {
    return props_.size();
}

} // namespace bs3d
