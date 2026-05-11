#pragma once

#include "WorldDataLoader.h"
#include "WorldArtTypes.h"

#include "bs3d/core/InteractionResolver.h"

#include <optional>
#include <string>
#include <vector>

namespace bs3d {

struct WorldObjectInteractionAffordance {
    std::string candidateId;
    std::string objectId;
    std::string outcomeId;
    std::string prompt;
    std::string speaker;
    std::string line;
    InteractionAction action = InteractionAction::UseMarker;
    InteractionInput input = InteractionInput::Use;
    Vec3 position{};
    WorldLocationTag location = WorldLocationTag::Unknown;
    float radius = 1.8f;
    int priority = 20;
};

struct WorldObjectInteractionEvent {
    WorldEventType type = WorldEventType::PublicNuisance;
    Vec3 position{};
    float intensity = 0.0f;
    std::string source;
    float cooldownSeconds = 0.0f;
};

std::optional<WorldObjectInteractionAffordance> worldObjectInteractionAffordance(const WorldObject& object);
WorldObjectInteractionAffordance worldObjectAffordanceWithOutcomeData(
    const WorldObjectInteractionAffordance& affordance,
    const ObjectOutcomeCatalogData* outcomeCatalog);
std::optional<WorldObjectInteractionEvent> worldEventForObjectAffordance(
    const WorldObjectInteractionAffordance& affordance);
std::optional<WorldObjectInteractionEvent> worldEventForObjectAffordance(
    const WorldObjectInteractionAffordance& affordance,
    const ObjectOutcomeCatalogData* outcomeCatalog);
std::vector<InteractionCandidate> buildWorldObjectInteractionCandidates(const std::vector<WorldObject>& objects);
std::optional<WorldObjectInteractionAffordance> findWorldObjectInteractionAffordance(
    const std::vector<WorldObject>& objects,
    const std::string& candidateId);

} // namespace bs3d
