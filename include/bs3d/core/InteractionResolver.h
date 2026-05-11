#pragma once

#include "bs3d/core/Types.h"

#include <string>
#include <vector>

namespace bs3d {

enum class InteractionAction {
    None,
    Talk,
    StartMission,
    EnterVehicle,
    ExitVehicle,
    UseMarker,
    UseDoor,
    PickUp
};

enum class InteractionInput {
    Use,
    Vehicle,
    Primary
};

struct InteractionCandidate {
    std::string id;
    InteractionAction action = InteractionAction::None;
    InteractionInput input = InteractionInput::Use;
    std::string prompt;
    Vec3 position{};
    float radius = 2.0f;
    int priority = 0;
    bool enabled = true;
};

struct InteractionResolution {
    bool available = false;
    std::string id;
    InteractionAction action = InteractionAction::None;
    InteractionInput input = InteractionInput::Use;
    std::string prompt;
    float distance = 0.0f;
    int priority = 0;
};

class InteractionResolver {
public:
    void clear();
    void addCandidate(InteractionCandidate candidate);
    InteractionResolution resolve(Vec3 actorPosition, InteractionInput input) const;
    const std::vector<InteractionCandidate>& candidates() const;

private:
    std::vector<InteractionCandidate> candidates_;
};

} // namespace bs3d
